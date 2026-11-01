#include "cst816.h"

#include "board_pins.h"
#include "lcd_init.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
 * CST816T 触摸驱动（ESP-IDF v6.1，新版 driver/i2c_master.h）。
 *
 * 排障约定：本文件保留一整套上电自检日志（静态电平、线序 bit-bang 探测、
 * 逐地址 probe 结果、0x15 读写测试）。这些日志成本很低，也是后续整机
 * 自检的现成手段，不随“问题修好”而删除。
 *
 * 关键事实：
 *   - CST816T 7 位地址 0x15；i2c_master_probe() 直接传 7 位地址，
 *     由驱动内部完成 <<1 | R/W，不要自己移位（厂商 bit-bang 例程才需要）。
 *   - probe 返回 ESP_ERR_NOT_FOUND = 有 ACK 时序但该地址无人应答；
 *     返回 ESP_ERR_TIMEOUT     = 地址帧都没发完（上拉/短路/线序/地）。
 *     两者含义完全不同，必须分别统计。
 */

static const char *TAG = "CST816";

/* i2c_master 句柄；init 成功后长期有效，运行期只读 */
static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;

/* 实际使用的 SDA/SCL；bit-bang 探测发现线序接反时会交换 */
static int s_sda_pin = BSP_TOUCH_PIN_SDA;
static int s_scl_pin = BSP_TOUCH_PIN_SCL;

#define CST816_I2C_TIMEOUT_MS   100   /* 单次寄存器读写超时 */
#define CST816_PROBE_TIMEOUT_MS 20    /* 单个地址 probe 超时 */
#define CST816_BB_DELAY_US      20    /* bit-bang 半周期；放宽以容忍仅内部上拉 */
#define CST816_SCAN_FIRST_ADDR  0x08  /* 跳过 0x00~0x07 保留地址 */
#define CST816_SCAN_LAST_ADDR   0x77  /* 跳过 0x78~0x7F 保留地址 */

/**
 * 读取连续寄存器。
 * @param reg 起始寄存器地址；@param buf 输出缓冲；@param len 读取字节数。
 * @return ESP_OK 成功；ESP_ERR_INVALID_RESPONSE 为从机 NACK；ESP_ERR_TIMEOUT 为总线超时。
 * 调用上下文：任务上下文，非 ISR；s_dev 由 cst816_init() 创建，调用前必须非空。
 */
static esp_err_t cst816_read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
	return i2c_master_transmit_receive(s_dev, &reg, 1, buf, len, CST816_I2C_TIMEOUT_MS);
}

/**
 * 写单个寄存器。
 * @param reg 寄存器地址；@param val 写入值。
 * @return 同 i2c_master_transmit()：NACK 时返回 ESP_ERR_INVALID_RESPONSE。
 * 调用上下文：任务上下文。
 */
static esp_err_t cst816_write_reg(uint8_t reg, uint8_t val)
{
	uint8_t buf[2] = {reg, val};
	return i2c_master_transmit(s_dev, buf, sizeof(buf), CST816_I2C_TIMEOUT_MS);
}

/* ---------------- bit-bang 诊断用的 GPIO 原语 ---------------- */

/** bit-bang 半周期延时；用 ROM 延时避免 FreeRTOS tick 粒度太粗。 */
static void cst816_bb_delay(void)
{
	esp_rom_delay_us(CST816_BB_DELAY_US);
}

