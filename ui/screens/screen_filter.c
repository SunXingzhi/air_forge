/**
 * @file screen_filter.c
 * P3 滤芯寿命：四条寿命条 + 长按复位。
 */

#include "screen_filter.h"

#include <stdio.h>

#include "ui.h"
#include "ui_theme.h"
#include "ui_state.h"
#include "ui_popup.h"

#include "ui_progress.h"
#include "ui_statusbar.h"

typedef struct {
    lv_obj_t * pct;
    lv_obj_t * days;
    lv_obj_t * bar;
} filter_row_t;

typedef struct {
    filter_row_t rows[UI_FILTER_COUNT];
    lv_timer_t * timer;
} filter_ctx_t;

static void filter_refresh(lv_obj_t * root)
{
    const ui_theme_t * theme = ui_theme();
    filter_ctx_t * ctx = lv_obj_get_user_data(root);
    char buf[16];

    if(ctx == NULL) {
        return;
    }

    for(int32_t i = 0; i < UI_FILTER_COUNT; i++) {
        int pct = g_ui.filter_pct[i];
        snprintf(buf, sizeof(buf), "%d%%", pct);
        lv_label_set_text(ctx->rows[i].pct, buf);
        lv_obj_set_style_text_color(ctx->rows[i].pct,
                                    pct <= 15 ? theme->warn : theme->text, 0);

        ui_filter_days(i, buf, sizeof(buf));
        lv_label_set_text(ctx->rows[i].days, buf);
        lv_obj_set_style_text_color(ctx->rows[i].days,
                                    pct <= 15 ? theme->warn : theme->muted, 0);

        ui_progress_set_value(ctx->rows[i].bar, pct);
    }
}

static void filter_timer_cb(lv_timer_t * timer)
{
    filter_refresh(lv_timer_get_user_data(timer));
}

static void filter_row_hold_cb(lv_event_t * e)
{
    lv_obj_t * row = lv_event_get_target(e);
    filter_ctx_t * ctx = lv_event_get_user_data(e);
    int32_t index = (int32_t)(lv_intptr_t)lv_obj_get_user_data(row);
    char buf[24];

    if(ctx == NULL || index < 0 || index >= UI_FILTER_COUNT) {
        return;
    }

    ui_filter_reset(index);
    filter_refresh(lv_obj_get_parent(row));

    static const char *const names[] = {"金属网", "中效棉", "活性炭A", "活性炭B"};
    lv_snprintf(buf, sizeof(buf), "%s复位", names[index]);
    ui_toast(buf);
}

static void filter_delete_cb(lv_event_t * e)
{
    lv_obj_t * root = lv_event_get_target(e);
    filter_ctx_t * ctx = lv_obj_get_user_data(root);

    if(ctx == NULL) {
        return;
    }
    if(ctx->timer) {
        lv_timer_delete(ctx->timer);
    }
    lv_free(ctx);
    lv_obj_set_user_data(root, NULL);
}

static void filter_row_create(lv_obj_t * root, filter_ctx_t * ctx, int32_t index,
                              const char * name, int32_t y)
{
    const ui_theme_t * theme = ui_theme();
    filter_row_t * r = &ctx->rows[index];

    lv_obj_t * row = lv_obj_create(root);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(row, 12, y);
    lv_obj_set_size(row, 216, 44);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_color(row, theme->accent, LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(row, LV_OPA_30, LV_STATE_PRESSED);
    lv_obj_set_style_border_color(row, theme->dark, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    ui_text_create(row, name, UI_FONT_TITLE, theme->text, 8, 16);
    r->bar = ui_progress_create(row, 78, 18, 80, 8, 10, 0);
    r->pct = ui_text_create(row, "100%", UI_FONT_TITLE, theme->text, 164, 8);
    /* 内容为“NN天”，含中文，必须用 12px */
    r->days = ui_text_create(row, "30天", UI_FONT_TITLE, theme->muted, 164, 26);

    lv_obj_set_user_data(row, (void *)(lv_intptr_t)index);
    lv_obj_add_event_cb(row, filter_row_hold_cb, LV_EVENT_LONG_PRESSED, ctx);
}

void screen_filter_create(lv_obj_t * root)
{
    const ui_theme_t * theme = ui_theme();

    filter_ctx_t * ctx = lv_malloc(sizeof(filter_ctx_t));
    lv_memzero(ctx, sizeof(filter_ctx_t));
    lv_obj_set_user_data(root, ctx);

    ui_attach_back(root);
    ui_statusbar_create(root);

    ui_text_create(root, "<", UI_FONT_TITLE, theme->text, 8, 33);
    ui_text_create(root, "滤芯寿命", UI_FONT_TITLE, theme->text, 52, 33);

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

    filter_row_create(root, ctx, UI_FILTER_MESH, "金属网", 56);
    filter_row_create(root, ctx, UI_FILTER_MEDIA, "中效棉", 100);
    filter_row_create(root, ctx, UI_FILTER_CARBON_A, "活性炭A", 144);
    filter_row_create(root, ctx, UI_FILTER_CARBON_B, "活性炭B", 188);

    ui_text_create(root, "长按 1.5s 复位", UI_FONT_TITLE, theme->muted, 12, 238);

    lv_obj_add_event_cb(root, filter_delete_cb, LV_EVENT_DELETE, NULL);
    ctx->timer = lv_timer_create(filter_timer_cb, 1000, root);

    filter_refresh(root);
}
