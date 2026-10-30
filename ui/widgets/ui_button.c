/**
 * @file ui_button.c
 * 像素按钮外壳 + 文本/图标组合，支持任意子内容。
 */

#include "ui_button.h"

#include "ui_theme.h"

typedef struct {
    lv_color_t fg_normal;
    lv_color_t fg_pressed;
    bool tint_enabled;
} btn_ctx_t;

/* 递归染色：标签改文字色，图片改 recolor；USER_1 标记的对象跳过 */
static void tint_tree(lv_obj_t * obj, lv_color_t color)
{
    if(lv_obj_has_flag(obj, LV_OBJ_FLAG_USER_1)) {
        return;
    }

    if(lv_obj_check_type(obj, &lv_label_class)) {
        lv_obj_set_style_text_color(obj, color, 0);
    }
    else if(lv_obj_check_type(obj, &lv_image_class)) {
        lv_obj_set_style_image_recolor(obj, color, 0);
        lv_obj_set_style_image_recolor_opa(obj, LV_OPA_COVER, 0);
    }

    uint32_t count = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < count; i++) {
        tint_tree(lv_obj_get_child(obj, i), color);
    }
}

static void button_pressed_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target(e);
    btn_ctx_t * ctx = lv_obj_get_user_data(btn);

    if(ctx && ctx->tint_enabled) {
        tint_tree(btn, ctx->fg_pressed);
    }
}

static void button_released_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target(e);
    btn_ctx_t * ctx = lv_obj_get_user_data(btn);

    if(ctx && ctx->tint_enabled) {
        tint_tree(btn, ctx->fg_normal);
    }
}

static void button_delete_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_current_target(e);
    lv_free(lv_obj_get_user_data(btn));
    lv_obj_set_user_data(btn, NULL);
}

lv_obj_t * ui_button_create(lv_obj_t * parent,
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

    /* 按下视觉由状态样式驱动，不再依赖任何具体内容对象 */
    lv_obj_set_style_translate_y(btn, 2, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(btn, theme->accent, LV_STATE_PRESSED);

    btn_ctx_t * ctx = lv_malloc(sizeof(btn_ctx_t));
    lv_memzero(ctx, sizeof(btn_ctx_t));
    ctx->fg_normal = theme->text;
    ctx->fg_pressed = theme->accent_text;
    lv_obj_set_user_data(btn, ctx);

    lv_obj_add_event_cb(btn, button_pressed_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(btn, button_released_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(btn, button_released_cb, LV_EVENT_PRESS_LOST, NULL);
    lv_obj_add_event_cb(btn, button_delete_cb, LV_EVENT_DELETE, NULL);

    if(on_click) {
        lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, user_data);
    }

    return btn;
}

void ui_button_set_fg_colors(lv_obj_t * btn, lv_color_t normal, lv_color_t pressed)
{
    btn_ctx_t * ctx = lv_obj_get_user_data(btn);

    if(ctx == NULL) {
        return;
    }
    ctx->fg_normal = normal;
    ctx->fg_pressed = pressed;
    ctx->tint_enabled = true;

    tint_tree(btn, normal);
}

lv_obj_t * ui_button_create_text(lv_obj_t * parent, const char * text,
                                 int32_t x, int32_t y, int32_t w, int32_t h,
                                 lv_event_cb_t on_click, void * user_data)
{
    const ui_theme_t * theme = ui_theme();

    lv_obj_t * btn = ui_button_create(parent, x, y, w, h, on_click, user_data);
    lv_obj_t * label = ui_text_create(btn, text, UI_FONT_TITLE, theme->text, 0, 0);
    lv_obj_center(label);

    ui_button_set_fg_colors(btn, theme->text, theme->accent_text);
    return btn;
}

lv_obj_t * ui_button_create_with_icon(lv_obj_t * parent, const lv_image_dsc_t * icon_src,
                                      int32_t x, int32_t y, int32_t w, int32_t h,
                                      lv_event_cb_t on_click, void * user_data)
{
    const ui_theme_t * theme = ui_theme();

    lv_obj_t * btn = ui_button_create(parent, x, y, w, h, on_click, user_data);
    lv_obj_t * icon = lv_image_create(btn);
    lv_image_set_src(icon, icon_src);
    lv_obj_remove_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(icon);

    ui_button_set_fg_colors(btn, theme->text, theme->accent_text);
    return btn;
}