/** 模拟开漏“拉低”：输出 0。 */
static void cst816_bb_low(int pin)
{
	gpio_set_level(pin, 0);
	gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

/** 模拟开漏“释放”：切输入，由内部/外部上拉拉高。 */
static void cst816_bb_release(int pin)
{
	gpio_set_level(pin, 1);
	gpio_set_direction(pin, GPIO_MODE_INPUT);
}

/**
 * 产生 I2C START：SCL 高电平期间 SDA 由高变低。
 * 调用上下文：仅诊断阶段，I2C 外设尚未接管引脚。
 */
static void cst816_bb_start(int sda, int scl)
{
	cst816_bb_release(sda);
	cst816_bb_release(scl);
	cst816_bb_delay();
	cst816_bb_low(sda);
	cst816_bb_delay();
	cst816_bb_low(scl);
	cst816_bb_delay();
}

/**
 * 产生 I2C STOP：SCL 高电平期间 SDA 由低变高。
 * 调用上下文：仅诊断阶段。
 */
static void cst816_bb_stop(int sda, int scl)
{
	cst816_bb_low(scl);
	cst816_bb_delay();
	cst816_bb_low(sda);
	cst816_bb_delay();
	cst816_bb_release(scl);
	cst816_bb_delay();
	cst816_bb_release(sda);
	cst816_bb_delay();
}

/**
 * bit-bang 发送 1 字节并采样第 9 个时钟的 ACK。
 * @return true = 从机拉低 SDA（ACK）；false = NACK 或总线时序没走通。
 * 调用上下文：仅诊断阶段；SCL 由本函数驱动，SDA 在 ACK 位切为输入。
 */
static bool cst816_bb_write_byte(int sda, int scl, uint8_t byte)
{
	for (int i = 0; i < 8; ++i) {
		if (byte & 0x80) {
			cst816_bb_release(sda);
		} else {
			cst816_bb_low(sda);
		}
		cst816_bb_delay();
		cst816_bb_release(scl);
		cst816_bb_delay();
		cst816_bb_low(scl);
		cst816_bb_delay();
		byte <<= 1;
	}

	/* 第 9 个时钟：主机释放 SDA，从机若在线会把它拉低 */
	cst816_bb_release(sda);
	cst816_bb_delay();
	cst816_bb_release(scl);
	cst816_bb_delay();
	bool ack = (gpio_get_level(sda) == 0);
	cst816_bb_low(scl);
	cst816_bb_delay();
	return ack;
}

/**
 * bit-bang 读取 1 字节。
 * @param ack true = 回 ACK（后面还有数据）；false = 回 NACK（本次是最后一个字节）。
 * @return 读到的字节；SDA 全程由外部上拉拉高，读到的位即从机输出。
 * 调用上下文：仅诊断阶段。
 */
static uint8_t cst816_bb_read_byte(int sda, int scl, bool ack)
{
	uint8_t val = 0;

	cst816_bb_release(sda);
	for (int i = 0; i < 8; ++i) {
		val <<= 1;
		cst816_bb_release(scl);
		cst816_bb_delay();
		if (gpio_get_level(sda)) {
			val |= 0x01;
		}
		cst816_bb_low(scl);
		cst816_bb_delay();
	}
	/* 第 9 个时钟回 ACK/NACK */
	if (ack) {
		cst816_bb_release(sda);
	} else {
		cst816_bb_low(sda);
	}
	cst816_bb_delay();
	cst816_bb_release(scl);
	cst816_bb_delay();
	cst816_bb_low(scl);
	cst816_bb_delay();
	cst816_bb_release(sda);

	return val;
}

/**
 * 用与厂商例程等价的 bit-bang 时序独立验证 0x15 是否 ACK，并读一次芯片 ID。
 *
 * 目的：把“触摸芯片/转接板/线序”与“ESP-IDF i2c_master 配置”两种故障分开。
 * 若 bit-bang 能 ACK 而 i2c_master 不能，问题在驱动/时钟/上拉强度；
 * 若两者都不能 ACK，问题在供电、复位、线序或芯片本身。
 *
 * @param sda/scl 本次尝试的引脚映射；@param addr 7 位地址。
 * @param chip_id 输出：ACK 后读到的 0xA7 寄存器值（失败时保持不变）。
 * @return true = 地址被 ACK（芯片在总线上）。
 * 调用上下文：cst816_init() 中，i2c 外设尚未接管引脚；不可并发调用。
 */
static bool cst816_bb_probe(int sda, int scl, uint8_t addr, uint8_t *chip_id)
{
	gpio_config_t io = {
		.pin_bit_mask = (1ULL << sda) | (1ULL << scl),
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_INPUT_OUTPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
	};
	gpio_config(&io);

	cst816_bb_release(sda);
	cst816_bb_release(scl);
	vTaskDelay(pdMS_TO_TICKS(2));

	/* 若 SDA 被从机拉住，先补 9 个 SCL 时钟 + STOP 做总线恢复 */
	if (gpio_get_level(sda) == 0) {
		for (int i = 0; i < 9; ++i) {
			cst816_bb_low(scl);
			cst816_bb_delay();
			cst816_bb_release(scl);
			cst816_bb_delay();
		}
		cst816_bb_stop(sda, scl);
	}

	bool ack = false;
	uint8_t id = 0;

	cst816_bb_start(sda, scl);
	ack = cst816_bb_write_byte(sda, scl, (uint8_t)(addr << 1));
	cst816_bb_stop(sda, scl);

	ESP_LOGI(TAG, "bitbang probe 0x%02X (SDA=%d SCL=%d): %s",
	         addr, sda, scl, ack ? "ACK" : "NACK");

	if (ack) {
		/* 标准寄存器读：地址+W → 寄存器 → 重复 START → 地址+R → 1 字节 + NACK */
		bool ok = true;
		cst816_bb_start(sda, scl);
		ok = ok && cst816_bb_write_byte(sda, scl, (uint8_t)(addr << 1));
		ok = ok && cst816_bb_write_byte(sda, scl, CST816_REG_CHIP_ID);
		if (ok) {
			cst816_bb_start(sda, scl);
			ok = cst816_bb_write_byte(sda, scl, (uint8_t)((addr << 1) | 1));
			if (ok) {
				id = cst816_bb_read_byte(sda, scl, false);
				*chip_id = id;
			}
		}
		cst816_bb_stop(sda, scl);
		ESP_LOGI(TAG, "bitbang chip id(0xA7) = 0x%02X (%s)", id,
		         ok ? "read ok" : "register read failed");
	}

	/* 释放引脚，交给后续 i2c_new_master_bus() 接管 */
	gpio_set_direction(sda, GPIO_MODE_INPUT);
	gpio_set_direction(scl, GPIO_MODE_INPUT);

	return ack;
}

/**
 * 上电静态电平检查 + 总线恢复。
 *
 * 在 i2c 外设接管引脚之前用普通 GPIO 读电平：一旦引脚切到 I2C IO_MUX，
 * gpio_get_level() 的读数不再可信，所以这一步必须先做。
 *
 * 判读：
 *   SDA=1 SCL=1 → 上拉在位、总线空闲（正常）。
 *   SDA=0         → 被从机拉住 / 对地短路 / 没上拉且被拉低。
 *   SCL=0         → 同上，通常是线序或短路。
 *   RST=0         → TRST 被短路到地，或该 GPIO 被别的东西占用。
 *
 * @return ESP_OK；gpio_config 失败时返回对应 esp_err_t。
 * 调用上下文：cst816_init() 开头，单线程。
 */
static esp_err_t cst816_gpio_precheck(void)
{
	gpio_config_t rst_conf = {
		.pin_bit_mask = (1ULL << BSP_TOUCH_PIN_RST),
		.intr_type = GPIO_INTR_DISABLE,
		/* INPUT_OUTPUT 才能用 gpio_get_level 回读实际电平；纯 OUTPUT 的输入缓冲被关掉 */
		.mode = GPIO_MODE_INPUT_OUTPUT,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pull_up_en = GPIO_PULLUP_DISABLE,
	};
	esp_err_t err = gpio_config(&rst_conf);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "reset gpio config failed: %s", esp_err_to_name(err));
		return err;
	}
	/* 诊断期间先释放复位，避免芯片一直被按在复位态 */
	gpio_set_level(BSP_TOUCH_PIN_RST, 1);

	gpio_config_t int_conf = {
		.pin_bit_mask = (1ULL << BSP_TOUCH_PIN_INT),
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_INPUT,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pull_up_en = GPIO_PULLUP_ENABLE,
	};
	err = gpio_config(&int_conf);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "int gpio config failed: %s", esp_err_to_name(err));
		return err;
	}

	gpio_config_t bus_conf = {
		.pin_bit_mask = (1ULL << BSP_TOUCH_PIN_SDA) | (1ULL << BSP_TOUCH_PIN_SCL),
		.intr_type = GPIO_INTR_DISABLE,
		.mode = GPIO_MODE_INPUT,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.pull_up_en = GPIO_PULLUP_ENABLE,
	};
	err = gpio_config(&bus_conf);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "sda/scl gpio config failed: %s", esp_err_to_name(err));
		return err;
	}
	vTaskDelay(pdMS_TO_TICKS(2));

	int sda = gpio_get_level(BSP_TOUCH_PIN_SDA);
	int scl = gpio_get_level(BSP_TOUCH_PIN_SCL);
	ESP_LOGI(TAG, "idle levels (internal pull-up): SDA=%d SCL=%d RST=%d INT=%d",
	         sda, scl, gpio_get_level(BSP_TOUCH_PIN_RST), gpio_get_level(BSP_TOUCH_PIN_INT));

	if (sda == 0) {
		ESP_LOGW(TAG, "SDA reads 0 with pull-up: 可能对地短路、线序接错，或从机仍拉住总线");
	}
	if (scl == 0) {
		ESP_LOGW(TAG, "SCL reads 0 with pull-up: 可能对地短路或线序接错");
	}

	/* 仅当 SDA 被拉住时补时钟；正常空闲不做无谓的总线动作 */
	if (sda == 0) {
		cst816_bb_low(BSP_TOUCH_PIN_SCL);
		for (int i = 0; i < 9; ++i) {
			cst816_bb_delay();
			cst816_bb_release(BSP_TOUCH_PIN_SCL);
			cst816_bb_delay();
			cst816_bb_low(BSP_TOUCH_PIN_SCL);
		}
		cst816_bb_stop(BSP_TOUCH_PIN_SDA, BSP_TOUCH_PIN_SCL);
		ESP_LOGI(TAG, "bus recovery done: SDA=%d SCL=%d",
		         gpio_get_level(BSP_TOUCH_PIN_SDA), gpio_get_level(BSP_TOUCH_PIN_SCL));
	}

	return ESP_OK;
}

