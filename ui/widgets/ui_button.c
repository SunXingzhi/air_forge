/**
 * @file ui_button.c
 */

#include "ui_button.h"

#include "ui_theme.h"

static void button_pressed_cb(lv_event_t * e)
{
    const ui_theme_t * theme = ui_theme();
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_event_get_user_data(e);

    /* 像素游戏手感：按下时整体下移 2px，同时反色 */
    lv_obj_set_style_translate_y(btn, 2, 0);
    lv_obj_set_style_bg_color(btn, theme->accent, 0);
    lv_obj_set_style_text_color(label, theme->accent_text, 0);
}

static void button_released_cb(lv_event_t * e)
{
    const ui_theme_t * theme = ui_theme();
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_event_get_user_data(e);

    lv_obj_set_style_translate_y(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, theme->panel, 0);
    lv_obj_set_style_text_color(label, theme->text, 0);
}

lv_obj_t * ui_button_create(lv_obj_t * parent, const char * text,
                            int32_t x, int32_t y, int32_t w, int32_t h,
                            lv_event_cb_t on_click, void * user_data)
{
    const ui_theme_t * theme = ui_theme();

    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, theme->panel, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn, theme->accent, 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    lv_obj_t * label = ui_text_create(btn, text, UI_FONT_TITLE, theme->text, 0, 0);
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, button_pressed_cb, LV_EVENT_PRESSED, label);
    lv_obj_add_event_cb(btn, button_released_cb, LV_EVENT_RELEASED, label);
    lv_obj_add_event_cb(btn, button_released_cb, LV_EVENT_PRESS_LOST, label);

    if(on_click) {
        lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, user_data);
    }

    return btn;
}
