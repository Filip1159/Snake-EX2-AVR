#!/usr/bin/env python3
"""build_reference_index.py — Build reference index for Snake EX2 AVR port.

Extracts from the frozen reference set:
  1. All 94 language strings from lang.xx → reference/strings.txt
  2. Six original 21×18 maze maps + derived 21×23 maps → reference/mazes.txt
  3. Sprite inventory from the d blob → reference/sprite_inventory.txt
  4. Power-up ID table → reference/powerups.txt
  5. Combined reference index → reference/index.txt

Usage:
  python3 tools/build_reference_index.py
"""

import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REF = ROOT / "reference"

# ─── Lang.xx extraction ──────────────────────────────────────────────────────

def parse_lang_xx(path: Path) -> list[tuple[int, str]]:
    """Parse Nokia lang.xx file. Returns list of (id, string) tuples."""
    data = path.read_bytes()
    file_size = len(data)

    # First part: big-endian 16-bit offset table
    # There are 94 entries, each 2 bytes = 188 bytes table
    if file_size < 2:
        return []

    # Read until the first offset value to determine table size
    first_offset = struct.unpack_from(">H", data, 0)[0]
    # The number of entries is first_offset // 2
    if first_offset < 2 or first_offset > file_size:
        # Try reading entries until we find one pointing to itself
        num_entries = first_offset // 2
    else:
        num_entries = first_offset // 2

    # Read offsets
    offsets = []
    for i in range(num_entries):
        off = struct.unpack_from(">H", data, i * 2)[0]
        offsets.append(off)

    # Read strings: each starts with 2-byte big-endian length, then UTF-8 data
    strings = []
    for idx, off in enumerate(offsets):
        if off >= file_size:
            break
        try:
            strlen = struct.unpack_from(">H", data, off)[0]
            s = data[off + 2 : off + 2 + strlen].decode("utf-8", errors="replace")
            strings.append((idx, s))
        except Exception:
            strings.append((idx, f"[PARSE ERROR at offset {off}]"))

    return strings


# ─── Maze extraction from d blob ────────────────────────────────────────────

def extract_mazes(d_path: Path) -> list[list[int]]:
    """Extract 6 original 21×18 maze maps from the d blob.

    Each maze is 378 bytes, stored sequentially at offset 0.
    Returns list of 6 byte arrays.
    """
    data = d_path.read_bytes()
    if len(data) < 6 * 378:
        raise ValueError(f"d blob too small for mazes: {len(data)} bytes, need {6*378}")

    mazes = []
    for i in range(6):
        maze = list(data[i * 378 : (i + 1) * 378])
        mazes.append(maze)
    return mazes


def mazes_to_ascii(maze_bytes: list[int], cols: int = 21) -> str:
    """Convert maze byte array to ASCII art.

    Low nibble 0 = empty (.), high nibble ignored.
    1..15 = hex digit representing wall tile index (1→1, 10→A, 15→F).
    """
    rows = len(maze_bytes) // cols
    lines = []
    for r in range(rows):
        row = []
        for c in range(cols):
            val = maze_bytes[r * cols + c] & 0x0F
            if val == 0:
                row.append(".")
            elif val < 10:
                row.append(str(val))
            else:
                # 10→A, 11→B, 12→C, 13→D, 14→E, 15→F
                row.append(chr(ord("A") + val - 10))
        lines.append("".join(row))
    return "\n".join(lines)


def derive_23row_maze(maze_18: list[int], index: int) -> list[int]:
    """Derive 21×23 (483 cells) maze from original 21×18 (378 cells).

    Rule (§1.5): Insert 5 rows after local row 9 (0-based), shifting
    rows 10..17 down by five.

    - For Box (maze 0): inserted rows repeat the side-wall row 2...................2
    - For others: inserted rows are all-zero (blank).
    """
    COLS = 21
    ORIG_ROWS = 18
    INSERT_AFTER = 9   # 0-based row index after which to insert
    INSERT_COUNT = 5

    # Reshape original to 2D
    assert len(maze_18) == COLS * ORIG_ROWS

    # Only maze 0 (Box) has continuous side walls; Blockade (3) has the same corner
    # tiles but open side tunnels, so detection must be by index, not by shape.
    is_box = index == 0

    # Figure out the side-wall content for box insertion
    # Box row 1 is 2...................2 → side walls at col 0 and 20
    side_wall_row = [0] * COLS
    side_wall_row[0] = 2  # left wall
    side_wall_row[COLS - 1] = 2  # right wall

    # Build the new maze
    new_maze = []
    for r in range(ORIG_ROWS):
        new_maze.extend(maze_18[r * COLS : (r + 1) * COLS])
        if r == INSERT_AFTER:
            for _ in range(INSERT_COUNT):
                if is_box:
                    new_maze.extend(side_wall_row)
                else:
                    new_maze.extend([0] * COLS)

    assert len(new_maze) == COLS * (ORIG_ROWS + INSERT_COUNT)
    return new_maze


