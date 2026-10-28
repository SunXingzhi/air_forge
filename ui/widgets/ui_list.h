/**
 * @file ui_list.h
 * 像素列表：选中块阶梯式滑动 + ">" 光标闪烁。
 */

#pragma once

#include "lvgl.h"

typedef void (*ui_list_cb_t)(lv_obj_t * list, int32_t index, void * user_data);

lv_obj_t * ui_list_create(lv_obj_t * parent, const char * const items[], int32_t count,
                          int32_t x, int32_t y, int32_t w, int32_t h,
                          ui_list_cb_t on_select, void * user_data);

int32_t ui_list_get_selected(lv_obj_t * list);
void ui_list_select(lv_obj_t * list, int32_t index, bool animate);
