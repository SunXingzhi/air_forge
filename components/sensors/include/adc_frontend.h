#pragma once

#include <stdint.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

/**
 * 一次 ADC 采样结果。
 * raw：12 bit 原始码（0~4095）；pin_mv：校准后的 ADC 引脚电压（mV）。
 * 注意 pin_mv 是分压后的电压，传感器端电压 = pin_mv × 各自 DIVIDER_RATIO。
 */
typedef struct {
	int raw;
	float pin_mv;
} adc_frontend_sample_t;

/**
 * 初始化 ADC1 oneshot 前端（MS1100：CH0/GPIO1；GP2Y：CH1/GPIO2）。
 *
 * 内部完成：创建 ADC1 oneshot 单元、配置两个通道（12 dB 衰减 / 12 bit）、
 * 为每个通道创建曲线拟合校准句柄。校准方案不可用时退回线性换算并打印告警，
 * 不阻断采样。
 *
 * @return ESP_OK 成功或已初始化；ESP_ERR_INVALID_STATE 参数/通道配置异常；
 *         ESP_ERR_NO_MEM 或驱动创建失败时透传 esp_err_t。
 * 调用上下文：任务上下文，只允许在传感器任务创建前调用；函数可重复调用，
 * 第二次起直接返回 ESP_OK。ADC oneshot 驱动内部带锁，运行期多通道读取安全。
 */
esp_err_t adc_frontend_init(void);

/**
 * 读取指定 ADC1 通道并换算为引脚电压。
 *
 * @param channel 必须是已配置的 ADC1 通道（BSP_MS1100_ADC_CHANNEL 或
 *                BSP_PM_ADC_CHANNEL）；传其他通道返回 ESP_ERR_INVALID_ARG。
 * @param out     输出缓冲，不可为 NULL。
 * @return ESP_OK；SPI/ADC 读取失败时透传驱动错误码。
 * 调用上下文：任务上下文。该 API 内部阻塞一次 ADC 转换（数十至上百 µs），
 * 禁止在 ISR 或定时器回调中调用。
 */
esp_err_t adc_frontend_read(adc_channel_t channel, adc_frontend_sample_t *out);