# ─── Sprite record parser ────────────────────────────────────────────────────

def parse_sprite_records(d_path: Path):
    """Parse 42 B-sprite records + 15 power-up frames + c,C,d,D from the d blob.

    Offset 11620, 3982 bytes total.
    Returns a dict with sprite inventory.
    """
    data = d_path.read_bytes()
    SPRITE_OFFSET = 11620
    SPRITE_TOTAL = 3982

    if len(data) < SPRITE_OFFSET + SPRITE_TOTAL:
        raise ValueError(f"d blob too small for sprite records: {len(data)} bytes")

    pos = SPRITE_OFFSET
    end = SPRITE_OFFSET + SPRITE_TOTAL

    # Sprite groups to parse
    groups = []
    # B0..B41: 42 sprites (B0..B29 = 6×6, B30..B35 = 9×9, B36..B41 = 11×11)
    for i in range(42):
        if i < 30:
            groups.append(("B", i, 6, 6))
        elif i < 36:
            groups.append(("B", i, 9, 9))
        else:
            groups.append(("B", i, 11, 11))

    # Power-up animation frames: 8 groups, frame counts 1,1,1,2,2,2,2,4 = 15 frames, 8×8
    for i in range(15):
        groups.append(("powerup_frame", i, 8, 8))

    # c: 10×10, C: 8×8, d: 19×21, D: 32×18
    groups.append(("c", 0, 10, 10))
    groups.append(("C", 0, 8, 8))
    groups.append(("d", 0, 19, 21))
    groups.append(("D", 0, 32, 18))

    inventory = []
    for name, index, expected_w, expected_h in groups:
        if pos >= end:
            break

        try:
            width = struct.unpack_from(">i", data, pos)[0]
            height = struct.unpack_from(">i", data, pos + 4)[0]
            palette_count = data[pos + 8]
            pos += 9

            palette = []
            for _ in range(palette_count):
                val = struct.unpack_from(">h", data, pos)[0]
                palette.append(val)
                pos += 2

            # Determine pixel data size
            pixels = width * height
            if palette_count <= 16:
                # nibble-packed
                pixel_bytes = (pixels + 1) // 2
            else:
                pixel_bytes = pixels

            pos += pixel_bytes  # skip pixel data

            entry = {
                "name": f"{name}{index}" if name.startswith("B") else name,
                "width": width,
                "height": height,
                "palette_count": palette_count,
                "palette": palette,
                "pixel_bytes": pixel_bytes,
            }
            inventory.append(entry)

        except Exception as e:
            inventory.append({
                "name": f"{name}{index}" if name.startswith("B") else name,
                "error": str(e),
                "offset": pos,
            })
            break

    return inventory


# ─── Power-up reference table ────────────────────────────────────────────────

