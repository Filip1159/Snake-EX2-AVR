#!/usr/bin/env python3
"""convert_assets.py — RGB565 conversion + preview sheets (BACKLOG task A2).

Usage:
  python3 tools/extract_snake_ex2.py        # first
  python3 tools/convert_assets.py [--assets build/assets]

Reads build/assets (from extract_snake_ex2.py) and writes:
  rgb565/<png>.rgb565      raw pixels, big-endian RGB565 (ST7735 wire order), row-major
  rgb565/<png>.json        width/height
  sprites_rgb565.json      per sprite: palette_rgb565[], transparent[] (indices), pixels[]
  mazes_21x23.bin          6 x 483 derived maze maps (BACKLOG §1.5)
  preview/sprites.png      all 61 sprites on a checker background, 4x
  preview/mazes_orig.png   6 original 21x18 mazes, 2x, drawn with the wall strip
  preview/mazes_21x23.png  6 derived mazes
  preview/screen_128x160.png  derived full-screen layout mock (maze 0 + HUD)
  preview/colorcheck.png   RGB565 round trip vs source, error stats
Fails loudly if a round-trip error exceeds the quantization bound.
"""
import argparse
import json
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
COLS, ROWS_ORIG, ROWS_NEW = 21, 18, 23
INSERT_AFTER, INSERT_COUNT = 9, 5
BOX = 0
BG = (0xEE, 0xDD, 0xBB)   # b.java A(): setColor(-1122885) = 0xFFEEDDBB playfield background


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def rgb565_to_rgb888(v):
    r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
    return ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))


def derive_maze(m, index):
    """Insert 5 rows after row 9. Box repeats its side-wall row, others blank."""
    out = []
    side = m[1 * COLS:2 * COLS]            # Box row 1 = '2...2'
    for r in range(ROWS_ORIG):
        out += m[r * COLS:(r + 1) * COLS]
        if r == INSERT_AFTER:
            for _ in range(INSERT_COUNT):
                out += side if index == BOX else [0] * COLS
    assert len(out) == COLS * ROWS_NEW
    return out


def load_walls(png_dir):
    strip = Image.open(png_dir / "wall_strip.png").convert("RGBA")
    return [strip.crop((6 * i, 0, 6 * i + 6, 6)) for i in range(15)]


