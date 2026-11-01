/**
 * @file ui.c
 * 屏幕管理、像素风切换动画、亮度/息屏、物理按键与弹窗调度。
 */

#include "ui.h"

#include "ui_popup.h"
#include "ui_state.h"
#include "ui_theme.h"

#include "screen_boot.h"
#include "screen_data.h"
#include "screen_fan.h"
#include "screen_filter.h"
#include "screen_home.h"
#include "screen_settings.h"

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

static lv_obj_t *dim_overlay;
static lv_obj_t *sleep_overlay;
static bool filter_reminder_shown;

static void screen_build(lv_obj_t *root, ui_screen_t id);
static void state_tick_cb(lv_timer_t *timer);
static void sleep_check_cb(lv_timer_t *timer);

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

static void screen_off_create(lv_obj_t *root) {
	const ui_theme_t *theme = ui_theme();

	lv_obj_t *title =
	    ui_text_create(root, "关机", UI_FONT_TITLE, theme->text, 0, 0);
	lv_obj_align(title, LV_ALIGN_CENTER, 0, -18);

	lv_obj_t *hint = ui_text_create(root, "按 P 键开机", UI_FONT_TITLE,
					theme->muted, 0, 0);
	lv_obj_align(hint, LV_ALIGN_CENTER, 0, 12);
}

