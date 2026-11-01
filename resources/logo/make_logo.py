#!/usr/bin/env python3
"""AirForge 像素风 logo 生成器（留声机 / 黑胶唱片语言）。

双网格方案，保证每个输出尺寸都是整数像素块、零抗锯齿：
    36×36 = 18×18 网格 ×2
    48×48 = 24×24 网格 ×2
唱片类图形由参数化算法生成（圆、纹路圈、中心轴孔、唱臂），
保证两档是同一种设计语言，只是细节随尺寸增加。

字符含义：
    '#' 盘面 / 主体    '*' 强调（唱臂、刻度）    '.' 透明

用法：python make_logo.py
"""

import math
from pathlib import Path

from PIL import Image, ImageDraw

OUT = Path(__file__).resolve().parent
SIZES = (36, 48)

PALETTES = {
	"stone": {
		"#": (0xF0, 0xE3, 0xCB, 255),  # text  亮米白
		"*": (0xE4, 0xB0, 0x4A, 255),  # accent 金
		"+": (0x0B, 0x07, 0x05, 255),  # dark  内阴影
		"bg": (0x17, 0x11, 0x0D, 255),
	},
	"oled": {
		"#": (0xFF, 0xFF, 0xFF, 255),
		"*": (0x8A, 0x8A, 0x8A, 255),
		"+": (0x00, 0x00, 0x00, 255),
		"bg": (0x00, 0x00, 0x00, 255),
	},
}


def disc(n, radius, grooves=(), hole=0.0, arm=None, frame=False):
	"""生成 n×n 的黑胶唱片矩阵。

	radius  盘面半径（格）
	grooves 纹路圈半径列表，落在圈上的像素挖成透明 => 黑胶纹路
	hole    中心轴孔边长（格）
	arm     唱臂线段 ((x0,y0),(x1,y1))，用强调色
	frame   是否叠加机身外框
	"""
	cx = (n - 1) / 2
	grid = [["." for _ in range(n)] for _ in range(n)]
	for y in range(n):
		for x in range(n):
			dx, dy = x - cx, y - cx
			r = math.hypot(dx, dy)
			if r <= radius:
				grid[y][x] = "#"
				if hole and abs(dx) <= hole / 2 and abs(dy) <= hole / 2:
					grid[y][x] = "."
					continue
				for g in grooves:
					if abs(r - g) < 0.48:
						grid[y][x] = "."
						break
	if arm:
		(x0, y0), (x1, y1) = arm
		steps = max(abs(x1 - x0), abs(y1 - y0))
		for i in range(steps + 1):
			x = round(x0 + (x1 - x0) * i / steps)
			y = round(y0 + (y1 - y0) * i / steps)
			if 0 <= y < n and 0 <= x < n:
				grid[y][x] = "*"
	if frame:
		for i in range(1, n - 1):
			grid[1][i] = "#"
			grid[n - 2][i] = "#"
			grid[i][1] = "#"
			grid[i][n - 2] = "#"
	return ["".join(r) for r in grid]


# 手绘矩阵（非圆形的字形标）
A_MATRIX = [
	"............",
	"............",
	"....##......",
	"...####.....",
	"..#....#....",
	"..#....#....",
	".#......#...",
	".########**.",
	".#......#...",
	".#......#...",
	"............",
	"............",
]

NOTE_MATRIX = [
	"............",
	".....###....",
	".....#......",
	".....#......",
	".....#......",
	".....#......",
	"..#####.....",
	"..#####.....",
	"...###......",
	"............",
	"............",
	"............",
]

