/**
 * @file screen_home.c
 * 首页：空气质量读数 + 分段进度条 + 功能列表 + 风机滑条 + 按键。
 */

#include "screen_home.h"

#include "ui.h"
#include "ui_theme.h"

#include "ui_button.h"
#include "ui_list.h"
#include "ui_panel.h"
#include "ui_progress.h"
#include "ui_slider.h"

// 
static const char *const home_items[] = {
    "FAN SPEED",
    "FILTER LIFE",
    "TIMER",
    "SETTINGS",
};

static const char* const check_items[] = {
	"PM2.5",
	"TVOC"
};

static lv_obj_t *fan_slider;
static lv_obj_t *fan_pct_label;

static void fan_slider_cb(lv_obj_t *slider, int32_t value, void *user_data) {
	LV_UNUSED(slider);
	LV_UNUSED(user_data);
	lv_label_set_text_fmt(fan_pct_label, "%d%%", (int)value);
}

static void boost_cb(lv_event_t *e) {
	LV_UNUSED(e);
	ui_slider_set_value(fan_slider, 100, true);
}

static void menu_cb(lv_event_t *e) {
	LV_UNUSED(e);
	ui_goto(UI_SCREEN_MENU, false);
}

static void list_cb(lv_obj_t *list, int32_t index, void *user_data) {
	LV_UNUSED(list);
	LV_UNUSED(user_data);
	if (index == 3) {
		ui_goto(UI_SCREEN_MENU, false);
	}
}

void screen_home_create(lv_obj_t *root) {
	const ui_theme_t *theme = ui_theme();

	/* ---------- 顶部标题 ---------- */
	ui_text_create(root, "AIRFORGE", UI_FONT_TITLE, theme->text, 12, 4);

	lv_obj_t *line = lv_obj_create(root);
	lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(line, 0, 24);
	lv_obj_set_size(line, 240, 2);
	lv_obj_set_style_bg_color(line, theme->accent, 0);
	lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(line, 0, 0);
	lv_obj_set_style_radius(line, 0, 0);
	lv_obj_set_style_pad_all(line, 0, 0);

	// 具体传感器读数
	/* ---------- PM2.5 读数 ---------- */
	lv_obj_t *pm_panel = ui_panel_create(root, 12, 32, 216, 56);
	ui_text_create(pm_panel, "PM2.5", UI_FONT_SMALL, theme->muted, 0, 0);
	ui_text_create(pm_panel, "12", UI_FONT_TITLE, theme->text, 0, 8);
	ui_text_create(pm_panel, "ug/m3", UI_FONT_SMALL, theme->muted, 36, 14);
	ui_progress_create(pm_panel, 0, 28, 196, 8, 10, 30);

	/* ---------- 功能列表 ---------- */
	lv_obj_t *list_panel = ui_panel_create(root, 12, 92, 216, 104);
	lv_obj_set_style_pad_all(list_panel, 4, 0);
	ui_list_create(list_panel, home_items, 4, 0, 0, 204, 92, list_cb, NULL);

	/* ---------- 风机滑条 ---------- */
	ui_text_create(root, "FAN", UI_FONT_TITLE, theme->text, 14, 200);
	fan_pct_label =
	    ui_text_create(root, "45%", UI_FONT_TITLE, theme->text, 0, 200);
	lv_obj_align(fan_pct_label, LV_ALIGN_TOP_RIGHT, -14, 200);

	fan_slider = ui_slider_create(root, 14, 220, 212, 16, 0, 100, 45,
				      fan_slider_cb, NULL);

	/* ---------- 底部按键 ---------- */
	ui_button_create(root, "BOOST", 12, 240, 100, 28, boost_cb, NULL);
	ui_button_create(root, "MENU", 128, 240, 100, 28, menu_cb, NULL);
}