/**
 * CST816T 硬复位。
 * 时序与厂商 CST816_RESET() 一致：RST 低 10 ms，释放后等 100 ms 再访问 I2C
 * （手册要求释放后 >50 ms 才允许总线操作，这里留一倍余量）。
 * 调用上下文：任务上下文；要求 RST 已配置为 INPUT_OUTPUT。
 */
static void cst816_hw_reset(void)
{
	gpio_set_level(BSP_TOUCH_PIN_RST, 0);
	vTaskDelay(pdMS_TO_TICKS(10));
	gpio_set_level(BSP_TOUCH_PIN_RST, 1);
	vTaskDelay(pdMS_TO_TICKS(100));
}

/**
 * 创建 i2c_master 总线并挂载 0x15 设备，逐步骤打印返回值。
 * @param sda/scl 已确认（或默认）的引脚映射。
 * @return ESP_OK；失败时返回 i2c_new_master_bus / i2c_master_bus_add_device 的错误码，
 *         并保证 s_bus/s_dev 回到 NULL，避免留下半初始化状态。
 * 调用上下文：cst816_init()；重复调用前必须先 cst816_bus_destroy()。
 */
static esp_err_t cst816_bus_create(int sda, int scl)
{
	i2c_master_bus_config_t bus_cfg = {
		.i2c_port = BSP_TOUCH_I2C_PORT,
		.sda_io_num = sda,
		.scl_io_num = scl,
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.glitch_ignore_cnt = 7,
		/* 内部上拉约 45 kΩ，只作为兜底；转接板必须外接 4.7 kΩ 才能稳定跑 100 kHz */
		.flags.enable_internal_pullup = true,
	};

	esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus);
	ESP_LOGI(TAG, "i2c_new_master_bus(port=%d SDA=%d SCL=%d) -> %s",
	         BSP_TOUCH_I2C_PORT, sda, scl, esp_err_to_name(err));
	if (err != ESP_OK) {
		s_bus = NULL;
		return err;
	}

	i2c_device_config_t dev_cfg = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address = BSP_TOUCH_I2C_ADDR,
		.scl_speed_hz = BSP_TOUCH_I2C_HZ,
	};
	err = i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev);
	ESP_LOGI(TAG, "i2c_master_bus_add_device(addr=0x%02X @%d Hz) -> %s",
	         BSP_TOUCH_I2C_ADDR, BSP_TOUCH_I2C_HZ, esp_err_to_name(err));
	if (err != ESP_OK) {
		s_dev = NULL;
		i2c_del_master_bus(s_bus);
		s_bus = NULL;
		return err;
	}

	return ESP_OK;
}

