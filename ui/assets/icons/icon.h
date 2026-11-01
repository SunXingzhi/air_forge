/**
 * @file icon.h
 * 图标资源统一声明。
 *
 * 约定：
 * - 每个图标一个编译单元：icons/xxx_icon.c
 * - 对外符号名与文件名一致：xxx_icon
 * - 本头文件只放 extern 声明，不放定义
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_image_dsc_t setting_icon;
extern const lv_image_dsc_t fan_icon;
extern const lv_image_dsc_t filter_icon;
extern const lv_image_dsc_t data_icon;
extern const lv_image_dsc_t logo_icon;

#ifdef __cplusplus
}
#endif
