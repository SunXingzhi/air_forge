#!/usr/bin/env python3
"""拉取桌面仿真的第三方依赖到 third_party/。

- LVGL 9.3.0：所有平台
- SDL2 2.32.10 MinGW 开发包：仅 Windows（Linux/macOS 使用系统 SDL2）
"""

import argparse
import os
import shutil
import sys
import tarfile
import tempfile
import urllib.request
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
THIRD_PARTY = os.path.join(ROOT, "third_party")

LVGL_VERSION = "9.3.0"
LVGL_URL = f"https://codeload.github.com/lvgl/lvgl/zip/refs/tags/v{LVGL_VERSION}"

SDL_VERSION = "2.32.10"
SDL_URL = (
    "https://github.com/libsdl-org/SDL/releases/download/"
    f"release-{SDL_VERSION}/SDL2-devel-{SDL_VERSION}-mingw.tar.gz"
)


def download(url, dest):
    print(f"下载 {url}")
    req = urllib.request.Request(url, headers={"User-Agent": "air-forge-fetch-deps"})
    with urllib.request.urlopen(req, timeout=120) as resp, open(dest, "wb") as f:
        shutil.copyfileobj(resp, f)
    print(f"  已保存 {dest} ({os.path.getsize(dest) / 1024 / 1024:.1f} MB)")


def extract_zip(src, dest):
    with tempfile.TemporaryDirectory() as tmp:
        with zipfile.ZipFile(src) as zf:
            zf.extractall(tmp)
        top = os.path.join(tmp, os.listdir(tmp)[0])
        shutil.move(top, dest)


def extract_tar(src, dest):
    with tempfile.TemporaryDirectory() as tmp:
        with tarfile.open(src, "r:gz") as tf:
            tf.extractall(tmp)
        top = os.path.join(tmp, os.listdir(tmp)[0])
        shutil.move(top, dest)


def main():
    parser = argparse.ArgumentParser(description="拉取 AirForge 仿真依赖")
    parser.add_argument(
        "--lvgl-only",
        action="store_true",
        help="只拉取 LVGL（Linux/macOS 使用系统 SDL2 时）",
    )
    args = parser.parse_args()

    os.makedirs(THIRD_PARTY, exist_ok=True)

    lvgl_dir = os.path.join(THIRD_PARTY, "lvgl")
    if os.path.exists(os.path.join(lvgl_dir, "lvgl.h")):
        print(f"LVGL 已存在：{lvgl_dir}")
    else:
        tmp_zip = os.path.join(THIRD_PARTY, "lvgl.zip")
        download(LVGL_URL, tmp_zip)
        if os.path.exists(lvgl_dir):
            shutil.rmtree(lvgl_dir)
        extract_zip(tmp_zip, lvgl_dir)
        os.remove(tmp_zip)
        print(f"LVGL {LVGL_VERSION} -> {lvgl_dir}")

    if args.lvgl_only or sys.platform != "win32":
        print("跳过 SDL2（请使用系统包：libsdl2-dev / brew install sdl2）")
        return

    sdl_dir = os.path.join(THIRD_PARTY, "SDL2")
    sdl_header = os.path.join(
        sdl_dir, "x86_64-w64-mingw32", "include", "SDL2", "SDL.h"
    )
    if os.path.exists(sdl_header):
        print(f"SDL2 已存在：{sdl_dir}")
        return

    tmp_tar = os.path.join(THIRD_PARTY, "SDL2-mingw.tar.gz")
    download(SDL_URL, tmp_tar)
    if os.path.exists(sdl_dir):
        shutil.rmtree(sdl_dir)
    extract_tar(tmp_tar, sdl_dir)
    os.remove(tmp_tar)
    print(f"SDL2 {SDL_VERSION} -> {sdl_dir}")


if __name__ == "__main__":
    main()
