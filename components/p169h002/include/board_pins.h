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