/**
 * 逐地址 probe 全总线，并把 ACK / NACK / TIMEOUT 分开统计。
 *
 * 结果判读（这是本次排障的核心）：
 *   出现 ACK              → 总线上确有设备，地址见日志。
 *   全部 NACK             → 地址帧能发完（上拉与时钟正常），但 0x15 不应答：
 *                           触摸芯片未上电、被按在复位、线序错、或芯片损坏。
 *   出现 TIMEOUT          → 地址帧都发不完：SCL/SDA 上拉缺失、对地/对电源短路、
 *                           两根线接反、地不共。此类故障优先查硬件，不要改软件。
 *
 * 调用上下文：任务上下文；需要 s_bus 有效。probe 期间总线锁由驱动内部持有。
 * 副作用：会占用总线约 (地址数 × probe 超时)，全部超时最坏约 2.2 s。
 */
static void cst816_i2c_scan(void)
{
	int ack_cnt = 0;
	int nack_cnt = 0;
	int timeout_cnt = 0;
	int other_cnt = 0;

	ESP_LOGI(TAG, "i2c scan start: 0x%02X..0x%02X, SDA=%d SCL=%d",
	         CST816_SCAN_FIRST_ADDR, CST816_SCAN_LAST_ADDR, s_sda_pin, s_scl_pin);

	for (uint16_t addr = CST816_SCAN_FIRST_ADDR; addr <= CST816_SCAN_LAST_ADDR; ++addr) {
		esp_err_t err = i2c_master_probe(s_bus, addr, CST816_PROBE_TIMEOUT_MS);

		/* 三种结果含义不同，必须逐条打印，不能只打印“找到” */
		if (err == ESP_OK) {
			ESP_LOGI(TAG, "scan 0x%02X -> ACK", addr);
			++ack_cnt;
		} else if (err == ESP_ERR_NOT_FOUND) {
			ESP_LOGI(TAG, "scan 0x%02X -> NACK", addr);
			++nack_cnt;
		} else if (err == ESP_ERR_TIMEOUT) {
			ESP_LOGW(TAG, "scan 0x%02X -> TIMEOUT", addr);
			++timeout_cnt;
		} else {
			ESP_LOGW(TAG, "scan 0x%02X -> %s", addr, esp_err_to_name(err));
			++other_cnt;
		}
	}

	ESP_LOGI(TAG, "i2c scan summary: ACK=%d NACK=%d TIMEOUT=%d OTHER=%d",
	         ack_cnt, nack_cnt, timeout_cnt, other_cnt);

	if (ack_cnt > 0) {
		ESP_LOGI(TAG, "verdict: 总线有设备应答（见上面的 ACK 行）");
	} else if (timeout_cnt > 0) {
		ESP_LOGE(TAG, "verdict: 总线物理层异常（TIMEOUT=%d）——先查 4.7k 上拉、TSDA/TSCL 是否接反、"
		              "GND 是否共地、VCC 是否为 3.0~3.2V；不要先改软件", timeout_cnt);
	} else {
		ESP_LOGW(TAG, "verdict: 总线时序正常（全部 NACK），但 0x%02X 不应答——查触摸供电、"
		              "TRST 是否真正到达芯片、转接板 1/2/3/6/7/8 针脚顺序",
		         BSP_TOUCH_I2C_ADDR);
	}
}