POWERUP_TABLE = [
    {
        "id": 1,
        "name": "Speed Up",
        "sfx": 0,
        "description": "Movement cadence -= 2 frames/cell (clamped to >=1). The effect timer is checked once per movement step.",
        "duration": "25 + rand(50) movement steps (the field slot itself lives 25+rand(50) 2-frame ticks)",
        "code": "b.java: switch case 1; c() 1826, C() 1913",
        "icon_index": 0,
    },
    {
        "id": 2,
        "name": "Slow Down",
        "sfx": 1,
        "description": "Movement cadence += 2 frames/cell (clamped to <=10).",
        "duration": "25 + rand(50) movement steps",
        "code": "b.java: switch case 2; c() 1826, C() 1913",
        "icon_index": 1,
    },
    {
        "id": 3,
        "name": "Wall Ghost",
        "sfx": 2,
        "description": "Pass through maze walls; body self-collision still applies.",
        "duration": "25 + rand(50) movement steps",
        "code": "b.java: switch case 3; c() 1826, C() 1913",
        "icon_index": 2,
    },
    {
        "id": 4,
        "name": "Wall Stamp",
        "sfx": 3,
        "description": "Tries to stamp a random 3x3 wall pattern (one of 9 in ua[9][9]) at a random location. Success plays SFX 3; failure falls through to the speed-up effect (ID 1).",
        "duration": "25 + rand(50) movement steps; on failure becomes speed-up",
        "code": "b.java: switch case 4; c() 1826, B() 1750, C() 1913",
        "icon_index": 3,
    },
    {
        "id": 5,
        "name": "Bonus Boost",
        "sfx": 7,
        "description": "The next three bonus foods spawn with +24 on their 25-74 countdown (fa += 24).",
        "duration": "three bonus-food spawns",
        "code": "b.java: switch case 5; bonus spawn ~1590",
        "icon_index": 4,
    },
    {
        "id": 6,
        "name": "Shrink",
        "sfx": 4,
        "description": "Each movement step shortens the snake by one extra segment (in addition to the normal tail drop), never below length 3.",
        "duration": "three movement steps",
        "code": "b.java: switch case 6; movement ~1390",
        "icon_index": 5,
    },
    {
        "id": 7,
        "name": "Grow",
        "sfx": 5,
        "description": "Each movement step grows the snake by one extra segment (ka=true).",
        "duration": "three movement steps",
        "code": "b.java: switch case 7; movement ~1405",
        "icon_index": 6,
    },
    {
        "id": 8,
        "name": "Coin Rush",
        "sfx": 8,
        "description": "One odd-indexed empty map cell gets the 0x20 coin overlay per frame; head/tongue collect +1 each. Clears power-up slots, bonus and normal food. After the add scan it removes coins one per frame, then restores normal food. Normal food/portal suppressed while active.",
        "duration": "until the add+remove scan finishes (~2 x field cells frames)",
        "code": "b.java: switch case 8; scan ~1455, C() 1913",
        "icon_index": 7,
    },
    {
        "id": 9,
        "name": "Moving Food",
        "sfx": 6,
        "description": "Moves the NORMAL food to a new cell every Ea frames: Ea = (15+rand%15)*((8-speedIndex>>1)+1). The HUD bonus digits show Ea. Expires after five normal foods are eaten.",
        "duration": "five normal-food pickups",
        "code": "b.java: switch case 9; relocation ~1720",
        "icon_index": 8,
    },
    {
        "id": 10,
        "name": "Lights Out",
        "sfx": 9,
        "description": "Packed countdown v=0x02020200+(10+rand%10); each byte expiry toggles the field background (four toggles per fill). While dark the base cells are black; the HUD stays visible.",
        "duration": "25 + rand(50) movement steps (the packed v countdown controls the toggles)",
        "code": "b.java: switch case 10; C() cleanup 1913, toggle ~1501",
        "icon_index": 9,
    },
]


# ─── Main output writers ──────────────────────────────────────────────────────

def write_strings(strings: list[tuple[int, str]], out_path: Path):
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("Snake EX2 Language Strings\n")
        f.write("=========================\n")
        f.write(f"Source: reference/lang.xx (94 entries, 0-indexed)\n")
        f.write(f"\n")
        for idx, s in strings:
            # Show non-printable chars as escapes for debugging
            escaped = repr(s)[1:-1] if s else "(empty)"
            f.write(f"  [{idx:2d}] {s}\n")


def write_mazes(mazes: list[list[int]], out_path: Path):
    maze_names = ["Box", "Tunnel", "Spiral", "Blockade", "Twisted", "L-shape (unused)"]

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("Snake EX2 Maze Maps\n")
        f.write("==================\n")
        f.write(f"Source: reference/d, offsets 0–2267 (6 × 378 bytes)\n")
        f.write(f"Legend: . = empty, 1..F = wall tile ID (1-based for original, low-nibble)\n")
        f.write(f"\n")

        for i, maze_bytes in enumerate(mazes):
            name = maze_names[i] if i < len(maze_names) else f"Maze {i}"
            derived = derive_23row_maze(maze_bytes, i)

            f.write(f"{'─' * 70}\n")
            f.write(f"Maze {i} — {name} (original 21×18)\n")
            f.write(f"{'─' * 70}\n")
            f.write(mazes_to_ascii(maze_bytes, 21))
            f.write(f"\n\n")
            f.write(f"{'─' * 70}\n")
            f.write(f"Maze {i} — {name} (derived 21×23, 128×160 target)\n")
            f.write(f"{'─' * 70}\n")
            f.write(mazes_to_ascii(derived, 21))
            f.write(f"\n\n")


