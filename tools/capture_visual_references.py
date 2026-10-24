#!/usr/bin/env python3
"""capture_visual_references.py — Generate visual reference catalog for Snake EX2 AVR port.

Produces reference/screens/ directory with:
  - Named, cropped extracts from the 8 extracted PNG assets
  - All 6 derived 21×23 maze renders using actual wall tiles
  - Snake/food/power-up sprite renders decoded from the d blob
  - A 128×160 composite layout mockup
  - IGDB screenshot copies with descriptive names

Usage:
  python3 tools/capture_visual_references.py
"""

import os
import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
REF = ROOT / "reference"
SCREENS = REF / "screens"

# ─── 4444 ARGB → RGB565 conversion ──────────────────────────────────────────

def argb4444_to_rgb565(val: int) -> tuple[int, int, int] | None:
    """Convert 16-bit ARGB 4444 to (R,G,B) 8-bit or None if transparent.

    -241 (0xFF0F) is the special transparent sentinel.
    Alpha nibble 0 also means transparent.
    """
    if val == -241 or val == 0xFF0F:
        return None
    alpha = (val >> 12) & 0xF
    if alpha == 0:
        return None
    r = ((val >> 8) & 0xF) * 17
    g = ((val >> 4) & 0xF) * 17
    b = (val & 0xF) * 17
    return (r, g, b)


# ─── PNG asset helpers ──────────────────────────────────────────────────────

def load_png_asset(name: str) -> Image.Image:
    """Load a PNG asset from reference/assets/. Converts palette to RGBA."""
    path = REF / "assets" / name
    img = Image.open(path)
    return img.convert("RGBA")


# ─── Sprite record decoder ──────────────────────────────────────────────────

def decode_sprite_record(data: bytes, offset: int) -> dict | None:
    """Decode one sprite record from the d blob. Returns dict or None on error."""
    try:
        pos = offset
        width = struct.unpack_from(">i", data, pos)[0]
        height = struct.unpack_from(">i", data, pos + 4)[0]
        palette_count = data[pos + 8]
        pos += 9

        if width <= 0 or height <= 0 or palette_count <= 0:
            return None

        # Read palette
        palette = []
        for _ in range(palette_count):
            val = struct.unpack_from(">h", data, pos)[0]
            palette.append(val)
            pos += 2

        pixels = width * height
        if palette_count <= 16:
            pixel_bytes = (pixels + 1) // 2  # nibble-packed
        else:
            pixel_bytes = pixels

        # Read indices
        indices = []
        if palette_count <= 16:
            for i in range(pixel_bytes):
                b = data[pos + i]
                hi = (b >> 4) & 0xF
                lo = b & 0xF
                indices.append(hi)
                if len(indices) < pixels:
                    indices.append(lo)
        else:
            for i in range(pixel_bytes):
                indices.append(data[pos + i])

        # Convert to RGBA image
        img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
        px = img.load()
        for i, idx in enumerate(indices):
            if idx >= palette_count:
                continue
            y = i // width
            x = i % width
            if y >= height:
                break
            color = argb4444_to_rgb565(palette[idx])
            if color is not None:
                px[x, y] = (*color, 255)

        return {
            "width": width,
            "height": height,
            "palette_count": palette_count,
            "palette": palette,
            "image": img,
            "pixel_bytes": pixel_bytes,
            "total_bytes": 9 + palette_count * 2 + pixel_bytes,
        }
    except Exception as e:
        return None