static void screen_build(lv_obj_t *root, ui_screen_t id) {
	switch (id) {
		case UI_SCREEN_BOOT:
			screen_boot_create(root);
			break;
		case UI_SCREEN_FAN:
			screen_fan_create(root);
			break;
		case UI_SCREEN_FILTER:
			screen_filter_create(root);
			break;
		case UI_SCREEN_DATA:
			screen_data_create(root);
			break;
		case UI_SCREEN_SETTINGS:
			screen_settings_create(root);
			break;
		case UI_SCREEN_OFF:
			screen_off_create(root);
			break;
		default:
			screen_home_create(root);
			break;
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

	ui_state_init();
	ui_theme_set(g_ui.theme_stone ? UI_THEME_STONE : UI_THEME_OLED);

	lv_indev_t *indev = NULL;
	while ((indev = lv_indev_get_next(indev)) != NULL) {
		lv_indev_set_long_press_time(indev, 1500);
	}

	/* 亮度遮罩在最底层，弹窗/息屏层后创建，天然位于其上方 */
	dim_overlay = lv_obj_create(lv_layer_top());
	lv_obj_remove_flag(dim_overlay, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_remove_flag(dim_overlay, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(dim_overlay, 0, 0);
	lv_obj_set_size(dim_overlay, UI_W, UI_H);
	lv_obj_set_style_bg_color(dim_overlay, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(dim_overlay, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(dim_overlay, 0, 0);
	lv_obj_set_style_radius(dim_overlay, 0, 0);
	lv_obj_set_style_pad_all(dim_overlay, 0, 0);
	ui_apply_brightness();

	scr_id = UI_SCREEN_BOOT;
	scr_root = screen_root_create();
	screen_build(scr_root, scr_id);

	lv_timer_create(state_tick_cb, 1000, NULL);
	lv_timer_create(sleep_check_cb, 1000, NULL);
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

static void back_hot_cb(lv_event_t *e) {
	LV_UNUSED(e);
	if (scr_id != UI_SCREEN_HOME) {
		ui_goto(UI_SCREEN_HOME, true);
	}
}

static void root_gesture_cb(lv_event_t *e) {
	LV_UNUSED(e);
	lv_indev_t *indev = lv_indev_active();
	if (indev && lv_indev_get_gesture_dir(indev) == LV_DIR_LEFT) {
		if (scr_id != UI_SCREEN_HOME) {
			ui_goto(UI_SCREEN_HOME, true);
		}
	}
}

void ui_attach_back(lv_obj_t *root) {
	lv_obj_add_event_cb(root, root_gesture_cb, LV_EVENT_GESTURE, NULL);

	lv_obj_t *hot = lv_obj_create(root);
	lv_obj_remove_flag(hot, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(hot, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(hot, 0, 0);
	lv_obj_set_size(hot, 44, 44);
	lv_obj_set_style_bg_opa(hot, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(hot, 0, 0);
	lv_obj_set_style_radius(hot, 0, 0);
	lv_obj_set_style_pad_all(hot, 0, 0);
	lv_obj_add_event_cb(hot, back_hot_cb, LV_EVENT_CLICKED, NULL);
}

void ui_apply_brightness(void) {
	if (dim_overlay == NULL) {
		return;
	}

	lv_opa_t opa;
	if (g_ui.brightness <= 0) {
		opa = LV_OPA_50;
	} else if (g_ui.brightness == 1) {
		opa = LV_OPA_20;
	} else {
		opa = LV_OPA_TRANSP;
	}
	lv_obj_set_style_bg_opa(dim_overlay, opa, 0);
}

static void sleep_overlay_click_cb(lv_event_t *e) {
	LV_UNUSED(e);
	if (sleep_overlay) {
		lv_obj_delete(sleep_overlay);
		sleep_overlay = NULL;
	}
	lv_display_trigger_activity(NULL);
}

static void ui_sleep_now(void) {
	if (sleep_overlay || !g_ui.powered) {
		return;
	}

	ui_popup_close();

	sleep_overlay = lv_obj_create(lv_layer_top());
	lv_obj_remove_flag(sleep_overlay, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(sleep_overlay, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_pos(sleep_overlay, 0, 0);
	lv_obj_set_size(sleep_overlay, UI_W, UI_H);
	lv_obj_set_style_bg_color(sleep_overlay, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(sleep_overlay, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(sleep_overlay, 0, 0);
	lv_obj_set_style_radius(sleep_overlay, 0, 0);
	lv_obj_set_style_pad_all(sleep_overlay, 0, 0);
	lv_obj_add_event_cb(sleep_overlay, sleep_overlay_click_cb,
			    LV_EVENT_CLICKED, NULL);
	lv_obj_move_foreground(sleep_overlay);
}

bool ui_wake_if_sleeping(void) {
	if (sleep_overlay == NULL) {
		return false;
	}

	lv_obj_delete(sleep_overlay);
	sleep_overlay = NULL;
	lv_display_trigger_activity(NULL);
	return true;
}

static void sleep_check_cb(lv_timer_t *timer) {
	LV_UNUSED(timer);

	if (!g_ui.powered || sleep_overlay != NULL) {
		return;
	}
	if (scr_id == UI_SCREEN_BOOT || scr_id == UI_SCREEN_OFF) {
		return;
	}
	if (g_ui.screen_off_sec == 0) {
		return;
	}

	uint32_t inactive = lv_display_get_inactive_time(NULL);
	if (inactive >= (uint32_t)g_ui.screen_off_sec * 1000u) {
		ui_sleep_now();
	}
}

static void state_tick_cb(lv_timer_t *timer) {
	LV_UNUSED(timer);
	ui_state_tick();
}

void ui_key_fn(void) {
	if (!g_ui.powered) {
		return;
	}

	lv_display_trigger_activity(NULL);
	if (ui_wake_if_sleeping()) {
		return;
	}

	if (g_ui.fan == UI_FAN_AUTO) {
		g_ui.fan = UI_FAN_STOP;
	} else {
		g_ui.fan = (ui_fan_t)(((int)g_ui.fan + 1) % 4);
	}

	char buf[24];
	lv_snprintf(buf, sizeof(buf), "风量: %s", ui_fan_name(g_ui.fan));
	ui_toast(buf);
}

void ui_key_power_short(void) {
	lv_display_trigger_activity(NULL);

	if (!g_ui.powered) {
		g_ui.powered = true;
		g_ui.fan = UI_FAN_STOP;
		g_ui.session_sec = 0;
		ui_goto(UI_SCREEN_BOOT, false);
		return;
	}

	if (ui_wake_if_sleeping()) {
		return;
	}

	ui_sleep_now();
}

static void power_off_confirm(void) {
	g_ui.powered = false;
	g_ui.fan = UI_FAN_STOP;
	ui_wake_if_sleeping();
	ui_goto(UI_SCREEN_OFF, false);
}

void ui_key_power_long(void) {
	if (!g_ui.powered) {
		return;
	}

	lv_display_trigger_activity(NULL);
	ui_wake_if_sleeping();

	ui_popup_show("关机", "停止风机并关闭屏幕?",
		      "取消", NULL, "关机", power_off_confirm, true);
}

static void theme_toggle_async(void *param) {
	LV_UNUSED(param);
	g_ui.theme_stone = !g_ui.theme_stone;
	ui_theme_set(g_ui.theme_stone ? UI_THEME_STONE : UI_THEME_OLED);
	ui_reload();

	char buf[24];
	lv_snprintf(buf, sizeof(buf), "主题: %s", ui_theme_name());
	ui_toast(buf);
}

void ui_request_theme_toggle(void) { lv_async_call(theme_toggle_async, NULL); }

static void factory_reset_async(void *param) {
	LV_UNUSED(param);
	ui_state_init();
	g_ui.theme_stone = false;
	ui_theme_set(UI_THEME_OLED);
	filter_reminder_shown = false;
	ui_apply_brightness();
	ui_goto(UI_SCREEN_HOME, true);
	ui_toast("恢复出厂完成");
}

void ui_request_factory_reset(void) {
	lv_async_call(factory_reset_async, NULL);
}

static void reminder_replace_cb(void) { ui_goto(UI_SCREEN_FILTER, false); }

static void reminder_timer_cb(lv_timer_t *timer) {
	lv_timer_delete(timer);

	if (!g_ui.powered || scr_id == UI_SCREEN_OFF ||
	    scr_id == UI_SCREEN_BOOT) {
		return;
	}
	if (filter_reminder_shown) {
		return;
	}
	if (ui_filter_min_pct() > 15) {
		return;
	}

	filter_reminder_shown = true;
	ui_popup_show("滤芯提醒",
		      "滤芯寿命低于15%,请尽快更换。", "稍后",
		      NULL, "更换", reminder_replace_cb, true);
}

void ui_after_boot(void) { lv_timer_create(reminder_timer_cb, 900, NULL); }
