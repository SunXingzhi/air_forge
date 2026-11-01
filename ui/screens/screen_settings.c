/**
 * @file screen_settings.c
 * P5 设置：主题 / 亮度 / 息屏 / 单位 / 校准 / 恢复出厂 / 关于。
 */

#include "screen_settings.h"

#include "ui.h"
#include "ui_popup.h"
#include "ui_state.h"
#include "ui_theme.h"

#include "ui_row.h"
#include "ui_statusbar.h"

static const char *brightness_name(int level)
{
    static const char *const names[] = {"低", "中", "高"};
    if(level < 0 || level > 2) {
        return "MID";
    }
    return names[level];
}

static const char *screen_off_name(uint16_t sec)
{
    if(sec == 0) return "常亮";
    if(sec == 30) return "30秒";
    return "60秒";
}

static void theme_tap(lv_obj_t * row, void * user_data)
{
    LV_UNUSED(row);
    LV_UNUSED(user_data);
    ui_request_theme_toggle();
}

static void brightness_tap(lv_obj_t * row, void * user_data)
{
    LV_UNUSED(user_data);
    g_ui.brightness = (int8_t)((g_ui.brightness + 1) % 3);
    ui_row_set_value(row, brightness_name(g_ui.brightness));
    ui_apply_brightness();
    char buf[24];
    lv_snprintf(buf, sizeof(buf), "亮度: %s", brightness_name(g_ui.brightness));
    ui_toast(buf);
}

static void screen_off_tap(lv_obj_t * row, void * user_data)
{
    LV_UNUSED(user_data);

    if(g_ui.screen_off_sec == 30) {
        g_ui.screen_off_sec = 60;
    }
    else if(g_ui.screen_off_sec == 60) {
        g_ui.screen_off_sec = 0;
    }
    else {
        g_ui.screen_off_sec = 30;
    }

    lv_display_trigger_activity(NULL);
    ui_row_set_value(row, screen_off_name(g_ui.screen_off_sec));
    ui_toast("息屏已更新");
}

static void unit_tap(lv_obj_t * row, void * user_data)
{
    LV_UNUSED(user_data);
    g_ui.voc_unit = (uint8_t)((g_ui.voc_unit + 1) % 2);
    ui_row_set_value(row, ui_voc_unit_name());
    ui_toast("VOC单位已更新");
}

static void calibrate_start(void)
{
    ui_toast("校准已开始");
}

static void calibrate_hold(lv_obj_t * row, void * user_data)
{
    LV_UNUSED(row);
    LV_UNUSED(user_data);
    ui_popup_show("校准?",
                  "请在洁净空气中放置约30秒。",
                  "取消", NULL,
                  "开始", calibrate_start,
                  false);
}

static void factory_hold(lv_obj_t * row, void * user_data)
{
    LV_UNUSED(row);
    LV_UNUSED(user_data);
    ui_popup_show("恢复出厂?",
                  "所有设置与滤芯计数将被清除。",
                  "取消", NULL,
                  "重置", ui_request_factory_reset,
                  true);
}

static void about_tap(lv_obj_t * row, void * user_data)
{
    LV_UNUSED(row);
    LV_UNUSED(user_data);
    ui_popup_show("关于",
                  "AIRFORGE 固件 0.1.0\n序列号 AF-2026-001",
                  "确定", NULL, NULL, NULL, false);
}

void screen_settings_create(lv_obj_t * root)
{
    const ui_theme_t * theme = ui_theme();

    ui_attach_back(root);
    ui_statusbar_create(root);

    ui_text_create(root, "<", UI_FONT_TITLE, theme->text, 8, 33);
    ui_text_create(root, "设置", UI_FONT_TITLE, theme->text, 52, 33);

    lv_obj_t * line = lv_obj_create(root);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_pos(line, 0, 52);
    lv_obj_set_size(line, 240, 2);
    lv_obj_set_style_bg_color(line, theme->accent, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);

    lv_obj_t * list = lv_obj_create(root);
    lv_obj_remove_flag(list, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_pos(list, 12, 56);
    lv_obj_set_size(list, 216, 216);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_radius(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    ui_row_create(list, 0, 0, 216, 44, "主题",
                  ui_theme_name(), theme_tap, NULL, NULL);
    ui_row_create(list, 0, 44, 216, 44, "亮度",
                  brightness_name(g_ui.brightness),
                  brightness_tap, NULL, NULL);
    ui_row_create(list, 0, 88, 216, 44, "息屏",
                  screen_off_name(g_ui.screen_off_sec),
                  screen_off_tap, NULL, NULL);
    ui_row_create(list, 0, 132, 216, 44, "VOC单位",
                  ui_voc_unit_name(), unit_tap, NULL, NULL);
    ui_row_create(list, 0, 176, 216, 44, "校准", "长按1.5s",
                  NULL, calibrate_hold, NULL);
    ui_row_create(list, 0, 220, 216, 44, "恢复出厂", "长按1.5s",
                  NULL, factory_hold, NULL);
    ui_row_create(list, 0, 264, 216, 44, "关于", "v0.1",
                  about_tap, NULL, NULL);
}
