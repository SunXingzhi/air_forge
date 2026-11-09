#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/*
 * MS1100 VOC 模块驱动（模块内为 KQ-2801 半导体气敏元件 + 比较器）。
 *
 * 电气事实（KQ-2801 规格书 / MS1100-P111 模块特性）：
 *   - 加热电压 5.0±0.2 V，加热电流 <80 mA；测量电压 5.0±0.2 V；
 *   - 负载电阻 RL 1~50 kΩ 可调，典型 20 kΩ；
 *   - 洁净空气输出 0.5~1.0 V，检测气体时典型 2.5~3.5 V（RL=20 kΩ）；
 *   - 预热 2 min（本项目按 180 s 留余量），响应 ≤10 s，恢复 ≤20 s；
 *   - 模块 AO 输出可到约 4.9 V，必须 2:1 分压后再进 ESP32-S3 ADC。
 *
 * 浓度换算使用模块手册给出的输出电压—浓度表（甲醛 / 甲苯两条曲线）做分段线性
 * 插值。该表是“模块级”特性，误差约 ±7%，未做温湿度补偿；输出应标注为
 * “甲醛/甲苯等效 ppm”，不能当作分项气体浓度。
 *
 * Rs 仅用于相对变化与 R0 基准诊断：Rs = (Vc − Vout) / Vout × RL。
 */

/* 模块手册浓度曲线：默认按甲醛等效输出，可切换到甲苯等效 */
typedef enum {
	MS1100_CURVE_FORMALDEHYDE = 0,
	MS1100_CURVE_TOLUENE,
} ms1100_curve_t;

#define MS1100_PREHEAT_MS       180000  /* 规格书 2 min，本项目留 180 s 余量 */
#define MS1100_DEFAULT_RL_OHM   20000.0f
#define MS1100_VC_MV            5000.0f
#define MS1100_FILTER_SAMPLES   8
#define MS1100_FILTER_INTERVAL_MS 2

/**
 * 一次滤波后的 MS1100 读数。
 * raw：最后一次 ADC 原始码；pin_mv：滤波后 ADC 引脚电压；
 * module_mv：还原后的模块 AO 电压；rs_ohm：气敏电阻；
 * r0_ohm / ratio：洁净空气基准与相对比值（R0 未标定时 r0_valid=false）；
 * ppm：按当前等效曲线插值出的浓度。
 */
typedef struct {
	int raw;
	float pin_mv;
	float module_mv;
	float rs_ohm;
	float r0_ohm;
	float ratio;
	float ppm;
	bool valid;
	bool preheated;
	bool r0_valid;
} ms1100_sample_t;

/**
 * 初始化 MS1100 驱动：拉起 ADC 前端、记录预热起点、装载默认 RL 与曲线。
 *
 * @return ESP_OK；ADC 前端初始化失败时透传 esp_err_t。
 * 调用上下文：任务上下文，必须在采样任务创建前调用；重复调用返回 ESP_OK。
 */
esp_err_t ms1100_init(void);

/**
 * 读取一次 MS1100：内部连续采样 8 次（间隔 2 ms），去极值后取均值，
 * 再换算 Rs、R0、Rs/R0 与等效 ppm。
 *
 * 预热满 180 s 后的第一次有效读数会自动把当前 Rs 记为 R0（假设当时空气洁净）。
 * 若现场空气不洁净，应改用 ms1100_capture_r0() 在已知洁净空气中重新标定。
 *
 * @param out 输出结果，不可为 NULL。
 * @return ESP_OK；未初始化返回 ESP_ERR_INVALID_STATE；ADC 读取失败透传错误码。
 * 调用上下文：任务上下文；函数内阻塞约 16 ms，禁止在 ISR 中调用。
 */
esp_err_t ms1100_read(ms1100_sample_t *out);

/**
 * 用当前有效读数重新标定 R0（必须在洁净空气中、且已完成预热）。
 * @return ESP_OK；未预热或读数无效返回 ESP_ERR_INVALID_STATE。
 * 调用上下文：任务上下文。会阻塞一次 ms1100_read() 的时间。
 */
esp_err_t ms1100_capture_r0(void);

/**
 * 设置模块负载电阻 RL（用于 Rs 计算）。
 * @param rl_ohm 取值范围 1000~50000 Ω，超出范围返回 ESP_ERR_INVALID_ARG。
 * 调用上下文：建议初始化阶段调用；运行期修改与采样任务存在竞争。
 */
esp_err_t ms1100_set_rl(float rl_ohm);

/**
 * 直接写入 R0 标定值（供 NVS 恢复使用）。
 * @param r0_ohm 必须 >0；<=0 返回 ESP_ERR_INVALID_ARG。
 * 调用上下文：初始化阶段调用。
 */
esp_err_t ms1100_set_r0(float r0_ohm);

/**
 * 读取当前 R0；未标定时返回 0。
 * 调用上下文：任务上下文，只读。
 */
float ms1100_get_r0(void);

/**
 * 切换等效浓度曲线（甲醛 / 甲苯）。
 * 调用上下文：初始化阶段调用；运行期修改与采样任务存在竞争。
 */
void ms1100_set_curve(ms1100_curve_t curve);

/**
 * 把模块 AO 电压换算为等效 ppm。
 * @param module_mv 模块 AO 电压（未分压，mV）。
 * @param curve     使用的浓度曲线。
 * @return ppm；低于曲线起点返回 0，高于终点按终点值钳位。
 * 调用上下文：任意任务上下文，纯函数。
 */
float ms1100_voltage_to_ppm(float module_mv, ms1100_curve_t curve);

/**
 * 查询是否已完成预热。
 * 调用上下文：任意任务上下文；无阻塞。
 */
bool ms1100_is_preheated(void);
