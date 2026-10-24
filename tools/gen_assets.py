#!/usr/bin/env python3
"""gen_assets.py — Generate PROGMEM C sources from build/assets (BACKLOG A3+).

Usage: python3 tools/extract_snake_ex2.py && python3 tools/convert_assets.py && python3 tools/gen_assets.py

Writes src/assets/assets_gen.h / assets_gen.cpp. Never hand-edit those files.
Currently emits:
  WALL_PALETTE[]      RGB565 palette of wall_strip.png (<=16 colours)
  WALL_TILES[15][18]  15 tiles of 6x6, 4-bit indices, high nibble first
  MAZES_4BPP[5][189]  original 21x18 maps (tile id 0..15), 4-bit packed.
                      Maze 5 (unused L-shape) is omitted: Extreme cycles -1,0..4.
  SPRITES[63]         B0..B41, power-up/bonus-food frames P0_0..P7_3 (15), C, c, and the four
                      tongue rotations TONGUE_R/U/L/D (4 bpp + palette, transparent index or
                      0xFF). d/D (high-score art) not emitted yet.
  IMG_TITLE, IMG_HUD, IMG_HUD_ICON_BG, IMG_DIGITS[10] (8x9), IMG_POWERUPS[10] (13x13)
                      palette + RLE streams (token>=0x80: run of next index, len=(t&0x7F)+1;
                      else t+1 literal indices). See render/image.h.
"""
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
A = ROOT / "build" / "assets"
OUT = ROOT / "src" / "assets"
MAZE_COUNT = 5


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def fmt(vals, width, per_line):
    lines = []
    for i in range(0, len(vals), per_line):
        lines.append("    " + ", ".join(f"0x{v:0{width}X}" for v in vals[i:i + per_line]) + ",")
    return "\n".join(lines)


def encode_rle(idx):
    out = bytearray()
    i = 0
    n = len(idx)
    while i < n:
        j = i
        while j + 1 < n and idx[j + 1] == idx[i] and j - i < 127:
            j += 1
        if j > i:
            out += bytes((0x80 | (j - i), idx[i]))
            i = j + 1
        else:
            k = i
            while k + 1 < n and k - i < 126 and not (k + 2 < n and idx[k + 1] == idx[k + 2]):
                k += 1
            out.append(k - i)
            out += bytes(idx[i:k + 1])
            i = k + 1
    return bytes(out)


def decode_rle(data, count):
    out = []
    i = 0
    while i < len(data):
        t = data[i]
        i += 1
        if t & 0x80:
            out += [data[i]] * ((t & 0x7F) + 1)
            i += 1
        else:
            out += list(data[i:i + t + 1])
            i += t + 1
    assert len(out) == count, (len(out), count)
    return out


def palette_of(img):
    cols = []
    for p in (img.get_flattened_data() if hasattr(img, "get_flattened_data") else img.getdata()):
        if p not in cols:
            cols.append(p)
    return cols


def image_group(name, parts, cols):
    """parts: list of (suffix, RGB image). Shared palette `cols`. Returns (c_defs, entries)."""
    if len(cols) > 128:
        sys.exit(f"ERROR: {name}: {len(cols)} colours")
    pal = [rgb565(*c) for c in cols]
    defs = [f"static const uint16_t PAL_{name}[{len(pal)}] PROGMEM = {{\n{fmt(pal, 4, 8)}\n}};"]
    entries = []
    total = 0
    for suf, im in parts:
        px = im.get_flattened_data() if hasattr(im, "get_flattened_data") else im.getdata()
        idx = [cols.index(p) for p in px]
        data = encode_rle(idx)
        assert decode_rle(data, im.width * im.height) == idx
        total += len(data)
        sym = f"DATA_{name}{suf}"
        defs.append(f"static const uint8_t {sym}[{len(data)}] PROGMEM = {{\n{fmt(list(data), 2, 16)}\n}};")
        entries.append(f"{{{im.width}, {im.height}, PAL_{name}, {sym}, {len(data)}}}")
    return defs, entries, total + 2 * len(pal)