def render_maze(m, rows, walls, scale):
    img = Image.new("RGB", (COLS * 6, rows * 6), BG)
    for r in range(rows):
        for c in range(COLS):
            t = m[r * COLS + c] & 0x0F
            if t:
                img.paste(walls[t - 1], (c * 6, r * 6), walls[t - 1])
    return img.resize((img.width * scale, img.height * scale), Image.NEAREST)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--assets", type=Path, default=ROOT / "build" / "assets")
    a = ap.parse_args()
    A = a.assets
    for need in ("mazes.bin", "sprites.json", "png/title.png"):
        if not (A / need).exists():
            print(f"ERROR: {A/need} missing; run tools/extract_snake_ex2.py first", file=sys.stderr)
            sys.exit(1)
    (A / "rgb565").mkdir(exist_ok=True)
    (A / "preview").mkdir(exist_ok=True)

    # ---- PNGs -> RGB565 ----
    max_err = 0
    for p in sorted((A / "png").glob("*.png")):
        im = Image.open(p).convert("RGB")
        raw = bytearray()
        for (r, g, b) in im.getdata():
            v = rgb565(r, g, b)
            raw += bytes((v >> 8, v & 0xFF))
            rr, gg, bb = rgb565_to_rgb888(v)
            max_err = max(max_err, abs(rr - r), abs(gg - g), abs(bb - b))
        (A / "rgb565" / (p.stem + ".rgb565")).write_bytes(raw)
        (A / "rgb565" / (p.stem + ".json")).write_text(json.dumps({"w": im.width, "h": im.height}))
    if max_err > 8:
        print(f"ERROR: PNG RGB565 round-trip error {max_err} > 8", file=sys.stderr)
        sys.exit(1)

    # ---- sprites ----
    sprites = json.loads((A / "sprites.json").read_text())
    out = []
    for s in sprites:
        pal565, tr = [], list(s["transparent"])
        for i, (r, g, b, al) in enumerate(s["palette_rgba8"]):
            pal565.append(rgb565(r, g, b))
            if al < 128 and i not in tr:
                tr.append(i)
            rr, gg, bb = rgb565_to_rgb888(pal565[-1])
            max_err = max(max_err, abs(rr - r), abs(gg - g), abs(bb - b))
        out.append({"name": s["name"], "w": s["w"], "h": s["h"],
                    "palette_rgb565": pal565, "transparent": sorted(tr), "pixels": s["pixels"]})
    (A / "sprites_rgb565.json").write_text(json.dumps(out))
    if max_err > 8:
        print(f"ERROR: sprite RGB565 round-trip error {max_err} > 8", file=sys.stderr)
        sys.exit(1)

    # ---- tongue (f.png): four rotated 4 bpp sprites with one transparent index ----
    def argb4444(r, g, b, a):
        return ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4)

    tongue = Image.open(A / "png" / "f.png").convert("RGBA")
    tpal = [(255, 255, 255, 0)]          # index 0 = transparent
    for (r, g, b, a) in tongue.getdata():
        c = (r, g, b, 255)
        if a >= 128 and c not in tpal:
            tpal.append(c)
    # f.png points right. PIL ROTATE_90 turns counter-clockwise, i.e. right -> up;
    # the original uses DirectGraphics transforms 90/180/270 for up/left/down.
    frames = (("TONGUE_R", None), ("TONGUE_U", Image.ROTATE_90),
              ("TONGUE_L", Image.ROTATE_180), ("TONGUE_D", Image.ROTATE_270))
    tongue_out = []
    for name, rot in frames:
        im = tongue if rot is None else tongue.transpose(rot)
        px = [0 if a < 128 else tpal.index((r, g, b, 255))
              for (r, g, b, a) in im.getdata()]
        tongue_out.append({
            "name": name, "w": im.width, "h": im.height,
            "palette_argb4444": [argb4444(*c) for c in tpal],
            "palette_rgba8": [list(c) for c in tpal],
            "transparent": [0], "pixels": px})
    (A / "tongue.json").write_text(json.dumps(tongue_out, indent=1), encoding="utf-8")
    for e in tongue_out:                 # include in the preview sheet
        out.append({"name": e["name"], "w": e["w"], "h": e["h"],
                    "palette_rgb565": [rgb565(*c[:3]) for c in e["palette_rgba8"]],
                    "transparent": [0], "pixels": e["pixels"]})

    # ---- mazes ----
    data = (A / "mazes.bin").read_bytes()
    orig = [list(data[i * 378:(i + 1) * 378]) for i in range(6)]
    derived = [derive_maze(m, i) for i, m in enumerate(orig)]
    (A / "mazes_21x23.bin").write_bytes(bytes(sum(derived, [])))
    for i, (o, d) in enumerate(zip(orig, derived)):
        # the original rows must survive unchanged around the insertion
        assert d[:10 * COLS] == o[:10 * COLS] and d[15 * COLS:] == o[10 * COLS:], f"maze {i}"

    # ---- previews ----
    walls = load_walls(A / "png")
    per_row = 10
    n = len(out)
    rows = (n + per_row - 1) // per_row
    W, H = per_row * 40, rows * 40
    sheet = Image.new("RGB", (W, H), BG)
    for k, s in enumerate(out):
        im = Image.new("RGBA", (s["w"], s["h"]))
        px = []
        for idx in s["pixels"]:
            if idx in s["transparent"]:
                px.append((0, 0, 0, 0))
            else:
                r, g, b = rgb565_to_rgb888(s["palette_rgb565"][idx])
                px.append((r, g, b, 255))
        im.putdata(px)
        sheet.paste(im, ((k % per_row) * 40 + 2, (k // per_row) * 40 + 2), im)
    sheet = sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST)
    sheet.save(A / "preview" / "sprites.png")

    def tile_sheet(mazes, rows_, name):
        w, h = COLS * 6 * 2, rows_ * 6 * 2
        sh = Image.new("RGB", (3 * (w + 8) + 8, 2 * (h + 8) + 8), (40, 40, 40))
        for i, m in enumerate(mazes):
            sh.paste(render_maze(m, rows_, walls, 2), (8 + (i % 3) * (w + 8), 8 + (i // 3) * (h + 8)))
        sh.save(A / "preview" / name)

    tile_sheet(orig, ROWS_ORIG, "mazes_orig.png")
    tile_sheet(derived, ROWS_NEW, "mazes_21x23.png")

    # derived 128x160 mock: border, maze 1 at (1,1), HUD at (0,139)
    hud = Image.open(A / "png" / "hud.png").convert("RGB")
    for i in (0, 1):
        scr = Image.new("RGB", (128, 160), (0, 0, 0))
        scr.paste(render_maze(derived[i], ROWS_NEW, walls, 1), (1, 1))
        scr.paste(hud, (0, 139))
        scr.resize((384, 480), Image.NEAREST).save(A / "preview" / f"screen_128x160_maze{i}.png")

    print(f"OK: {len(list((A/'png').glob('*.png')))} PNGs, {len(out)} sprites, 6 derived mazes; "
          f"max RGB565 round-trip error {max_err}/255 -> {A}")


if __name__ == "__main__":
    main()
