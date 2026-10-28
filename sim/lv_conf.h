/**
 * @file lv_conf.h
 * LVGL 9.3 桌面仿真配置（AirForge）
 *
 * 目标面板：P169H002 240x280 竖屏 / RGB565
 * 只覆盖仿真需要的配置项，未列出的项由 lv_conf_internal.h 使用默认值。
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* ---------- 显示 ---------- */

/* 与实机 RGB565 保持一致 */
#define LV_COLOR_DEPTH 16

/* 仿真给足内存，方便跑复杂页面；实机需按 PSRAM 情况单独调小 */
#define LV_MEM_SIZE (512U * 1024U)

/* 60 FPS 目标刷新周期 */
#define LV_DEF_REFR_PERIOD 16

#define LV_USE_OS LV_OS_NONE

/* ---------- 桌面 SDL 驱动 ---------- */
/* 鼠标 = 触摸输入，键盘 = 按键/编码器输入，鼠标滚轮 = 编码器 */
#define LV_USE_SDL 1
#if LV_USE_SDL
    #define LV_SDL_INCLUDE_PATH     <SDL2/SDL.h>
    #define LV_SDL_RENDER_MODE      LV_DISPLAY_RENDER_MODE_DIRECT
    #define LV_SDL_BUF_COUNT        1
    #define LV_SDL_ACCELERATED      1
    #define LV_SDL_FULLSCREEN       0
    #define LV_SDL_DIRECT_EXIT      1
    #define LV_SDL_MOUSEWHEEL_MODE  LV_SDL_MOUSEWHEEL_MODE_ENCODER
#endif

/* ---------- 字体 ---------- */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_28 0

/* 像素字体：UNSCII 8/16，1bpp 位图，放大不糊 */
#define LV_FONT_UNSCII_8  1
#define LV_FONT_UNSCII_16 1

/* ---------- 性能面板 ---------- */
/* 右上角实时显示 FPS / CPU 占用，用来评估页面渲染开销 */
#define LV_USE_SYSMON 1
#if LV_USE_SYSMON
    #define LV_USE_PERF_MONITOR 1
    /* 性能数据走串口日志，不在屏幕上盖住像素 UI */
    #define LV_USE_PERF_MONITOR_LOG_MODE 1
    #define LV_USE_PERF_MONITOR_POS LV_ALIGN_TOP_RIGHT
    #define LV_USE_MEM_MONITOR  0
#endif

#endif /* LV_CONF_H */
