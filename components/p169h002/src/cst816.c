#include "cst816.h"

#include "board_pins.h"
#include "lcd_init.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "CST816";

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_dev = NULL;

static esp_err_t cst816_read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, len, 100);
}

static esp_err_t cst816_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 100);
}

static void cst816_rst_line_test(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << BSP_TOUCH_PIN_RST),
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io);
    vTaskDelay(pdMS_TO_TICKS(2));
    int pull_up = gpio_get_level(BSP_TOUCH_PIN_RST);

    io.pull_up_en = GPIO_PULLUP_DISABLE;
    io.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io);
    vTaskDelay(pdMS_TO_TICKS(2));
    int pull_down = gpio_get_level(BSP_TOUCH_PIN_RST);

    ESP_LOGI(TAG, "TRST line test: pull-up=%d pull-down=%d "
                  "(1/0=high-Z, 0/0=short GND, 1/1=short VCC)",
             pull_up, pull_down);

    io.mode = GPIO_MODE_INPUT_OUTPUT;
    io.pull_up_en = GPIO_PULLUP_DISABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io);
}

static void cst816_hw_reset(void)
{
    gpio_set_level(BSP_TOUCH_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(BSP_TOUCH_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void cst816_i2c_scan(int sda, int scl)
{
    if (s_bus == NULL) {
        ESP_LOGW(TAG, "i2c bus not initialized; skip scan");
        return;
    }

    int found = 0;
    for (uint16_t addr = 0x08; addr < 0x78; ++addr) {
        if (i2c_master_probe(s_bus, addr, 20) == ESP_OK) {
            ESP_LOGI(TAG, "i2c device found at 0x%02X (SDA=%d SCL=%d)", addr, sda, scl);
            ++found;
        }
    }
    if (found == 0) {
        ESP_LOGW(TAG, "no i2c device on SDA=%d SCL=%d; check VCC/GND, 4.7k pull-ups, "
                      "TSDA/TSCL swap, TRST wiring",
                 sda, scl);
    }
}

static esp_err_t cst816_bus_create(int sda, int scl)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = BSP_TOUCH_I2C_PORT,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&bus_cfg, &s_bus), TAG, "i2c bus init failed");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BSP_TOUCH_I2C_ADDR,
        .scl_speed_hz = BSP_TOUCH_I2C_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev), TAG,
                        "i2c device add failed");
    return ESP_OK;
}

static void cst816_bus_destroy(void)
{
    if (s_dev != NULL) {
        i2c_master_bus_rm_device(s_dev);
        s_dev = NULL;
    }
    if (s_bus != NULL) {
        i2c_del_master_bus(s_bus);
        s_bus = NULL;
    }
}

esp_err_t cst816_init(void)
{
    gpio_config_t rst_conf = {
        .pin_bit_mask = (1ULL << BSP_TOUCH_PIN_RST),
        .intr_type = GPIO_INTR_DISABLE,
        /* INPUT_OUTPUT 才能用 gpio_get_level 回读实际电平；纯 OUTPUT 的输入缓冲被关掉 */
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&rst_conf), TAG, "reset gpio config failed");

    gpio_config_t int_conf = {
        .pin_bit_mask = (1ULL << BSP_TOUCH_PIN_INT),
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&int_conf), TAG, "int gpio config failed");

    ESP_LOGI(TAG, "pins: SDA=%d SCL=%d RST=%d INT=%d addr=0x%02X @%d Hz",
             BSP_TOUCH_PIN_SDA, BSP_TOUCH_PIN_SCL, BSP_TOUCH_PIN_RST,
             BSP_TOUCH_PIN_INT, BSP_TOUCH_I2C_ADDR, BSP_TOUCH_I2C_HZ);

    esp_err_t err = cst816_bus_create(BSP_TOUCH_PIN_SDA, BSP_TOUCH_PIN_SCL);
    if (err != ESP_OK) {
        return err;
    }

    ESP_LOGI(TAG, "bus idle levels: SDA=%d SCL=%d (expect 1/1 with pull-ups)",
             gpio_get_level(BSP_TOUCH_PIN_SDA), gpio_get_level(BSP_TOUCH_PIN_SCL));

    cst816_rst_line_test();
    cst816_hw_reset();
    ESP_LOGI(TAG, "TRST level after release: %d", gpio_get_level(BSP_TOUCH_PIN_RST));

    uint8_t id = 0;
    err = cst816_read_regs(CST816_REG_CHIP_ID, &id, 1);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "chip id = 0x%02X", id);
    } else {
        /* 先按正常 SDA/SCL 扫描；仍无设备时自动尝试交换两根线 */
        ESP_LOGW(TAG, "chip id read failed: %s", esp_err_to_name(err));
        cst816_i2c_scan(BSP_TOUCH_PIN_SDA, BSP_TOUCH_PIN_SCL);

        ESP_LOGW(TAG, "retry with SDA/SCL swapped (%d/%d)",
                 BSP_TOUCH_PIN_SCL, BSP_TOUCH_PIN_SDA);
        cst816_bus_destroy();

        err = cst816_bus_create(BSP_TOUCH_PIN_SCL, BSP_TOUCH_PIN_SDA);
        if (err == ESP_OK) {
            cst816_hw_reset();
            err = cst816_read_regs(CST816_REG_CHIP_ID, &id, 1);
        }
        if (err == ESP_OK) {
            ESP_LOGW(TAG, "chip id = 0x%02X with swapped SDA/SCL; rewire TSDA/TSCL "
                          "or keep this mapping", id);
        } else {
            ESP_LOGW(TAG, "swapped SDA/SCL also failed: %s", esp_err_to_name(err));
            cst816_i2c_scan(BSP_TOUCH_PIN_SCL, BSP_TOUCH_PIN_SDA);
            cst816_bus_destroy();
            err = cst816_bus_create(BSP_TOUCH_PIN_SDA, BSP_TOUCH_PIN_SCL);
            if (err != ESP_OK) {
                return err;
            }
        }
    }

    /* 厂商例程：5 s 无触摸进入低功耗；首次触摸会唤醒 */
    err = cst816_write_reg(CST816_REG_AUTO_SLEEP, 5);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "auto-sleep config failed: %s", esp_err_to_name(err));
    }

    return ESP_OK;
}

esp_err_t cst816_read_touch(cst816_touch_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buf[5] = {0};   /* 0x02 手指, 0x03/0x04 X, 0x05/0x06 Y */
    esp_err_t err = cst816_read_regs(CST816_REG_FINGER_NUM, buf, sizeof(buf));
    if (err != ESP_OK) {
        out->pressed = false;
        out->fingers = 0;
        return err;
    }

    out->fingers = (buf[0] == 0xFF) ? 0 : buf[0];   /* 0xFF = 睡眠 */
    out->pressed = (out->fingers > 0);
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

esp_err_t cst816_read_gesture(uint8_t *gesture)
{
    if (gesture == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return cst816_read_regs(CST816_REG_GESTURE, gesture, 1);
}

esp_err_t cst816_read_chip_id(uint8_t *id)
{
    if (id == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return cst816_read_regs(CST816_REG_CHIP_ID, id, 1);
}

esp_err_t cst816_sleep(void)
{
    return cst816_write_reg(CST816_REG_SLEEP_MODE, 0x03);
}

esp_err_t cst816_wakeup(void)
{
    cst816_hw_reset();
    return ESP_OK;
}
