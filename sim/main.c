/**
 * @file main.c
 * AirForge LVGL 桌面仿真入口（Windows / SDL2）
 *
 * 运行后弹出一个 240x280 的窗口（放大 2 倍显示），
 * 鼠标左键 = 触摸，键盘 = 按键/编码器输入，滚轮 = 编码器旋转。
 */

#include <SDL2/SDL.h>

#include "lvgl.h"
#include "ui.h"

/* 与 components/p169h002/include/board_pins.h 中的面板分辨率保持一致 */
#define SIM_HOR_RES 240
#define SIM_VER_RES 280

/* 桌面显示放大倍数，1.0 = 与实机 1:1 像素；
 * 像素 UI 必须用整数倍，配合 DPI aware 才能保证每个像素是方块 */
#define SIM_ZOOM 2.0f

int main(void) {
#ifdef _WIN32
	/* 让窗口使用真实物理像素，避免 Windows 位图拉伸把像素糊掉 */
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif
	/* SDL 缩放使用最近邻，保持像素边缘锐利 */
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

	lv_init();

	lv_display_t *disp = lv_sdl_window_create(SIM_HOR_RES, SIM_VER_RES);
	if (disp == NULL) {
		SDL_Log("failed to create SDL window");
		return 1;
	}
	lv_sdl_window_set_title(disp, "AirForge UI Simulator - 240x280");
	lv_sdl_window_set_zoom(disp, SIM_ZOOM);

	lv_indev_t *mouse = lv_sdl_mouse_create();
	lv_indev_set_display(mouse, disp);

	lv_indev_t *keyboard = lv_sdl_keyboard_create();
	lv_indev_set_display(keyboard, disp);

	lv_indev_t *wheel = lv_sdl_mousewheel_create();
	lv_indev_set_display(wheel, disp);

	ui_init();

	while (1) {
		uint32_t idle_ms = lv_timer_handler();
		if (idle_ms < 5)
			idle_ms = 5;
		if (idle_ms > 20)
			idle_ms = 20;
		SDL_Delay(idle_ms);
	}

	return 0;
}
