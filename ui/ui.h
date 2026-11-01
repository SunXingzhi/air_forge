/**
 * @file ui.h
 * AirForge UI 层入口（像素风）。
 *
 * 只依赖 LVGL，可在桌面仿真与 ESP32 实机上共用。
 */

#pragma once

#include <stdbool.h>

#include "lvgl.h"

typedef enum {
	UI_SCREEN_BOOT = 0,
	UI_SCREEN_HOME,
	UI_SCREEN_FAN,
	UI_SCREEN_FILTER,
	UI_SCREEN_DATA,
	UI_SCREEN_SETTINGS,
	UI_SCREEN_OFF
} ui_screen_t;

/* 初始化并显示启动页（需在 lv_init() 和显示驱动就绪后调用） */
void ui_init(void);

/* 切换屏幕；back=true 时反向滑动（用于返回） */
void ui_goto(ui_screen_t screen, bool back);

/* 按当前主题重建当前屏幕（换肤用） */
void ui_reload(void);

/* 子页面通用：左滑返回 + 44x44 返回热区 */
void ui_attach_back(lv_obj_t *root);

/* 亮度遮罩（仿真用；实机由背光 PWM 接管） */
void ui_apply_brightness(void);

/* 物理按键：功能键短按、电源键短按 / 长按 */
void ui_key_fn(void);
void ui_key_power_short(void);
void ui_key_power_long(void);

/* 息屏时唤醒；返回 true 表示本次输入只用于唤醒 */
bool ui_wake_if_sleeping(void);

/* 异步换肤 / 恢复出厂（避免在事件回调里删除当前页面） */
void ui_request_theme_toggle(void);
void ui_request_factory_reset(void);

/* 启动页结束后检查滤芯提醒 */
void ui_after_boot(void);