def main():
    for need in ("png/wall_strip.png", "mazes.bin"):
        if not (A / need).exists():
            sys.exit(f"ERROR: {A/need} missing; run extract_snake_ex2.py first")

    strip = Image.open(A / "png" / "wall_strip.png").convert("RGB")
    colors = []
    for p in strip.get_flattened_data() if hasattr(strip, "get_flattened_data") else strip.getdata():
        if p not in colors:
            colors.append(p)
    if len(colors) > 16:
        sys.exit(f"ERROR: wall strip has {len(colors)} colours (>16)")
    pal = [rgb565(*c) for c in colors]
    tiles = []
    for t in range(15):
        idx = [colors.index(strip.getpixel((6 * t + x, y))) for y in range(6) for x in range(6)]
        tiles += [(idx[i] << 4) | idx[i + 1] for i in range(0, 36, 2)]

    d = (A / "mazes.bin").read_bytes()
    mazes = []
    for m in range(MAZE_COUNT):
        cells = d[m * 378:(m + 1) * 378]
        if any(c > 15 for c in cells):
            sys.exit(f"ERROR: maze {m} has tile id > 15")
        mazes += [(cells[i] << 4) | cells[i + 1] for i in range(0, 378, 2)]


    # ---- sprites ----
    import json
    sp = {x["name"]: x for x in json.loads((A / "sprites.json").read_text())}
    for x in json.loads((A / "tongue.json").read_text()):
        sp[x["name"]] = x
    order = [f"B{i}" for i in range(42)]
    for g, n in enumerate([1, 1, 1, 2, 2, 2, 2, 4]):
        order += [f"P{g}_{f}" for f in range(n)]
    order += ["C", "c"]
    order += ["TONGUE_R", "TONGUE_U", "TONGUE_L", "TONGUE_D"]
    spr_defs, spr_entries = [], []
    for k, name in enumerate(order):
        x = sp[name]
        npal = len(x["palette_argb4444"])
        if npal > 16:
            sys.exit(f"ERROR: sprite {name} has {npal} colours")
        spal = [rgb565(*c[:3]) for c in x["palette_rgba8"]]
        tr = x["transparent"]
        if len(tr) > 1:
            sys.exit(f"ERROR: sprite {name} has {len(tr)} transparent colours")
        px = x["pixels"] + ([0] if len(x["pixels"]) % 2 else [])
        packed = [(px[i] << 4) | px[i + 1] for i in range(0, len(px), 2)]
        spr_defs.append(f"static const uint16_t SP{k}_PAL[{npal}] PROGMEM = {{{', '.join(f'0x{v:04X}' for v in spal)}}};")
        spr_defs.append(f"static const uint8_t SP{k}_PIX[{len(packed)}] PROGMEM = {{{', '.join(f'0x{v:02X}' for v in packed)}}};  /* {name} */")
        spr_entries.append(f"{{{x['w']}, {x['h']}, {tr[0] if tr else 0xFF}, SP{k}_PAL, SP{k}_PIX}}")
    spr_bytes = sum(len(x["pixels"]) // 2 + 1 for x in (sp[n] for n in order))

    # ---- RLE images ----
    def load(n):
        return Image.open(A / "png" / f"{n}.png").convert("RGB")
    title, hud, digits, pu = load("title"), load("hud"), load("digits"), load("powerup_strip")
    img_defs, img_sizes = [], {}
    d, e, sz = image_group("TITLE", [("", title)], palette_of(title))
    img_defs += d; title_e = e[0]; img_sizes["title"] = sz
    hud_cols = palette_of(hud)
    d, e, sz = image_group("HUD", [("", hud), ("_ICONBG", hud.crop((110, 3, 123, 16)))], hud_cols)
    img_defs += d; hud_e, iconbg_e = e; img_sizes["hud"] = sz
    d, e, sz = image_group("DIGITS", [(f"{k}", digits.crop((7 * k, 0, 7 * k + 8, 9))) for k in range(10)], palette_of(digits))
    img_defs += d; dig_e = e; img_sizes["digits"] = sz
    d, e, sz = image_group("PU", [(f"{k}", pu.crop((13 * k, 0, 13 * k + 13, 13))) for k in range(10)], palette_of(pu))
    img_defs += d; pu_e = e; img_sizes["powerups"] = sz

    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "assets_gen.h").write_text(f"""/* GENERATED by tools/gen_assets.py — do not edit. */
#ifndef ASSETS_GEN_H
#define ASSETS_GEN_H
#include <stdint.h>
#include <avr/pgmspace.h>

#define WALL_PALETTE_SIZE {len(pal)}
#define WALL_TILE_BYTES   18          /* 6x6 px, 4 bit */
#define MAZE_COUNT        {MAZE_COUNT}
#define MAZE_SRC_BYTES    189         /* 21x18 cells, 4 bit */

/* Palette + RLE image, stored in flash (read with memcpy_P). */
typedef struct {{
    uint8_t w, h;
    const uint16_t *pal;     /* PROGMEM RGB565 palette */
    const uint8_t *data;     /* PROGMEM RLE stream */
    uint16_t len;
}} img_t;

/* 4 bpp sprite; pixels row-major, high nibble first, rows NOT padded. */
typedef struct {{
    uint8_t w, h;
    uint8_t transparent;     /* palette index that is transparent, 0xFF = none */
    const uint16_t *pal;
    const uint8_t *pix;
}} spr_t;
#define SPRITE_COUNT {len(order)}
extern const spr_t SPRITES[SPRITE_COUNT] PROGMEM;

extern const img_t IMG_TITLE PROGMEM;
extern const img_t IMG_HUD PROGMEM;
extern const img_t IMG_HUD_ICON_BG PROGMEM;   /* 13x13 empty power-up slot at HUD (110,3) */
extern const img_t IMG_DIGITS[10] PROGMEM;    /* 8x9 cells, glyph k = strip columns 7k..7k+7 */
extern const img_t IMG_POWERUPS[10] PROGMEM;  /* 13x13, index = power-up id - 1 */

extern const uint16_t WALL_PALETTE[WALL_PALETTE_SIZE] PROGMEM;
extern const uint8_t  WALL_TILES[15][WALL_TILE_BYTES] PROGMEM;
extern const uint8_t  MAZES_4BPP[MAZE_COUNT][MAZE_SRC_BYTES] PROGMEM;

#endif
""", encoding="utf-8")
    body = ["/* GENERATED by tools/gen_assets.py — do not edit. */", '#include "assets/assets_gen.h"', "",
            f"const uint16_t WALL_PALETTE[WALL_PALETTE_SIZE] PROGMEM = {{\n{fmt(pal, 4, 8)}\n}};", "",
            "const uint8_t WALL_TILES[15][WALL_TILE_BYTES] PROGMEM = {"]
    for t in range(15):
        body.append("    {" + ", ".join(f"0x{v:02X}" for v in tiles[t * 18:(t + 1) * 18]) + "},")
    body += ["};", ""] + img_defs + ["",
            f"const img_t IMG_TITLE PROGMEM = {title_e};",
            f"const img_t IMG_HUD PROGMEM = {hud_e};",
            f"const img_t IMG_HUD_ICON_BG PROGMEM = {iconbg_e};",
            "const img_t IMG_DIGITS[10] PROGMEM = {\n    " + ",\n    ".join(dig_e) + ",\n};",
            "const img_t IMG_POWERUPS[10] PROGMEM = {\n    " + ",\n    ".join(pu_e) + ",\n};", ""]
    body += spr_defs + ["", "const spr_t SPRITES[SPRITE_COUNT] PROGMEM = {\n    " + ",\n    ".join(spr_entries) + ",\n};", ""]
    body += ["const uint8_t MAZES_4BPP[MAZE_COUNT][MAZE_SRC_BYTES] PROGMEM = {"]
    for m in range(MAZE_COUNT):
        body.append("    {\n" + fmt(mazes[m * 189:(m + 1) * 189], 2, 16) + "\n    },")
    body.append("};")
    (OUT / "assets_gen.cpp").write_text("\n".join(body) + "\n", encoding="utf-8")
    print("image bytes (RLE+palette):", img_sizes, "sprite pixel bytes:", spr_bytes)
    print(f"OK: wall palette {len(pal)} colours, 15 tiles ({len(tiles)} B), {MAZE_COUNT} mazes ({len(mazes)} B)")


if __name__ == "__main__":
    main()
