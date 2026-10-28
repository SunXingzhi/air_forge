/**
 * @file ui_slider.c
 */

#include "ui_slider.h"

#include "ui_theme.h"

typedef struct {
    ui_slider_cb_t on_change;
    void * user_data;
} slider_ctx_t;

static void slider_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    slider_ctx_t * ctx = lv_event_get_user_data(e);

    if(ctx && ctx->on_change) {
        ctx->on_change(slider, lv_slider_get_value(slider), ctx->user_data);
    }
}

static void slider_delete_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    slider_ctx_t * ctx = lv_event_get_user_data(e);
    lv_free(ctx);
    lv_obj_set_user_data(slider, NULL);
}

static void value_anim_cb(void * var, int32_t value)
{
    lv_obj_t * slider = (lv_obj_t *)var;
    lv_slider_set_value(slider, (value / 10) * 10, LV_ANIM_OFF);
    /* LVGL 9 的 lv_slider_set_value 不会主动发 VALUE_CHANGED，这里补一发 */
    lv_obj_send_event(slider, LV_EVENT_VALUE_CHANGED, NULL);
}

lv_obj_t * ui_slider_create(lv_obj_t * parent, int32_t x, int32_t y, int32_t w, int32_t h,
                            int32_t min, int32_t max, int32_t value,
                            ui_slider_cb_t on_change, void * user_data)
{
    const ui_theme_t * theme = ui_theme();

    lv_obj_t * slider = lv_slider_create(parent);
    lv_obj_set_pos(slider, x, y);
    lv_obj_set_size(slider, w, h);
    lv_slider_set_range(slider, min, max);
    lv_slider_set_value(slider, value, LV_ANIM_OFF);

    /* 轨道 */
    lv_obj_set_style_bg_color(slider, theme->dark, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(slider, theme->light, LV_PART_MAIN);
    lv_obj_set_style_border_width(slider, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(slider, 0, LV_PART_MAIN);

    /* 已填充部分 */
    lv_obj_set_style_bg_color(slider, theme->accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_border_width(slider, 0, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 0, LV_PART_INDICATOR);

    /* 方块滑块 */
    lv_obj_set_style_bg_color(slider, theme->panel, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_border_color(slider, theme->light, LV_PART_KNOB);
    lv_obj_set_style_border_width(slider, 2, LV_PART_KNOB);
    lv_obj_set_style_radius(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(slider, 0, LV_PART_KNOB);
    lv_obj_set_style_width(slider, 14, LV_PART_KNOB);
    lv_obj_set_style_height(slider, 14, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 0, LV_PART_KNOB);

    slider_ctx_t * ctx = lv_malloc(sizeof(slider_ctx_t));
    ctx->on_change = on_change;
    ctx->user_data = user_data;
    lv_obj_set_user_data(slider, ctx);

    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, ctx);
    lv_obj_add_event_cb(slider, slider_delete_cb, LV_EVENT_DELETE, ctx);

    return slider;
}

int32_t ui_slider_get_value(lv_obj_t * slider)
{
    return lv_slider_get_value(slider);
}

void ui_slider_set_value(lv_obj_t * slider, int32_t value, bool animate)
{
    if(!animate) {
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
        lv_obj_send_event(slider, LV_EVENT_VALUE_CHANGED, NULL);
        return;
    }

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, slider);
    lv_anim_set_values(&anim, lv_slider_get_value(slider), value);
    lv_anim_set_duration(&anim, 180);
    lv_anim_set_exec_cb(&anim, value_anim_cb);
    lv_anim_set_path_cb(&anim, lv_anim_path_linear);
    lv_anim_start(&anim);
}
