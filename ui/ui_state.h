/**
 * @file ui_state.h
 * 设备运行状态与仿真数据。UI 页面只读这里，硬件接入后由传感器/风机层写入。
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define UI_FILTER_COUNT 4
#define UI_HISTORY_LEN 60

typedef enum {
	UI_FAN_STOP = 0,
	UI_FAN_LOW,
	UI_FAN_MID,
	UI_FAN_HIGH,
	UI_FAN_AUTO,
	UI_FAN_MODE_COUNT
} ui_fan_t;

typedef enum {
	UI_FILTER_MESH = 0,
	UI_FILTER_MEDIA,
	UI_FILTER_CARBON_A,
	UI_FILTER_CARBON_B
} ui_filter_t;

typedef struct {
	bool powered;
	ui_fan_t fan;

	uint8_t filter_pct[UI_FILTER_COUNT];

	bool pm_ok;
	bool voc_ok;
	bool overheat;

	float pm_before;
	float pm_after;
	float voc_before;
	float voc_after;
	uint8_t warmup_sec;

	uint32_t session_sec;
	uint32_t total_sec;

	int8_t brightness;	 /* 0 低 / 1 中 / 2 高 */
	uint16_t screen_off_sec; /* 0 = 常亮 */
	uint8_t voc_unit;	 /* 0 = ppm / 1 = mg/m3 */
	bool theme_stone;	 /* false = OLED 单色（默认） */

	float hist_before[UI_HISTORY_LEN];
	float hist_after[UI_HISTORY_LEN];
	uint8_t hist_len;
	uint8_t hist_pos;
} ui_state_t;

extern ui_state_t g_ui;

void ui_state_init(void);
void ui_state_tick(void);

const char *ui_fan_name(ui_fan_t fan);
int ui_fan_effective(void);
int ui_efficiency_pct(void);
int ui_filter_min_pct(void);
void ui_filter_reset(int index);

float ui_voc_value(float ppm);
const char *ui_voc_unit_name(void);
void ui_filter_days(int index, char *buf, int size);

void ui_runtime_str(char *buf, int size);
void ui_total_str(char *buf, int size);
