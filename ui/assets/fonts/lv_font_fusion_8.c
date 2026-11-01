/*******************************************************************************
 * Size: 8 px
 * Bpp: 1
 * Opts: --font fusion-pixel-8px-proportional-zh_hans.ttf --size 8 --bpp 1 --format lvgl --no-compress --no-prefilter --no-kerning --range 0x20-0x7E --lv-font-name lv_font_fusion_8 --lv-include lvgl.h -o ../../ui/assets/fonts/lv_font_fusion_8.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef LV_FONT_FUSION_8
#define LV_FONT_FUSION_8 1
#endif

#if LV_FONT_FUSION_8

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xf4,

    /* U+0022 "\"" */
    0xb4,

    /* U+0023 "#" */
    0x57, 0xd4, 0xaf, 0xa8,

    /* U+0024 "$" */
    0x23, 0xe8, 0xe2, 0xf8, 0x80,

    /* U+0025 "%" */
    0x92, 0x24, 0x90,

    /* U+0026 "&" */
    0x4a, 0x4b, 0xa5,

    /* U+0027 "'" */
    0xc0,

    /* U+0028 "(" */
    0x6a, 0xa4,

    /* U+0029 ")" */
    0x95, 0x58,

    /* U+002A "*" */
    0x5d, 0x50,

    /* U+002B "+" */
    0x5d, 0x0,

    /* U+002C "," */
    0x60,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x25, 0x25, 0x20,

    /* U+0030 "0" */
    0x69, 0xbd, 0x96,

    /* U+0031 "1" */
    0x59, 0x25, 0xc0,

    /* U+0032 "2" */
    0x69, 0x16, 0x8f,

    /* U+0033 "3" */
    0x69, 0x21, 0x96,

    /* U+0034 "4" */
    0x26, 0xaa, 0xf2,

    /* U+0035 "5" */
    0xf8, 0xe1, 0x1e,

    /* U+0036 "6" */
    0x68, 0xe9, 0x96,

    /* U+0037 "7" */
    0xf1, 0x22, 0x44,

    /* U+0038 "8" */
    0x69, 0x69, 0x96,

    /* U+0039 "9" */
    0x69, 0x97, 0x16,

    /* U+003A ":" */
    0x90,

    /* U+003B ";" */
    0x41, 0x80,

    /* U+003C "<" */
    0x2a, 0x22,

    /* U+003D "=" */
    0xe3, 0x80,

    /* U+003E ">" */
    0x88, 0xa8,

    /* U+003F "?" */
    0x69, 0x12, 0x2,

    /* U+0040 "@" */
    0x6b, 0xdb, 0x87,

    /* U+0041 "A" */
    0x69, 0x9f, 0x99,

    /* U+0042 "B" */
    0xe9, 0xe9, 0x9e,

    /* U+0043 "C" */
    0x78, 0x88, 0x87,

    /* U+0044 "D" */
    0xe9, 0x99, 0x9e,

    /* U+0045 "E" */
    0xf8, 0xe8, 0x8f,

    /* U+0046 "F" */
    0xf8, 0xe8, 0x88,

    /* U+0047 "G" */
    0x78, 0x8b, 0x97,

    /* U+0048 "H" */
    0x99, 0xf9, 0x99,

    /* U+0049 "I" */
    0xe9, 0x25, 0xc0,

    /* U+004A "J" */
    0x11, 0x11, 0x96,

    /* U+004B "K" */
    0x9a, 0xca, 0x99,

    /* U+004C "L" */
    0x92, 0x49, 0xc0,

    /* U+004D "M" */
    0x8e, 0xf7, 0x5a, 0xd4,

    /* U+004E "N" */
    0x9d, 0xb9, 0x99,

    /* U+004F "O" */
    0x69, 0x99, 0x96,

    /* U+0050 "P" */
    0xe9, 0x9e, 0x88,

    /* U+0051 "Q" */
    0x69, 0x99, 0x96, 0x10,

    /* U+0052 "R" */
    0xe9, 0x9e, 0x99,

    /* U+0053 "S" */
    0x78, 0x61, 0x1e,

    /* U+0054 "T" */
    0xf9, 0x8, 0x42, 0x10,

    /* U+0055 "U" */
    0x99, 0x99, 0x96,

    /* U+0056 "V" */
    0x99, 0x9a, 0xac,

    /* U+0057 "W" */
    0xad, 0x6b, 0x55, 0x28,

    /* U+0058 "X" */
    0x99, 0x69, 0x99,

    /* U+0059 "Y" */
    0x8a, 0x94, 0x42, 0x10,

    /* U+005A "Z" */
    0xf1, 0x24, 0x8f,

    /* U+005B "[" */
    0xea, 0xac,

    /* U+005C "\\" */
    0x91, 0x24, 0x48,

    /* U+005D "]" */
    0xd5, 0x5c,

    /* U+005E "^" */
    0x54,

    /* U+005F "_" */
    0xe0,

    /* U+0060 "`" */
    0x90,

    /* U+0061 "a" */
    0x76, 0xb0,

    /* U+0062 "b" */
    0x93, 0x5b, 0x80,

    /* U+0063 "c" */
    0x72, 0x30,

    /* U+0064 "d" */
    0x25, 0xda, 0xc0,

    /* U+0065 "e" */
    0x77, 0x30,

    /* U+0066 "f" */
    0x2b, 0xa4, 0x80,

    /* U+0067 "g" */
    0x76, 0xb3, 0x80,

    /* U+0068 "h" */
    0x93, 0x5b, 0x40,

    /* U+0069 "i" */
    0x43, 0x25, 0xc0,

    /* U+006A "j" */
    0x23, 0x92, 0x4e,

    /* U+006B "k" */
    0x92, 0xeb, 0x40,

    /* U+006C "l" */
    0xc9, 0x24, 0xc0,

    /* U+006D "m" */
    0xd5, 0x6b, 0x50,

    /* U+006E "n" */
    0xd6, 0xd0,

    /* U+006F "o" */
    0x56, 0xa0,

    /* U+0070 "p" */
    0xd6, 0xe9, 0x0,

    /* U+0071 "q" */
    0x76, 0xb2, 0x40,

    /* U+0072 "r" */
    0xba, 0x40,

    /* U+0073 "s" */
    0x79, 0xe0,

    /* U+0074 "t" */
    0x5d, 0x22,

    /* U+0075 "u" */
    0xb6, 0xb0,

    /* U+0076 "v" */
    0xb5, 0x20,

    /* U+0077 "w" */
    0xad, 0x54, 0xa0,

    /* U+0078 "x" */
    0xaa, 0xd0,

    /* U+0079 "y" */
    0xb6, 0xb3, 0x80,

    /* U+007A "z" */
    0xea, 0x70,

    /* U+007B "{" */
    0x29, 0x44, 0x88,

    /* U+007C "|" */
    0xfe,

    /* U+007D "}" */
    0x89, 0x14, 0xa0,

    /* U+007E "~" */
    0x5a
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 80, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 64, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 96, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 3, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 7, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 12, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 15, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 18, .adv_w = 64, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 19, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 21, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 23, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 25, .adv_w = 64, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 27, .adv_w = 64, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 28, .adv_w = 64, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 29, .adv_w = 64, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 64, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 33, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 36, .adv_w = 80, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 39, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 42, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 45, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 48, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 51, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 54, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 60, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 63, .adv_w = 64, .box_w = 1, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 64, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 66, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 64, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 70, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 72, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 78, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 81, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 93, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 102, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 105, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 114, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 118, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 124, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 127, .adv_w = 80, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 131, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 141, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 147, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 154, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 161, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 163, .adv_w = 64, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 166, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 168, .adv_w = 64, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 169, .adv_w = 64, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 170, .adv_w = 64, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 171, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 178, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 181, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 183, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 186, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 189, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 192, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 195, .adv_w = 64, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 198, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 201, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 96, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 211, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 214, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 217, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 219, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 225, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 96, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 230, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 232, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 235, .adv_w = 64, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 237, .adv_w = 64, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 240, .adv_w = 64, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 241, .adv_w = 64, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 244, .adv_w = 80, .box_w = 4, .box_h = 2, .ofs_x = 0, .ofs_y = 2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_fusion_8 = {
#else
lv_font_t lv_font_fusion_8 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 8,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_FUSION_8*/

