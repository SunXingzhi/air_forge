/**
 * @file ui.c
 * 屏幕管理与像素风切换动画。
 */

#include "ui.h"

#include "ui_theme.h"

#include "screen_home.h"
#include "screen_menu.h"

#define UI_W 240
#define UI_H 280
#define TRANS_STEPS 8
#define TRANS_STEP_PX 30
#define TRANS_PERIOD_MS 26

static lv_obj_t *scr_root;
static ui_screen_t scr_id;

static lv_timer_t *trans_timer;
static lv_obj_t *trans_new;
static lv_obj_t *trans_old;
static int32_t trans_step;
static int32_t trans_dir;

static lv_obj_t *screen_root_create(void) {
	const ui_theme_t *theme = ui_theme();

	lv_obj_t *root = lv_obj_create(lv_screen_active());
	lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_size(root, UI_W, UI_H);
	lv_obj_set_pos(root, 0, 0);
	lv_obj_set_style_bg_color(root, theme->bg, 0);
	lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(root, 0, 0);
	lv_obj_set_style_radius(root, 0, 0);
	lv_obj_set_style_pad_all(root, 0, 0);
	return root;
}

static void screen_build(lv_obj_t *root, ui_screen_t id) {
	if (id == UI_SCREEN_MENU) {
		screen_menu_create(root);
	} else {
		screen_home_create(root);
	}
}

/* 每帧整块位移 30px，8 帧滑完整屏，不做插值 */
static void trans_timer_cb(lv_timer_t *timer) {
	trans_step++;
	int32_t offset = TRANS_STEP_PX * trans_step;

	lv_obj_set_x(trans_new, trans_dir * (UI_W - offset));
	lv_obj_set_x(trans_old, -trans_dir * offset);

	if (trans_step >= TRANS_STEPS) {
		lv_obj_delete(trans_old);
		trans_old = NULL;
		scr_root = trans_new;
		trans_new = NULL;
		lv_timer_delete(timer);
		trans_timer = NULL;
	}
}

void ui_init(void) {
	lv_obj_t *screen = lv_screen_active();
	lv_obj_set_style_bg_color(screen, ui_theme()->bg, 0);
	lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_all(screen, 0, 0);
	lv_obj_set_style_border_width(screen, 0, 0);

	scr_id = UI_SCREEN_HOME;
	scr_root = screen_root_create();
	screen_build(scr_root, scr_id);
}

void ui_goto(ui_screen_t screen, bool back) {
	if (screen == scr_id || trans_timer != NULL) {
		return;
	}

	trans_dir = back ? -1 : 1;
	trans_old = scr_root;
	scr_id = screen;

	trans_new = screen_root_create();
	screen_build(trans_new, screen);
	lv_obj_set_x(trans_new, trans_dir * UI_W);

	trans_step = 0;
	trans_timer = lv_timer_create(trans_timer_cb, TRANS_PERIOD_MS, NULL);
}

void ui_reload(void) {
	if (trans_timer) {
		lv_timer_delete(trans_timer);
		trans_timer = NULL;
		if (trans_new) {
			lv_obj_delete(trans_new);
			trans_new = NULL;
		}
		trans_old = NULL;
	}

	lv_obj_t *old = scr_root;
	lv_obj_set_style_bg_color(lv_screen_active(), ui_theme()->bg, 0);

	scr_root = screen_root_create();
	screen_build(scr_root, scr_id);

	if (old) {
		lv_obj_delete(old);
	}
}
