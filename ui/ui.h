/**
 * @file ui.h
 * AirForge UI 层入口（像素风）。
 *
 * 只依赖 LVGL，可在桌面仿真与 ESP32 实机上共用。
 */

#pragma once

#include <stdbool.h>

typedef enum {
	UI_SCREEN_HOME = 0,
	UI_SCREEN_MENU,
} ui_screen_t;

/* 初始化并显示首页（需在 lv_init() 和显示驱动就绪后调用） */
void ui_init(void);

/* 切换屏幕；back=true 时反向滑动（用于返回） */
void ui_goto(ui_screen_t screen, bool back);

/* 按当前主题重建当前屏幕（换肤用） */
void ui_reload(void);
