/**
 * @file screen_fan.c
 * P2 风量控制：停 / 弱 / 中 / 强 / 自动，44px 行。
 */

#include "screen_fan.h"

#include "ui.h"
#include "ui_theme.h"
#include "ui_state.h"
#include "ui_popup.h"

#include "ui_statusbar.h"

typedef struct {
    lv_obj_t * row;
    lv_obj_t * cursor;
    lv_obj_t * label;
    lv_obj_t * bars[4];
    int32_t index;
} fan_row_t;

typedef struct {
    fan_row_t rows[5];
    lv_timer_t * timer;
} fan_ctx_t;

static void fan_apply(fan_ctx_t * ctx)
{
    const ui_theme_t * theme = ui_theme();

    for(int32_t i = 0; i < 5; i++) {
        fan_row_t * r = &ctx->rows[i];
        bool selected = (g_ui.fan == (ui_fan_t)i);

        lv_obj_set_style_bg_color(r->row, selected ? theme->accent : theme->panel, 0);
        lv_obj_set_style_bg_opa(r->row, selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(r->label, selected ? theme->accent_text : theme->text, 0);
        lv_obj_set_style_text_color(r->cursor,
                                    selected ? theme->accent_text : theme->accent, 0);
        if(selected) {
            lv_obj_remove_flag(r->cursor, LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_add_flag(r->cursor, LV_OBJ_FLAG_HIDDEN);
        }

        int fill = (int)i;
        for(int32_t b = 0; b < 4; b++) {
            lv_obj_set_style_bg_color(r->bars[b],
                                      b < fill ? (selected ? theme->accent_text
                                                           : theme->accent)
                                               : theme->dark,
                                      0);
        }
    }
}

static void fan_refresh(lv_obj_t * root)
{
    fan_ctx_t * ctx = lv_obj_get_user_data(root);
    if(ctx) {
        fan_apply(ctx);
    }
}

static void fan_timer_cb(lv_timer_t * timer)
{
    fan_refresh(lv_timer_get_user_data(timer));
}

static void fan_row_cb(lv_event_t * e)
{
    lv_obj_t * row = lv_event_get_target(e);
    fan_ctx_t * ctx = lv_event_get_user_data(e);
    int32_t index = (int32_t)(lv_intptr_t)lv_obj_get_user_data(row);

    if(ctx == NULL || index < 0 || index >= 5) {
        return;
    }

    g_ui.fan = (ui_fan_t)index;
    fan_apply(ctx);

    char buf[24];
    lv_snprintf(buf, sizeof(buf), "风量: %s", ui_fan_name(g_ui.fan));
    ui_toast(buf);
}

static void fan_delete_cb(lv_event_t * e)
{
    lv_obj_t * root = lv_event_get_target(e);
    fan_ctx_t * ctx = lv_obj_get_user_data(root);

    if(ctx == NULL) {
        return;
    }
    if(ctx->timer) {
        lv_timer_delete(ctx->timer);
    }
    lv_free(ctx);
    lv_obj_set_user_data(root, NULL);
}

static void fan_row_create(lv_obj_t * parent, fan_ctx_t * ctx, int32_t index,
                           const char * name, int32_t y)
{
    const ui_theme_t * theme = ui_theme();
    fan_row_t * r = &ctx->rows[index];

    r->row = lv_obj_create(parent);
    lv_obj_remove_flag(r->row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(r->row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(r->row, 12, y);
    lv_obj_set_size(r->row, 216, 44);
    lv_obj_set_style_bg_opa(r->row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(r->row, theme->dark, 0);
    lv_obj_set_style_border_width(r->row, 1, 0);
    lv_obj_set_style_border_side(r->row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(r->row, 0, 0);
    lv_obj_set_style_pad_all(r->row, 0, 0);

    r->cursor = ui_text_create(r->row, ">", UI_FONT_TITLE, theme->accent, 4, 16);
    r->label = ui_text_create(r->row, name, UI_FONT_TITLE, theme->text, 18, 16);

    for(int32_t b = 0; b < 4; b++) {
        r->bars[b] = lv_obj_create(r->row);
        lv_obj_remove_flag(r->bars[b], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(r->bars[b], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_pos(r->bars[b], 150 + b * 8, 16);
        lv_obj_set_size(r->bars[b], 6, 12);
        lv_obj_set_style_bg_color(r->bars[b], theme->dark, 0);
        lv_obj_set_style_bg_opa(r->bars[b], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(r->bars[b], 0, 0);
        lv_obj_set_style_radius(r->bars[b], 0, 0);
        lv_obj_set_style_pad_all(r->bars[b], 0, 0);
    }

    lv_obj_set_user_data(r->row, (void *)(lv_intptr_t)index);
    lv_obj_add_event_cb(r->row, fan_row_cb, LV_EVENT_CLICKED, ctx);
}

void screen_fan_create(lv_obj_t * root)
{
    const ui_theme_t * theme = ui_theme();

    fan_ctx_t * ctx = lv_malloc(sizeof(fan_ctx_t));
    lv_memzero(ctx, sizeof(fan_ctx_t));
    lv_obj_set_user_data(root, ctx);

    ui_attach_back(root);
    ui_statusbar_create(root);

    ui_text_create(root, "<", UI_FONT_TITLE, theme->text, 8, 33);
    ui_text_create(root, "风量控制", UI_FONT_TITLE, theme->text, 52, 33);

    lv_obj_t * line = lv_obj_create(root);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(line, 0, 52);
    lv_obj_set_size(line, 240, 2);
    lv_obj_set_style_bg_color(line, theme->accent, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);

    fan_row_create(root, ctx, 0, "停止", 56);
    fan_row_create(root, ctx, 1, "弱", 100);
    fan_row_create(root, ctx, 2, "中", 144);
    fan_row_create(root, ctx, 3, "强", 188);
    fan_row_create(root, ctx, 4, "自动", 232);

    lv_obj_add_event_cb(root, fan_delete_cb, LV_EVENT_DELETE, NULL);
    ctx->timer = lv_timer_create(fan_timer_cb, 1000, root);

    fan_apply(ctx);
}