VARIANTS = {
	"recArm": {
		"cn": "唱片 + 唱臂",
		"desc": "黑胶盘面 + 纹路圈 + 中心轴孔，金色唱臂斜压盘面。唱臂即万向臂，唱片即顶部旋转换芯部件，与结构方案逐条对应。",
		"mats": {
			36: disc(18, 7.2, (5.4,), 1.8, ((17, 1), (10, 8))),
			48: disc(24, 10.0, (8.6, 5.4), 2.2, ((23, 2), (13, 12))),
		},
	},
	"vinyl": {
		"cn": "黑胶唱片",
		"desc": "去掉唱臂的纯盘面，纹路圈与轴孔完整保留。最克制，适合当 36px 角标或加载态图标。",
		"mats": {
			36: disc(18, 7.2, (5.4,), 1.8),
			48: disc(24, 10.0, (8.6, 5.4), 2.2),
		},
	},
	"turntable": {
		"cn": "转盘俯视",
		"desc": "机身外框 + 内部旋转换芯盘 + 轴孔 + 唱臂。直接对应「顶部结构视觉简图」的俯视关系。",
		"mats": {
			36: disc(18, 5.8, (4.2,), 1.5, ((16, 2), (9, 8)), frame=True),
			48: disc(24, 8.6, (7.0, 4.5), 2.0, ((22, 2), (12, 11)), frame=True),
		},
	},
	"airspeed": {
		"cn": "风速 A",
		"desc": "字母 A 的横杠向右喷出金色尾迹。品牌字标方向，不依赖留声机造型，作为对照保留。",
		"mats": {36: A_MATRIX, 48: A_MATRIX},
	},
	"note": {
		"cn": "音符",
		"desc": "单枚像素音符，呼应结构方案里的「情绪按键（灯光 / 音乐）」。可与唱片标搭配当副标。",
		"mats": {36: NOTE_MATRIX, 48: NOTE_MATRIX},
	},
}


def validate():
	for key, v in VARIANTS.items():
		for size, rows in v["mats"].items():
			n = len(rows)
			assert size % n == 0, f"{key} {size}px: 网格 {n} 无法整除"
			for i, row in enumerate(rows):
				assert len(row) == n, f"{key} {size}px 第 {i} 行宽度 {len(row)} ≠ {n}: {row}"
				assert set(row) <= set("#*+. "), f"{key} {size}px 第 {i} 行含非法字符: {row}"


def scale_of(rows, size):
	return size // len(rows)


def to_svg(rows, size, palette):
	s = scale_of(rows, size)
	parts = []
	for y, row in enumerate(rows):
		for x, ch in enumerate(row):
			if ch in "#*+":
				r, g, b, _ = PALETTES[palette][ch]
				parts.append(
					f'<rect x="{x * s}" y="{y * s}" width="{s}" height="{s}" '
					f'fill="#{r:02X}{g:02X}{b:02X}"/>'
				)
	return (
		f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" '
		f'viewBox="0 0 {size} {size}" shape-rendering="crispEdges">'
		f"{''.join(parts)}</svg>\n"
	)


def to_png(rows, size, palette, background=False, upscale=1):
	s = scale_of(rows, size)
	img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
	d = ImageDraw.Draw(img)
	for y, row in enumerate(rows):
		for x, ch in enumerate(row):
			if ch in "#*+":
				d.rectangle([x * s, y * s, x * s + s - 1, y * s + s - 1], fill=PALETTES[palette][ch])
	if background:
		bg = Image.new("RGBA", (size, size), PALETTES[palette]["bg"])
		img = Image.alpha_composite(bg, img)
	if upscale != 1:
		img = img.resize((size * upscale, size * upscale), Image.NEAREST)
	return img


