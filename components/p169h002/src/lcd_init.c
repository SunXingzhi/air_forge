#include "lcd_init.h"

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LCD";

spi_device_handle_t spi = NULL;

static const spi_bus_config_t s_buscfg = {
    .miso_io_num = -1,              /* 4 线 SPI 没有 MISO */
    .mosi_io_num = BSP_LCD_PIN_MOSI,
    .sclk_io_num = BSP_LCD_PIN_SCLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4096,
};

static const spi_device_interface_config_t s_devcfg = {
    .clock_speed_hz = BSP_LCD_SPI_CLOCK_HZ,
    .mode = 0,
    .spics_io_num = BSP_LCD_PIN_CS,
    .queue_size = 7,
    .flags = SPI_DEVICE_HALFDUPLEX,
};

static void LCD_GPIO_Init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BSP_LCD_PIN_RST) |
                        (1ULL << BSP_LCD_PIN_DC) |
                        (1ULL << BSP_LCD_PIN_BL),
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    LCD_RES_Set();
    LCD_DC_Set();
    LCD_BLK_Clr();                  /* 先关背光，等清屏后再开，避免白闪 */
}

static void LCD_Bus_Init(void)
{
    ESP_ERROR_CHECK(spi_bus_initialize(BSP_LCD_SPI_HOST, &s_buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(BSP_LCD_SPI_HOST, &s_devcfg, &spi));
}

void LCD_Writ_Bus(uint8_t dat)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &dat;
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
}

void LCD_WR_DATA(uint16_t dat)
{
    spi_transaction_t t;
    uint8_t tx_buf[2] = {(uint8_t)(dat >> 8), (uint8_t)(dat & 0xFF)};
    memset(&t, 0, sizeof(t));
    t.length = 16;
    t.tx_buffer = tx_buf;
    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &t));
}

void LCD_WR_REG(uint8_t dat)
{
    LCD_DC_Clr();
    LCD_Writ_Bus(dat);
    LCD_DC_Set();
}

void LCD_Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    LCD_WR_REG(0x2A);
    LCD_WR_DATA(x1);
    LCD_WR_DATA(x2);

    LCD_WR_REG(0x2B);
    LCD_WR_DATA((uint16_t)(y1 + BSP_LCD_Y_OFFSET));
    LCD_WR_DATA((uint16_t)(y2 + BSP_LCD_Y_OFFSET));

    LCD_WR_REG(0x2C);
}

void LCD_Backlight(bool on)
{
    if (on) {
        LCD_BLK_Set();
    } else {
        LCD_BLK_Clr();
    }
}

void LCD_Init(void)
{
    LCD_GPIO_Init();
    LCD_Bus_Init();

    LCD_RES_Clr();
    vTaskDelay(pdMS_TO_TICKS(100));
    LCD_RES_Set();
    vTaskDelay(pdMS_TO_TICKS(100));

    LCD_WR_REG(0x11);               /* Sleep out */
    vTaskDelay(pdMS_TO_TICKS(120));

    LCD_WR_REG(0x36);               /* MADCTL：竖屏 */
    LCD_WR_DATA(0x00);

    LCD_WR_REG(0x3A);               /* COLMOD：RGB565 */
    LCD_WR_DATA(0x05);

    LCD_WR_REG(0xB2);               /* Porch setting */
    LCD_WR_DATA(0x0C);
    LCD_WR_DATA(0x0C);
    LCD_WR_DATA(0x00);
    LCD_WR_DATA(0x33);
    LCD_WR_DATA(0x33);

    LCD_WR_REG(0xB7);               /* Gate control */
    LCD_WR_DATA(0x35);

    LCD_WR_REG(0xBB);               /* VCOM */
    LCD_WR_DATA(0x32);

    LCD_WR_REG(0xC2);               /* VDV/VRH enable */
    LCD_WR_DATA(0x01);

    LCD_WR_REG(0xC3);               /* VRH */
    LCD_WR_DATA(0x15);

    LCD_WR_REG(0xC4);               /* VDV */
    LCD_WR_DATA(0x20);

    LCD_WR_REG(0xC6);               /* Frame rate 60 Hz */
    LCD_WR_DATA(0x0F);

    LCD_WR_REG(0xD0);               /* Power control */
    LCD_WR_DATA(0xA4);
    LCD_WR_DATA(0xA1);

    LCD_WR_REG(0xE0);               /* Positive gamma */
    LCD_WR_DATA(0xD0);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x0E);
    LCD_WR_DATA(0x09);
    LCD_WR_DATA(0x09);
    LCD_WR_DATA(0x05);
    LCD_WR_DATA(0x31);
    LCD_WR_DATA(0x33);
    LCD_WR_DATA(0x48);
    LCD_WR_DATA(0x17);
    LCD_WR_DATA(0x14);
    LCD_WR_DATA(0x15);
    LCD_WR_DATA(0x31);
    LCD_WR_DATA(0x34);

    LCD_WR_REG(0xE1);               /* Negative gamma */
    LCD_WR_DATA(0xD0);
    LCD_WR_DATA(0x08);
    LCD_WR_DATA(0x0E);
    LCD_WR_DATA(0x09);
    LCD_WR_DATA(0x09);
    LCD_WR_DATA(0x15);
    LCD_WR_DATA(0x31);
    LCD_WR_DATA(0x33);
    LCD_WR_DATA(0x48);
    LCD_WR_DATA(0x17);
    LCD_WR_DATA(0x14);
    LCD_WR_DATA(0x15);
    LCD_WR_DATA(0x31);
    LCD_WR_DATA(0x34);

    LCD_WR_REG(0x21);               /* Display inversion on（厂商序列） */
    LCD_WR_REG(0x29);               /* Display on */

    ESP_LOGI(TAG, "ST7789T3 init done (%dx%d, y_offset=%d, SPI=%d Hz)",
             LCD_W, LCD_H, BSP_LCD_Y_OFFSET, BSP_LCD_SPI_CLOCK_HZ);
    ESP_LOGI(TAG, "pins: SCLK=%d MOSI=%d CS=%d DC=%d RST=%d BL=%d (BL on level=%d)",
             BSP_LCD_PIN_SCLK, BSP_LCD_PIN_MOSI, BSP_LCD_PIN_CS,
             BSP_LCD_PIN_DC, BSP_LCD_PIN_RST, BSP_LCD_PIN_BL,
             BSP_LCD_BL_ON_LEVEL);
}