def parse_all_sprites(d_path: Path) -> list[dict]:
    """Parse all 61 sprite records from the d blob."""
    data = d_path.read_bytes()
    SPRITE_OFFSET = 11620
    SPRITE_TOTAL = 3982

    sprites = []

    # 42 B-sprites
    # B0..B29: 6×6, B30..B35: 9×9, B36..B41: 11×11
    for i in range(42):
        if i < 30:
            sprites.append(("B", i, 6, 6))
        elif i < 36:
            sprites.append(("B", i, 9, 9))
        else:
            sprites.append(("B", i, 11, 11))

    # 15 power-up animation frames (8×8)
    for i in range(15):
        sprites.append(("powerup_frame", i, 8, 8))

    # c: 10×10, C: 8×8, d: 19×21, D: 32×18
    sprites.append(("c", 0, 10, 10))
    sprites.append(("C", 0, 8, 8))
    sprites.append(("d", 0, 19, 21))
    sprites.append(("D", 0, 32, 18))

    # Non-indexed sprite names: c, C, d, D
    SINGLE_NAMES = {"c", "C", "d", "D"}

    results = []
    pos = SPRITE_OFFSET
    for name, index, expected_w, expected_h in sprites:
        decoded = decode_sprite_record(data, pos)
        if decoded is None:
            print(f"  WARNING: failed to decode {name} at offset {pos}")
            break
        decoded["name"] = name if name in SINGLE_NAMES else f"{name}{index}"
        decoded["offset"] = pos
        results.append(decoded)
        pos += decoded["total_bytes"]

    return results


# ─── Maze renderer ──────────────────────────────────────────────────────────

def extract_derived_mazes(d_path: Path) -> list[list[int]]:
    """Return 6 derived 21×23 maze byte arrays."""
    data = d_path.read_bytes()
    COLS = 21
    INSERT_AFTER = 9
    INSERT_COUNT = 5

    mazes = []
    for mi in range(6):
        maze = list(data[mi * 378 : (mi + 1) * 378])
        is_box = (
            (maze[0] & 0x0F) == 7
            and (maze[-1] & 0x0F) == 0xA
        )

        new_maze = []
        for r in range(18):
            new_maze.extend(maze[r * COLS : (r + 1) * COLS])
            if r == INSERT_AFTER:
                for _ in range(INSERT_COUNT):
                    if is_box:
                        row = [0] * COLS
                        row[0] = 2
                        row[COLS - 1] = 2
                        new_maze.extend(row)
                    else:
                        new_maze.extend([0] * COLS)

        assert len(new_maze) == 483
        mazes.append(new_maze)
    return mazes


def build_wall_tile_map(wall_strip: Image.Image) -> list[Image.Image]:
    """Slice wall_strip.png (120×6) into 20 tiles of 6×6.

    Returns list of RGBA tile images indexed 0..19.
    Tile ID in maze = index + 1 (1-based in original code).
    """
    tiles = []
    for i in range(20):
        tile = wall_strip.crop((i * 6, 0, (i + 1) * 6, 6))
        tiles.append(tile)
    return tiles


def render_maze_image(maze_bytes: list[int], wall_tiles: list[Image.Image],
                      cell_size: int = 6) -> Image.Image:
    """Render a maze as a PIL image.

    maze_bytes: 483 bytes, low nibble = wall tile ID (0=empty, 1..15).
    wall_tiles: list of 6×6 tile images at index = (tile_id - 1).
    """
    COLS = 21
    ROWS = 23
    width = COLS * cell_size
    height = ROWS * cell_size  # 126×138

    img = Image.new("RGBA", (width, height), (32, 56, 32, 255))  # playfield bg
    for r in range(ROWS):
        for c in range(COLS):
            tile_id = maze_bytes[r * COLS + c] & 0x0F
            if tile_id == 0:
                continue
            idx = tile_id - 1
            if 0 <= idx < len(wall_tiles):
                tile = wall_tiles[idx]
                img.paste(tile, (c * cell_size, r * cell_size), tile)

    return img


# ─── 128×160 composite mockup ──────────────────────────────────────────────

def build_128x160_mockup(
    maze_img: Image.Image,
    hud_img: Image.Image,
    title_img: Image.Image | None = None,
) -> Image.Image:
    """Create a 128×160 composite with 1px borders, 21×23 playfield at (1,1),
    and HUD at (0,139)."""
    canvas = Image.new("RGBA", (128, 160), (0, 0, 0, 255))

    # Playfield at (1, 1) — 126×138
    canvas.paste(maze_img, (1, 1))

    # Top/bottom borders are already black (canvas background)
    # HUD at (0, 139) — 128×20. Ensure it's RGBA.
    hud_rgba = hud_img.convert("RGBA")
    canvas.paste(hud_rgba, (0, 139), hud_rgba)

    # 1px left/right border columns: x=0, x=127
    draw = ImageDraw.Draw(canvas)
    # Already black from background, but make sure
    for y in range(160):
        draw.point((0, y), (0, 0, 0, 255))
        draw.point((127, y), (0, 0, 0, 255))

    return canvas


