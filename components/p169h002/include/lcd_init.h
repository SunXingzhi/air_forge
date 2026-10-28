#ifndef __LCD_INIT_H
#define __LCD_INIT_H

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "board_pins.h"

#define LCD_W BSP_LCD_H_RES
#define LCD_H BSP_LCD_V_RES

extern spi_device_handle_t spi;

#define LCD_RES_Clr()   gpio_set_level(BSP_LCD_PIN_RST, 0)
#define LCD_RES_Set()   gpio_set_level(BSP_LCD_PIN_RST, 1)

#define LCD_DC_Clr()    gpio_set_level(BSP_LCD_PIN_DC, 0)
#define LCD_DC_Set()    gpio_set_level(BSP_LCD_PIN_DC, 1)

#define LCD_BLK_Clr()   gpio_set_level(BSP_LCD_PIN_BL, BSP_LCD_BL_OFF_LEVEL)
#define LCD_BLK_Set()   gpio_set_level(BSP_LCD_PIN_BL, BSP_LCD_BL_ON_LEVEL)

void LCD_Init(void);
void LCD_Backlight(bool on);
void LCD_Writ_Bus(uint8_t dat);
void LCD_WR_DATA(uint16_t dat);
void LCD_WR_REG(uint8_t dat);
void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

#endif
