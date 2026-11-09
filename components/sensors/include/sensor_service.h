#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/*
 * 传感器服务：唯一持有 GP2Y1014AU0F / MS1100 采样时序的任务。
 *
 * 采样节奏：
 *   - GP2Y：每 10 ms 一次 LED 脉冲采样，1 s 内 100 点做去极值均值；
 *   - MS1100：每 1 s 一次（驱动内部 8 点滤波）；
 *   - 两个传感器都满足预热后才把数据标记为有效。
 *
 * 快照通过互斥锁双缓冲发布，UI/业务层只读取副本，不得直接访问驱动状态。
 */

/**
 * 传感器快照（1 Hz 更新）。
 * 字段带 valid / preheated 标记，调用方不得把无效值当真实测量值使用。
 */
typedef struct {
	bool pm_valid;
	bool pm_preheated;
	int pm_raw;
	float pm_pin_mv;
	float pm_sensor_mv;
	float pm_ug_m3;

	bool voc_valid;
	bool voc_preheated;
	int voc_raw;
	float voc_module_mv;
	float voc_rs_ohm;
	float voc_ratio;
	float voc_ppm;

	uint32_t seq;          /* 每次发布 +1，用于判断是否更新 */
	int64_t timestamp_us;  /* esp_timer 时间戳 */
} sensor_snapshot_t;

/**
 * 启动传感器服务：初始化两路驱动并创建 100 Hz 采样任务。
 *
 * 若某一路初始化失败，仍会启动任务并把该路标记为无效；两路都失败时返回错误。
 *
 * @return ESP_OK 已启动或已启动过；ESP_ERR_NO_MEM 互斥锁/任务创建失败；
 *         两路驱动均初始化失败时返回第一个错误码。
 * 调用上下文：app_main 或其他任务上下文，只调用一次；不可在 ISR 中调用。
 */
esp_err_t sensor_service_start(void);

/**
 * 获取最新快照的副本。
 * @param out 输出缓冲，不可为 NULL。
 * @return ESP_OK；未启动返回 ESP_ERR_INVALID_STATE；取锁超时返回 ESP_ERR_TIMEOUT。
 * 调用上下文：任意任务上下文；内部最多等待 100 ms，不在持锁期间做任何硬件操作。
 */
esp_err_t sensor_service_get(sensor_snapshot_t *out);
