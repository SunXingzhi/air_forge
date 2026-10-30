/**
 * @file ui_button.h
 * 像素按钮：
 * - ui_button_create：空壳，只负责尺寸/描边/按下动画，内容由调用者添加
 * - ui_button_create_text：文本按钮
 * - ui_button_create_with_icon：图标按钮
 *
 * 按下时按钮本体的位移和底色由 LV_STATE_PRESSED 驱动，与内容无关；
 * 若需要文字/图标一起反色，调用 ui_button_set_fg_colors() 注册前景色。
 */

#pragma once

#include "lvgl.h"

lv_obj_t * ui_button_create(lv_obj_t * parent,
                            int32_t x, int32_t y, int32_t w, int32_t h,
                            lv_event_cb_t on_click, void * user_data);

lv_obj_t * ui_button_create_text(lv_obj_t * parent, const char * text,
                                 int32_t x, int32_t y, int32_t w, int32_t h,
                                 lv_event_cb_t on_click, void * user_data);

lv_obj_t * ui_button_create_with_icon(lv_obj_t * parent, const lv_image_dsc_t * icon,
                                      int32_t x, int32_t y, int32_t w, int32_t h,
                                      lv_event_cb_t on_click, void * user_data);

/*
 * 注册按钮前景反色：按下时所有子标签改文字色、图片改 recolor，松开恢复。
 * 调用时机：内容添加完成之后。
 * 不希望被染色的子对象加 LV_OBJ_FLAG_USER_1 标记即可跳过。
 */
void ui_button_set_fg_colors(lv_obj_t * btn, lv_color_t normal, lv_color_t pressed);
