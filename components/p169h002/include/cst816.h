#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/* CST816T 寄存器（来自厂商 P169H002 例程） */
#define CST816_REG_GESTURE      0x01
#define CST816_REG_FINGER_NUM   0x02
#define CST816_REG_X_H          0x03
#define CST816_REG_X_L          0x04
#define CST816_REG_Y_H          0x05
#define CST816_REG_Y_L          0x06
#define CST816_REG_CHIP_ID      0xA7
#define CST816_REG_SLEEP_MODE   0xE5
#define CST816_REG_AUTO_SLEEP   0xF9

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t fingers;
    bool pressed;
} cst816_touch_t;

esp_err_t cst816_init(void);
esp_err_t cst816_read_touch(cst816_touch_t *out);
esp_err_t cst816_read_gesture(uint8_t *gesture);
esp_err_t cst816_read_chip_id(uint8_t *id);
esp_err_t cst816_sleep(void);
esp_err_t cst816_wakeup(void);
