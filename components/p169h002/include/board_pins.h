#pragma once

#include "driver/spi_master.h"
#include "driver/i2c_master.h"

/*
 * P169H002-CTP + 14 pin FPC 转接板，接到 ESP32-S3-DevKitC-1。
 *
 * 引脚选择避开了：GPIO0/3/45/46（strapping）、GPIO19/20（USB）、
 * GPIO26~37（Flash/Octal PSRAM）、GPIO43/44（UART0）。
 * 与转接板丝印的对应关系见 Doc/ESP32-IDF开发文档-v1.0.md §6.2.1。
 */

/* ---------- LCD：4 线 SPI，只有一根双向 SDA（DSA） ---------- */
#define BSP_LCD_SPI_HOST        SPI2_HOST
#define BSP_LCD_PIN_SCLK        12
#define BSP_LCD_PIN_MOSI        11
#define BSP_LCD_PIN_CS          13
#define BSP_LCD_PIN_DC          14
#define BSP_LCD_PIN_RST         15
#define BSP_LCD_PIN_BL          16
/* 厂商例程 LCD_BLK_Set() 为高电平开背光；若实测 PWR 低有效，把 ON/OFF 对调 */
#define BSP_LCD_BL_ON_LEVEL     1
#define BSP_LCD_BL_OFF_LEVEL    0

#define BSP_LCD_H_RES           240
#define BSP_LCD_V_RES           280
#define BSP_LCD_Y_OFFSET        20      /* 厂商例程：竖屏写 GRAM 时 y+20 */
#define BSP_LCD_SPI_CLOCK_HZ    SPI_MASTER_FREQ_10M

/* ---------- Touch：CST816T，I2C ---------- */
#define BSP_TOUCH_I2C_PORT      I2C_NUM_0
#define BSP_TOUCH_PIN_SDA       8
#define BSP_TOUCH_PIN_SCL       9
#define BSP_TOUCH_PIN_INT       10
#define BSP_TOUCH_PIN_RST       18
#define BSP_TOUCH_I2C_ADDR      0x15
#define BSP_TOUCH_I2C_HZ        100000  /* 首次点亮用 100 kHz；稳定后可改 400 kHz */
#define BSP_TOUCH_Y_OFFSET      5       /* 厂商例程：触摸 y+5 */

/*
 * ---------- 模拟传感器：GP2Y1014AU0F（PM2.5）+ MS1100（VOC） ----------
 *
 * 两个传感器均为 5 V 器件，禁止接 3.3 V 轨。5 V 取自 DevKitC-1 的 5V 引脚
 * （USB VBUS）；若改用外部电源，必须与主控共地。总电流约 150 mA
 * （MS1100 加热 80 mA + GP2Y 20 mA + LED 脉冲 20 mA），USB 供电足够。
 *
 * 两者输出都可能超过 ESP32-S3 ADC 的 3.3 V 上限，必须做 2:1 分压，
 * ADC 端看到的电压 = 传感器输出 / 2：
 *   - GP2Y 是 100 Hz 脉冲输出，分压后必须快速建立：4.7 kΩ 上臂 + 4.7 kΩ
 *     下臂，分压点对 GND 并 10 nF（τ≈24 µs，280 µs 采样点时已充分建立）；
 *     该网络对 Vo 的等效负载约 9.4 kΩ，与规格书 RL=4.7 kΩ 的测试条件同量级。
 *   - MS1100 输出源阻抗较高（模块内部负载约 20 kΩ），分压电阻不能太小，
 *     否则会把 AO 拉低：100 kΩ 上臂 + 100 kΩ 下臂，分压点对 GND 并 10 nF；
 *     200 kΩ 外部负载对模块输出的影响约 9%，在手册 ±7% 误差附近，标定时一并修正。
 *
 * GP2Y1014AU0F（6 pin）接线：
 *   1 V-LED   → 5 V 经 150 Ω；V-LED 与 GND 之间并 220 µF（电解电容正极接 V-LED）
 *   2 LED-GND → GND
 *   3 LED     → NPN 集电极（如 2N3904）；发射极 → GND；基极经 1 kΩ → GPIO21，
 *               基极-发射极并 10 kΩ 下拉。GPIO 高电平 → LED 点亮（见下方电平宏）
 *   4 S-GND   → GND（与数字地单点汇聚，靠近传感器）
 *   5 Vo      → 2:1 分压 → GPIO2（ADC1_CH1）
 *   6 Vcc     → 5 V，就近 100 nF + 10 µF 去耦
 *
 * MS1100 模块（4 pin，KQ-2801 传感器 + 比较器）接线：
 *   VCC → 5 V；GND → GND；AO → 2:1 分压 → GPIO1（ADC1_CH0）；DO 悬空不用
 *
 * 采样约束：GP2Y 的 LED 必须以 100 Hz、0.32 ms 脉宽脉冲驱动（本驱动按
 * 点亮后 280 µs 采样）；ADC1 与 Wi-Fi 共用，必须使用 ADC1（ADC2 与 Wi-Fi 冲突）。
 */

/* ADC1 通道号：CH0=GPIO1，CH1=GPIO2，CH2=GPIO3……不可任意重映射 */
#define BSP_MS1100_ADC_CHANNEL   0
#define BSP_MS1100_ADC_GPIO      1
#define BSP_MS1100_DIVIDER_RATIO 2.0f

#define BSP_PM_ADC_CHANNEL       1
#define BSP_PM_ADC_GPIO          2
#define BSP_PM_DIVIDER_RATIO     2.0f

#define BSP_PM_LED_GPIO          21
#define BSP_PM_LED_ON_LEVEL      1      /* NPN 基极高电平导通 → 传感器 LED 点亮 */
#define BSP_PM_LED_OFF_LEVEL     0
