#!/usr/bin/env python3
"""Generate the two Series 40 fonts: src/render/bigfont.inc and src/render/smallfont.inc.

Labels of the six reference screenshots (Nokia-GUI*.jpg, 128x160) are the model:

  big, bold   list items and the centre soft key   Video, Night mode on, Self-timer on,
              (cap height 11, stroke 2)             Img. sequence on, Effects, White balance,
                                                    Normal, False colours, Greyscale, Sepia,
                                                    Negative, Solarise, Auto, Daylight,
                                                    Tungsten, Fluorescent, Alarm clock,
                                                    Calendar, On, Off, Select, Menu
  small       titles, side soft keys, status text   Standard photo, Effects, White balance,
              (cap height 9, stroke 1)              Organiser, Stereo widening, Back, Go to,
                                                    Names, Silent, orange

Big font: the glyphs of those labels are cut out of the screenshots (text pixels are found
by colour, columns are split between glyphs, every line is aligned on its baseline, glyphs
seen more than once are majority-voted).  The characters the game needs but the screenshots
do not show (H M B T k w x z and the digits) are drawn here in the same geometry, as are "O" and "f",
whose cut-outs came out smeared or too thin; a few glyphs get single-pixel touch-ups (BIG_PATCH).
Small font: too small to cut out reliably, so it is drawn here from the screenshot metrics.

    python3 tools/gen_fonts.py [--preview out.png]     # needs numpy + opencv-python-headless
"""
import argparse
from pathlib import Path

import cv2
import numpy as np

ROOT = Path(__file__).resolve().parent.parent

# ── Big font ────────────────────────────────────────────────────────────────────
BIG_H = 15          # frame rows: ascender top 0, caps 1..11, baseline 11, descenders 12..14
BIG_BASE = 11

# (image suffix, list row, text, white text on the selection band?)
BIG_LINES = []
for img, rows in {"": ["Video", "Night mode on", "Self-timer on", "Img. sequence on", "Effects",
                       "White balance"],
                  "2": ["Normal", "False colours", "Greyscale", "Sepia", "Negative", "Solarise"],
                  "3": ["Auto", "Daylight", "Tungsten", "Fluorescent"],
                  "6": ["On", "Off"]}.items():
    for i, t in enumerate(rows):
        # list rows are 18 px apart, the first one starts at y=28
        BIG_LINES.append((img, 28 + 18 * i, 0, 118, t, i == 0))
BIG_LINES += [("5", 27, 34, 118, "Alarm clock", True), ("5", 63, 34, 118, "Calendar", False)]


def load(suffix):
    return cv2.imread(str(ROOT / "tools" / "img" / f"Nokia-GUI{suffix}.jpg")).astype(np.int32)


def ink_mask(im, light):
    r, g = im[..., 2], im[..., 1]
    return (r > 135) if light else ((r < 85) & (g < 120))


def runs(cols):
    out, s = [], None
    for x, v in enumerate(cols):
        if v and s is None:
            s = x
        if not v and s is not None:
            out.append((s, x))
            s = None
    if s is not None:
        out.append((s, len(cols)))
    return out


def extract_big():
    glyphs = {}
    for img, band_top, x0, x1, text, light in BIG_LINES:
        m = ink_mask(load(img), light)
        # align on the baseline: the lowest row that is still about as inky as the x-height rows
        region = m[band_top + 1:band_top + 19, x0:x1]
        prof = region.sum(1)
        base = band_top + 1 + max(i for i, p in enumerate(prof) if p >= 0.5 * prof.max())
        top = base - BIG_BASE
        line = m[top:top + BIG_H, x0:x1]
        r = runs(line.any(0))
        chars = text.replace(" ", "")
        if len(r) != len(chars):
            continue            # touching letters: other lines supply these characters
        for ch, (a, b) in zip(chars, r):
            glyphs.setdefault(ch, []).append(line[:, a:b].astype(np.int32))

    out = {}
    for ch, bs in glyphs.items():
        by_w = {}
        for b in bs:
            by_w.setdefault(b.shape[1], []).append(b)
        w = max(by_w, key=lambda k: len(by_w[k]))
        votes = np.mean(by_w[w], axis=0)
        g = votes > 0.5 if len(by_w[w]) >= 3 else votes >= 0.5
        # drop JPEG specks: pixels without a 4-neighbour
        pad = np.pad(g, 1)
        nb = pad[:-2, 1:-1] | pad[2:, 1:-1] | pad[1:-1, :-2] | pad[1:-1, 2:]
        out[ch] = g & nb
    return out


