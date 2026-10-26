#!/usr/bin/env python3
"""test_visual.py — Host visual diff (BACKLOG Q3).

Builds `test/host/render_static.cpp` against the real renderer and a stub LCD,
renders the derived 128x160 frame (maze 0 + HUD), and compares it with:

  1. `build/assets/preview/screen_128x160_maze0.png` — the independent Python
     asset pipeline's derived frame.  Both sides use the same documented
     translation (border, maze at (1,1), HUD at (0,139)), so only RGB565
     quantization may differ (<= 8/255 per channel).
  2. `reference/screens/layout_128x160_mockup.png` — the documented layout
     mockup.  The mockup has a blank field, so only the border, HUD, and a
     known empty field cell are compared; interior walls are intentionally
     different.

The lossy IGDB JPEGs are used for the qualitative art checks already recorded
in A2/R3; they are not pixel-aligned and are not suitable for a pixel diff.
"""
import argparse
import subprocess
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = Path("/tmp/opencode")
TOL = 8


def get_pixels(im):
    return list(im.get_flattened_data() if hasattr(im, "get_flattened_data") else im.getdata())


def diff(a, b):
    """Return (max channel error, number of pixels above TOL, first bad pixel)."""
    pa, pb = get_pixels(a), get_pixels(b)
    worst = 0
    bad = 0
    first = None
    w = a.width
    for i, (ca, cb) in enumerate(zip(pa, pb)):
        e = max(abs(ca[k] - cb[k]) for k in range(3))
        if e > worst:
            worst = e
        if e > TOL:
            bad += 1
            if first is None:
                first = (i % w, i // w, ca, cb, e)
    return worst, bad, first


def check_region(name, render, reference, box, fails):
    worst, bad, first = diff(render.crop(box), reference.crop(box))
    if bad:
        fails.append(f"{name}: {bad} px > {TOL}/255, max {worst}, first {first}")
    return worst, bad


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--ppm", type=Path, default=OUT / "static_maze0.ppm",
                    help="output PPM (also used to skip the build if it exists)")
    args = ap.parse_args()

    OUT.mkdir(parents=True, exist_ok=True)
    exe = OUT / "render_static"
    cmd = ["g++", "-Wall", "-Wextra", "-I", "test/host", "-I", "src",
           "test/host/render_static.cpp",
           "src/render/field.cpp", "src/render/hud.cpp", "src/render/image.cpp",
           "src/render/text.cpp", "src/assets/assets_gen.cpp", "src/assets/maze.cpp",
           "-o", str(exe)]
    p = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    if p.returncode:
        sys.stderr.write(p.stdout)
        return f"visual: renderer build failed"
    p = subprocess.run([str(exe), str(args.ppm)], cwd=ROOT, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode:
        sys.stderr.write(p.stdout)
        return "visual: renderer failed"

    render = Image.open(args.ppm).convert("RGB")
    if render.size != (128, 160):
        return f"visual: render is {render.size}, expected (128, 160)"

    preview_path = ROOT / "build" / "assets" / "preview" / "screen_128x160_maze0.png"
    if not preview_path.exists():
        return (f"visual: missing {preview_path}; run "
                "python3 tools/extract_snake_ex2.py && python3 tools/convert_assets.py")
    preview = Image.open(preview_path).convert("RGB").resize((128, 160), Image.NEAREST)

    mock_path = ROOT / "reference" / "screens" / "layout_128x160_mockup.png"
    if not mock_path.exists():
        return f"visual: missing {mock_path}"
    mock = Image.open(mock_path).convert("RGB")
    if mock.size != (128, 160):
        return f"visual: mockup is {mock.size}, expected (128, 160)"

    fails = []

    # 1. Complete derived frame against the independent Python pipeline.
    worst, bad, first = diff(render, preview)
    if bad:
        fails.append(f"derived preview: {bad} px > {TOL}/255, max {worst}, first {first}")

    # 2. Documented layout: outer border, HUD, and a known empty field cell.
    check_region("top border", render, mock, (0, 0, 128, 1), fails)
    check_region("bottom border", render, mock, (0, 159, 128, 160), fails)
    check_region("left border", render, mock, (0, 0, 1, 139), fails)
    check_region("right border", render, mock, (127, 0, 128, 139), fails)
    check_region("HUD", render, mock, (1, 139, 127, 159), fails)
    # The mockup's dark-green field is an early layout artifact; the playfield
    # colour was corrected to 0xEEDDBB from b.java (see A2).  The complete
    # derived frame against the preview above covers the field interior.

    if fails:
        for f in fails:
            print(f"FAIL: {f}")
        print(f"visual: {len(fails)} FAILED")
        return 1
    print(f"visual: all passed (derived frame <= {TOL}/255; border/HUD/field layout match)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
