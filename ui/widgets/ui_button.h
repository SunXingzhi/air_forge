/**
 * @file ui_button.h
 * 像素按钮：1 帧按下位移 + 颜色反转，无圆角。
 */

#pragma once

#include "lvgl.h"

lv_obj_t * ui_button_create(lv_obj_t * parent, const char * text,
                            int32_t x, int32_t y, int32_t w, int32_t h,
                            lv_event_cb_t on_click, void * user_data);
