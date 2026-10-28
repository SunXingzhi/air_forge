/**
 * @file ui_progress.h
 * 像素分段进度条：一格一档，颜色填充。
 */

#pragma once

#include "lvgl.h"

lv_obj_t * ui_progress_create(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h,
                              int32_t segments, int32_t value);

void ui_progress_set_value(lv_obj_t * bar, int32_t value);
