#!/usr/bin/env python3
"""
把命令行传入的 SVG 保存到本脚本所在目录（resources/gui_icons/）。

支持的输入形式：
  1. 内联 SVG 文本：      add_svg_icon.py "<svg ...>...</svg>" --name setting_icon
  2. 远程 URL：           add_svg_icon.py https://example.com/gear.svg
  3. 本地 SVG 文件：      add_svg_icon.py ./gear.svg
  4. 标准输入：           add_svg_icon.py - --name gear < gear.svg

输出文件名规则：
  - 指定 --name 时使用该名称（自动补 .svg）
  - URL / 本地文件未指定名称时，沿用原文件名
  - 内联 SVG 未指定名称时，用内容 SHA1 前 8 位生成 icon_<hash>.svg

Windows PowerShell 提示：
  内联 SVG 含双引号，直接从命令行传参会被拆成多个参数，建议用管道：
      $svg = '<svg ...>...</svg>'
      $svg | python add_svg_icon.py - --name gear
  或先把 SVG 存成文件，再传文件路径。
"""

import argparse
import hashlib
import os
import re
import sys
import urllib.request
from urllib.parse import urlparse

ICON_DIR = os.path.dirname(os.path.abspath(__file__))


def load_svg(source):
    if source == "-":
        return sys.stdin.read()

    if source.lstrip().startswith("<"):
        return source

    if re.match(r"https?://", source):
        req = urllib.request.Request(
            source, headers={"User-Agent": "air-forge-icon-fetch"}
        )
        with urllib.request.urlopen(req, timeout=30) as resp:
            return resp.read().decode("utf-8")

    if os.path.isfile(source):
        with open(source, "r", encoding="utf-8") as f:
            return f.read()

    raise SystemExit("无法识别的输入：请传内联 SVG、http(s) URL、本地文件路径或 -")


def validate_svg(content):
    text = content.strip()
    if "<svg" not in text.lower():
        raise SystemExit("输入内容不是 SVG：未找到 <svg> 标签")
    if "</svg>" not in text.lower() and not re.search(r"<svg[^>]*/\s*>", text, re.I):
        raise SystemExit("SVG 内容不完整：缺少 </svg>")
    return text + "\n"


def pick_filename(source, content, explicit_name):
    if explicit_name:
        base = explicit_name
    elif re.match(r"https?://", source):
        base = os.path.basename(urlparse(source).path) or "icon"
    elif os.path.isfile(source):
        base = os.path.basename(source)
    else:
        base = "icon_" + hashlib.sha1(content.encode("utf-8")).hexdigest()[:8]

    base = base.strip()
    if base.lower().endswith(".svg"):
        base = base[:-4]
    base = re.sub(r'[<>:"/\\|?*]+', "_", base)
    if not base:
        base = "icon"
    return base + ".svg"


def report_viewbox(content):
    m = re.search(r'viewBox="\s*([0-9.\-]+)\s+([0-9.\-]+)\s+([0-9.\-]+)\s+([0-9.\-]+)\s*"', content)
    if m:
        print(f"viewBox: {m.group(1)} {m.group(2)} {m.group(3)} {m.group(4)}")


def main():
    parser = argparse.ArgumentParser(
        description="接收 SVG 内容/URL/文件并在 gui_icons 目录生成 .svg 文件"
    )
    parser.add_argument(
        "source",
        nargs="?",
        default=None,
        help="内联 SVG、http(s) URL、本地 .svg 路径，或 - 表示从 stdin 读取；省略时自动读管道输入",
    )
    parser.add_argument(
        "-n", "--name",
        help="输出文件名（不含 .svg）；不填则按 URL/文件名/内容哈希自动命名",
    )
    parser.add_argument(
        "-f", "--force", action="store_true",
        help="允许覆盖同名文件",
    )
    args = parser.parse_args()

    source = args.source
    if source is None:
        if sys.stdin.isatty():
            raise SystemExit(
                "缺少输入：请传内联 SVG / URL / 文件路径，"
                "或通过管道/重定向把 SVG 交给脚本"
            )
        source = "-"

    content = validate_svg(load_svg(source))
    filename = pick_filename(source, content, args.name)
    path = os.path.join(ICON_DIR, filename)

    if os.path.exists(path) and not args.force:
        raise SystemExit(f"已存在：{path}\n如需覆盖，请加 --force")

    os.makedirs(ICON_DIR, exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)

    print(f"已生成：{path}")
    report_viewbox(content)


if __name__ == "__main__":
    main()