/**
 * 对 0x15 做定向读写测试：读芯片 ID、读手指寄存器、写回 0xF9 原值并回读。
 * 只做“写回原值”，不改变芯片配置；全部结果都打印 esp_err_t 名字。
 * 调用上下文：任务上下文，cst816_init() 中扫描之后调用。
 */
static void cst816_diag_rw(void)
{
	uint8_t id = 0;
	esp_err_t err = cst816_read_regs(CST816_REG_CHIP_ID, &id, 1);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "diag: read chip id(0xA7) failed: %s（0x%02X 未应答）",
		         esp_err_to_name(err), BSP_TOUCH_I2C_ADDR);
		return;
	}
	ESP_LOGI(TAG, "diag: chip id(0xA7) = 0x%02X", id);
	if (id != 0xB5 && id != 0xB6) {
		ESP_LOGW(TAG, "diag: chip id 非 0xB5/0xB6，核对是否 CST816T 或线序/地址是否错");
	}

	uint8_t finger = 0;
	err = cst816_read_regs(CST816_REG_FINGER_NUM, &finger, 1);
	ESP_LOGI(TAG, "diag: reg 0x02(finger) = 0x%02X (%s)", finger, esp_err_to_name(err));

	/* 写回测试：读原值 → 写回同一值 → 再读，验证写通路且不改变配置 */
	uint8_t orig = 0;
	uint8_t readback = 0;
	err = cst816_read_regs(CST816_REG_AUTO_SLEEP, &orig, 1);
	if (err != ESP_OK) {
		ESP_LOGW(TAG, "diag: read 0xF9 failed: %s", esp_err_to_name(err));
		return;
	}
	err = cst816_write_reg(CST816_REG_AUTO_SLEEP, orig);
	if (err != ESP_OK) {
		ESP_LOGW(TAG, "diag: write 0xF9 failed: %s", esp_err_to_name(err));
		return;
	}
	err = cst816_read_regs(CST816_REG_AUTO_SLEEP, &readback, 1);
	ESP_LOGI(TAG, "diag: write/read-back 0xF9: wrote 0x%02X read 0x%02X (%s)",
	         orig, readback, esp_err_to_name(err));
	if (err == ESP_OK && readback != orig) {
		ESP_LOGW(TAG, "diag: 写回值不一致，写通路可疑");
	}
}

