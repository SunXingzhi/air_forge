#!/usr/bin/env python3
"""从 Fusion Pixel TTF 生成 LVGL 子集字体。

源字体：
  resources/fonts/fusion-pixel-12px-proportional-zh_hans.ttf
  resources/fonts/fusion-pixel-8px-proportional-zh_hans.ttf

字符集：
  ASCII 0x20-0x7E + ui/ 源码字符串字面量里出现的所有非 ASCII 字符
  （改中文文案后重跑本脚本，字库会自动跟随）

依赖：
  Node.js + lv_font_conv（仅生成时需要；生成后的 C 文件随仓库提交）
  npm install lv_font_conv
"""

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FONT_DIR = Path(__file__).resolve().parent
OUT_DIR = ROOT / "ui" / "assets" / "fonts"
UI_DIR = ROOT / "ui"

# 8px 中文字形过于密集，UI 里中文一律用 12px；8px 字体只保留 ASCII
# 小字（单位、英文标签），可把字库从 ~36KB 压到 ~4KB，节省 Flash。
FONTS = (
    ("fusion-pixel-12px-proportional-zh_hans.ttf", 12, "lv_font_fusion_12", True),
    ("fusion-pixel-8px-proportional-zh_hans.ttf", 8, "lv_font_fusion_8", False),
)


def collect_symbols():
    """扫描 ui/ 源码中的字符串字面量，返回去重后的非 ASCII 字符。"""
    chars = set()
    string_re = re.compile(r'"(?:\\.|[^"\\])*"')

    for path in UI_DIR.rglob("*.c"):
        if OUT_DIR in path.parents:
            continue
        text = path.read_text(encoding="utf-8")
        for match in string_re.finditer(text):
            for ch in match.group(0):
                if ord(ch) > 0x7F:
                    chars.add(ch)
    return "".join(sorted(chars))


def find_node():
    """返回 node 可执行文件路径：优先 PATH，其次 .cache 中的便携版。"""
    node = shutil.which("node")
    if node:
        return node

    cache = ROOT / ".cache" / "fontgen" / "node"
    for candidate in cache.glob("node-*/node.exe"):
        return str(candidate)
    raise SystemExit("未找到 node；请安装 Node.js 或解压便携版到 .cache/fontgen/node/")


def find_lv_font_conv():
    """返回 lv_font_conv.js 路径：优先 PATH，其次 .cache/fontgen/node_modules。"""
    local = ROOT / ".cache" / "fontgen" / "node_modules" / "lv_font_conv" / "lv_font_conv.js"
    if local.is_file():
        return str(local)
    raise SystemExit("未找到 lv_font_conv.js；请先执行 npm install lv_font_conv")


def build_font(node, converter, ttf_name, size, font_name, symbols):
    """调用 lv_font_conv 生成单个 LVGL 字体 C 文件。"""
    out_name = f"{font_name}.c"
    cmd = [
        node,
        converter,
        "--font", ttf_name,
        "--size", str(size),
        "--bpp", "1",
        "--format", "lvgl",
        "--no-compress",
        "--no-prefilter",
        "--no-kerning",
        "--range", "0x20-0x7E",
        "--lv-font-name", font_name,
        "--lv-include", "lvgl.h",
        "-o", f"../../ui/assets/fonts/{out_name}",
    ]
    if symbols:
        cmd.extend(["--symbols", symbols])

    # cwd 固定为 resources/fonts，保证生成文件头注释里是相对路径
    result = subprocess.run(cmd, cwd=FONT_DIR, text=True,
                            encoding="utf-8", capture_output=True)
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        raise SystemExit(f"{font_name} 生成失败")

    out_path = OUT_DIR / out_name
    print(f"{out_path.relative_to(ROOT)}  {out_path.stat().st_size} bytes  "
          f"{size}px  {len(symbols)} glyphs")


def main():
    """收集字符集并生成 12px / 8px 两套字体。"""
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    symbols = collect_symbols()
    print(f"non-ASCII glyphs: {len(symbols)}")

    node = find_node()
    converter = find_lv_font_conv()

    for ttf, size, font_name, with_symbols in FONTS:
        ttf_path = FONT_DIR / ttf
        if not ttf_path.is_file():
            raise SystemExit(f"缺少源字体：{ttf_path}")
        used = symbols if with_symbols else ""
        build_font(node, converter, ttf, size, font_name, used)


if __name__ == "__main__":
    main()
