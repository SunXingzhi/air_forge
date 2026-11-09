#include "ms1100.h"

#include <stddef.h>

#include "adc_frontend.h"
#include "board_pins.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

static const char *TAG = "MS1100";

/*
 * 模块手册提供的输出电压—浓度对照表（MS1100-P111，RL=20 kΩ，25 ℃，无温湿度补偿）。
 * 电压单位 mV，浓度单位 ppm。表按电压升序排列，用于分段线性插值。
 */
typedef struct {
	uint16_t mv;
	uint16_t ppm;
} ms1100_point_t;

static const ms1100_point_t k_curve_formaldehyde[] = {
	{640, 0},   {2100, 1},  {2380, 2},  {2570, 3},  {2710, 4},  {2820, 5},
	{2920, 6},  {3010, 7},  {3090, 8},  {3160, 9},  {3220, 10}, {3490, 15},
	{3690, 20}, {3870, 25}, {4020, 30}, {4150, 35}, {4270, 40}, {4380, 45},
	{4480, 50}, {4570, 55}, {4660, 60}, {4740, 65}, {4820, 70}, {4900, 75},
};

static const ms1100_point_t k_curve_toluene[] = {
	{600, 0},   {3270, 1},  {3470, 2},  {3600, 3},  {3690, 4},  {3770, 5},
	{3830, 6},  {3880, 7},  {3930, 8},  {3980, 9},  {4010, 10}, {4170, 15},
	{4290, 20}, {4380, 25}, {4460, 30}, {4530, 35}, {4590, 40}, {4650, 45},
	{4700, 50}, {4750, 55}, {4790, 60}, {4830, 65}, {4860, 70}, {4900, 75},
};

#define MS1100_CURVE_POINTS(arr) (sizeof(arr) / sizeof((arr)[0]))

/* 运行期标定参数；默认 RL 取规格书典型值 20 kΩ */
static float s_rl_ohm = MS1100_DEFAULT_RL_OHM;
static float s_r0_ohm = 0.0f;
static bool s_r0_valid = false;
static ms1100_curve_t s_curve = MS1100_CURVE_FORMALDEHYDE;

static bool s_initialized = false;
static int64_t s_init_us = 0;

/**
 * 对一张升序电压表做分段线性插值。
 * @param table 电压—浓度表；@param count 表长；@param mv 模块 AO 电压（mV）。
 * @return 插值 ppm；低于表起点返回 0，高于表终点返回末点浓度。
 * 调用上下文：任意任务上下文，只读常量表。
 */
static float ms1100_interpolate(const ms1100_point_t *table, size_t count, float mv)
{
	if (count == 0) {
		return 0.0f;
	}
	if (mv <= (float)table[0].mv) {
		return 0.0f;
	}

	for (size_t i = 1; i < count; ++i) {
		if (mv <= (float)table[i].mv) {
			float span_mv = (float)(table[i].mv - table[i - 1].mv);
			float span_ppm = (float)(table[i].ppm - table[i - 1].ppm);
			return (float)table[i - 1].ppm +
			       (mv - (float)table[i - 1].mv) * span_ppm / span_mv;
		}
	}

	/* 超出曲线量程按满量程输出，避免外插产生离谱数值 */
	return (float)table[count - 1].ppm;
}

/**
 * 连续采样 8 次并做“去极值均值”滤波。
 * @param pin_mv 输出滤波后的 ADC 引脚电压（mV）；@param last_raw 输出最后一次原始码。
 * @return ESP_OK；任一 ADC 读取失败时透传错误码。
 * 调用上下文：任务上下文；相邻采样之间忙等 2 ms，让 ADC 输入 RC 充分建立。
 */
static esp_err_t ms1100_read_filtered(float *pin_mv, int *last_raw)
{
	float sum = 0.0f;
	float min = 1e9f;
	float max = -1e9f;
	int raw = 0;

	for (int i = 0; i < MS1100_FILTER_SAMPLES; ++i) {
		adc_frontend_sample_t sample = {0};
		esp_err_t err = adc_frontend_read((adc_channel_t)BSP_MS1100_ADC_CHANNEL, &sample);
		if (err != ESP_OK) {
			return err;
		}

		sum += sample.pin_mv;
		if (sample.pin_mv < min) {
			min = sample.pin_mv;
		}
		if (sample.pin_mv > max) {
			max = sample.pin_mv;
		}
		raw = sample.raw;

		/* 通道刚切换时第一次读数可能受 RC 建立影响，间隔 2 ms 可显著改善 */
		if (i != MS1100_FILTER_SAMPLES - 1) {
			esp_rom_delay_us(MS1100_FILTER_INTERVAL_MS * 1000);
		}
	}

	/* 8 点去掉一个最小、一个最大后取均值，抑制脉冲干扰 */
	*pin_mv = (sum - min - max) / (float)(MS1100_FILTER_SAMPLES - 2);
	*last_raw = raw;
	return ESP_OK;
}

