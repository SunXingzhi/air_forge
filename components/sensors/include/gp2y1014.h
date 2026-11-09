#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/*
 * GP2Y1014AU0F 光学粉尘传感器驱动（ESP32-S3 / ESP-IDF v6.1）。
 *
 * 电气事实（Sharp ED-15G001A 规格书）：
 *   - 供电 5±0.5 V，LED 端子灌电流 10~20 mA；
 *   - LED 脉冲周期 10±1 ms、脉宽 0.32±0.02 ms，点亮后 0.28 ms 采样 Vo；
 *   - 洁净空气输出 Voc 0.1/0.6/1.1 V（min/typ/max），灵敏度 K 0.425/0.5/0.575
 *     V per 100 µg/m³；输出电压可达 3.4 V 以上，因此硬件必须 2:1 分压后再进 ADC。
 *
 * 换算：粉尘浓度 µg/m³ = (Vout − Voc) × 100 / K；Voc/K 必须按单机标定覆盖，
 * 默认值仅用于点亮验证。
 */

#define GP2Y1014_PREHEAT_MS                 60000
#define GP2Y1014_DEFAULT_ZERO_MV            600.0f  /* Voc 典型值 */
#define GP2Y1014_DEFAULT_K_MV_PER_100UG     500.0f  /* 灵敏度典型值 */
#define GP2Y1014_MAX_UG_M3                  1000.0f /* 超出后按满量程钳位 */

/**
 * 一次脉冲采样结果。
 * raw：ADC 原始码；pin_mv：分压后 ADC 引脚电压；
 * sensor_mv：还原后的传感器 Vo 电压；dust_ug_m3：换算粉尘浓度（已钳位 ≥0）。
 * preheated：距初始化是否已超过 60 s 预热时间。
 */
typedef struct {
	int raw;
	float pin_mv;
	float sensor_mv;
	float dust_ug_m3;
	bool preheated;
} gp2y1014_sample_t;

/**
 * 初始化 GP2Y1014AU0F：配置 LED 驱动 GPIO（默认关闭）并拉起 ADC 前端。
 *
 * @return ESP_OK；GPIO 配置失败或 ADC 前端初始化失败时透传 esp_err_t。
 * 调用上下文：任务上下文，必须在创建传感器采样任务前调用；重复调用返回 ESP_OK。
 * 初始化同时记录预热起点（60 s）。
 */
esp_err_t gp2y1014_init(void);

/**
 * 执行一次“LED 点亮 → 延时 280 µs → ADC 采样 → LED 熄灭”。
 *
 * 时序说明：ADC oneshot 转换本身需要数十至上百 µs，实际脉宽会略超过规格的
 * 320 µs（约 350~400 µs）。这是 bring-up 可用路径；量产若需严格 0.32 ms 脉宽，
 * 应改为 LEDC 100 Hz 输出 + 定时器触发采样，且 ADC 转换不能放进 ISR。
 *
 * @param out 输出结果，不可为 NULL。
 * @return ESP_OK；未初始化返回 ESP_ERR_INVALID_STATE；ADC 读取失败透传错误码。
 * 调用上下文：任务上下文；函数内忙等 280 µs 并阻塞一次 ADC 转换。
 * 调用周期必须保持 ≥10 ms，禁止在 ISR 中调用。
 */
esp_err_t gp2y1014_read(gp2y1014_sample_t *out);

/**
 * 用采样得到的传感器电压换算粉尘浓度。
 * @param sensor_mv 传感器 Vo（未分压，mV）。
 * @return µg/m³，低于 Voc 时返回 0，超过上限时钳位到 GP2Y1014_MAX_UG_M3。
 * 调用上下文：任意任务上下文；只读标定参数，无锁。
 */
float gp2y1014_convert_mv(float sensor_mv);

/**
 * 覆盖标定参数（标定值应来自洁净空气与参考仪器的两点拟合）。
 * @param zero_mv         洁净空气 Voc，单位 mV，建议 0~2000。
 * @param k_mv_per_100ug  灵敏度，单位 mV/100 µg/m³，必须 >0。
 * 调用上下文：建议仅在初始化阶段调用；运行期修改会与采样任务产生数据竞争，
 * 如需在线标定，请先暂停采样任务。
 */
void gp2y1014_set_calibration(float zero_mv, float k_mv_per_100ug);

/**
 * 读取当前标定参数。
 * @param zero_mv 输出 Voc（可为 NULL）；@param k_mv_per_100ug 输出灵敏度（可为 NULL）。
 * 调用上下文：任务上下文，只读。
 */
void gp2y1014_get_calibration(float *zero_mv, float *k_mv_per_100ug);

/**
 * 查询是否已完成 60 s 预热。
 * @return true 已预热；false 仍在预热或未初始化。
 * 调用上下文：任意任务上下文；无阻塞。
 */
bool gp2y1014_is_preheated(void);
