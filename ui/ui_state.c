/**
 * @file ui_state.c
 * 桌面仿真用的数据模型：传感器读数、滤芯寿命、运行时长。
 */

#include "ui_state.h"

#include <stdio.h>
#include <string.h>

ui_state_t g_ui;

static uint32_t rng_state = 0x12345678u;

static uint32_t rng_next(void) {
	rng_state = rng_state * 1664525u + 1013904223u;
	return rng_state;
}

static int fan_effect_pct(int level) {
	switch (level) {
		case UI_FAN_LOW:
			return 35;
		case UI_FAN_MID:
			return 60;
		case UI_FAN_HIGH:
			return 80;
		default:
			return 0;
	}
}

void ui_state_init(void) {
	memset(&g_ui, 0, sizeof(g_ui));

	g_ui.powered = true;
	g_ui.fan = UI_FAN_STOP;
	g_ui.filter_pct[UI_FILTER_MESH] = 82;
	g_ui.filter_pct[UI_FILTER_MEDIA] = 54;
	g_ui.filter_pct[UI_FILTER_CARBON_A] = 31;
	g_ui.filter_pct[UI_FILTER_CARBON_B] = 12;

	g_ui.pm_ok = true;
	g_ui.voc_ok = false;
	g_ui.warmup_sec = 8;

	g_ui.pm_before = 35.0f;
	g_ui.pm_after = 35.0f;
	g_ui.voc_before = 1.20f;
	g_ui.voc_after = 1.20f;

	g_ui.brightness = 1;
	g_ui.screen_off_sec = 60;
	g_ui.voc_unit = 0;
	g_ui.theme_stone = false;

	/* 预填一段 60s 历史，让数据页首次打开就有趋势可看 */
	static const float wave[12] = {33.0f, 34.0f, 35.0f, 36.0f,
				       37.0f, 36.0f, 35.0f, 34.0f,
				       33.0f, 32.0f, 31.0f, 32.0f};
	for (uint8_t i = 0; i < UI_HISTORY_LEN; i++) {
		float before = wave[i % 12u];
		g_ui.hist_before[i] = before;
		g_ui.hist_after[i] = before * 0.45f + (float)(i % 2u);
	}
	g_ui.hist_len = UI_HISTORY_LEN;
	g_ui.hist_pos = 0;
}

const char *ui_fan_name(ui_fan_t fan) {
	switch (fan) {
		case UI_FAN_LOW:
			return "弱";
		case UI_FAN_MID:
			return "中";
		case UI_FAN_HIGH:
			return "强";
		case UI_FAN_AUTO:
			return "自动";
		default:
			return "停止";
	}
}

int ui_fan_effective(void) {
	if (g_ui.fan != UI_FAN_AUTO) {
		return (int)g_ui.fan;
	}

	/* 自动档占位算法：按处理后 PM2.5 分档，后续由风机控制策略替换 */
	if (g_ui.pm_after >= 75.0f)
		return UI_FAN_HIGH;
	if (g_ui.pm_after >= 35.0f)
		return UI_FAN_MID;
	if (g_ui.pm_after >= 15.0f)
		return UI_FAN_LOW;
	return UI_FAN_STOP;
}

int ui_efficiency_pct(void) {
	if (g_ui.pm_before <= 0.1f || ui_fan_effective() == UI_FAN_STOP) {
		return 0;
	}

	int eff =
	    (int)((g_ui.pm_before - g_ui.pm_after) * 100.0f / g_ui.pm_before);
	if (eff < 0)
		eff = 0;
	if (eff > 99)
		eff = 99;
	return eff;
}

int ui_filter_min_pct(void) {
	int min = 100;
	for (int i = 0; i < UI_FILTER_COUNT; i++) {
		if (g_ui.filter_pct[i] < min) {
			min = g_ui.filter_pct[i];
		}
	}
	return min;
}

void ui_filter_reset(int index) {
	if (index < 0 || index >= UI_FILTER_COUNT) {
		return;
	}
	g_ui.filter_pct[index] = 100;
}

static void history_push(float before, float after) {
	g_ui.hist_before[g_ui.hist_pos] = before;
	g_ui.hist_after[g_ui.hist_pos] = after;
	g_ui.hist_pos = (uint8_t)((g_ui.hist_pos + 1) % UI_HISTORY_LEN);
	if (g_ui.hist_len < UI_HISTORY_LEN) {
		g_ui.hist_len++;
	}
}

void ui_state_tick(void) {
	if (!g_ui.powered) {
		return;
	}

	g_ui.session_sec++;

	int level = ui_fan_effective();
	if (level != UI_FAN_STOP) {
		g_ui.total_sec++;
	}

	if (g_ui.warmup_sec > 0) {
		g_ui.warmup_sec--;
	}
	g_ui.voc_ok = (g_ui.warmup_sec == 0);

	float before_target = 30.0f + (float)(rng_next() % 16u);
	g_ui.pm_before = g_ui.pm_before * 0.9f + before_target * 0.1f;

	int eff = fan_effect_pct(level);
	float after_target = g_ui.pm_before * (float)(100 - eff) / 100.0f;
	after_target += (float)(rng_next() % 3u);
	g_ui.pm_after = g_ui.pm_after * 0.8f + after_target * 0.2f;

	if (g_ui.voc_ok) {
		float voc_target = 1.0f + (float)(rng_next() % 60u) / 100.0f;
		g_ui.voc_before = g_ui.voc_before * 0.9f + voc_target * 0.1f;
		g_ui.voc_after = g_ui.voc_before * (float)(100 - eff) / 100.0f;
	} else {
		g_ui.voc_after = g_ui.voc_before;
	}

	history_push(g_ui.pm_before, g_ui.pm_after);
}

float ui_voc_value(float ppm) { return g_ui.voc_unit == 0 ? ppm : ppm * 2.5f; }

const char *ui_voc_unit_name(void) {
	return g_ui.voc_unit == 0 ? "ppm" : "mg/m3";
}

void ui_filter_days(int index, char *buf, int size) {
	int pct = 0;
	if (index >= 0 && index < UI_FILTER_COUNT) {
		pct = g_ui.filter_pct[index];
	}
	int days = pct * 30 / 100;
	snprintf(buf, size, "%d天", days);
}

void ui_runtime_str(char *buf, int size) {
	uint32_t sec = g_ui.session_sec;
	uint32_t minutes = sec / 60;
	uint32_t seconds = sec % 60;
	snprintf(buf, size, "%02u:%02u", (unsigned)minutes, (unsigned)seconds);
}

void ui_total_str(char *buf, int size) {
	snprintf(buf, size, "%uh", (unsigned)(g_ui.total_sec / 3600u));
}