# ─── Sprite sheet builders ──────────────────────────────────────────────────

def build_snake_sprite_sheet(sprites: list[dict]) -> Image.Image:
    """Build a labeled sprite sheet of all B0..B41 sprites.

    Layout: 7 columns, rows as needed, with labels.
    """
    b_sprites = [s for s in sprites if s["name"].startswith("B")]
    b_sprites.sort(key=lambda s: int(s["name"][1:]))

    cols = 7
    rows = (len(b_sprites) + cols - 1) // cols
    cell_w = 50
    cell_h = 50
    pad = 4

    sheet_w = cols * cell_w + pad
    sheet_h = rows * cell_h + pad
    sheet = Image.new("RGBA", (sheet_w, sheet_h), (60, 60, 60, 255))
    draw = ImageDraw.Draw(sheet)

    for i, spr in enumerate(b_sprites):
        col = i % cols
        row = i // cols
        x = pad + col * cell_w
        y = pad + row * cell_h

        # Center the sprite in the cell
        sw = spr["width"]
        sh = spr["height"]
        ox = x + (cell_w - pad - sw) // 2
        oy = y + (cell_h - pad - sh) // 2
        sheet.paste(spr["image"], (ox, oy), spr["image"])

        # Label
        draw.text((x + 2, y + 2), spr["name"], fill=(255, 255, 255, 255))
        draw.text((x + 2, y + 12), f"{sw}×{sh}", fill=(200, 200, 200, 255))

    return sheet


def build_powerup_spawn_sheet(
    sprites: list[dict], powerup_strip: Image.Image
) -> Image.Image:
    """Build a sheet showing power-up icons from the strip and their spawn
    item sprites (C sprites)."""
    # Extract individual icons from the strip
    icons = []
    for i in range(10):
        icon = powerup_strip.crop((i * 13, 0, (i + 1) * 13, 13))
        icons.append(icon)

    # C sprite (power-up pickup item, 8×8)
    c_sprites = [s for s in sprites if s["name"].startswith("C")]

    cols = 10
    rows = 3  # strip, C icons, labels
    cell_w = 24
    cell_h = 24

    sheet_w = cols * cell_w + 4
    sheet_h = rows * cell_h + 4
    sheet = Image.new("RGBA", (sheet_w, sheet_h), (60, 60, 60, 255))
    draw = ImageDraw.Draw(sheet)

    # Row 0: strip icons (13×13)
    for i in range(10):
        x = 6 + i * cell_w
        y = 6
        sheet.paste(icons[i], (x, y), icons[i])
        draw.text((x + 1, y + 14), f"#{i+1}", fill=(255, 255, 200, 255))

    # Row 1: C pickup sprite if available
    if c_sprites:
        cs = c_sprites[0]
        cx = 6 + cell_w * 5
        cy = 6 + cell_h
        sheet.paste(cs["image"], (cx, cy), cs["image"])
        draw.text((cx + 1, cy + 10), "C pickup", fill=(255, 255, 200, 255))

    return sheet