/**
 * 触摸初始化：静态电平 → 硬复位 → bit-bang 线序探测 → i2c_master 建链 →
 * 全地址扫描 → 0x15 读写测试。
 *
 * 线序策略：先用 bit-bang 在 (SDA=8,SCL=9) 上试 0x15；失败再试 (9,8)。
 * 只有“默认失败且交换成功”时才改用交换后的映射，避免误判。
 *
 * @return ESP_OK：初始化流程走完（不保证芯片应答，结果见日志）；
 *         其他：gpio 或 i2c 建链失败，按错误码返回。
 * 调用上下文：app_main 单线程；不可重入。
 */
esp_err_t cst816_init(void)
{
	ESP_LOGI(TAG, "pins: SDA=%d SCL=%d RST=%d INT=%d addr=0x%02X @%d Hz",
	         BSP_TOUCH_PIN_SDA, BSP_TOUCH_PIN_SCL, BSP_TOUCH_PIN_RST,
	         BSP_TOUCH_PIN_INT, BSP_TOUCH_I2C_ADDR, BSP_TOUCH_I2C_HZ);

	esp_err_t err = cst816_gpio_precheck();
	if (err != ESP_OK) {
		return err;
	}

	/* 复位必须发生在任何 I2C 访问之前，否则芯片可能还在复位态 */
	cst816_hw_reset();
	ESP_LOGI(TAG, "TRST level after release: %d (expect 1)",
	         gpio_get_level(BSP_TOUCH_PIN_RST));

	uint8_t bb_id = 0;
	bool bb_ack = cst816_bb_probe(s_sda_pin, s_scl_pin, BSP_TOUCH_I2C_ADDR, &bb_id);
	if (!bb_ack) {
		uint8_t swapped_id = 0;
		if (cst816_bb_probe(s_scl_pin, s_sda_pin, BSP_TOUCH_I2C_ADDR, &swapped_id)) {
			ESP_LOGW(TAG, "bitbang: 仅交换后的映射应答，改用 SDA=%d SCL=%d",
			         s_scl_pin, s_sda_pin);
			int tmp = s_sda_pin;
			s_sda_pin = s_scl_pin;
			s_scl_pin = tmp;
			bb_ack = true;
		}
	}

	/* 交换探测后可能改写过引脚，重新复位一次再进 i2c_master */
	cst816_hw_reset();

	err = cst816_bus_create(s_sda_pin, s_scl_pin);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "touch i2c init failed: %s", esp_err_to_name(err));
		return err;
	}

	cst816_i2c_scan();
	cst816_diag_rw();

	if (bb_ack) {
		ESP_LOGI(TAG, "TOUCH: 控制器在线（bit-bang ACK，chip id=0x%02X）", bb_id);
	} else {
		ESP_LOGE(TAG, "TOUCH: 控制器 0x%02X 无应答——按上面 scan / bitbang 结论查硬件",
		         BSP_TOUCH_I2C_ADDR);
	}

	/* 说明：厂商例程会在 init 末尾写 AutoSleepTime=5。bring-up 阶段不写，
	 * 否则一旦 TRST 未真正连通，芯片睡眠后 MCU 复位也无法唤醒，会把
	 * “接线故障”伪装成“软件扫描失败”，干扰定位。 */
	return ESP_OK;
}

