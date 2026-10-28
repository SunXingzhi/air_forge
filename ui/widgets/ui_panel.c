/**
 * @file ui_panel.c
 */

#include "ui_panel.h"

#include "ui_theme.h"

lv_obj_t * ui_panel_create(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h)
{
    const ui_theme_t * theme = ui_theme();

    /* 先放一块实心暗色，偏移 3px，模拟像素游戏的硬阴影 */
    lv_obj_t * shadow = lv_obj_create(parent);
    lv_obj_remove_flag(shadow, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(shadow, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(shadow, x + 3, y + 3);
    lv_obj_set_size(shadow, w, h);
    lv_obj_set_style_bg_color(shadow, theme->dark, 0);
    lv_obj_set_style_bg_opa(shadow, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(shadow, 0, 0);
    lv_obj_set_style_radius(shadow, 0, 0);
    lv_obj_set_style_pad_all(shadow, 0, 0);

    lv_obj_t * panel = lv_obj_create(parent);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, w, h);
    lv_obj_set_style_bg_color(panel, theme->panel, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_image_src(panel, theme->texture, 0);
    lv_obj_set_style_bg_image_tiled(panel, true, 0);
    lv_obj_set_style_bg_image_opa(panel, LV_OPA_60, 0);
    lv_obj_set_style_border_color(panel, theme->light, 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_pad_all(panel, 8, 0);

    return panel;
}
