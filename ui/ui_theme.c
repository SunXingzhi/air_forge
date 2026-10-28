/**
 * @file ui_theme.c
 */

#include "ui_theme.h"

#include "ui_assets.h"

static const ui_theme_t themes[UI_THEME_COUNT] = {
    [UI_THEME_STONE] =
	{
	    .bg = LV_COLOR_MAKE(0x17, 0x11, 0x0D),
	    .panel = LV_COLOR_MAKE(0x24, 0x1A, 0x15),
	    .light = LV_COLOR_MAKE(0xC8, 0x9B, 0x54),
	    .dark = LV_COLOR_MAKE(0x0B, 0x07, 0x05),
	    .text = LV_COLOR_MAKE(0xF0, 0xE3, 0xCB),
	    .muted = LV_COLOR_MAKE(0xA3, 0x8A, 0x6D),
	    .accent = LV_COLOR_MAKE(0xE4, 0xB0, 0x4A),
	    .accent_text = LV_COLOR_MAKE(0x1A, 0x12, 0x09),
	    .texture = &ui_tex_stone,
	},
    [UI_THEME_OLED] =
	{
	    .bg = LV_COLOR_MAKE(0x00, 0x00, 0x00),
	    .panel = LV_COLOR_MAKE(0x05, 0x05, 0x05),
	    .light = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
	    .dark = LV_COLOR_MAKE(0x2A, 0x2A, 0x2A),
	    .text = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
	    .muted = LV_COLOR_MAKE(0x8A, 0x8A, 0x8A),
	    .accent = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF),
	    .accent_text = LV_COLOR_MAKE(0x00, 0x00, 0x00),
	    .texture = &ui_tex_oled,
	},
};

static ui_theme_id_t current_theme = UI_THEME_STONE;

const ui_theme_t *ui_theme(void) { return &themes[current_theme]; }

ui_theme_id_t ui_theme_get(void) { return current_theme; }

const char *ui_theme_name(void) {
	return current_theme == UI_THEME_STONE ? "STONE" : "OLED";
}

void ui_theme_next(void) {
	current_theme = (ui_theme_id_t)((current_theme + 1) % UI_THEME_COUNT);
}

lv_obj_t *ui_text_create(lv_obj_t *parent, const char *text,
			 const lv_font_t *font, lv_color_t color, int32_t x,
			 int32_t y) {
	lv_obj_t *label = lv_label_create(parent);
	lv_label_set_text(label, text);
	lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_CLIP);
	lv_obj_set_style_text_font(label, font, 0);
	lv_obj_set_style_text_color(label, color, 0);
	lv_obj_set_pos(label, x, y);
	return label;
}
