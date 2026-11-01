/**
 * @file ui_statusbar.c
 */

#include "ui_statusbar.h"

#include "ui_state.h"
#include "ui_theme.h"

typedef struct {
	lv_obj_t *fan;
	lv_obj_t *filter;
	lv_obj_t *alert;
	lv_timer_t *timer;
} statusbar_ctx_t;

void ui_statusbar_refresh(lv_obj_t *bar) {
	const ui_theme_t *theme = ui_theme();
	statusbar_ctx_t *ctx = lv_obj_get_user_data(bar);

	if (ctx == NULL) {
		return;
	}

	int effective = ui_fan_effective();
	if (g_ui.fan == UI_FAN_AUTO) {
		lv_label_set_text(ctx->fan, "自动");
	} else {
		lv_label_set_text_fmt(ctx->fan, "风量 %d", effective);
	}
	lv_obj_set_style_text_color(
	    ctx->fan, effective != UI_FAN_STOP ? theme->accent : theme->muted,
	    0);

	int min_pct = ui_filter_min_pct();
	lv_label_set_text_fmt(ctx->filter, "滤芯 %d%%", min_pct);
	lv_obj_set_style_text_color(
	    ctx->filter, min_pct <= 15 ? theme->warn : theme->text, 0);

	bool alert = min_pct <= 15 || !g_ui.voc_ok || g_ui.overheat;
	lv_label_set_text(ctx->alert, alert ? "!" : "");
	lv_obj_set_style_text_color(ctx->alert, theme->warn, 0);
}

static void statusbar_timer_cb(lv_timer_t *timer) {
	ui_statusbar_refresh(lv_timer_get_user_data(timer));
}

static void statusbar_delete_cb(lv_event_t *e) {
	lv_obj_t *bar = lv_event_get_target(e);
	statusbar_ctx_t *ctx = lv_obj_get_user_data(bar);

	if (ctx == NULL) {
		return;
	}
	if (ctx->timer) {
		lv_timer_delete(ctx->timer);
	}
	lv_free(ctx);
	lv_obj_set_user_data(bar, NULL);
}

lv_obj_t *ui_statusbar_create(lv_obj_t *parent) {
	const ui_theme_t *theme = ui_theme();

	lv_obj_t *bar = lv_obj_create(parent);
	lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_remove_flag(bar, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(bar, 0, 0);
	lv_obj_set_size(bar, 240, 28);
	lv_obj_set_style_bg_color(bar, theme->panel, 0);
	lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(bar, theme->dark, 0);
	lv_obj_set_style_border_width(bar, 2, 0);
	lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, 0);
	lv_obj_set_style_radius(bar, 0, 0);
	lv_obj_set_style_pad_all(bar, 0, 0);

	statusbar_ctx_t *ctx = lv_malloc(sizeof(statusbar_ctx_t));
	lv_memzero(ctx, sizeof(statusbar_ctx_t));
	lv_obj_set_user_data(bar, ctx);

	/* 中文在 8px 像素字体下笔画会糊，状态栏统一用 12px */
	ctx->fan =
	    ui_text_create(bar, "风量 0", UI_FONT_TITLE, theme->muted, 8, 8);
	ctx->filter =
	    ui_text_create(bar, "滤芯 100%", UI_FONT_TITLE, theme->text, 64, 8);
	ctx->alert =
	    ui_text_create(bar, "", UI_FONT_TITLE, theme->warn, 136, 8);

	lv_obj_add_event_cb(bar, statusbar_delete_cb, LV_EVENT_DELETE, NULL);
	ctx->timer = lv_timer_create(statusbar_timer_cb, 1000, bar);

	ui_statusbar_refresh(bar);
	return bar;
}
