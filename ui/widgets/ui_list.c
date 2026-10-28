/**
 * @file ui_list.c
 */

#include "ui_list.h"

#include "ui_theme.h"

#define ROW_H 22

typedef struct {
	lv_obj_t *hl;
	lv_obj_t *arrow;
	lv_obj_t **rows;
	lv_obj_t **labels;
	int32_t count;
	int32_t selected;
	ui_list_cb_t on_select;
	void *user_data;
	lv_timer_t *blink_timer;
	bool arrow_visible;
} list_ctx_t;

/* 选中块按 6px 阶梯移动，避免平滑插值破坏像素感 */
static void highlight_anim_cb(void *var, int32_t value) {
	lv_obj_set_y((lv_obj_t *)var, (value / 6) * 6);
}

static void blink_timer_cb(lv_timer_t *timer) {
	list_ctx_t *ctx = lv_timer_get_user_data(timer);
	ctx->arrow_visible = !ctx->arrow_visible;
	if (ctx->arrow_visible) {
		lv_obj_remove_flag(ctx->arrow, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(ctx->arrow, LV_OBJ_FLAG_HIDDEN);
	}
}

static void list_delete_cb(lv_event_t *e) {
	lv_obj_t *list = lv_event_get_target(e);
	list_ctx_t *ctx = lv_obj_get_user_data(list);

	if (ctx == NULL) {
		return;
	}
	if (ctx->blink_timer) {
		lv_timer_delete(ctx->blink_timer);
	}
	lv_free(ctx->rows);
	lv_free(ctx->labels);
	lv_free(ctx);
	lv_obj_set_user_data(list, NULL);
}

static void list_select_apply(lv_obj_t *list, int32_t index, bool animate) {
	list_ctx_t *ctx = lv_obj_get_user_data(list);
	const ui_theme_t *theme = ui_theme();

	if (ctx == NULL || ctx->count == 0) {
		return;
	}
	if (index < 0)
		index = 0;
	if (index >= ctx->count)
		index = ctx->count - 1;

	ctx->selected = index;

	for (int32_t i = 0; i < ctx->count; i++) {
		lv_obj_set_style_text_color(
		    ctx->labels[i],
		    i == index ? theme->accent_text : theme->text, 0);
	}

	int32_t target_y = index * ROW_H;
	if (animate && lv_obj_get_y(ctx->hl) != target_y) {
		lv_anim_t anim;
		lv_anim_init(&anim);
		lv_anim_set_var(&anim, ctx->hl);
		lv_anim_set_values(&anim, lv_obj_get_y(ctx->hl), target_y);
		lv_anim_set_duration(&anim, 90);
		lv_anim_set_exec_cb(&anim, highlight_anim_cb);
		lv_anim_set_path_cb(&anim, lv_anim_path_linear);
		lv_anim_start(&anim);
	} else {
		lv_obj_set_y(ctx->hl, target_y);
	}

	lv_obj_scroll_to_view(ctx->rows[index], LV_ANIM_OFF);
}

static void row_click_cb(lv_event_t *e) {
	lv_obj_t *row = lv_event_get_target(e);
	lv_obj_t *list = lv_event_get_user_data(e);
	list_ctx_t *ctx = lv_obj_get_user_data(list);
	int32_t index = (int32_t)(lv_intptr_t)lv_obj_get_user_data(row);

	if (ctx == NULL) {
		return;
	}
	list_select_apply(list, index, true);
	if (ctx->on_select) {
		ctx->on_select(list, index, ctx->user_data);
	}
}

lv_obj_t *ui_list_create(lv_obj_t *parent, const char *const items[],
			 int32_t count, int32_t x, int32_t y, int32_t w,
			 int32_t h, ui_list_cb_t on_select, void *user_data) {
	const ui_theme_t *theme = ui_theme();

	lv_obj_t *list = lv_obj_create(parent);
	lv_obj_remove_flag(list, LV_OBJ_FLAG_SCROLL_ELASTIC);
	lv_obj_set_pos(list, x, y);
	lv_obj_set_size(list, w, h);
	lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(list, 0, 0);
	lv_obj_set_style_radius(list, 0, 0);
	lv_obj_set_style_pad_all(list, 0, 0);
	lv_obj_set_scroll_dir(list, LV_DIR_VER);
	lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

	list_ctx_t *ctx = lv_malloc(sizeof(list_ctx_t));
	lv_memzero(ctx, sizeof(list_ctx_t));
	ctx->count = count;
	ctx->on_select = on_select;
	ctx->user_data = user_data;
	ctx->rows = lv_malloc(sizeof(lv_obj_t *) * count);
	ctx->labels = lv_malloc(sizeof(lv_obj_t *) * count);
	lv_obj_set_user_data(list, ctx);

	/* 选中底色块（先创建，位于所有行下面） */
	ctx->hl = lv_obj_create(list);
	lv_obj_remove_flag(ctx->hl, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_remove_flag(ctx->hl, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(ctx->hl, 0, 0);
	lv_obj_set_size(ctx->hl, w, ROW_H);
	lv_obj_set_style_bg_color(ctx->hl, theme->accent, 0);
	lv_obj_set_style_bg_opa(ctx->hl, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(ctx->hl, 0, 0);
	lv_obj_set_style_radius(ctx->hl, 0, 0);
	lv_obj_set_style_pad_all(ctx->hl, 0, 0);

	ctx->arrow = ui_text_create(ctx->hl, ">", UI_FONT_TITLE,
				    theme->accent_text, 3, 3);
	ctx->arrow_visible = true;

	for (int32_t i = 0; i < count; i++) {
		lv_obj_t *row = lv_obj_create(list);
		lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_set_pos(row, 0, i * ROW_H);
		lv_obj_set_size(row, w, ROW_H);
		lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(row, 0, 0);
		lv_obj_set_style_radius(row, 0, 0);
		lv_obj_set_style_pad_all(row, 0, 0);

		ctx->rows[i] = row;
		ctx->labels[i] = ui_text_create(row, items[i], UI_FONT_TITLE,
						theme->text, 18, 3);
		lv_obj_set_user_data(row, (void *)(lv_intptr_t)i);
		lv_obj_add_event_cb(row, row_click_cb, LV_EVENT_CLICKED, list);
	}

	ctx->blink_timer = lv_timer_create(blink_timer_cb, 320, ctx);
	lv_obj_add_event_cb(list, list_delete_cb, LV_EVENT_DELETE, NULL);

	list_select_apply(list, 0, false);
	return list;
}

int32_t ui_list_get_selected(lv_obj_t *list) {
	list_ctx_t *ctx = lv_obj_get_user_data(list);
	return ctx ? ctx->selected : 0;
}

void ui_list_select(lv_obj_t *list, int32_t index, bool animate) {
	list_select_apply(list, index, animate);
}
