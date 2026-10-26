#!/usr/bin/env python3
"""test_assets.py — Asset golden tests (BACKLOG Q2).

Regenerates the complete asset pipeline into a temporary directory and checks:
  * the frozen `reference/d` size/SHA and the original 6 x 378 maze bytes;
  * the documented 21x23 maze extension (5 inserted rows after row 9);
  * 61 original sprite records, their names/sizes/palette bounds, the four tongue
    rotations, and the 63-entry firmware sprite table;
  * the extractor's SHA-256 manifest and a CRC32/size manifest of every firmware
    asset (mazes, PNGs, RGB565 data, JSON metadata, and assets_gen.{h,cpp});
  * that regenerating produces byte-identical `assets_gen.{h,cpp}`.

Usage:
  python3 tools/test_assets.py            # verify against test/golden/
  python3 tools/test_assets.py --update   # refresh the golden manifests
"""
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOLS = ROOT / "tools"
GOLDEN = ROOT / "test" / "golden"
sys.dont_write_bytecode = True
sys.path.insert(0, str(TOOLS))
import extract_snake_ex2 as ex  # noqa: E402


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def crc32(path):
    return zlib.crc32(path.read_bytes()) & 0xFFFFFFFF


def derive_maze(m, index):
    """Must match convert_assets.derive_maze: insert 5 rows after row 9."""
    cols, rows = 21, 18
    out = []
    side = m[1 * cols:2 * cols]
    for r in range(rows):
        out += m[r * cols:(r + 1) * cols]
        if r == 9:
            for _ in range(5):
                out += side if index == 0 else [0] * cols
    assert len(out) == cols * 23
    return out


def expected_files():
    """Firmware assets whose CRC is frozen. Diagnostic previews are excluded."""
    names = [p[0] for p in ex.PNGS]
    files = [
        ("assets/mazes.bin", "assets"),
        ("assets/mazes_21x23.bin", "assets"),
        ("assets/sprites.json", "assets"),
        ("assets/sprites_rgb565.json", "assets"),
        ("assets/tongue.json", "assets"),
        ("src/assets/assets_gen.cpp", "gen"),
        ("src/assets/assets_gen.h", "gen"),
    ]
    files += [(f"assets/png/{n}.png", "assets") for n in names]
    files += [(f"assets/rgb565/{n}.rgb565", "assets") for n in names]
    files += [(f"assets/rgb565/{n}.json", "assets") for n in names]
    return sorted(files)


