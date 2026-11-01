/**
 * @file ui_popup.c
 */

#include "ui_popup.h"

#include "ui_button.h"
#include "ui_theme.h"

static lv_obj_t *popup_overlay;
static ui_popup_cb_t popup_left_cb;
static ui_popup_cb_t popup_right_cb;

static lv_obj_t *toast_obj;
static lv_timer_t *toast_timer;

static void popup_left_event_cb(lv_event_t *e) {
	LV_UNUSED(e);
	ui_popup_cb_t cb = popup_left_cb;
	ui_popup_close();
	if (cb) {
		cb();
	}
}

static void popup_right_event_cb(lv_event_t *e) {
	LV_UNUSED(e);
	ui_popup_cb_t cb = popup_right_cb;
	ui_popup_close();
	if (cb) {
		cb();
	}
}

static void popup_button_create(lv_obj_t *parent, const char *text, int32_t x,
				int32_t w, lv_event_cb_t cb) {
	const ui_theme_t *theme = ui_theme();

	lv_obj_t *btn = ui_button_create(parent, x, 96, w, 44, cb, NULL);
	lv_obj_t *label =
	    ui_text_create(btn, text, UI_FONT_TITLE, theme->text, 0, 0);
	lv_obj_center(label);
	ui_button_set_fg_colors(btn, theme->text, theme->accent_text);
}

void ui_popup_close(void) {
	if (popup_overlay) {
		lv_obj_delete(popup_overlay);
		popup_overlay = NULL;
	}
	popup_left_cb = NULL;
	popup_right_cb = NULL;
}

bool ui_popup_is_open(void) { return popup_overlay != NULL; }

void ui_popup_show(const char *title, const char *body, const char *left_text,
		   ui_popup_cb_t left_cb, const char *right_text,
		   ui_popup_cb_t right_cb, bool warning) {
	const ui_theme_t *theme = ui_theme();

	ui_popup_close();

	popup_left_cb = left_cb;
	popup_right_cb = right_cb;

	popup_overlay = lv_obj_create(lv_layer_top());
	lv_obj_remove_flag(popup_overlay, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(popup_overlay, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(popup_overlay, 0, 0);
	lv_obj_set_size(popup_overlay, 240, 280);
	lv_obj_set_style_bg_color(popup_overlay, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(popup_overlay, LV_OPA_70, 0);
	lv_obj_set_style_border_width(popup_overlay, 0, 0);
	lv_obj_set_style_radius(popup_overlay, 0, 0);
	lv_obj_set_style_pad_all(popup_overlay, 0, 0);

	lv_obj_t *dialog = lv_obj_create(popup_overlay);
	lv_obj_remove_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(dialog, 216, 150);
	lv_obj_center(dialog);
	lv_obj_set_style_bg_color(dialog, theme->panel, 0);
	lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(dialog,
				      warning ? theme->warn : theme->accent, 0);
	lv_obj_set_style_border_width(dialog, 2, 0);
	lv_obj_set_style_radius(dialog, 0, 0);
	lv_obj_set_style_pad_all(dialog, 0, 0);

	lv_obj_t *title_label =
	    ui_text_create(dialog, title, UI_FONT_TITLE,
			   warning ? theme->warn : theme->text, 8, 8);
	LV_UNUSED(title_label);

	lv_obj_t *body_label = lv_label_create(dialog);
	lv_label_set_text(body_label, body);
	lv_label_set_long_mode(body_label, LV_LABEL_LONG_MODE_WRAP);
	lv_obj_set_width(body_label, 200);
	lv_obj_set_pos(body_label, 8, 30);
	lv_obj_set_style_text_font(body_label, UI_FONT_TITLE, 0);
	lv_obj_set_style_text_color(body_label, theme->text, 0);

	if (right_text) {
		popup_button_create(dialog, left_text ? left_text : "确定", 8, 96,
				    popup_left_event_cb);
		popup_button_create(dialog, right_text, 112, 96,
				    popup_right_event_cb);
	} else {
		popup_button_create(dialog, left_text ? left_text : "确定", 8,
				    200, popup_left_event_cb);
	}
}

static void toast_timer_cb(lv_timer_t *timer) {
	LV_UNUSED(timer);
	toast_timer = NULL;
	if (toast_obj) {
		lv_obj_delete(toast_obj);
		toast_obj = NULL;
	}
}

void ui_toast(const char *text) {
	const ui_theme_t *theme = ui_theme();

	if (toast_timer) {
		lv_timer_delete(toast_timer);
		toast_timer = NULL;
	}
	if (toast_obj) {
		lv_obj_delete(toast_obj);
		toast_obj = NULL;
	}

	toast_obj = lv_obj_create(lv_layer_top());
	lv_obj_remove_flag(toast_obj, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_remove_flag(toast_obj, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(toast_obj, 20, 240);
	lv_obj_set_size(toast_obj, 200, 28);
	lv_obj_set_style_bg_color(toast_obj, theme->panel, 0);
	lv_obj_set_style_bg_opa(toast_obj, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(toast_obj, theme->accent, 0);
	lv_obj_set_style_border_width(toast_obj, 2, 0);
	lv_obj_set_style_radius(toast_obj, 0, 0);
	lv_obj_set_style_pad_all(toast_obj, 0, 0);

	lv_obj_t *label =
	    ui_text_create(toast_obj, text, UI_FONT_TITLE, theme->text, 0, 0);
	lv_obj_center(label);

	toast_timer = lv_timer_create(toast_timer_cb, 2000, NULL);
}
