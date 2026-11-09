#include "adc_frontend.h"

#include <stdbool.h>
#include <stddef.h>

#include "board_pins.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "ADC_FE";

/* 前端固定支持两路：CH0（MS1100）与 CH1（GP2Y）；通道号来自 board_pins.h */
#define ADC_FE_CHANNEL_COUNT 2

typedef struct {
	adc_channel_t channel;
	adc_cali_handle_t cali;
	bool configured;
} adc_fe_channel_t;

static adc_oneshot_unit_handle_t s_unit = NULL;
static adc_fe_channel_t s_channels[ADC_FE_CHANNEL_COUNT];
static bool s_ready = false;

/**
 * 按通道号找到槽位。
 * @param channel 目标 ADC1 通道。
 * @return 槽位下标；未配置的通道返回 -1。
 * 调用上下文：任务上下文，s_channels 初始化后只读。
 */
static int adc_fe_slot(adc_channel_t channel)
{
	for (int i = 0; i < ADC_FE_CHANNEL_COUNT; ++i) {
		if (s_channels[i].channel == channel) {
			return i;
		}
	}
	return -1;
}

/**
 * 配置单个通道并尝试建立校准句柄。
 * @param slot  s_channels 下标；内部会写入 channel/configured/cali。
 * @return ESP_OK；通道配置失败透传错误码。校准失败只记录告警，仍返回 ESP_OK。
 * 调用上下文：仅 adc_frontend_init() 内调用一次，不并发。
 */
static esp_err_t adc_fe_config_channel(int slot)
{
	adc_oneshot_chan_cfg_t chan_cfg = {
		.atten = ADC_ATTEN_DB_12,     /* 约 0~3.3 V 量程，覆盖分压后的信号 */
		.bitwidth = ADC_BITWIDTH_12,
	};

	esp_err_t err = adc_oneshot_config_channel(s_unit, s_channels[slot].channel, &chan_cfg);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "adc_oneshot_config_channel(ch%d) failed: %s",
		         (int)s_channels[slot].channel, esp_err_to_name(err));
		return err;
	}

	/* 校准按通道创建：ESP32-S3 的曲线拟合补偿与具体通道相关 */
	adc_cali_curve_fitting_config_t cali_cfg = {
		.unit_id = ADC_UNIT_1,
		.chan = s_channels[slot].channel,
		.atten = ADC_ATTEN_DB_12,
		.bitwidth = ADC_BITWIDTH_12,
	};
	if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_channels[slot].cali) != ESP_OK) {
		/* eFuse 未校准或方案不可用时退回线性换算，保证驱动仍能出数 */
		s_channels[slot].cali = NULL;
		ESP_LOGW(TAG, "ch%d calibration unavailable, fallback to linear", s_channels[slot].channel);
	}

	s_channels[slot].configured = true;
	return ESP_OK;
}

esp_err_t adc_frontend_init(void)
{
	if (s_ready) {
		return ESP_OK;
	}

	adc_oneshot_unit_init_cfg_t unit_cfg = {
		.unit_id = ADC_UNIT_1,
		/* clk_src 留 0 由驱动选择默认时钟（ESP32-S3 为 APB） */
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};

	esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_unit);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "adc1 unit create failed: %s", esp_err_to_name(err));
		s_unit = NULL;
		return err;
	}

	s_channels[0] = (adc_fe_channel_t){.channel = (adc_channel_t)BSP_MS1100_ADC_CHANNEL};
	s_channels[1] = (adc_fe_channel_t){.channel = (adc_channel_t)BSP_PM_ADC_CHANNEL};

	for (int i = 0; i < ADC_FE_CHANNEL_COUNT; ++i) {
		err = adc_fe_config_channel(i);
		if (err != ESP_OK) {
			ESP_LOGE(TAG, "config ch%d failed: %s", (int)s_channels[i].channel,
			         esp_err_to_name(err));
			adc_oneshot_del_unit(s_unit);
			s_unit = NULL;
			return err;
		}
	}

	s_ready = true;
	ESP_LOGI(TAG, "ADC1 ready: ch%d=GPIO%d(MS1100), ch%d=GPIO%d(GP2Y)",
	         BSP_MS1100_ADC_CHANNEL, BSP_MS1100_ADC_GPIO, BSP_PM_ADC_CHANNEL, BSP_PM_ADC_GPIO);
	return ESP_OK;
}

esp_err_t adc_frontend_read(adc_channel_t channel, adc_frontend_sample_t *out)
{
	if (out == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!s_ready) {
		return ESP_ERR_INVALID_STATE;
	}

	int slot = adc_fe_slot(channel);
	if (slot < 0) {
		return ESP_ERR_INVALID_ARG;
	}

	int raw = 0;
	esp_err_t err = adc_oneshot_read(s_unit, channel, &raw);
	if (err != ESP_OK) {
		return err;
	}

	int mv = 0;
	if (s_channels[slot].cali != NULL) {
		err = adc_cali_raw_to_voltage(s_channels[slot].cali, raw, &mv);
		if (err != ESP_OK) {
			return err;
		}
	} else {
		/* 线性回退：12 dB 衰减近似满量程 3300 mV */
		mv = (int)((float)raw * 3300.0f / 4095.0f);
	}

	out->raw = raw;
	out->pin_mv = (float)mv;
	return ESP_OK;
}