def build_body_part_reference(
    sprites: list[dict], maze_img: Image.Image
) -> Image.Image:
    """Build a reference sheet overlaying key body-part sprites on a maze
    background, so the alignment against walls is visible."""
    # Select only key reference sprites
    key_indices = {
        # Body turns
        6: "turn U→R",
        7: "turn U→L",
        8: "turn R→D",
        9: "turn R→U",
        # Tongue variants
        12: "tongue L",
        13: "tongue U",
        14: "tongue R",
        17: "tongue D",
        # Tail
        24: "tail R",
        25: "tail L",
        26: "tail U",
        29: "tail D",
        # Heads
        30: "head R",
        31: "head D",
        33: "head U",
        34: "head D2",
        # Eating heads
        36: "eat R",
        37: "eat D",
        39: "eat U",
        40: "eat D2",
    }

    b_sprites = {int(s["name"][1:]): s for s in sprites if s["name"].startswith("B")}

    cols = 5
    items = list(key_indices.items())
    rows = (len(items) + cols - 1) // cols
    cell_w = 50
    cell_h = 50

    sheet_w = cols * cell_w + 4
    sheet_h = rows * cell_h + 4
    sheet = Image.new("RGBA", (sheet_w, sheet_h), (60, 60, 60, 255))
    draw = ImageDraw.Draw(sheet)

    # Background: small maze patch behind each sprite
    bg_patch = maze_img.crop((0, 0, min(50, maze_img.width), min(50, maze_img.height)))

    for item_i, (idx, desc) in enumerate(items):
        col = item_i % cols
        row = item_i // cols
        x = 2 + col * cell_w
        y = 2 + row * cell_h

        # Background hint
        if bg_patch:
            sheet.paste(bg_patch.resize((cell_w - 4, cell_h - 4)), (x, y))

        if idx in b_sprites:
            spr = b_sprites[idx]
            sw, sh = spr["width"], spr["height"]
            ox = x + (cell_w - sw) // 2
            oy = y + (cell_h - sh) // 2
            sheet.paste(spr["image"], (ox, oy), spr["image"])

        draw.text((x + 2, y + 2), f"B{idx}", fill=(255, 255, 255, 255))
        draw.text((x + 2, y + 12), desc, fill=(200, 200, 200, 255))

    return sheet


# ─── Main ────────────────────────────────────────────────────────────────────