def run(cmd):
    p = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    if p.returncode:
        sys.stderr.write(p.stdout)
        sys.exit(f"ERROR: {' '.join(map(str, cmd))} failed")
    return p.stdout


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--update", action="store_true", help="refresh test/golden manifests")
    args = ap.parse_args()

    GOLDEN.mkdir(parents=True, exist_ok=True)
    (ROOT / "build").mkdir(exist_ok=True)
    tmp = Path(tempfile.mkdtemp(prefix="_asset_check_", dir=ROOT / "build"))
    try:
        assets = tmp / "assets"
        gen = tmp / "gen"
        run([sys.executable, str(TOOLS / "extract_snake_ex2.py"), "--out", str(assets)])
        run([sys.executable, str(TOOLS / "convert_assets.py"), "--assets", str(assets)])
        run([sys.executable, str(TOOLS / "gen_assets.py"), "--assets", str(assets),
             "--out", str(gen)])

        fails = []

        def check(cond, msg):
            if not cond:
                fails.append(msg)

        # ---- frozen input + original mazes ----
        d = (ROOT / "reference" / "d").read_bytes()
        check(len(d) == ex.D_SIZE, f"reference/d size {len(d)} != {ex.D_SIZE}")
        check(hashlib.sha256(d).hexdigest() == ex.D_SHA, "reference/d SHA-256 mismatch")
        mazes = (assets / "mazes.bin").read_bytes()
        check(mazes == d[:ex.MAZE_COUNT * ex.MAZE_SIZE],
              "mazes.bin does not match the frozen first 6 x 378 bytes of d")
        check(len(mazes) == ex.MAZE_COUNT * ex.MAZE_SIZE,
              f"mazes.bin is {len(mazes)} bytes, expected {ex.MAZE_COUNT * ex.MAZE_SIZE}")
        check(all(b <= 0x1F for b in mazes), "maze byte outside 0..0x1F")

        # ---- documented 21x23 extension ----
        orig = [list(mazes[i * ex.MAZE_SIZE:(i + 1) * ex.MAZE_SIZE])
                for i in range(ex.MAZE_COUNT)]
        derived = [derive_maze(m, i) for i, m in enumerate(orig)]
        want = bytes(sum(derived, []))
        got = (assets / "mazes_21x23.bin").read_bytes()
        check(len(got) == 6 * 483, f"mazes_21x23.bin is {len(got)} bytes, expected 2898")
        check(got == want, "derived 21x23 maze bytes do not match the documented extension")
        for i, (o, drow) in enumerate(zip(orig, derived)):
            check(drow[:10 * 21] == o[:10 * 21] and drow[15 * 21:] == o[10 * 21:],
                  f"maze {i}: original rows changed around the insertion")

        # ---- sprite records ----
        sprites = json.loads((assets / "sprites.json").read_text())
        check(len(sprites) == 61, f"sprites.json has {len(sprites)} records, expected 61")
        for s, (name, ew, eh) in zip(sprites, ex.EXPECTED):
            check(s["name"] == name and s["w"] == ew and s["h"] == eh,
                  f"sprite {s.get('name')} size/name mismatch")
            n = len(s["palette_argb4444"])
            check(1 <= n <= 255, f"sprite {name}: palette {n}")
            check(len(s["pixels"]) == ew * eh, f"sprite {name}: pixel count")
            check(max(s["pixels"]) < n, f"sprite {name}: pixel index outside palette")
            check(all(0 <= t < n for t in s["transparent"]),
                  f"sprite {name}: invalid transparent index")
            if n <= 16:
                check(len(s["pixels"]) == ew * eh, f"sprite {name}: 4bpp count")

        rgb = json.loads((assets / "sprites_rgb565.json").read_text())
        check(len(rgb) == 61, f"sprites_rgb565.json has {len(rgb)} records, expected 61")
        for s, r in zip(sprites, rgb):
            check(s["name"] == r["name"] and s["w"] == r["w"] and s["h"] == r["h"],
                  f"RGB565 sprite {s['name']} metadata mismatch")
            check(len(r["palette_rgb565"]) == len(s["palette_argb4444"]),
                  f"RGB565 sprite {s['name']} palette size mismatch")
            check(r["transparent"] == sorted(r["transparent"]),
                  f"RGB565 sprite {s['name']} transparent indices are not sorted")
            check(all(0 <= t < len(r["palette_rgb565"]) for t in r["transparent"]),
                  f"RGB565 sprite {s['name']} invalid transparent index")

        tongue = json.loads((assets / "tongue.json").read_text())
        check(len(tongue) == 4, f"tongue.json has {len(tongue)} records, expected 4")
        for name, t in zip(("TONGUE_R", "TONGUE_U", "TONGUE_L", "TONGUE_D"), tongue):
            check(t["name"] == name and t["w"] == 11 and t["h"] == 11,
                  f"tongue {name} name/size mismatch")
            check(1 <= len(t["palette_argb4444"]) <= 16,
                  f"tongue {name}: palette {len(t['palette_argb4444'])}")
            check(t["transparent"] == [0] and len(t["pixels"]) == 121,
                  f"tongue {name}: transparency/pixel count")

        # ---- generated header constants ----
        hdr = (gen / "assets_gen.h").read_text()
        for token in ("#define WALL_PALETTE_SIZE 6", "#define MAZE_COUNT        5",
                      "#define MAZE_SRC_BYTES    189", "#define SPRITE_COUNT 63"):
            check(token in hdr, f"generated header missing {token!r}")

        # ---- byte-identical generated C sources ----
        for name in ("assets_gen.h", "assets_gen.cpp"):
            a = (gen / name).read_bytes()
            b = (ROOT / "src" / "assets" / name).read_bytes()
            check(a == b, f"regenerated {name} differs from the checked-in copy")

        # ---- extractor SHA-256 manifest ----
        manifest = (assets / "manifest.sha256").read_text()
        golden_manifest = GOLDEN / "assets_manifest.sha256"
        if args.update:
            golden_manifest.write_text(manifest)
        else:
            check(golden_manifest.exists(), "missing test/golden/assets_manifest.sha256")
            if golden_manifest.exists():
                check(manifest == golden_manifest.read_text(),
                      "regenerated extractor manifest differs from the golden copy")

        # ---- CRC32/size manifest of every firmware asset ----
        lines = []
        for logical, kind in expected_files():
            if kind == "assets":
                p = assets / logical[len("assets/"):]
            else:
                p = gen / Path(logical).name
            check(p.exists(), f"missing generated asset {logical}")
            if p.exists():
                lines.append(f"{crc32(p):08X}  {p.stat().st_size:7d}  {logical}")
        crc_text = "\n".join(lines) + "\n"
        golden_crc = GOLDEN / "assets_crc32.txt"
        if args.update:
            golden_crc.write_text(crc_text)
        else:
            check(golden_crc.exists(), "missing test/golden/assets_crc32.txt")
            if golden_crc.exists():
                check(crc_text == golden_crc.read_text(),
                      "regenerated asset CRC manifest differs from the golden copy")

        if fails:
            for f in fails:
                print(f"FAIL: {f}")
            print(f"assets: {len(fails)} FAILED")
            return 1
        print(f"assets: all passed ({len(expected_files())} firmware files, "
              f"61 sprite records, 63 firmware sprites, 6 derived mazes)")
        return 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
