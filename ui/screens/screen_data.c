/**
 * @file screen_data.c
 * P4 实时数据：处理前后对比 + 60s 迷你趋势 + 传感器状态。
 */

#include "screen_data.h"

#include <stdio.h>

#include "ui.h"
#include "ui_theme.h"
#include "ui_state.h"

#include "ui_statusbar.h"

typedef struct {
    lv_obj_t * pm_b;
    lv_obj_t * pm_a;
    lv_obj_t * voc_b;
    lv_obj_t * voc_a;
    lv_obj_t * eff;
    lv_obj_t * sensors;
    lv_obj_t * chart;
    lv_chart_series_t * ser_before;
    lv_chart_series_t * ser_after;
    lv_timer_t * timer;
} data_ctx_t;

static void data_matrix_cell(lv_obj_t * parent, int32_t x, int32_t y,
                             const char * caption, lv_obj_t ** value,
                             const char * unit)
{
    const ui_theme_t * theme = ui_theme();

    lv_obj_t * cell = lv_obj_create(parent);
    lv_obj_remove_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(cell, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(cell, x, y);
    lv_obj_set_size(cell, 104, 38);
    lv_obj_set_style_bg_color(cell, theme->panel, 0);
    lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(cell, theme->dark, 0);
    lv_obj_set_style_border_width(cell, 1, 0);
    lv_obj_set_style_radius(cell, 0, 0);
    lv_obj_set_style_pad_all(cell, 0, 0);

    /* 标题含中文，用 12px；单位是纯 ASCII，保留 8px 小字 */
    ui_text_create(cell, caption, UI_FONT_TITLE, theme->muted, 6, 3);
    *value = ui_text_create(cell, "0", UI_FONT_TITLE, theme->text, 6, 17);
    ui_text_create(cell, unit, UI_FONT_SMALL, theme->muted, 58, 24);
}

static void data_refresh(lv_obj_t * root)
{
    data_ctx_t * ctx = lv_obj_get_user_data(root);
    char buf[32];

    if(ctx == NULL) {
        return;
    }

    snprintf(buf, sizeof(buf), "%03d", (int)g_ui.pm_before);
    lv_label_set_text(ctx->pm_b, buf);
    snprintf(buf, sizeof(buf), "%03d", (int)g_ui.pm_after);
    lv_label_set_text(ctx->pm_a, buf);

    if(g_ui.voc_ok) {
        snprintf(buf, sizeof(buf), "%.1f", (double)g_ui.voc_before);
        lv_label_set_text(ctx->voc_b, buf);
        snprintf(buf, sizeof(buf), "%.1f", (double)ui_voc_value(g_ui.voc_after));
        lv_label_set_text(ctx->voc_a, buf);
    }
    else {
        lv_label_set_text(ctx->voc_b, "---");
        lv_label_set_text(ctx->voc_a, "---");
    }

    snprintf(buf, sizeof(buf), "效率 %d%%", ui_efficiency_pct());
    lv_label_set_text(ctx->eff, buf);
    if(g_ui.overheat) {
        lv_label_set_text(ctx->sensors, "过热 风机已停");
        lv_obj_set_style_text_color(ctx->sensors, ui_theme()->warn, 0);
    }
    else if(g_ui.voc_ok) {
        lv_label_set_text(ctx->sensors, "PM正常  VOC正常");
        lv_obj_set_style_text_color(ctx->sensors, ui_theme()->muted, 0);
    }
    else {
        lv_label_set_text(ctx->sensors, "PM正常  VOC预热");
        lv_obj_set_style_text_color(ctx->sensors, ui_theme()->warn, 0);
    }

    int before = (int)(g_ui.pm_before * 2.0f);
    int after = (int)(g_ui.pm_after * 2.0f);
    if(before > 100) before = 100;
    if(after > 100) after = 100;
    lv_chart_set_next_value(ctx->chart, ctx->ser_before, before);
    lv_chart_set_next_value(ctx->chart, ctx->ser_after, after);
}

static void data_timer_cb(lv_timer_t * timer)
{
    data_refresh(lv_timer_get_user_data(timer));
}

static void data_delete_cb(lv_event_t * e)
{
    lv_obj_t * root = lv_event_get_target(e);
    data_ctx_t * ctx = lv_obj_get_user_data(root);

    if(ctx == NULL) {
        return;
    }
    if(ctx->timer) {
        lv_timer_delete(ctx->timer);
    }
    lv_free(ctx);
    lv_obj_set_user_data(root, NULL);
}

void screen_data_create(lv_obj_t * root)
{
    const ui_theme_t * theme = ui_theme();

    data_ctx_t * ctx = lv_malloc(sizeof(data_ctx_t));
    lv_memzero(ctx, sizeof(data_ctx_t));
    lv_obj_set_user_data(root, ctx);

    ui_attach_back(root);
    ui_statusbar_create(root);

    ui_text_create(root, "<", UI_FONT_TITLE, theme->text, 8, 33);
    ui_text_create(root, "实时数据", UI_FONT_TITLE, theme->text, 52, 33);
    ctx->eff = ui_text_create(root, "效率 0%", UI_FONT_TITLE, theme->accent, 0, 0);
    lv_obj_align(ctx->eff, LV_ALIGN_TOP_RIGHT, -12, 38);

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

    data_matrix_cell(root, 12, 56, "PM前", &ctx->pm_b, "ug/m3");
    data_matrix_cell(root, 122, 56, "PM后", &ctx->pm_a, "ug/m3");
    data_matrix_cell(root, 12, 100, "VOC前", &ctx->voc_b, "ppm");
    data_matrix_cell(root, 122, 100, "VOC后", &ctx->voc_a, "ppm");

    lv_obj_t * chart_box = lv_obj_create(root);
    lv_obj_remove_flag(chart_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(chart_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(chart_box, 12, 146);
    lv_obj_set_size(chart_box, 216, 82);
    lv_obj_set_style_bg_color(chart_box, theme->panel, 0);
    lv_obj_set_style_bg_opa(chart_box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(chart_box, theme->dark, 0);
    lv_obj_set_style_border_width(chart_box, 1, 0);
    lv_obj_set_style_radius(chart_box, 0, 0);
    lv_obj_set_style_pad_all(chart_box, 0, 0);

    ctx->chart = lv_chart_create(chart_box);
    lv_obj_set_pos(ctx->chart, 4, 4);
    lv_obj_set_size(ctx->chart, 208, 56);
    lv_chart_set_type(ctx->chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(ctx->chart, UI_HISTORY_LEN);
    lv_chart_set_axis_range(ctx->chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_div_line_count(ctx->chart, 2, 4);
    lv_obj_set_style_bg_opa(ctx->chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ctx->chart, 0, 0);
    lv_obj_set_style_pad_all(ctx->chart, 0, 0);
    lv_obj_set_style_line_width(ctx->chart, 1, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(ctx->chart, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_line_color(ctx->chart, theme->dark, LV_PART_MAIN);

    ctx->ser_before = lv_chart_add_series(ctx->chart, theme->muted,
                                          LV_CHART_AXIS_PRIMARY_Y);
    ctx->ser_after = lv_chart_add_series(ctx->chart, theme->accent,
                                         LV_CHART_AXIS_PRIMARY_Y);

    int start = (g_ui.hist_pos + UI_HISTORY_LEN - g_ui.hist_len) % UI_HISTORY_LEN;
    for(int32_t i = 0; i < UI_HISTORY_LEN; i++) {
        int32_t idx;
        int32_t before = 0;
        int32_t after = 0;
        if(i < g_ui.hist_len) {
            idx = (start + i) % UI_HISTORY_LEN;
            before = (int)(g_ui.hist_before[idx] * 2.0f);
            after = (int)(g_ui.hist_after[idx] * 2.0f);
        }
        lv_chart_set_next_value(ctx->chart, ctx->ser_before, before);
        lv_chart_set_next_value(ctx->chart, ctx->ser_after, after);
    }

    ui_text_create(chart_box, "60秒 PM2.5", UI_FONT_TITLE, theme->muted, 6, 62);
    lv_obj_t * legend = ui_text_create(chart_box, "处理前 / 处理后", UI_FONT_TITLE,
                                       theme->muted, 0, 62);
    lv_obj_align(legend, LV_ALIGN_TOP_RIGHT, -6, 62);

    ctx->sensors = ui_text_create(root, "PM正常  VOC预热", UI_FONT_TITLE,
                                  theme->warn, 12, 232);

    lv_obj_add_event_cb(root, data_delete_cb, LV_EVENT_DELETE, NULL);
    ctx->timer = lv_timer_create(data_timer_cb, 1000, root);

    data_refresh(root);
}
