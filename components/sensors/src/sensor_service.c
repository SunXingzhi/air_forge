#include "sensor_service.h"

#include <stddef.h>

#include "board_pins.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "gp2y1014.h"
#include "ms1100.h"

static const char *TAG = "SENSOR";

#define SENSOR_TASK_STACK       5120
#define SENSOR_TASK_PRIORITY    6
#define SENSOR_TASK_CORE        1
#define SENSOR_PM_PERIOD_MS     10      /* GP2Y 100 Hz 脉冲周期 */
#define SENSOR_PUBLISH_US       1000000 /* 1 s 发布一次 */
#define SENSOR_LOG_EVERY_N      5       /* 每 5 s 打印一次汇总日志 */
#define SENSOR_ADC_RAW_MIN      30      /* 低于该码视为断线/未上电 */
#define SENSOR_ADC_RAW_MAX      4080    /* 接近满量程视为饱和/短路 */

/* 快照与互斥锁：写端只由传感器任务持有，读端只拿副本 */
static sensor_snapshot_t s_snapshot;
static SemaphoreHandle_t s_lock = NULL;
static TaskHandle_t s_task = NULL;
static bool s_started = false;

/* 驱动可用性：初始化失败时对应通道永久无效，不阻塞另一路 */
static bool s_pm_ok = false;
static bool s_voc_ok = false;

/**
 * 在锁保护下发布快照。
 * @param next 待发布内容（按值传入，函数内复制）。
 * 调用上下文：仅传感器任务；取锁最多等待 100 ms，失败时丢弃本次发布。
 */
static void sensor_publish(const sensor_snapshot_t *next)
{
	if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
		ESP_LOGW(TAG, "publish skipped: snapshot lock timeout");
		return;
	}
	s_snapshot = *next;
	xSemaphoreGive(s_lock);
}

/**
 * 传感器采样任务主体。
 *
 * 每 10 ms 触发一次 GP2Y LED 脉冲采样并累加；每 1 s 汇总一次 PM 均值、
 * 读取一次 MS1100，然后发布快照。任务优先级高于 UI，但单次循环内的
 * 阻塞（GP2Y 280 µs 忙等、ADC 转换、MS1100 约 16 ms 滤波）都在一个
 * 10 ms 周期预算之外，允许在 1 s 边界短暂拉长，不触发看门狗。
 *
 * @param arg 未使用。
 * 调用上下文：FreeRTOS 任务，绑定 CPU1；任务永不返回。
 */