def build_gallery(zoom=6):
	cell = 48 * zoom
	pad, label_h = 16, 26
	width = pad * 2 + cell * 4
	height = pad + len(VARIANTS) * (cell + label_h + pad)
	sheet = Image.new("RGBA", (width, height), (0x0A, 0x0A, 0x0C, 255))
	d = ImageDraw.Draw(sheet)
	y = pad
	for key, v in VARIANTS.items():
		x = pad
		for palette in ("stone", "oled"):
			for size in SIZES:
				img = to_png(v["mats"][size], size, palette, background=True, upscale=zoom)
				sheet.alpha_composite(img, (x + (cell - img.width) // 2, y))
				d.text((x + 6, y + cell + 6), f"{key} {size}", fill=(170, 170, 170))
				x += cell
		d.text((pad, y - 2), v["cn"], fill=(235, 200, 120))
		y += cell + label_h + pad
	return sheet


HTML_HEAD = """<!doctype html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>AirForge · 留声机像素 logo 提案</title>
<style>
:root{
  --bg:#17110D;--panel:#241A15;--light:#C89B54;--dark:#0B0705;
  --text:#F0E3CB;--muted:#A38A6D;--accent:#E4B04A;
}
*{box-sizing:border-box}
html,body{margin:0;padding:0}
body{
  background:var(--bg);color:var(--text);
  font:13px/1.65 ui-monospace,"Cascadia Mono","Courier New",monospace;
  background-image:
    repeating-linear-gradient(0deg,transparent 0 3px,rgba(255,255,255,.012) 3px 4px),
    repeating-linear-gradient(90deg,transparent 0 3px,rgba(255,255,255,.012) 3px 4px);
  padding:28px 20px 60px;
}
header{max-width:1080px;margin:0 auto 26px;border-bottom:2px solid var(--light);padding-bottom:14px}
h1{font-size:20px;letter-spacing:2px;margin:0 0 6px;color:var(--accent)}
.sub{color:var(--muted);margin:0}
.spec{color:var(--muted);font-size:12px;margin:8px 0 0}
.spec code{color:var(--text);background:var(--panel);padding:1px 5px;border:1px solid var(--dark)}
main{max-width:1080px;margin:0 auto}
.card{
  display:grid;grid-template-columns:300px 1fr;gap:22px;
  background:var(--panel);border:2px solid var(--dark);
  box-shadow:4px 4px 0 var(--dark);
  padding:18px;margin-bottom:22px;
}
.meta h2{font-size:15px;margin:0 0 4px;color:var(--text);letter-spacing:1px}
.meta h2 small{color:var(--accent);font-size:12px;margin-left:8px;letter-spacing:0}
.meta p{margin:0 0 8px;color:var(--muted);font-size:12px}
.files{font-size:11px;line-height:1.9}
.files a{color:var(--light);text-decoration:none;border-bottom:1px dotted var(--light)}
.files a:hover{color:var(--accent)}
.thumbs{display:grid;grid-template-columns:repeat(4,1fr);gap:10px;align-items:start}
.thumb{margin:0;border:2px solid var(--dark);padding:8px 4px 4px;text-align:center}
.thumb.stone{background:var(--bg)}
.thumb.oled{background:#000}
.thumb img{image-rendering:pixelated;display:block;margin:0 auto}
.thumb figcaption{color:var(--muted);font-size:10px;margin-top:4px}
.band{max-width:1080px;margin:0 auto 22px;background:var(--panel);border:2px solid var(--dark);box-shadow:4px 4px 0 var(--dark);padding:16px 18px}
.band h2{font-size:14px;margin:0 0 12px;color:var(--accent);letter-spacing:1px}
.row{display:flex;flex-wrap:wrap;gap:18px;align-items:flex-end}
.actual{display:flex;flex-direction:column;align-items:center;gap:4px}
.actual img{image-rendering:pixelated}
.actual span{color:var(--muted);font-size:10px}
.screen{
  width:240px;height:280px;background:var(--bg);border:3px solid var(--light);
  box-shadow:inset 0 0 0 2px var(--dark),6px 6px 0 var(--dark);
  display:flex;flex-direction:column;align-items:center;justify-content:center;gap:14px;position:relative;
}
.screen .brand{display:flex;align-items:center;gap:8px}
.screen .brand img{image-rendering:pixelated}
.screen .name{font-size:11px;letter-spacing:3px;color:var(--accent)}
.screen .hint{font-size:10px;color:var(--muted);border:2px solid var(--dark);padding:6px 10px;box-shadow:2px 2px 0 var(--dark)}
footer{max-width:1080px;margin:26px auto 0;color:var(--muted);font-size:11px;border-top:2px solid var(--dark);padding-top:12px}
</style>
</head>
<body>
<header>
  <h1>Air<span style="color:var(--text)">FORGE</span> · 留声机像素 logo 提案</h1>
  <p class="sub">桌面焊接废气净化装置 &nbsp;·&nbsp; 顶部仿黑胶唱片旋转换芯 + 万向臂</p>
  <p class="spec">36px = 18×18 网格 ×2 ，48px = 24×24 网格 ×2 &nbsp;|&nbsp;
  配色取项目主题 <code>STONE</code> / <code>OLED</code> &nbsp;|&nbsp; 透明背景</p>
</header>
<main>
"""

HTML_TAIL = """</main>
<footer>
  素材位于 resources/logo/svg 与 resources/logo/png，重新生成执行 python resources/logo/make_logo.py。<br>
  若要接进 LVGL，建议 A8（单色随主题染色）或 RGB565A8 的 C 数组，命名对齐 ui/assets/icons/。
</footer>
</body>
</html>
"""


def build_html():
	cards = []
	for key, v in VARIANTS.items():
		thumbs = []
		for pal in ("stone", "oled"):
			for size in SIZES:
				thumbs.append(
					f'      <figure class="thumb {pal}">'
					f'<img src="svg/airforge_{key}_{size}_{pal}.svg" '
					f'width="{size * 6}" height="{size * 6}" alt="{key} {size}px {pal}">'
					f"<figcaption>{size}px · {pal}</figcaption></figure>"
				)
		links = " · ".join(
			f'<a href="svg/airforge_{key}_{size}_{pal}.svg">{size}-{pal}</a>'
			for pal in ("stone", "oled")
			for size in SIZES
		)
		cards.append(
			f"""    <section class="card">
      <div class="meta">
        <h2>{key}<small>{v['cn']}</small></h2>
        <p>{v['desc']}</p>
        <p class="files">{links}</p>
      </div>
      <div class="thumbs">
{chr(10).join(thumbs)}
      </div>
    </section>"""
		)

	actual = []
	for key, v in VARIANTS.items():
		for size in SIZES:
			actual.append(
				f'      <div class="actual"><img src="svg/airforge_{key}_{size}_stone.svg" '
				f'width="{size}" height="{size}" alt="{key} {size}"><span>{key} {size}</span></div>'
			)

	sim = """  <section class="band">
    <h2>实机尺寸模拟 · 240×280</h2>
    <div class="row">
      <div class="screen">
        <div class="brand"><img src="svg/airforge_recArm_36_stone.svg" width="36" height="36" alt="logo"><span class="name">AIRFORGE</span></div>
        <div class="hint">焊接废气净化</div>
        <img src="svg/airforge_recArm_48_stone.svg" width="48" height="48" alt="48px logo">
      </div>
      <div class="screen" style="background:#000;border-color:#fff">
        <div class="brand"><img src="svg/airforge_recArm_36_oled.svg" width="36" height="36" alt="logo"><span class="name" style="color:#fff">AIRFORGE</span></div>
        <div class="hint" style="color:#8A8A8A;border-color:#2A2A2A;box-shadow:2px 2px 0 #2A2A2A">焊接废气净化</div>
        <img src="svg/airforge_recArm_48_oled.svg" width="48" height="48" alt="48px logo">
      </div>
    </div>
  </section>"""

	page = (
		HTML_HEAD
		+ f"""  <section class="band">
    <h2>1:1 实际像素对照</h2>
    <div class="row">
{chr(10).join(actual)}
    </div>
  </section>

"""
		+ sim
		+ "\n\n"
		+ "\n".join(cards)
		+ "\n"
		+ HTML_TAIL
	)
	(OUT / "showcase.html").write_text(page, encoding="utf-8")


def main():
	validate()
	for sub in ("svg", "png"):
		d = OUT / sub
		d.mkdir(exist_ok=True)
		for old in d.iterdir():
			old.unlink()
	for key, v in VARIANTS.items():
		for size in SIZES:
			rows = v["mats"][size]
			for palette in PALETTES:
				(OUT / "svg" / f"airforge_{key}_{size}_{palette}.svg").write_text(
					to_svg(rows, size, palette), encoding="utf-8"
				)
				to_png(rows, size, palette).save(OUT / "png" / f"airforge_{key}_{size}_{palette}.png")
	build_gallery().save(OUT / "preview_gallery.png")
	build_html()
	print(f"OK  {len(VARIANTS)} variants × {len(SIZES)} sizes × {len(PALETTES)} palettes")
	print(f"gallery: {OUT / 'preview_gallery.png'}")
	print(f"showcase: {OUT / 'showcase.html'}")


if __name__ == "__main__":
	main()