def art(top, rows, h):
    """Rows of '#'/'.' placed at row `top` of an h-row frame -> boolean array."""
    w = len(rows[0])
    assert all(len(r) == w for r in rows), rows
    a = np.zeros((h, w), bool)
    for i, r in enumerate(rows):
        a[top + i] = [c == "#" for c in r]
    return a


# Touch-ups of cut-out glyphs: (char, row, new pixels of that row).
BIG_PATCH = [
    ("S", 12, "......."),     # three stray pixels below the baseline
    ("o", 11, ".#####."),     # stray pixel in the bottom-left corner
    ("N", 12, "........"),    # stray pixel below the baseline
    ("d", 0, ".....##"),      # missing top-right corner of the ascender
    ("d", 11, ".####.#"),     # bottom row as in "u"
]

# Characters the screenshots do not show, drawn in the big font's geometry (top row, rows).
BIG_EXTRA = {
    "O": (1, [".######."] + ["##....##"] * 9 + [".######."]),   # the cut-out one is smeared
    # the cut-out "f" is only 1 px thick; this is the one in "Effects" (Nokia-GUI.jpg)
    "f": (0, ["..###", ".##..", ".##..", "#####"] + [".##.."] * 8),
    "H": (1, ["##....##"] * 5 + ["########"] * 2 + ["##....##"] * 4),
    "M": (1, ["##.....##", "###...###", "###...###", "##.#.#.##", "##.#.#.##", "##..#..##",
              "##..#..##", "##.....##", "##.....##", "##.....##", "##.....##"]),
    "B": (1, ["######.", "##...##", "##...##", "##...##", "##...##", "######.", "##...##",
              "##...##", "##...##", "##...##", "######."]),
    "T": (1, ["######"] + ["..##.."] * 10),
    "k": (0, ["##.....", "##.....", "##.....", "##...##", "##..##.", "##.##..", "####...",
              "####...", "##.##..", "##..##.", "##...##", "##...##"]),
    "w": (3, ["##..##..##"] * 7 + [".########.", "..##..##.."]),
    "x": (3, ["##...##", "##...##", ".##.##.", "..###..", "..###..", "..###..", ".##.##.",
              "##...##", "##...##"]),
    "z": (3, ["#######", ".....##", "....##.", "...##..", "..##...", ".##....", "##.....",
              "##.....", "#######"]),
    "0": (1, [".#####.", "##...##", "##...##", "##...##", "##...##", "##...##", "##...##",
              "##...##", "##...##", "##...##", ".#####."]),
    "1": (1, ["..##", ".###", "####", "..##", "..##", "..##", "..##", "..##", "..##", "..##",
              "..##"]),
    "2": (1, [".#####.", "##...##", ".....##", ".....##", "....##.", "...##..", "..##...",
              ".##....", "##.....", "##.....", "#######"]),
    "3": (1, [".#####.", "##...##", ".....##", ".....##", "..####.", ".....##", ".....##",
              ".....##", ".....##", "##...##", ".#####."]),
    "4": (1, ["....##.", "...###.", "..####.", ".##.##.", "##..##.", "#######", "....##.",
              "....##.", "....##.", "....##.", "....##."]),
    "5": (1, ["#######", "##.....", "##.....", "######.", ".....##", ".....##", ".....##",
              ".....##", ".....##", "##...##", ".#####."]),
    "6": (1, [".#####.", "##.....", "##.....", "######.", "##...##", "##...##", "##...##",
              "##...##", "##...##", "##...##", ".#####."]),
    "7": (1, ["#######", ".....##", ".....##", "....##.", "....##.", "...##..", "...##..",
              "..##...", "..##...", "..##...", "..##..."]),
    "8": (1, [".#####.", "##...##", "##...##", "##...##", ".#####.", "##...##", "##...##",
              "##...##", "##...##", "##...##", ".#####."]),
    "9": (1, [".#####.", "##...##", "##...##", "##...##", ".######", ".....##", ".....##",
              ".....##", ".....##", "##...##", ".#####."]),
}

