/**
 * @file screen_home.c
 * P1 主页：状态栏 + 处理前/后数据 + 2x2 入口 + 运行时长。
 */

#include "screen_home.h"

#include <stdio.h>

#include "icon.h"
#include "ui.h"
#include "ui_state.h"
#include "ui_theme.h"

#include "ui_button.h"
#include "ui_panel.h"
#include "ui_statusbar.h"

typedef struct {
	lv_obj_t *pm_before;
	lv_obj_t *pm_after;
	lv_obj_t *voc_before;
	lv_obj_t *voc_after;
	lv_obj_t *voc_unit;
	lv_obj_t *eff;
	lv_obj_t *run;
	lv_obj_t *total;
	lv_timer_t *timer;
} home_ctx_t;

static void home_refresh(lv_obj_t *root) {
	home_ctx_t *ctx = lv_obj_get_user_data(root);
	char buf[24];

	if (ctx == NULL) {
		return;
	}

	snprintf(buf, sizeof(buf), "%03d", (int)g_ui.pm_before);
	lv_label_set_text(ctx->pm_before, buf);
	snprintf(buf, sizeof(buf), "%03d", (int)g_ui.pm_after);
	lv_label_set_text(ctx->pm_after, buf);

	if (g_ui.voc_ok) {
		snprintf(buf, sizeof(buf), "%.1f", (double)g_ui.voc_before);
		lv_label_set_text(ctx->voc_before, buf);
		snprintf(buf, sizeof(buf), "%.1f",
			 (double)ui_voc_value(g_ui.voc_after));
		lv_label_set_text(ctx->voc_after, buf);
		lv_label_set_text(ctx->voc_unit, ui_voc_unit_name());
	} else {
		lv_label_set_text(ctx->voc_before, "---");
		lv_label_set_text(ctx->voc_after, "---");
		lv_label_set_text(ctx->voc_unit, "预热");
	}

	snprintf(buf, sizeof(buf), "%d%%", ui_efficiency_pct());
	lv_label_set_text(ctx->eff, buf);

	char time_buf[16];
	ui_runtime_str(time_buf, sizeof(time_buf));
	snprintf(buf, sizeof(buf), "运行 %s", time_buf);
	lv_label_set_text(ctx->run, buf);

	ui_total_str(time_buf, sizeof(time_buf));
	snprintf(buf, sizeof(buf), "累计 %s", time_buf);
	lv_label_set_text(ctx->total, buf);
}

static void home_timer_cb(lv_timer_t *timer) {
	home_refresh(lv_timer_get_user_data(timer));
}

static void home_delete_cb(lv_event_t *e) {
	lv_obj_t *root = lv_event_get_target(e);
	home_ctx_t *ctx = lv_obj_get_user_data(root);

	if (ctx == NULL) {
		return;
	}
	if (ctx->timer) {
		lv_timer_delete(ctx->timer);
	}
	lv_free(ctx);
	lv_obj_set_user_data(root, NULL);
}

static void data_double_cb(lv_event_t *e) {
	LV_UNUSED(e);
	ui_key_fn();
}

static void data_long_cb(lv_event_t *e) {
	LV_UNUSED(e);
	ui_goto(UI_SCREEN_SETTINGS, false);
}

static void tile_fan_cb(lv_event_t *e) {
	LV_UNUSED(e);
	ui_goto(UI_SCREEN_FAN, false);
}

static void tile_filter_cb(lv_event_t *e) {
	LV_UNUSED(e);
	ui_goto(UI_SCREEN_FILTER, false);
}

static void tile_data_cb(lv_event_t *e) {
	LV_UNUSED(e);
	ui_goto(UI_SCREEN_DATA, false);
}

static void tile_settings_cb(lv_event_t *e) {
	LV_UNUSED(e);
	ui_goto(UI_SCREEN_SETTINGS, false);
}