static void sensor_task(void *arg)
{
	(void)arg;

	TickType_t last_wake = xTaskGetTickCount();
	int64_t last_publish_us = esp_timer_get_time();
	uint32_t log_counter = 0;
	uint32_t seq = 0;

	/* PM 1 s 聚合窗口的状态 */
	float pm_sum_mv = 0.0f;
	float pm_min_mv = 1e9f;
	float pm_max_mv = -1e9f;
	int pm_last_raw = 0;
	int pm_count = 0;
	int pm_bad_count = 0;

	while (true) {
		/* ---- GP2Y：10 ms 周期脉冲采样 ---- */
		if (s_pm_ok) {
			gp2y1014_sample_t sample = {0};
			if (gp2y1014_read(&sample) == ESP_OK) {
				pm_sum_mv += sample.sensor_mv;
				if (sample.sensor_mv < pm_min_mv) {
					pm_min_mv = sample.sensor_mv;
				}
				if (sample.sensor_mv > pm_max_mv) {
					pm_max_mv = sample.sensor_mv;
				}
				pm_last_raw = sample.raw;
				pm_count++;

				/* 统计越界码；少量毛刺不判无效，避免偶发单点导致整秒置灰 */
				if (sample.raw < SENSOR_ADC_RAW_MIN || sample.raw > SENSOR_ADC_RAW_MAX) {
					pm_bad_count++;
				}
			}
		}

		int64_t now_us = esp_timer_get_time();
		if (now_us - last_publish_us >= SENSOR_PUBLISH_US) {
			last_publish_us = now_us;

			sensor_snapshot_t snap = {0};
			snap.timestamp_us = now_us;

			/* ---- PM 汇总：去极值均值 ---- */
			if (s_pm_ok && pm_count > 0) {
				float avg_mv;
				if (pm_count >= 3) {
					avg_mv = (pm_sum_mv - pm_min_mv - pm_max_mv) / (float)(pm_count - 2);
				} else {
					avg_mv = pm_sum_mv / (float)pm_count;
				}

				snap.pm_preheated = gp2y1014_is_preheated();
				snap.pm_raw = pm_last_raw;
				snap.pm_sensor_mv = avg_mv;
				snap.pm_pin_mv = avg_mv / BSP_PM_DIVIDER_RATIO;
				snap.pm_ug_m3 = gp2y1014_convert_mv(avg_mv);
				snap.pm_valid = snap.pm_preheated && pm_count > 0 && pm_bad_count <= 2;
			} else {
				snap.pm_preheated = s_pm_ok && gp2y1014_is_preheated();
			}

			/* ---- MS1100：1 Hz 读取 ---- */
			if (s_voc_ok) {
				ms1100_sample_t voc = {0};
				if (ms1100_read(&voc) == ESP_OK) {
					snap.voc_preheated = voc.preheated;
					snap.voc_raw = voc.raw;
					snap.voc_module_mv = voc.module_mv;
					snap.voc_rs_ohm = voc.rs_ohm;
					snap.voc_ratio = voc.ratio;
					snap.voc_ppm = voc.ppm;
					snap.voc_valid = voc.valid && voc.preheated;
				} else {
					ESP_LOGW(TAG, "ms1100 read failed");
				}
			}

			snap.seq = ++seq;
			sensor_publish(&snap);

			if (++log_counter >= SENSOR_LOG_EVERY_N) {
				log_counter = 0;
				ESP_LOGI(TAG,
				         "PM %s %.1f ug/m3 (%.1f mV, raw %d) | VOC %s %.1f ppm (%.0f mV, rs %.0f)",
				         snap.pm_valid ? "ok" : "invalid", snap.pm_ug_m3,
				         snap.pm_sensor_mv, snap.pm_raw, snap.voc_valid ? "ok" : "invalid",
				         snap.voc_ppm, snap.voc_module_mv, snap.voc_rs_ohm);
			}

			/* 重置聚合窗口 */
			pm_sum_mv = 0.0f;
			pm_min_mv = 1e9f;
			pm_max_mv = -1e9f;
			pm_count = 0;
			pm_bad_count = 0;

			/* MS1100 滤波约 16 ms，超过 10 ms 周期；必须重置节拍基准，
			   否则 xTaskDelayUntil 会连续补拍，导致 GP2Y LED 被高频连闪 */
			last_wake = xTaskGetTickCount();
		}

		/* 10 ms 周期；last_wake 在 1 s 汇总后已重置，不会出现补拍连闪 */
		xTaskDelayUntil(&last_wake, pdMS_TO_TICKS(SENSOR_PM_PERIOD_MS));
	}
}

esp_err_t sensor_service_start(void)
{
	if (s_started) {
		return ESP_OK;
	}

	s_lock = xSemaphoreCreateMutex();
	if (s_lock == NULL) {
		ESP_LOGE(TAG, "create mutex failed");
		return ESP_ERR_NO_MEM;
	}

	esp_err_t pm_err = gp2y1014_init();
	s_pm_ok = (pm_err == ESP_OK);
	if (!s_pm_ok) {
		ESP_LOGE(TAG, "gp2y1014 init failed: %s", esp_err_to_name(pm_err));
	}

	esp_err_t voc_err = ms1100_init();
	s_voc_ok = (voc_err == ESP_OK);
	if (!s_voc_ok) {
		ESP_LOGE(TAG, "ms1100 init failed: %s", esp_err_to_name(voc_err));
	}

	if (!s_pm_ok && !s_voc_ok) {
		vSemaphoreDelete(s_lock);
		s_lock = NULL;
		return pm_err != ESP_OK ? pm_err : voc_err;
	}

	/* 绑定 CPU1：CPU0 留给 app_main/UI，避免 100 Hz 采样与触摸轮询抢占 */
	if (xTaskCreatePinnedToCore(sensor_task, "sensor", SENSOR_TASK_STACK, NULL,
	                            SENSOR_TASK_PRIORITY, &s_task, SENSOR_TASK_CORE) != pdPASS) {
		ESP_LOGE(TAG, "create sensor task failed");
		vSemaphoreDelete(s_lock);
		s_lock = NULL;
		s_task = NULL;
		return ESP_ERR_NO_MEM;
	}

	s_started = true;
	ESP_LOGI(TAG, "started: PM=%s, VOC=%s", s_pm_ok ? "ok" : "fail",
	         s_voc_ok ? "ok" : "fail");
	return ESP_OK;
}

esp_err_t sensor_service_get(sensor_snapshot_t *out)
{
	if (out == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (!s_started) {
		return ESP_ERR_INVALID_STATE;
	}
	if (xSemaphoreTake(s_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
		return ESP_ERR_TIMEOUT;
	}
	*out = s_snapshot;
	xSemaphoreGive(s_lock);
	return ESP_OK;
}
