/**
 * @file ui_statusbar.h
 * 常驻状态栏：风量档位、最低滤芯寿命、告警提示。
 */

#pragma once

#include "lvgl.h"

lv_obj_t *ui_statusbar_create(lv_obj_t *parent);
void ui_statusbar_refresh(lv_obj_t *bar);