static lv_obj_t *home_tile(lv_obj_t *root, int32_t x, int32_t y,
			   const char *title, const char *sub,
			   const lv_image_dsc_t *icon, lv_event_cb_t cb) {
	const ui_theme_t *theme = ui_theme();

	lv_obj_t *btn = ui_button_create(root, x, y, 104, 48, cb, NULL);

	lv_obj_t *img = lv_image_create(btn);
	lv_image_set_src(img, icon);
	lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(img, 6, 12);
	lv_obj_set_style_image_recolor(img, theme->text, 0);
	lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);

	ui_text_create(btn, title, UI_FONT_TITLE, theme->text, 34, 6);
	ui_text_create(btn, sub, UI_FONT_TITLE, theme->muted, 34, 26);
	ui_button_set_fg_colors(btn, theme->text, theme->accent_text);
	return btn;
}

void screen_home_create(lv_obj_t *root) {
	const ui_theme_t *theme = ui_theme();

	home_ctx_t *ctx = lv_malloc(sizeof(home_ctx_t));
	lv_memzero(ctx, sizeof(home_ctx_t));
	lv_obj_set_user_data(root, ctx);

	ui_statusbar_create(root);

	lv_obj_t *data = ui_panel_create(root, 12, 32, 216, 88);
	lv_obj_add_event_cb(data, data_double_cb, LV_EVENT_DOUBLE_CLICKED,
			    NULL);
	lv_obj_add_event_cb(data, data_long_cb, LV_EVENT_LONG_PRESSED, NULL);

	ui_text_create(data, "处理前", UI_FONT_TITLE, theme->muted, 0, 0);
	ui_text_create(data, "处理后", UI_FONT_TITLE, theme->muted, 64, 0);
	ui_text_create(data, "效率", UI_FONT_TITLE, theme->muted, 120, 0);
	ctx->eff =
	    ui_text_create(data, "0%", UI_FONT_TITLE, theme->accent, 0, 0);
	lv_obj_align(ctx->eff, LV_ALIGN_TOP_RIGHT, 0, 0);

	ui_text_create(data, "PM2.5", UI_FONT_SMALL, theme->muted, 0, 20);
	ctx->pm_before =
	    ui_text_create(data, "035", UI_FONT_TITLE, theme->text, 48, 16);
	ctx->pm_after =
	    ui_text_create(data, "008", UI_FONT_TITLE, theme->accent, 104, 16);
	ui_text_create(data, "ug/m3", UI_FONT_SMALL, theme->muted, 156, 20);

	ui_text_create(data, "TVOC", UI_FONT_SMALL, theme->muted, 0, 44);
	ctx->voc_before =
	    ui_text_create(data, "1.2", UI_FONT_TITLE, theme->text, 48, 40);
	ctx->voc_after =
	    ui_text_create(data, "0.3", UI_FONT_TITLE, theme->accent, 104, 40);
	/* 该标签会在 ppm 与“预热”之间切换，统一用 12px 保证中文可读 */
	ctx->voc_unit =
	    ui_text_create(data, "ppm", UI_FONT_TITLE, theme->muted, 156, 40);

	home_tile(root, 12, 128, "风量", "风速", &fan_icon, tile_fan_cb);
	home_tile(root, 124, 128, "滤芯", "寿命", &filter_icon, tile_filter_cb);
	home_tile(root, 12, 184, "数据", "实时", &data_icon, tile_data_cb);
	home_tile(root, 124, 184, "设置", "配置", &setting_icon,
		  tile_settings_cb);

	ctx->run = ui_text_create(root, "RUN 00:00", UI_FONT_TITLE,
				  theme->muted, 12, 244);
	ctx->total = ui_text_create(root, "TOTAL 0h", UI_FONT_TITLE,
				    theme->muted, 0, 244);
	lv_obj_align(ctx->total, LV_ALIGN_TOP_RIGHT, -12, 244);

	lv_obj_add_event_cb(root, home_delete_cb, LV_EVENT_DELETE, NULL);
	ctx->timer = lv_timer_create(home_timer_cb, 1000, root);

	home_refresh(root);
}