/**
 * 读取一次触摸状态（5 个连续寄存器：0x02 手指, 0x03/0x04 X, 0x05/0x06 Y）。
 * @param out 输出结构；函数总会填充，失败时 pressed=false、fingers=0。
 * @return ESP_OK；总线错误时返回 i2c_master_transmit_receive 的错误码。
 * 调用上下文：任务上下文，建议 10~20 ms 轮询；不可在 ISR 调用。
 */
esp_err_t cst816_read_touch(cst816_touch_t *out)
{
	if (out == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	uint8_t buf[5] = {0};
	esp_err_t err = cst816_read_regs(CST816_REG_FINGER_NUM, buf, sizeof(buf));
	if (err != ESP_OK) {
		out->pressed = false;
		out->fingers = 0;
		return err;
	}

	/* 0xFF 是芯片睡眠标志，不是“5 个手指”，必须按 0 处理 */
	out->fingers = (buf[0] == 0xFF) ? 0 : buf[0];
	out->pressed = (out->fingers > 0);
	/* X/Y 高 4 位有效（低 4 位保留）；Y 另加面板 5 像素偏移 */
	out->x = (uint16_t)(((buf[1] & 0x0F) << 8) | buf[2]);
	out->y = (uint16_t)((((buf[3] & 0x0F) << 8) | buf[4]) + BSP_TOUCH_Y_OFFSET);

	if (out->x >= LCD_W) {
		out->x = LCD_W - 1;
	}
	if (out->y >= LCD_H) {
		out->y = LCD_H - 1;
	}

	return ESP_OK;
}

/**
 * 读取手势寄存器 0x01。
 * @param gesture 输出指针，不能为 NULL。
 * @return ESP_OK 或总线错误码。
 * 调用上下文：任务上下文。
 */
esp_err_t cst816_read_gesture(uint8_t *gesture)
{
	if (gesture == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	return cst816_read_regs(CST816_REG_GESTURE, gesture, 1);
}

/**
 * 读取芯片 ID（0xA7）。
 * @param id 输出指针，不能为 NULL。
 * @return ESP_OK 或总线错误码（NACK 时为 ESP_ERR_INVALID_RESPONSE）。
 * 调用上下文：任务上下文。
 */
esp_err_t cst816_read_chip_id(uint8_t *id)
{
	if (id == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	return cst816_read_regs(CST816_REG_CHIP_ID, id, 1);
}

/**
 * 让芯片进入睡眠（0xE5=0x03）。
 * @return ESP_OK 或总线错误码。
 * 调用上下文：任务上下文；进入睡眠后需硬复位才能唤醒。
 */
esp_err_t cst816_sleep(void)
{
	return cst816_write_reg(CST816_REG_SLEEP_MODE, 0x03);
}

/**
 * 硬复位唤醒芯片。
 * @return 固定 ESP_OK（gpio 操作无失败返回）；复位成功与否需由后续读 ID 验证。
 * 调用上下文：任务上下文。
 */
esp_err_t cst816_wakeup(void)
{
	cst816_hw_reset();
	return ESP_OK;
}
