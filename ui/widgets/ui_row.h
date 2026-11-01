/**
 * @file ui_row.h
 * 44px 触控行：左侧标题、右侧值、选中反色，可选长按回调。
 */

#pragma once

#include "lvgl.h"

typedef void (*ui_row_cb_t)(lv_obj_t *row, void *user_data);

lv_obj_t *ui_row_create(lv_obj_t *parent, int32_t x, int32_t y, int32_t w,
			int32_t h, const char *label, const char *value,
			ui_row_cb_t on_tap, ui_row_cb_t on_hold,
			void *user_data);

void ui_row_set_value(lv_obj_t *row, const char *value);
void ui_row_set_selected(lv_obj_t *row, bool selected);
void ui_row_set_warn(lv_obj_t *row, bool warn);