def write_sprite_inventory(inventory: list[dict], out_path: Path):
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("Snake EX2 Sprite Inventory\n")
        f.write("==========================\n")
        f.write(f"Source: reference/d, offsets 11620–15601 (3982 bytes)\n")
        f.write(f"Record format: int32 width, int32 height, uint8 paletteCount,\n")
        f.write(f"               int16 palette[paletteCount], then pixel data.\n")
        f.write(f"Transparent color: -241 (0xFF0F) in 4444 ARGB.\n")
        f.write(f"\n")

        f.write(f"{'Name':<20} {'W×H':<12} {'Palette':<8} {'PxBytes':<10} Notes\n")
        f.write(f"{'─' * 20} {'─' * 12} {'─' * 8} {'─' * 10} {'─' * 40}\n")

        # Group B sprites
        for entry in inventory:
            name = entry["name"]
            if "error" in entry:
                f.write(f"{name:<20} ERROR at offset {entry['offset']}: {entry['error']}\n")
                continue
            w = entry["width"]
            h = entry["height"]
            pc = entry["palette_count"]
            pb = entry["pixel_bytes"]

            notes = ""
            if name.startswith("B"):
                idx = int(name[1:])
                if idx <= 29:
                    notes = "snake body 6×6"
                elif idx <= 35:
                    notes = "snake head 9×9 (9px = 6px body + 3px offset?)"
                elif idx <= 41:
                    notes = "eating head 11×11"
            elif name == "c":
                notes = "ghost effect overlay 10×10"
            elif name == "C":
                notes = "power-up pickup item 8×8"
            elif name == "d":
                notes = "death/effect sprite 19×21"
            elif name == "D":
                notes = "large effect sprite 32×18"

            f.write(f"{name:<20} {w}×{h:<9} {pc:<8} {pb:<10} {notes}\n")

        # B-sprite mapping reference
        f.write(f"\n\nB-Sprite Part Mapping (from b.java draw method)\n")
        f.write(f"{'─' * 60}\n")
        mapping = {
            "Tail": {24: "right", 25: "left", 26: "up", 29: "down", 27: "up cross", 28: "down cross"},
            "Body turns": {6: "up→right / down→left", 7: "up→left / down→right",
                          8: "right→down / left→up", 9: "right→up / left→down"},
            "Cross body": {3: "straight dir2", 4: "straight dir5", 10: "turn (6+cross)", 11: "turn (7+cross)"},
            "Tongue body": {12: "straight dir8", 13: "straight dir1", 14: "straight dir2",
                           17: "straight dir4", 18: "turn (6+tongue)", 19: "turn (7+tongue)",
                           20: "turn (8+tongue)", 21: "turn (9+tongue)"},
            "Cross tongue": {15: "straight (14+cross)", 16: "straight (17+cross)",
                            22: "turn (18+cross)", 23: "turn (19+cross)"},
            "Head": {30: "right", 31: "down", 32: "up+tongue", 33: "up", 34: "down", 35: "down+tongue"},
            "Eating head": {36: "right", 37: "down", 38: "up+tongue", 39: "up", 40: "down", 41: "down+tongue"},
        }
        for group, sprites in mapping.items():
            f.write(f"\n  {group}:\n")
            for idx, desc in sorted(sprites.items()):
                f.write(f"    B{idx:2d}  →  {desc}\n")


def write_powerups(out_path: Path):
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("Snake EX2 Power-Up Reference\n")
        f.write("============================\n")
        f.write(f"Source: b.java (power-up activation c(), expiry C(), spawn logic)\n")
        f.write(f"Icon strip: SnakeCanvas.E, 130×13, 10 icons × 13×13, strip offset index = (Aa-1)*13\n")
        f.write(f"Spawn: 4 slots (Ba[4][4]), Extreme mode only; each empty slot independently rolls (rand&0xF)==15 when FOOD IS EATEN; id=1+rand%10, slot life=25+rand%50 decremented every 2 frames. Spawning flashes the backlight (Snake.A(11)), not a sound.\n")
        f.write(f"Pickup: head or tongue cells (k,l)/(K,L); the original loop can collect multiple overlapping slots.\n")
        f.write(f"Expiry: activation timer ba is 25+rand%50, decremented once per movement step for types 1,2,3,4,10; types 5/6/7 use 3 events, 8 uses the scan, 9 uses five foods. Aa=15 is the one-frame erase/HUD-clear state.\n")
        f.write(f"\n")

        for pu in POWERUP_TABLE:
            f.write(f"{'─' * 70}\n")
            f.write(f"  ID {pu['id']} — {pu['name']}  (SFX #{pu['sfx']})\n")
            f.write(f"{'─' * 70}\n")
            f.write(f"    Description : {pu['description']}\n")
            f.write(f"    Duration    : {pu['duration']}\n")
            f.write(f"    Icon index  : {pu['icon_index']} (x={(pu['icon_index'])*13} in E strip)\n")
            f.write(f"    Code        : {pu['code']}\n")
            f.write(f"\n")

        f.write(f"\nPower-Up Icon Layout (SnakeCanvas.E, 130×13 strip):\n")
        f.write(f"  Icon strip source x = (Aa-1) * 13; HUD destination x = 110 fixed, y = 142 (target 128×160 layout)\n")
        f.write(f"  Original HUD destination x = 110 fixed, y = 111 (original 128×128 layout)\n")
        f.write(f"  Icons are 13×13, sequential from left to right: ID1 at offset 0, ID10 at offset 117\n")


