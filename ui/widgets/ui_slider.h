/**
 * @file ui_slider.h
 * 像素滑条：方角轨道 + 方块滑块，支持阶梯式动画赋值。
 */

#pragma once

#include "lvgl.h"

typedef void (*ui_slider_cb_t)(lv_obj_t * slider, int32_t value, void * user_data);

lv_obj_t * ui_slider_create(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h,
                            int32_t min, int32_t max, int32_t value,
                            ui_slider_cb_t on_change, void * user_data);

int32_t ui_slider_get_value(lv_obj_t * slider);

/* animate=true 时按 10% 一档跳变，保持像素感 */
void ui_slider_set_value(lv_obj_t * slider, int32_t value, bool animate);
