#include "gp2y1014.h"

#include <stddef.h>

#include "adc_frontend.h"
#include "board_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

static const char *TAG = "GP2Y1014";

/* 规格书要求点灯后 280 µs 采样；这一段必须是精确短延时，不能用 vTaskDelay */
#define GP2Y1014_SAMPLE_DELAY_US 280

/* 标定参数：初始化默认值来自规格书典型值，运行期由 gp2y1014_set_calibration() 覆盖 */
static float s_zero_mv = GP2Y1014_DEFAULT_ZERO_MV;
static float s_k_mv_per_100ug = GP2Y1014_DEFAULT_K_MV_PER_100UG;

static bool s_initialized = false;
static int64_t s_init_us = 0;

/**
 * 配置 LED_DRV 为推挽输出并保持熄灭。
 * @return ESP_OK；GPIO 配置失败时透传错误码。
 * 调用上下文：仅 gp2y1014_init() 调用一次。
 */
static esp_err_t gp2y1014_led_init(void)
{
	gpio_config_t io = {
		.pin_bit_mask = (1ULL << BSP_PM_LED_GPIO),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	esp_err_t err = gpio_config(&io);
	if (err != ESP_OK) {
		return err;
	}

	/* 上电先保持 LED 熄灭，避免初始化期间 LED 长亮拉低 5 V 轨 */
	return gpio_set_level(BSP_PM_LED_GPIO, BSP_PM_LED_OFF_LEVEL);
}

esp_err_t gp2y1014_init(void)
{
	if (s_initialized) {
		return ESP_OK;
	}

	esp_err_t err = gp2y1014_led_init();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "led gpio init failed: %s", esp_err_to_name(err));
		return err;
	}

	err = adc_frontend_init();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "adc frontend init failed: %s", esp_err_to_name(err));
		return err;
	}

	s_init_us = esp_timer_get_time();
	s_initialized = true;
	ESP_LOGI(TAG, "init ok: LED=GPIO%d, Vo=GPIO%d (2:1 divider)",
	         BSP_PM_LED_GPIO, BSP_PM_ADC_GPIO);
	return ESP_OK;
}

float gp2y1014_convert_mv(float sensor_mv)
{
	if (s_k_mv_per_100ug <= 0.0f) {
		return 0.0f;
	}

	/* 规格书线性关系：浓度 = (Vo − Voc) × 100 / K；低于 Voc 视为 0 */
	float ug = (sensor_mv - s_zero_mv) * 100.0f / s_k_mv_per_100ug;
	if (ug < 0.0f) {
		ug = 0.0f;
	}
	if (ug > GP2Y1014_MAX_UG_M3) {
		ug = GP2Y1014_MAX_UG_M3;
	}
	return ug;
}

esp_err_t gp2y1014_read(gp2y1014_sample_t *out)
{
	if (out == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!s_initialized) {
		return ESP_ERR_INVALID_STATE;
	}

	adc_frontend_sample_t adc = {0};

	/* 状态迁移：LED 熄灭 → 点亮 → 固定 280 µs 等待散射光建立 → ADC 采样 → 熄灭 */
	gpio_set_level(BSP_PM_LED_GPIO, BSP_PM_LED_ON_LEVEL);
	esp_rom_delay_us(GP2Y1014_SAMPLE_DELAY_US);
	esp_err_t err = adc_frontend_read((adc_channel_t)BSP_PM_ADC_CHANNEL, &adc);
	gpio_set_level(BSP_PM_LED_GPIO, BSP_PM_LED_OFF_LEVEL);

	if (err != ESP_OK) {
		ESP_LOGE(TAG, "adc read failed: %s", esp_err_to_name(err));
		return err;
	}

	/* 分压还原：传感器端电压 = ADC 引脚电压 × 2 */
	float sensor_mv = adc.pin_mv * BSP_PM_DIVIDER_RATIO;

	out->raw = adc.raw;
	out->pin_mv = adc.pin_mv;
	out->sensor_mv = sensor_mv;
	out->dust_ug_m3 = gp2y1014_convert_mv(sensor_mv);
	out->preheated = gp2y1014_is_preheated();
	return ESP_OK;
}

void gp2y1014_set_calibration(float zero_mv, float k_mv_per_100ug)
{
	if (k_mv_per_100ug <= 0.0f) {
		ESP_LOGW(TAG, "invalid K=%.1f, keep current calibration", k_mv_per_100ug);
		return;
	}
	s_zero_mv = zero_mv;
	s_k_mv_per_100ug = k_mv_per_100ug;
	ESP_LOGI(TAG, "calibration: Voc=%.1f mV, K=%.1f mV/100ug", s_zero_mv, s_k_mv_per_100ug);
}

void gp2y1014_get_calibration(float *zero_mv, float *k_mv_per_100ug)
{
	if (zero_mv != NULL) {
		*zero_mv = s_zero_mv;
	}
	if (k_mv_per_100ug != NULL) {
		*k_mv_per_100ug = s_k_mv_per_100ug;
	}
}

bool gp2y1014_is_preheated(void)
{
	if (!s_initialized) {
		return false;
	}
	return (esp_timer_get_time() - s_init_us) >= (GP2Y1014_PREHEAT_MS * 1000LL);
}
