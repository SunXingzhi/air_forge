/**
 * @file ui_popup.h
 * 模态弹窗与 2s 提示条（告警层 W）。
 */

#pragma once

#include <stdbool.h>

#include "lvgl.h"

typedef void (*ui_popup_cb_t)(void);

void ui_popup_show(const char *title, const char *body, const char *left_text,
		   ui_popup_cb_t left_cb, const char *right_text,
		   ui_popup_cb_t right_cb, bool warning);

void ui_popup_close(void);
bool ui_popup_is_open(void);

void ui_toast(const char *text);
