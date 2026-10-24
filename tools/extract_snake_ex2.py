#!/usr/bin/env python3
"""extract_snake_ex2.py — Strict extractor for the Snake EX2 `d` resource (BACKLOG §1.2, task A1).

Usage:
  python3 tools/extract_snake_ex2.py [--d reference/d] [--out build/assets]

Splits `d` into mazes, 8 PNGs and 61 sprite records. Every layout fact is
asserted: any unexpected size, offset, palette count or trailing byte aborts
with a non-zero exit code. Writes a manifest with SHA-256 checksums.

Output (build/assets/):
  mazes.bin            6 x 378 raw maze maps (21x18)
  png/<name>.png       the 8 embedded PNGs, byte-exact (g/G named by size: case-insensitive FS)
  sprites.json         all 61 records: name, w, h, palette (ARGB4444 + RGBA8888), pixel indices
  manifest.sha256      checksums of everything above + the input
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
D_SHA = "6eb869cceb6d8465dc45e9ce9a12771516056206ac2c95f83e90550db46174c8"
D_SIZE = 15602

MAZE_OFF, MAZE_COUNT, MAZE_SIZE = 0, 6, 378

# name, offset, length, width, height
PNGS = [
    ("title",         2268, 3616, 128, 128),
    ("digits",        5884, 1280,  77,   9),
    ("hud",           7164, 1833, 128,  20),
    ("g_11x11",        8997,  287,  11,  11),
    ("G_60x21",        9284,  403,  60,  21),
    ("powerup_strip", 9687, 1440, 130,  13),
    ("f",            11127,  184,  11,  11),
    ("wall_strip",   11311,  309, 120,   6),
]
SPRITE_OFF = 11620
SPRITE_END = D_SIZE
TRANSPARENT_4444 = 0xFF0F   # -241 as int16

# Expected record sequence: (name, w, h)
EXPECTED = (
    [(f"B{i}", 6, 6) for i in range(30)]
    + [(f"B{i}", 9, 9) for i in range(30, 36)]
    + [(f"B{i}", 11, 11) for i in range(36, 42)]
)
for _g, _n in enumerate([1, 1, 1, 2, 2, 2, 2, 4]):
    EXPECTED += [(f"P{_g}_{_f}", 8, 8) for _f in range(_n)]
EXPECTED += [("c", 10, 10), ("C", 8, 8), ("d", 19, 21), ("D", 32, 18)]

PNG_SIG = b"\x89PNG\r\n\x1a\n"


def fail(msg):
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def check_png(name, blob, w, h):
    if blob[:8] != PNG_SIG:
        fail(f"{name}: bad PNG signature")
    if blob[12:16] != b"IHDR":
        fail(f"{name}: IHDR not first chunk")
    pw, ph = struct.unpack(">II", blob[16:24])
    if (pw, ph) != (w, h):
        fail(f"{name}: dimensions {pw}x{ph}, expected {w}x{h}")
    if blob[-12:] != b"\x00\x00\x00\x00IEND\xaeB`\x82":
        fail(f"{name}: does not end with IEND (length/offset wrong)")


def argb4444_to_rgba8(v):
    a, r, g, b = (v >> 12) & 15, (v >> 8) & 15, (v >> 4) & 15, v & 15
    return [r * 17, g * 17, b * 17, a * 17]


def parse_sprites(data, pos):
    sprites = []
    for name, ew, eh in EXPECTED:
        if pos + 9 > len(data):
            fail(f"{name}: record header runs past end of data")
        w, h = struct.unpack_from(">ii", data, pos)
        n = data[pos + 8]
        pos += 9
        if (w, h) != (ew, eh):
            fail(f"{name}: size {w}x{h}, expected {ew}x{eh} (offset {pos - 9})")
        if not 1 <= n <= 255:
            fail(f"{name}: palette count {n}")
        if pos + 2 * n > len(data):
            fail(f"{name}: palette runs past end")
        pal = list(struct.unpack_from(f">{n}H", data, pos))
        pos += 2 * n
        count = w * h
        if n <= 16:
            nbytes = (count + 1) // 2
            raw = data[pos:pos + nbytes]
            if len(raw) != nbytes:
                fail(f"{name}: pixel data truncated")
            # SnakeCanvas.a() reads the low nibble first, then shifts the same
            # byte right by 4 for the next pixel (high nibble second).
            idx = []
            for b in raw:
                idx += [b & 15, b >> 4]
            idx = idx[:count]
        else:
            nbytes = count
            raw = data[pos:pos + nbytes]
            if len(raw) != nbytes:
                fail(f"{name}: pixel data truncated")
            idx = list(raw)
        pos += nbytes
        if max(idx) >= n:
            fail(f"{name}: pixel index {max(idx)} >= palette count {n}")
        sprites.append({
            "name": name, "w": w, "h": h,
            "palette_argb4444": pal,
            "palette_rgba8": [argb4444_to_rgba8(p) for p in pal],
            "transparent": [i for i, p in enumerate(pal) if p == TRANSPARENT_4444 or (p >> 12) == 0],
            "pixels": idx,
        })
    return sprites, pos


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--d", type=Path, default=ROOT / "reference" / "d")
    ap.add_argument("--out", type=Path, default=ROOT / "build" / "assets")
    args = ap.parse_args()

    if not args.d.is_file():
        fail(f"input not found: {args.d}")
    data = args.d.read_bytes()
    if len(data) != D_SIZE:
        fail(f"d is {len(data)} bytes, expected {D_SIZE}")
    if sha(data) != D_SHA:
        fail("d SHA-256 does not match the frozen reference")

    # layout contiguity: mazes | PNGs | sprites
    pos = MAZE_OFF + MAZE_COUNT * MAZE_SIZE
    for name, off, ln, _w, _h in PNGS:
        if off != pos:
            fail(f"{name}: offset {off}, expected contiguous {pos}")
        pos += ln
    if pos != SPRITE_OFF:
        fail(f"PNG region ends at {pos}, sprites expected at {SPRITE_OFF}")

    out = args.out
    (out / "png").mkdir(parents=True, exist_ok=True)
    files = []

    mazes = data[MAZE_OFF:MAZE_OFF + MAZE_COUNT * MAZE_SIZE]
    for m in range(MAZE_COUNT):
        if any(b > 0x1F for b in mazes[m * MAZE_SIZE:(m + 1) * MAZE_SIZE]):
            fail(f"maze {m}: byte outside 0..0x1F")
    (out / "mazes.bin").write_bytes(mazes)
    files.append(out / "mazes.bin")

    for name, off, ln, w, h in PNGS:
        blob = data[off:off + ln]
        check_png(name, blob, w, h)
        p = out / "png" / f"{name}.png"
        p.write_bytes(blob)
        files.append(p)

    sprites, end = parse_sprites(data, SPRITE_OFF)
    if end != SPRITE_END:
        fail(f"sprite parser consumed up to {end}, expected exactly {SPRITE_END} "
             f"({SPRITE_END - end} trailing/missing bytes)")
    if len(sprites) != 61:
        fail(f"{len(sprites)} sprite records, expected 61")
    (out / "sprites.json").write_text(json.dumps(sprites, indent=1), encoding="utf-8")
    files.append(out / "sprites.json")

    lines = [f"{sha(data)}  INPUT:d"]
    lines += [f"{sha(f.read_bytes())}  {f.relative_to(out).as_posix()}" for f in files]
    (out / "manifest.sha256").write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(f"OK: {MAZE_COUNT} mazes, {len(PNGS)} PNGs, {len(sprites)} sprite records, "
          f"parser consumed exactly {end} bytes -> {out}")


if __name__ == "__main__":
    main()
