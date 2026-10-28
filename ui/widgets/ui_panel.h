/**
 * @file ui_panel.h
 * 像素面板：硬阴影 + 2px 描边 + 平铺噪点底纹。
 */

#pragma once

#include "lvgl.h"

/* 创建面板，返回可作为父容器使用的内容对象 */
lv_obj_t * ui_panel_create(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h);
