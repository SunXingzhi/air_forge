/**
 * @file ui_row.c
 */

#include "ui_row.h"

#include "ui_theme.h"

typedef struct {
	lv_obj_t *label;
	lv_obj_t *value;
	lv_obj_t *cursor;
	ui_row_cb_t on_tap;
	ui_row_cb_t on_hold;
	void *user_data;
	bool selected;
	bool warn;
} row_ctx_t;

static void row_apply(row_ctx_t *ctx) {
	const ui_theme_t *theme = ui_theme();
	lv_obj_t *row = lv_obj_get_parent(ctx->label);

	if (ctx->selected) {
		lv_obj_set_style_bg_color(row, theme->accent, 0);
		lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
		lv_obj_set_style_text_color(ctx->label, theme->accent_text, 0);
		lv_obj_set_style_text_color(ctx->value, theme->accent_text, 0);
		lv_obj_remove_flag(ctx->cursor, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
		lv_obj_set_style_text_color(ctx->label, theme->text, 0);
		lv_obj_set_style_text_color(
		    ctx->value, ctx->warn ? theme->warn : theme->muted, 0);
		lv_obj_add_flag(ctx->cursor, LV_OBJ_FLAG_HIDDEN);
	}
}

static void row_click_cb(lv_event_t *e) {
	lv_obj_t *row = lv_event_get_target(e);
	row_ctx_t *ctx = lv_obj_get_user_data(row);

	if (ctx && ctx->on_tap) {
		ctx->on_tap(row, ctx->user_data);
	}
}

static void row_hold_cb(lv_event_t *e) {
	lv_obj_t *row = lv_event_get_target(e);
	row_ctx_t *ctx = lv_obj_get_user_data(row);

	if (ctx && ctx->on_hold) {
		ctx->on_hold(row, ctx->user_data);
	}
}

static void row_delete_cb(lv_event_t *e) {
	lv_obj_t *row = lv_event_get_target(e);
	lv_free(lv_obj_get_user_data(row));
	lv_obj_set_user_data(row, NULL);
}

lv_obj_t *ui_row_create(lv_obj_t *parent, int32_t x, int32_t y, int32_t w,
			int32_t h, const char *label, const char *value,
			ui_row_cb_t on_tap, ui_row_cb_t on_hold,
			void *user_data) {
	const ui_theme_t *theme = ui_theme();

	lv_obj_t *row = lv_obj_create(parent);
	lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(row, x, y);
	lv_obj_set_size(row, w, h);
	lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
	lv_obj_set_style_bg_color(row, theme->accent, LV_STATE_PRESSED);
	lv_obj_set_style_bg_opa(row, LV_OPA_30, LV_STATE_PRESSED);
	lv_obj_set_style_border_color(row, theme->dark, 0);
	lv_obj_set_style_border_width(row, 1, 0);
	lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
	lv_obj_set_style_radius(row, 0, 0);
	lv_obj_set_style_pad_all(row, 0, 0);

	row_ctx_t *ctx = lv_malloc(sizeof(row_ctx_t));
	lv_memzero(ctx, sizeof(row_ctx_t));
	ctx->on_tap = on_tap;
	ctx->on_hold = on_hold;
	ctx->user_data = user_data;
	lv_obj_set_user_data(row, ctx);

	/* 12px 字体行高 12，垂直居中到 44px 行内 */
	int32_t text_y = (h - 12) / 2;
	ctx->cursor = ui_text_create(row, ">", UI_FONT_TITLE, theme->accent, 4,
				     text_y);
	ctx->label =
	    ui_text_create(row, label, UI_FONT_TITLE, theme->text, 16, text_y);
	ctx->value = ui_text_create(row, value ? value : "", UI_FONT_TITLE,
				    theme->muted, 0, 0);
	lv_obj_align(ctx->value, LV_ALIGN_RIGHT_MID, -8, 0);

	lv_obj_add_event_cb(row, row_click_cb, LV_EVENT_CLICKED, NULL);
	lv_obj_add_event_cb(row, row_hold_cb, LV_EVENT_LONG_PRESSED, NULL);
	lv_obj_add_event_cb(row, row_delete_cb, LV_EVENT_DELETE, NULL);

	row_apply(ctx);
	return row;
}

void ui_row_set_value(lv_obj_t *row, const char *value) {
	row_ctx_t *ctx = lv_obj_get_user_data(row);

	if (ctx == NULL) {
		return;
	}
	lv_label_set_text(ctx->value, value);
}

void ui_row_set_selected(lv_obj_t *row, bool selected) {
	row_ctx_t *ctx = lv_obj_get_user_data(row);

	if (ctx == NULL) {
		return;
	}
	ctx->selected = selected;
	row_apply(ctx);
}

void ui_row_set_warn(lv_obj_t *row, bool warn) {
	row_ctx_t *ctx = lv_obj_get_user_data(row);

	if (ctx == NULL) {
		return;
	}
	ctx->warn = warn;
	row_apply(ctx);
}