def main():
    print("Capturing visual references...")
    SCREENS.mkdir(parents=True, exist_ok=True)

    d_path = REF / "d"

    # ── 1. Load PNG assets ──
    print("  [1/5] Loading PNG assets...")
    img0 = load_png_asset("img0.png")  # title 128×128
    img1 = load_png_asset("img1.png")  # digits 77×9
    img2 = load_png_asset("img2.png")  # HUD 128×20
    img3 = load_png_asset("img3.png")  # g arrows 11×11
    img4 = load_png_asset("img4.png")  # G "Vs" 60×21
    img5 = load_png_asset("img5.png")  # power-up strip 130×13
    img6 = load_png_asset("img6.png")  # f tongue 11×11
    img7 = load_png_asset("img7.png")  # wall strip 120×6

    # ── 2. Save simple extracts ──
    print("  [2/5] Saving named extracts...")
    saves = [
        ("title_128x128.png", img0),
        ("digits_77x9.png", img1),
        ("hud_bar_128x20.png", img2),
        ("tutorial_arrows_11x11.png", img3),
        ("vs_label_60x21.png", img4),
        ("powerup_strip_130x13.png", img5),
        ("tongue_11x11.png", img6),
        ("wall_strip_120x6.png", img7),
    ]
    for name, img in saves:
        path = SCREENS / name
        if img.mode == "RGBA" and name.endswith(".png"):
            img.save(path)
        else:
            img.convert("RGBA").save(path)
        print(f"    {name}")

    # ── 3. Wall tiles and power-up icons ──
    print("  [3/5] Extracting wall tiles and power-up icons...")
    for i in range(20):
        tile = img7.crop((i * 6, 0, (i + 1) * 6, 6))
        tile.save(SCREENS / f"wall_tile_{i:02d}.png")
    print(f"    20 wall tiles saved")

    for i in range(10):
        icon = img5.crop((i * 13, 0, (i + 1) * 13, 13))
        icon.save(SCREENS / f"powerup_icon_{i + 1:02d}.png")
    print(f"    10 power-up icons saved")

    # ── 4. Maze renders ──
    print("  [4/5] Rendering derived 21×23 mazes...")
    wall_tiles = build_wall_tile_map(img7)
    mazes = extract_derived_mazes(d_path)
    maze_names = ["box", "tunnel", "spiral", "blockade", "twisted", "lshape_unused"]

    maze_imgs = []
    for i, (maze_bytes, name) in enumerate(zip(mazes, maze_names)):
        maze_img = render_maze_image(maze_bytes, wall_tiles)
        path = SCREENS / f"maze_{i}_{name}_21x23.png"
        maze_img.save(path)
        maze_imgs.append(maze_img)
        print(f"    maze_{i}_{name}_21x23.png  ({maze_img.size[0]}×{maze_img.size[1]})")

    # Use Box maze (maze 0) for the composite mockup
    mockup = build_128x160_mockup(maze_imgs[0], img2)
    mockup.save(SCREENS / "layout_128x160_mockup.png")
    print(f"    layout_128x160_mockup.png  (128×160)")

    # ── 5. Decode sprites and build sheets ──
    print("  [5/5] Decoding sprite records and building reference sheets...")
    try:
        sprites = parse_all_sprites(d_path)
        print(f"    Decoded {len(sprites)} sprite records from d blob")

        # B-sprite sheet
        snake_sheet = build_snake_sprite_sheet(sprites)
        snake_sheet.save(SCREENS / "sprite_sheet_B0_B41.png")
        print(f"    sprite_sheet_B0_B41.png")

        # Body part reference overlay
        body_ref = build_body_part_reference(sprites, maze_imgs[0])
        body_ref.save(SCREENS / "body_part_reference.png")
        print(f"    body_part_reference.png")

        # Power-up spawn sheet
        powerup_sheet = build_powerup_spawn_sheet(sprites, img5)
        powerup_sheet.save(SCREENS / "powerup_spawn_reference.png")
        print(f"    powerup_spawn_reference.png")

        # Save individual B-sprites that are key reference points
        key_sprites = {
            6: "body_turn_UR_DL",
            7: "body_turn_UL_DR",
            8: "body_turn_RD_LU",
            9: "body_turn_RU_LD",
            24: "tail_right",
            25: "tail_left",
            26: "tail_up",
            29: "tail_down",
            30: "head_right",
            31: "head_down",
            33: "head_up",
            34: "head_down2",
            36: "eating_head_right",
            37: "eating_head_down",
            39: "eating_head_up",
            40: "eating_head_down2",
        }
        for spr in sprites:
            if not spr["name"].startswith("B"):
                continue
            idx = int(spr["name"][1:])
            if idx in key_sprites:
                name = f"sprite_B{idx:02d}_{key_sprites[idx]}.png"
                fpath = SCREENS / name
                if os.path.exists(fpath):
                    os.remove(fpath)
                spr["image"].save(fpath)

        # Save c, C, d, D sprites (case-sensitive names for case-insensitive FS)
        # On Windows/WSL, sprite_c.png and sprite_C.png collide.
        # Use distinct suffixes: _small, _caps, _medium, _large
        sprite_special_names = {
            "c": "sprite_c_small.png",
            "C": "sprite_C_caps.png",
            "d": "sprite_d_medium.png",
            "D": "sprite_D_large.png",
        }
        for spr in sprites:
            name = spr["name"]
            if name in sprite_special_names:
                fname = sprite_special_names[name]
                fpath = SCREENS / fname
                if os.path.exists(fpath):
                    os.remove(fpath)
                spr["image"].save(fpath)
                print(f"    {fname}")

    except Exception as e:
        print(f"    ERROR decoding sprites: {e}")

    # ── Copy IGDB screenshots with descriptive names ──
    print("  Copying IGDB screenshots...")
    import shutil
    screenshot_map = {
        "sc6mcj.jpg": "igdb_title_screen.jpg",
        "sc6mck.jpg": "igdb_gameplay_box.jpg",
        "sc6mcl.jpg": "igdb_gameplay_maze.jpg",
        "sc6mcm.jpg": "igdb_gameplay_action.jpg",
        "yt.jpg": "igdb_youtube_frame.jpg",
    }
    for src, dst in screenshot_map.items():
        src_path = REF / "screenshots" / src
        dst_path = SCREENS / dst
        if src_path.exists():
            shutil.copy2(src_path, dst_path)
            print(f"    {dst}")

    # ── Summary ──
    all_screens = sorted(p.name for p in SCREENS.glob("*"))
    print(f"\n  Total: {len(all_screens)} reference screen files in {SCREENS}/")
    print("  Done.")


if __name__ == "__main__":
    main()