def main():
    print("Building reference index...")

    # 1. Language strings
    print("  [1/4] Extracting language strings from lang.xx...")
    strings = parse_lang_xx(REF / "lang.xx")
    out = REF / "strings.txt"
    write_strings(strings, out)
    print(f"         Wrote {len(strings)} strings to {out.name}")

    # 2. Maze maps  
    print("  [2/4] Extracting maze maps from d blob...")
    mazes = extract_mazes(REF / "d")
    out = REF / "mazes.txt"
    write_mazes(mazes, out)
    print(f"         Wrote {len(mazes)} mazes (original + derived) to {out.name}")

    # 3. Sprite inventory
    print("  [3/4] Parsing sprite records from d blob...")
    try:
        inventory = parse_sprite_records(REF / "d")
        out = REF / "sprite_inventory.txt"
        write_sprite_inventory(inventory, out)
        print(f"         Wrote {len(inventory)} sprite entries to {out.name}")
    except ValueError as e:
        print(f"         ERROR: {e}")

    # 4. Power-up reference
    print("  [4/4] Building power-up reference...")
    out = REF / "powerups.txt"
    write_powerups(out)
    print(f"         Wrote {len(POWERUP_TABLE)} power-up entries to {out.name}")

    # 5. Combined index
    print("  Writing combined index...")
    combined = REF / "index.txt"
    with open(combined, "w", encoding="utf-8") as f:
        f.write("Snake EX2 Reference Index\n")
        f.write("=========================\n")
        f.write(f"Generated: {__import__('datetime').datetime.now().isoformat()}\n")
        f.write(f"\n")
        f.write(f"Reference files:\n")
        f.write(f"  strings.txt         — All 94 language strings from lang.xx\n")
        f.write(f"  mazes.txt           — 6 original 21×18 + derived 21×23 maze maps\n")
        f.write(f"  sprite_inventory.txt — 61 sprite records (42 B + 15 power-up frames + c,C,d,D)\n")
        f.write(f"  powerups.txt        — 10 power-up types with effects and code references\n")
        f.write(f"\n")
        f.write(f"Source anchors:\n")
        f.write(f"  snakeex2.jar  SHA-256: a0a3300e7048c8e05d7b2169ec1225391574100270091f188e69abc947257e40\n")
        f.write(f"  d              SHA-256: 6eb869cceb6d8465dc45e9ce9a12771516056206ac2c95f83e90550db46174c8\n")
        f.write(f"  sounds.nok     SHA-256: 1e7078a155f7284c735b594dd56acd455d4ebd15e838d7b4bf072f868f4c17ed\n")
        f.write(f"  lang.xx        1426 bytes, 94 entries\n")
        f.write(f"  d              15602 bytes: 2268 mazes + 3616 title + 1280 digits + 1833 HUD\n")
        f.write(f"                               + 287 g + 403 G + 1440 powerup_strip + 184 f\n")
        f.write(f"                               + 309 wall_strip + 3982 sprite records\n")
        f.write(f"\n")
        f.write(f"Key constants (target 128×160):\n")
        f.write(f"  COLS=21, ROWS=23, CELLS=483\n")
        f.write(f"  CELL_SIZE=6, PLAYFIELD=(126,138) at (1,1)\n")
        f.write(f"  HUD at (0,139), BORDERS at y=0, y=159\n")
        f.write(f"  HEAD_START=(7,9)=pixel(42,54)=screen(43,55)\n")
        f.write(f"  FOOD_START=(4,4)=pixel(24,24)=screen(25,25)\n")
        f.write(f"  START_LENGTH=6, FACING=RIGHT\n")
        f.write(f"  FPS=15, FRAMES_PER_CELL=2+(8-speedIndex)\n")
        f.write(f"  WRAP: x∈[0,126), y∈[0,138)\n")

    print(f"         Wrote combined index to {combined.name}")
    print()
    print("Done. All reference index files written to reference/.")


if __name__ == "__main__":
    main()