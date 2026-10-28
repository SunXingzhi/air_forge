#include "demo.h"

#include <stdbool.h>
#include <stdint.h>

#include "cst816.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd.h"

static const char *TAG = "P169H002";

/* "浦洋液晶" 的 GB2312 编码；lcdfont.h 的点阵表按 GB2312 索引，不能用 UTF-8 字面量 */
static const uint8_t k_cn_polcd[] = {0xC6, 0xD6, 0xD1, 0xF3, 0xD2, 0xBA, 0xBE, 0xA7, 0x00};

#define BTN_W 72
#define BTN_H 36

static void draw_clear_button(void)
{
    LCD_Fill(LCD_W - BTN_W, LCD_H - BTN_H, LCD_W - 1, LCD_H - 1, GRAY);
    LCD_ShowString(LCD_W - BTN_W + 8, LCD_H - BTN_H + 10,
                   (const uint8_t *)"Clear", BLACK, GRAY, 16, 0);
}

static bool in_clear_button(uint16_t x, uint16_t y)
{
    return (x >= (LCD_W - BTN_W)) && (y >= (LCD_H - BTN_H));
}

static void show_title(void)
{
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    LCD_ShowString(12, 12, (const uint8_t *)"AirForge", CYAN, BLACK, 24, 0);
    LCD_ShowString(12, 48, (const uint8_t *)"P169H002 240x280", WHITE, BLACK, 16, 0);
    LCD_ShowString(12, 72, (const uint8_t *)"ST7789T3 + CST816T", WHITE, BLACK, 16, 0);
    LCD_ShowChinese(12, 112, (uint8_t *)k_cn_polcd, YELLOW, BLACK, 32, 0);
    LCD_ShowString(12, 164, (const uint8_t *)"Touch to draw", GREEN, BLACK, 16, 0);
}

void p169h002_demo(void)
{
    LCD_Init();

    esp_err_t err = cst816_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "touch init failed: %s", esp_err_to_name(err));
    }

    /* 先清黑再开背光，避免上电白闪 */
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    vTaskDelay(pdMS_TO_TICKS(50));
    LCD_Backlight(true);

    ESP_LOGI(TAG, "demo start: %dx%d, SPI %d Hz", LCD_W, LCD_H, BSP_LCD_SPI_CLOCK_HZ);

    show_title();
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* 纯色测试：红/绿/蓝/白/黑，用于检查 BGR 与反色 */
    const uint16_t colors[] = {RED, GREEN, BLUE, WHITE, BLACK};
    for (uint32_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i) {
        LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, colors[i]);
        vTaskDelay(pdMS_TO_TICKS(800));
    }

    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    DrawColorBars();
    vTaskDelay(pdMS_TO_TICKS(3000));

    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    DrawGrayscale();
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* 触摸画板 */
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    draw_clear_button();

    uint16_t last_x = 0xFFFF;
    uint16_t last_y = 0xFFFF;
    uint32_t next_log_ms = 0;

    while (1) {
        cst816_touch_t touch = {0};
        esp_err_t ret = cst816_read_touch(&touch);

        if (ret == ESP_OK && touch.pressed) {
            uint16_t x = touch.x;
            uint16_t y = touch.y;

            if (in_clear_button(x, y)) {
                LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
                draw_clear_button();
                last_x = last_y = 0xFFFF;
            } else {
                if (x < 3) {
                    x = 3;
                }
                if (y < 3) {
                    y = 3;
                }
                if (x > LCD_W - 4) {
                    x = LCD_W - 4;
                }
                if (y > LCD_H - 4) {
                    y = LCD_H - 4;
                }

                if (last_x != 0xFFFF) {
                    LCD_DrawThickLine(last_x, last_y, x, y, WHITE, 3);
                } else {
                    LCD_DrawPoint(x, y, WHITE);
                }
                last_x = x;
                last_y = y;
            }

            uint32_t now = esp_log_timestamp();
            if (now >= next_log_ms) {
                ESP_LOGI(TAG, "touch (%u,%u) fingers=%u", touch.x, touch.y, touch.fingers);
                next_log_ms = now + 500;
            }
        } else {
            last_x = last_y = 0xFFFF;
        }

        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "touch read failed: %s", esp_err_to_name(ret));
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
