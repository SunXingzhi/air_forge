/**
 * @file ui_theme.h
 * 像素风主题：Terraria（暖色石材）与 OLED（单色高对比）。
 * 所有控件只通过这里取色和字体，切换主题后重建 UI 即可整体换肤。
 */

#pragma once

#include "lvgl.h"

/* 像素字体：1bpp 位图字体，放大后不会糊 */
#define UI_FONT_TITLE (&lv_font_unscii_16)
#define UI_FONT_SMALL (&lv_font_unscii_8)

typedef enum {
	UI_THEME_STONE = 0, /* Terraria风：石木底色 + 金色描边 */
	UI_THEME_OLED,	    /* 单色 OLED：纯黑底 + 白色描边 */
	UI_THEME_COUNT
} ui_theme_id_t;

typedef struct {
	lv_color_t bg;		       /* 屏幕底色 */
	lv_color_t panel;	       /* 面板底色 */
	lv_color_t light;	       /* 亮边、描边 */
	lv_color_t dark;	       /* 暗边、轨道、硬阴影 */
	lv_color_t text;	       /* 主文字 */
	lv_color_t muted;	       /* 次要文字 */
	lv_color_t accent;	       /* 强调色（选中/进度） */
	lv_color_t accent_text;	       /* 强调色上的文字 */
	const lv_image_dsc_t *texture; /* 8x8 平铺噪点纹理，可空 */
} ui_theme_t;

const ui_theme_t *ui_theme(void);
ui_theme_id_t ui_theme_get(void);
const char *ui_theme_name(void);
void ui_theme_next(void);

/* 小工具：按像素坐标创建一个文本标签 */
lv_obj_t *ui_text_create(lv_obj_t *parent, const char *text,
			 const lv_font_t *font, lv_color_t color, int32_t x,
			 int32_t y);