# ── Small font ──────────────────────────────────────────────────────────────────
SMALL_H = 13        # ascender top 0, caps 1..9, x-height 3..9, baseline 9, descenders 10..12

SMALL = {
    "A": (1, [".###.", "#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#", "#...#"]),
    "B": (1, ["####.", "#...#", "#...#", "#...#", "####.", "#...#", "#...#", "#...#", "####."]),
    "C": (1, [".####", "#....", "#....", "#....", "#....", "#....", "#....", "#....", ".####"]),
    "D": (1, ["####.", "#...#", "#...#", "#...#", "#...#", "#...#", "#...#", "#...#", "####."]),
    "E": (1, ["#####", "#....", "#....", "#....", "####.", "#....", "#....", "#....", "#####"]),
    "F": (1, ["#####", "#....", "#....", "#....", "####.", "#....", "#....", "#....", "#...."]),
    "G": (1, [".####", "#....", "#....", "#....", "#.###", "#...#", "#...#", "#...#", ".###."]),
    "H": (1, ["#...#"] * 4 + ["#####"] + ["#...#"] * 4),
    "I": (1, ["###"] + [".#."] * 7 + ["###"]),
    "J": (1, ["....#"] * 6 + ["#...#", "#...#", ".###."]),
    "K": (1, ["#...#", "#..#.", "#.#..", "##...", "#....", "##...", "#.#..", "#..#.", "#...#"]),
    "L": (1, ["#...."] * 8 + ["#####"]),
    "M": (1, ["#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#", "#...#", "#...#"]),
    "N": (1, ["#...#", "##..#", "##..#", "#.#.#", "#.#.#", "#..##", "#..##", "#...#", "#...#"]),
    "O": (1, [".###."] + ["#...#"] * 7 + [".###."]),
    "P": (1, ["####.", "#...#", "#...#", "#...#", "####.", "#....", "#....", "#....", "#...."]),
    "Q": (1, [".###.", "#...#", "#...#", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"]),
    "R": (1, ["####.", "#...#", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#", "#...#"]),
    "S": (1, [".####", "#....", "#....", ".###.", "....#", "....#", "....#", "....#", "####."]),
    "T": (1, ["#####"] + ["..#.."] * 8),
    "U": (1, ["#...#"] * 8 + [".###."]),
    "V": (1, ["#...#"] * 6 + [".#.#.", ".#.#.", "..#.."]),
    "W": (1, ["#...#"] * 6 + ["#.#.#", "#.#.#", ".#.#."]),
    "X": (1, ["#...#", "#...#", ".#.#.", ".#.#.", "..#..", ".#.#.", ".#.#.", "#...#", "#...#"]),
    "Y": (1, ["#...#", "#...#", ".#.#.", ".#.#.", "..#..", "..#..", "..#..", "..#..", "..#.."]),
    "Z": (1, ["#####", "....#", "....#", "...#.", "..#..", ".#...", "#....", "#....", "#####"]),
    "a": (3, [".##.", "...#", ".###", "#..#", "#..#", "#..#", ".###"]),
    "b": (0, ["#...", "#...", "#...", "###.", "#..#", "#..#", "#..#", "#..#", "#..#", "###."]),
    "c": (3, [".###", "#...", "#...", "#...", "#...", "#...", ".###"]),
    "d": (0, ["...#", "...#", "...#", ".###", "#..#", "#..#", "#..#", "#..#", "#..#", ".###"]),
    "e": (3, [".##.", "#..#", "#..#", "####", "#...", "#...", ".###"]),
    "f": (0, [".##", "#..", "#..", "###", "#..", "#..", "#..", "#..", "#..", "#.."]),
    "g": (3, [".###", "#..#", "#..#", "#..#", "#..#", "#..#", ".###", "...#", "...#", "###."]),
    "h": (0, ["#...", "#...", "#...", "###.", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#"]),
    "i": (1, ["#", ".", "#", "#", "#", "#", "#", "#", "#"]),
    "j": (1, [".#", "..", ".#", ".#", ".#", ".#", ".#", ".#", ".#", "##"]),
    "k": (0, ["#...", "#...", "#...", "#..#", "#.#.", "##..", "##..", "#.#.", "#..#", "#..#"]),
    "l": (0, ["#"] * 10),
    "m": (3, ["##.#.", "#.#.#", "#.#.#", "#.#.#", "#.#.#", "#.#.#", "#.#.#"]),
    "n": (3, ["###.", "#..#", "#..#", "#..#", "#..#", "#..#", "#..#"]),
    "o": (3, [".##.", "#..#", "#..#", "#..#", "#..#", "#..#", ".##."]),
    "p": (3, ["###.", "#..#", "#..#", "#..#", "#..#", "#..#", "###.", "#...", "#...", "#..."]),
    "q": (3, [".###", "#..#", "#..#", "#..#", "#..#", "#..#", ".###", "...#", "...#", "...#"]),
    "r": (3, ["#.#", "##.", "#..", "#..", "#..", "#..", "#.."]),
    "s": (3, [".###", "#...", "#...", ".##.", "...#", "...#", "###."]),
    "t": (1, [".#.", ".#.", "###", ".#.", ".#.", ".#.", ".#.", ".#.", ".##"]),
    "u": (3, ["#..#"] * 6 + [".###"]),
    "v": (3, ["#...#"] * 4 + [".#.#.", ".#.#.", "..#.."]),
    "w": (3, ["#...#"] * 4 + ["#.#.#", "#.#.#", ".#.#."]),
    "x": (3, ["#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"]),
    "y": (3, ["#..#"] * 6 + [".###", "...#", "...#", "###."]),
    "z": (3, ["####", "...#", "..#.", ".#..", "#...", "#...", "####"]),
    "0": (1, [".###.", "#...#", "#..##", "#.#.#", "#.#.#", "##..#", "#...#", "#...#", ".###."]),
    "1": (1, [".#.", "##.", ".#.", ".#.", ".#.", ".#.", ".#.", ".#.", "###"]),
    "2": (1, [".###.", "#...#", "....#", "....#", "...#.", "..#..", ".#...", "#....", "#####"]),
    "3": (1, [".###.", "#...#", "....#", "....#", "..##.", "....#", "....#", "#...#", ".###."]),
    "4": (1, ["...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#.", "...#.", "...#."]),
    "5": (1, ["#####", "#....", "#....", "####.", "....#", "....#", "....#", "#...#", ".###."]),
    "6": (1, [".###.", "#....", "#....", "####.", "#...#", "#...#", "#...#", "#...#", ".###."]),
    "7": (1, ["#####", "....#", "....#", "...#.", "...#.", "..#..", "..#..", "..#..", "..#.."]),
    "8": (1, [".###.", "#...#", "#...#", "#...#", ".###.", "#...#", "#...#", "#...#", ".###."]),
    "9": (1, [".###.", "#...#", "#...#", "#...#", ".####", "....#", "....#", "....#", ".###."]),
    ".": (8, ["##", "##"]),
    ",": (8, ["##", "##", "#."]),
    "-": (5, ["###"]),
    ":": (4, ["#", ".", ".", ".", "#"]),
    "/": (1, ["...#", "...#", "..#.", "..#.", "..#.", ".#..", ".#..", "#...", "#..."]),
}


def pack(a):
    """One row of a boolean array -> bits, leftmost pixel in the most significant bit."""
    return [int("".join("1" if v else "0" for v in row), 2) for row in a]


def write_inc(name, h, chars, glyphs, row_type, comment):
    order = sorted(chars)
    ws = [glyphs[c].shape[1] for c in order]
    wide = max(ws) > 8
    lines = [f"/* GENERATED by tools/gen_fonts.py — do not edit. */",
             f"/* {comment} */",
             f"/* chars: {''.join(order)} */",
             f'static const char {name}_CHARS[] = "{"".join(order).replace(chr(92), chr(92) * 2)}";',
             f"static const uint8_t {name}_W[] = {{{', '.join(map(str, ws))}}};",
             f"static const {row_type} {name}_ROWS[][{h}] PROGMEM = {{"]
    for c in order:
        rows = pack(glyphs[c])
        fmt = "0x%04X" if wide else "0x%02X"
        lines.append(f"    /* {c} */ {{{', '.join(fmt % r for r in rows)}}},")
    lines.append("};")
    path = ROOT / "src" / "render" / f"{name.lower()}font.inc"
    path.write_text("\n".join(lines) + "\n")
    return order


def preview(big, small, path):
    def line(font, text, h, scale=3, ink=(120, 60, 10)):
        w = sum((font[c].shape[1] if c in font else 4) + 1 for c in text) + 1
        img = np.full((h, w, 3), (235, 215, 180), np.uint8)
        x = 1
        for c in text:
            if c not in font:
                x += 5
                continue
            g = font[c]
            for yy in range(g.shape[0]):
                for xx in range(g.shape[1]):
                    if g[yy, xx]:
                        img[yy, x + xx] = ink
            x += g.shape[1] + 1
        return cv2.resize(img, None, fx=scale, fy=scale, interpolation=cv2.INTER_NEAREST)
    texts = ["Video", "Night mode on", "Self-timer on", "Img. sequence on", "Effects", "White balance",
             "Normal", "False colours", "Greyscale", "Sepia", "Negative", "Solarise", "Auto",
             "Daylight", "Tungsten", "Fluorescent", "Alarm clock", "Calendar", "To-do list", "On",
             "Off", "Select", "Menu", "New game", "High scores", "Settings", "Instructions",
             "Continue", "Classic", "Extreme", "Mode", "Maze", "Speed", "Sound", "Save", "No maze",
             "Box", "Tunnel", "Spiral", "Blockade", "Twisted", "0123456789"]
    rows = [line(big, t, BIG_H) for t in texts]
    stext = ["Standard photo", "Effects", "White balance", "Organiser", "Stereo widening", "Back",
             "Go to", "Names", "Silent", "orange", "Created by Filip Wisniewski.",
             "https://github.com/Filip1159/", "Snake-EX2-AVR", "Snake EX2 High scores Classic",
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz", "0123456789 :./-,"]
    rows += [line(small, t, SMALL_H) for t in stext]
    wmax = max(r.shape[1] for r in rows)
    canvas = np.full((sum(r.shape[0] + 4 for r in rows), wmax, 3), 255, np.uint8)
    y = 0
    for r in rows:
        canvas[y:y + r.shape[0], :r.shape[1]] = r
        y += r.shape[0] + 4
    cv2.imwrite(path, canvas)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--preview", help="write a PNG showing sample labels in both fonts")
    args = ap.parse_args()

    big = extract_big()
    seen = "".join(sorted(big))
    for ch, (top, rows) in BIG_EXTRA.items():
        big[ch] = art(top, rows, BIG_H)
    for ch, row, pixels in BIG_PATCH:
        assert len(pixels) == big[ch].shape[1], (ch, pixels)
        big[ch][row] = [c == "#" for c in pixels]
    for ch, g in big.items():
        assert g.shape[0] == BIG_H
    small = {ch: art(top, rows, SMALL_H) for ch, (top, rows) in SMALL.items()}

    write_inc("BIG", BIG_H, big.keys(), big, "uint16_t", "bold list font, 15 rows, rows are bitmaps")
    order = write_inc("SMALL", SMALL_H, small.keys(), small, "uint8_t", "small font, 13 rows")
    print("big   cut from the screenshots:", "".join(c for c in seen if c not in BIG_EXTRA))
    print("big   drawn:", "".join(sorted(BIG_EXTRA)))
    print("small drawn:", "".join(order))
    if args.preview:
        preview(big, small, args.preview)


if __name__ == "__main__":
    main()
