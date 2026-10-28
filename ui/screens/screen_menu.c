/**
 * @file screen_menu.c
 * 设置页：滚动列表 + 亮度滑条 + 主题切换。
 */

#include "screen_menu.h"

#include "ui.h"
#include "ui_theme.h"

#include "ui_button.h"
#include "ui_list.h"
#include "ui_panel.h"
#include "ui_slider.h"

static const char * const menu_items[] = {
    "FAN MODE",
    "OSCILLATE",
    "FILTER RST",
    "SOUND",
    "ABOUT",
    "POWER OFF",
};

static lv_obj_t * light_pct_label;

static void light_slider_cb(lv_obj_t * slider, int32_t value, void * user_data)
{
    LV_UNUSED(slider);
    LV_UNUSED(user_data);
    lv_label_set_text_fmt(light_pct_label, "%d%%", (int)value);
}

static void back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_goto(UI_SCREEN_HOME, true);
}

static void theme_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_theme_next();
    ui_reload();
}

static void menu_list_cb(lv_obj_t * list, int32_t index, void * user_data)
{
    LV_UNUSED(list);
    LV_UNUSED(user_data);
    LV_UNUSED(index);
}

void screen_menu_create(lv_obj_t * root)
{
    const ui_theme_t * theme = ui_theme();

    /* ---------- 顶部：返回 + 标题 ---------- */
    ui_button_create(root, "<", 12, 2, 36, 24, back_cb, NULL);
    ui_text_create(root, "MENU", UI_FONT_TITLE, theme->text, 60, 6);

    lv_obj_t * line = lv_obj_create(root);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(line, 0, 30);
    lv_obj_set_size(line, 240, 2);
    lv_obj_set_style_bg_color(line, theme->accent, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);

    /* ---------- 设置列表 ---------- */
    lv_obj_t * list_panel = ui_panel_create(root, 12, 34, 216, 148);
    lv_obj_set_style_pad_all(list_panel, 4, 0);
    ui_list_create(list_panel, menu_items, 6, 0, 0, 204, 140, menu_list_cb, NULL);

    /* ---------- 亮度滑条 ---------- */
    ui_text_create(root, "LIGHT", UI_FONT_TITLE, theme->text, 14, 186);
    light_pct_label = ui_text_create(root, "60%", UI_FONT_TITLE, theme->text, 0, 186);
    lv_obj_align(light_pct_label, LV_ALIGN_TOP_RIGHT, -14, 186);

    ui_slider_create(root, 14, 206, 212, 14, 0, 100, 60, light_slider_cb, NULL);

    /* ---------- 换肤按钮 ---------- */
    char text[32];
    lv_snprintf(text, sizeof(text), "THEME: %s", ui_theme_name());
    ui_button_create(root, text, 12, 228, 216, 28, theme_cb, NULL);
}