esp_err_t ms1100_init(void)
{
	if (s_initialized) {
		return ESP_OK;
	}

	esp_err_t err = adc_frontend_init();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "adc frontend init failed: %s", esp_err_to_name(err));
		return err;
	}

	s_init_us = esp_timer_get_time();
	s_initialized = true;
	ESP_LOGI(TAG, "init ok: AO=GPIO%d (2:1 divider), RL=%.0f ohm, curve=%s",
	         BSP_MS1100_ADC_GPIO, s_rl_ohm,
	         s_curve == MS1100_CURVE_FORMALDEHYDE ? "formaldehyde" : "toluene");
	return ESP_OK;
}

float ms1100_voltage_to_ppm(float module_mv, ms1100_curve_t curve)
{
	if (curve == MS1100_CURVE_TOLUENE) {
		return ms1100_interpolate(k_curve_toluene, MS1100_CURVE_POINTS(k_curve_toluene),
		                          module_mv);
	}
	return ms1100_interpolate(k_curve_formaldehyde,
	                          MS1100_CURVE_POINTS(k_curve_formaldehyde), module_mv);
}

esp_err_t ms1100_read(ms1100_sample_t *out)
{
	if (out == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!s_initialized) {
		return ESP_ERR_INVALID_STATE;
	}

	*out = (ms1100_sample_t){0};

	float pin_mv = 0.0f;
	int raw = 0;
	esp_err_t err = ms1100_read_filtered(&pin_mv, &raw);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "adc read failed: %s", esp_err_to_name(err));
		return err;
	}

	/* 分压还原：模块 AO 电压 = ADC 引脚电压 × 2 */
	float module_mv = pin_mv * BSP_MS1100_DIVIDER_RATIO;
	out->raw = raw;
	out->pin_mv = pin_mv;
	out->module_mv = module_mv;
	out->preheated = ms1100_is_preheated();

	/* 合理性判定：近 0 视为断线/未上电，接近 5 V 视为饱和或短路 */
	out->valid = (raw >= 10) && (module_mv >= 150.0f) && (module_mv <= 4900.0f);
	if (!out->valid) {
		return ESP_OK;
	}

	/* Rs = (Vc − Vout) / Vout × RL；Vout 下限已在上面过滤 */
	out->rs_ohm = (MS1100_VC_MV - module_mv) / module_mv * s_rl_ohm;

	/* 预热完成后第一次有效读数自动建立 R0 基准（假设现场为洁净空气） */
	if (!s_r0_valid && out->preheated && out->rs_ohm > 0.0f) {
		s_r0_ohm = out->rs_ohm;
		s_r0_valid = true;
		ESP_LOGI(TAG, "auto R0 = %.0f ohm (module %.0f mV)", s_r0_ohm, module_mv);
	}

	out->r0_ohm = s_r0_valid ? s_r0_ohm : 0.0f;
	out->r0_valid = s_r0_valid;
	out->ratio = s_r0_valid && s_r0_ohm > 0.0f ? out->rs_ohm / s_r0_ohm : 0.0f;
	out->ppm = ms1100_voltage_to_ppm(module_mv, s_curve);
	return ESP_OK;
}

esp_err_t ms1100_capture_r0(void)
{
	if (!s_initialized) {
		return ESP_ERR_INVALID_STATE;
	}
	if (!ms1100_is_preheated()) {
		ESP_LOGW(TAG, "capture R0 rejected: still preheating");
		return ESP_ERR_INVALID_STATE;
	}

	ms1100_sample_t sample = {0};
	esp_err_t err = ms1100_read(&sample);
	if (err != ESP_OK) {
		return err;
	}
	if (!sample.valid || sample.rs_ohm <= 0.0f) {
		ESP_LOGW(TAG, "capture R0 rejected: invalid reading");
		return ESP_ERR_INVALID_STATE;
	}

	s_r0_ohm = sample.rs_ohm;
	s_r0_valid = true;
	ESP_LOGI(TAG, "R0 captured: %.0f ohm (module %.0f mV)", s_r0_ohm, sample.module_mv);
	return ESP_OK;
}

esp_err_t ms1100_set_rl(float rl_ohm)
{
	if (rl_ohm < 1000.0f || rl_ohm > 50000.0f) {
		ESP_LOGE(TAG, "invalid RL=%.0f ohm (range 1k~50k)", rl_ohm);
		return ESP_ERR_INVALID_ARG;
	}
	s_rl_ohm = rl_ohm;
	return ESP_OK;
}

esp_err_t ms1100_set_r0(float r0_ohm)
{
	if (r0_ohm <= 0.0f) {
		return ESP_ERR_INVALID_ARG;
	}
	s_r0_ohm = r0_ohm;
	s_r0_valid = true;
	ESP_LOGI(TAG, "R0 restored: %.0f ohm", s_r0_ohm);
	return ESP_OK;
}

float ms1100_get_r0(void)
{
	return s_r0_valid ? s_r0_ohm : 0.0f;
}

void ms1100_set_curve(ms1100_curve_t curve)
{
	if (curve != MS1100_CURVE_FORMALDEHYDE && curve != MS1100_CURVE_TOLUENE) {
		ESP_LOGW(TAG, "invalid curve %d, ignored", (int)curve);
		return;
	}
	s_curve = curve;
}

bool ms1100_is_preheated(void)
{
	if (!s_initialized) {
		return false;
	}
	return (esp_timer_get_time() - s_init_us) >= (MS1100_PREHEAT_MS * 1000LL);
}
