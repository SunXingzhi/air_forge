#include "demo.h"

#include "esp_err.h"
#include "esp_log.h"
#include "sensor_service.h"

static const char *TAG = "MAIN";

/**
 * 应用入口。
 * 先启动传感器服务（100 Hz 采样任务绑定 CPU1），再进入屏幕 demo。
 * 传感器初始化失败不会阻断显示流程，只在日志中报告。
 * 调用上下文：ESP-IDF 主任务，仅执行一次。
 */
void app_main(void)
{
	esp_err_t err = sensor_service_start();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "sensor service start failed: %s", esp_err_to_name(err));
	}

	p169h002_demo();
}
