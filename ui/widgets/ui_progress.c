/**
 * @file ui_progress.c
 */

#include "ui_progress.h"

#include "ui_theme.h"

typedef struct {
    int32_t segments;
    int32_t value;
} progress_ctx_t;

static void progress_delete_cb(lv_event_t * e)
{
    lv_obj_t * bar = lv_event_get_target(e);
    lv_free(lv_obj_get_user_data(bar));
    lv_obj_set_user_data(bar, NULL);
}

lv_obj_t * ui_progress_create(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h,
                              int32_t segments, int32_t value)
{
    const ui_theme_t * theme = ui_theme();

    lv_obj_t * bar = lv_obj_create(parent);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(bar, x, y);
    lv_obj_set_size(bar, w, h);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bar, 2, 0);

    for(int32_t i = 0; i < segments; i++) {
        lv_obj_t * block = lv_obj_create(bar);
        lv_obj_remove_flag(block, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(block, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_height(block, lv_pct(100));
        lv_obj_set_flex_grow(block, 1);
        lv_obj_set_style_bg_color(block, theme->dark, 0);
        lv_obj_set_style_bg_opa(block, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(block, theme->light, 0);
        lv_obj_set_style_border_width(block, 1, 0);
        lv_obj_set_style_radius(block, 0, 0);
        lv_obj_set_style_pad_all(block, 0, 0);
    }

    progress_ctx_t * ctx = lv_malloc(sizeof(progress_ctx_t));
    ctx->segments = segments;
    ctx->value = 0;
    lv_obj_set_user_data(bar, ctx);
    lv_obj_add_event_cb(bar, progress_delete_cb, LV_EVENT_DELETE, NULL);

    ui_progress_set_value(bar, value);
    return bar;
}

void ui_progress_set_value(lv_obj_t * bar, int32_t value)
{
    const ui_theme_t * theme = ui_theme();
    progress_ctx_t * ctx = lv_obj_get_user_data(bar);

    if(ctx == NULL) {
        return;
    }
    if(value < 0) value = 0;
    if(value > 100) value = 100;
    ctx->value = value;

    int32_t filled = (value * ctx->segments + 50) / 100;
    int32_t count = lv_obj_get_child_count(bar);

    for(int32_t i = 0; i < count; i++) {
        lv_obj_t * block = lv_obj_get_child(bar, i);
        lv_obj_set_style_bg_color(block, i < filled ? theme->accent : theme->dark, 0);
    }
}
