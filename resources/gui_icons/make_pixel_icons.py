#!/usr/bin/env python3
"""生成 AirForge 像素风图标：24x24 A8 图标 + 48x48 RGB565A8 logo。

输出：
  resources/gui_icons/<name>.svg        24x24 矢量预览
  resources/gui_icons/preview_icons.png 放大预览
  ui/assets/icons/<name>_icon.c          LVGL A8 图标
  ui/assets/icons/logo_icon.c            LVGL RGB565A8 logo

图标为纯像素网格，无抗锯齿；颜色由 LVGL image_recolor 运行时决定。
"""

import math
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
SVG_DIR = ROOT / "resources" / "gui_icons"
C_DIR = ROOT / "ui" / "assets" / "icons"
LOGO_PNG = ROOT / "resources" / "logo" / "png" / "airforge_recArm_48_stone.png"

N = 24


def blank():
    return [[0 for _ in range(N)] for _ in range(N)]


def put(g, x, y):
    if 0 <= x < N and 0 <= y < N:
        g[y][x] = 1


def fan_icon():
    g = blank()
    cx = cy = (N - 1) / 2.0

    for y in range(N):
        for x in range(N):
            r = math.hypot(x - cx, y - cy)
            if 8.6 <= r <= 10.6:
                g[y][x] = 1

    for y in range(N):
        for x in range(N):
            r = math.hypot(x - cx, y - cy)
            if r > 8.4:
                continue
            ang = math.degrees(math.atan2(y - cy, x - cx))
            for base in (45, 135, 225, 315):
                diff = abs((ang - base + 180) % 360 - 180)
                if 2.0 <= r <= 8.2 and diff <= 18:
                    g[y][x] = 1
                    break

    for y in range(N):
        for x in range(N):
            if math.hypot(x - cx, y - cy) <= 2.0:
                g[y][x] = 1
    return g


def filter_icon():
    g = blank()
    for y in range(4, 20):
        for x in range(3, 21):
            edge = x in (3, 4, 19, 20) or y in (4, 5, 18, 19)
            if edge:
                g[y][x] = 1
    for y in (8, 9, 12, 13, 16, 17):
        for x in range(6, 18):
            g[y][x] = 1
    return g


def data_icon():
    g = blank()
    for y in (19, 20):
        for x in range(4, 21):
            g[y][x] = 1
    bars = ((5, 13), (10, 9), (15, 5))
    for x0, top in bars:
        for y in range(top, 19):
            for x in range(x0, x0 + 3):
                g[y][x] = 1
    return g


ICONS = {
    "fan": fan_icon(),
    "filter": filter_icon(),
    "data": data_icon(),
}


def grid_to_svg(g):
    rects = []
    for y in range(N):
        for x in range(N):
            if g[y][x]:
                rects.append(f'<rect x="{x}" y="{y}" width="1" height="1"/>')
    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" '
            'fill="currentColor" shape-rendering="crispEdges">' +
            "".join(rects) + "</svg>")


def grid_to_image(g, scale=1):
    img = Image.new("RGBA", (N, N), (0, 0, 0, 0))
    px = img.load()
    for y in range(N):
        for x in range(N):
            if g[y][x]:
                px[x, y] = (255, 255, 255, 255)
    if scale != 1:
        img = img.resize((N * scale, N * scale), Image.NEAREST)
    return img


def write_icon_c(name, g):
    values = ["0xff" if g[y][x] else "0x00" for y in range(N) for x in range(N)]
    rows = []
    for y in range(N):
        rows.append("    " + ", ".join(values[y * N:(y + 1) * N]) + ",")
    body = "\n".join(rows)
    text = f'''/**
 * @file {name}_icon.c
 * 24x24 单色像素图标（Alpha 8-bit）。由 make_pixel_icons.py 生成。
 * 显示时配合 image_recolor 跟随主题换色。
 */

#include "lvgl.h"

#include "icon.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
    #define LV_ATTRIBUTE_MEM_ALIGN
#endif

const LV_ATTRIBUTE_MEM_ALIGN uint8_t {name}_icon_map[24 * 24] = {{
{body}
}};

const lv_image_dsc_t {name}_icon = {{
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_A8,
    .header.flags = 0,
    .header.w = 24,
    .header.h = 24,
    .header.stride = 24,
    .data_size = sizeof({name}_icon_map),
    .data = {name}_icon_map,
}};
'''
    (C_DIR / f"{name}_icon.c").write_text(text, encoding="utf-8", newline="\n")


def write_logo_c():
    img = Image.open(LOGO_PNG).convert("RGBA")
    if img.size != (48, 48):
        raise SystemExit(f"logo 尺寸不是 48x48: {img.size}")

    rgb = []
    alpha = []
    px = img.load()
    for y in range(48):
        for x in range(48):
            r, g, b, a = px[x, y]
            v = (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)) & 0xffff
            rgb.append(v & 0xff)
            rgb.append((v >> 8) & 0xff)
            alpha.append(a)

    data = rgb + alpha
    rows = []
    for i in range(0, len(data), 24):
        rows.append("    " + ", ".join(f"0x{v:02x}" for v in data[i:i + 24]) + ",")
    body = "\n".join(rows)

    text = f'''/**
 * @file logo_icon.c
 * 48x48 AirForge 唱片唱臂 logo（RGB565A8）。
 * 源图 resources/logo/png/airforge_recArm_48_stone.png，由 make_pixel_icons.py 生成。
 */

#include "lvgl.h"

#include "icon.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
    #define LV_ATTRIBUTE_MEM_ALIGN
#endif

const LV_ATTRIBUTE_MEM_ALIGN uint8_t logo_icon_map[48 * 48 * 3] = {{
{body}
}};

const lv_image_dsc_t logo_icon = {{
    .header.magic = LV_IMAGE_HEADER_MAGIC,
    .header.cf = LV_COLOR_FORMAT_RGB565A8,
    .header.flags = 0,
    .header.w = 48,
    .header.h = 48,
    .header.stride = 48 * 2,
    .data_size = sizeof(logo_icon_map),
    .data = logo_icon_map,
}};
'''
    (C_DIR / "logo_icon.c").write_text(text, encoding="utf-8", newline="\n")


def write_preview():
    scale = 8
    gap = 16
    img = Image.new("RGBA", (len(ICONS) * (N * scale + gap) + gap,
                             N * scale + 2 * gap + 16), (23, 17, 13, 255))
    draw = ImageDraw.Draw(img)
    for i, (name, g) in enumerate(ICONS.items()):
        icon = grid_to_image(g, scale)
        x = gap + i * (N * scale + gap)
        img.alpha_composite(icon, (x, gap))
        draw.text((x, gap + N * scale + 2), name, fill=(240, 227, 203, 255))
    img.save(SVG_DIR / "preview_icons.png")


def main():
    SVG_DIR.mkdir(parents=True, exist_ok=True)
    C_DIR.mkdir(parents=True, exist_ok=True)

    for name, grid in ICONS.items():
        (SVG_DIR / f"{name}.svg").write_text(grid_to_svg(grid),
                                             encoding="utf-8", newline="\n")
        write_icon_c(name, grid)

    write_logo_c()
    write_preview()
    print("generated:", ", ".join(f"{n}_icon.c" for n in ICONS), "logo_icon.c")


if __name__ == "__main__":
    main()
