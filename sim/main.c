/**
 * @file main.c
 * AirForge LVGL 桌面仿真入口（Windows / SDL2）
 *
 * 鼠标左键 = 触摸；滚轮 = 列表滚动。
 * 键盘模拟物理控件：F = 功能键（风量循环），P = 电源键（短按息屏/唤醒，长按 3s
 * 关机确认）。
 */

#include <SDL2/SDL.h>

#include "lvgl.h"
#include "ui.h"

#define SIM_HOR_RES 240
#define SIM_VER_RES 280

/* 整数倍放大，配合 DPI aware 与最近邻缩放保证像素是方块 */
#define SIM_ZOOM 2.0f

int main(void) {
#ifdef _WIN32
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

	lv_init();

	lv_display_t *disp = lv_sdl_window_create(SIM_HOR_RES, SIM_VER_RES);
	if (disp == NULL) {
		SDL_Log("failed to create SDL window");
		return 1;
	}
	lv_sdl_window_set_title(disp,
				"AirForge UI - 240x280  [F=Fan  P=Power]");
	lv_sdl_window_set_zoom(disp, SIM_ZOOM);

	lv_indev_t *mouse = lv_sdl_mouse_create();
	lv_indev_set_display(mouse, disp);

	lv_indev_t *keyboard = lv_sdl_keyboard_create();
	lv_indev_set_display(keyboard, disp);

	lv_indev_t *wheel = lv_sdl_mousewheel_create();
	lv_indev_set_display(wheel, disp);

	ui_init();

	bool f_prev = false;
	bool p_prev = false;
	bool p_long = false;
	uint32_t p_down_ms = 0;

	while (1) {
		const Uint8 *keys = SDL_GetKeyboardState(NULL);

		bool f_now = keys[SDL_SCANCODE_F] != 0;
		if (f_now && !f_prev) {
			ui_key_fn();
		}
		f_prev = f_now;

		bool p_now = keys[SDL_SCANCODE_P] != 0;
		if (p_now && !p_prev) {
			p_down_ms = SDL_GetTicks();
			p_long = false;
		}
		if (p_now && !p_long && (SDL_GetTicks() - p_down_ms) >= 3000u) {
			p_long = true;
			ui_key_power_long();
		}
		if (!p_now && p_prev) {
			if (!p_long) {
				ui_key_power_short();
			}
		}
		p_prev = p_now;

		uint32_t idle_ms = lv_timer_handler();
		if (idle_ms < 5) {
			idle_ms = 5;
		}
		if (idle_ms > 20) {
			idle_ms = 20;
		}
		SDL_Delay(idle_ms);
	}

	return 0;
}
