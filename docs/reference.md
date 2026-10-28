# Snake EX2 Reference Set

Frozen on 2026-10-23 for the Snake-EX2-AVR project (now targeting ATmega644PA + ST7735 128×160 derived layout; MCU swap D11 on 2026-10-24, actual chip signature `0x1E960A`).

## Source

| Item | Value |
| --- | --- |
| Original JAR (source) | `https://javagames.cc/Games/snakeex2_m1osxrt4.jar` (obtained via javagames.cc; SHA-256 matches frozen copy) |
| Frozen JAR | `reference/snakeex2.jar` (43,396 bytes) |
| JAR SHA-256 | `a0a3300e7048c8e05d7b2169ec1225391574100270091f188e69abc947257e40` |
| MIDlet | `Snake EX2`, version `1.19`, vendor `Nokia`, category `Game`, profile `MIDP-1.0`, config `CLDC-1.0` |
| Icon | `/icons/snicon.png` |
| Main class | `com.nokia.mid.appl.snake.Snake` |

See `reference/MANIFEST.MF` for the full MIDlet manifest.

## Frozen layout

```
reference/
├── MANIFEST.MF        # MIDlet manifest (from JAR)
├── MANIFEST.sha256    # SHA-256 of all files under reference/ (excluding itself)
├── d                  # Asset blob (15,602 bytes)
├── sounds.nok         # Tone data (479 bytes)
├── snakeex2.jar       # Original JAR
├── icons/snicon.png   # App icon
├── lang.*             # Language files (ar, de, es-ES, fa, fr, ur, xx)
├── assets/            # Extracted PNGs from d (img0..img7)
├── decompiled/        # CFR 0.152 decompilation of all classes
├── screenshots/       # Reference screenshots (IGDB)
└── variants/          # Other JARs for audit/comparison
```

## Raw asset anchors

| File | Size (bytes) | SHA-256 |
| --- | --- | --- |
| `reference/snakeex2.jar` | 43396 | `a0a3300e7048c8e05d7b2169ec1225391574100270091f188e69abc947257e40` |
| `reference/d` | 15602 | `6eb869cceb6d8465dc45e9ce9a12771516056206ac2c95f83e90550db46174c8` |
| `reference/sounds.nok` | 479 | `1e7078a155f7284c735b594dd56acd455d4ebd15e838d7b4bf072f868f4c17ed` |
| `reference/MANIFEST.MF` | 310 | `68cebb96ee8a2d75819e341ebf387f42b401d81d2a83fd3e26424b55e3810ea2` |
| `reference/icons/snicon.png` | 220 | `a6f92929243a4367a00da1d38feade18e579b2c2d65b514e12c8e509c32f66c7` |

All language files and variant JARs are listed in `reference/MANIFEST.sha256`.

## Extracted assets

| File | Size (bytes) | SHA-256 |
| --- | --- | --- |
| `reference/assets/img0.png` | 3616 | `bff19e770fba6004d9dc177dc1d383cf7195888f5090ff70d689aab08951543d` |
| `reference/assets/img1.png` | 1280 | `c19e06c9579e4e8335af46f8b5d2dfcc6fabc75bb0001fd92440f8c115c969b6` |
| `reference/assets/img2.png` | 1833 | `e3199f57795eb007fa324b1496d1f6e9e1e3d29f6fedd71dc4a056b13634a2a2` |
| `reference/assets/img3.png` | 287 | `e77a64bbd5fbadc05658b57bad8caf04fb4d031312202a3e51db77e69c10e6e7` |
| `reference/assets/img4.png` | 403 | `275bc3343a6512612ea87cc2a13e4e12bb3ae23476de4a2c1fabc1b4fd69e8ce` |
| `reference/assets/img5.png` | 1440 | `4f8394b3d27eb4bd33a134fbc8ca08087e436619a37a5d7248c626a715e8da52` |
| `reference/assets/img6.png` | 184 | `cb155bfc398cb95e9d6b78726233e31cd0eba3591e6f6f2f227fcd1a21c96501` |
| `reference/assets/img7.png` | 309 | `a03a19e3629132d4874926c318159deb2893888183660f00b80192be1ac14c5f` |

## Screenshots (IGDB reference)

| File | Size (bytes) | SHA-256 |
| --- | --- | --- |
| `reference/screenshots/sc6mcj.jpg` | 11591 | `d1c54af895f636e67db7e070d2f4ab37f9564a7a0a67458cdb8ffd6a680663a9` |
| `reference/screenshots/sc6mck.jpg` | 8031 | `478c5327e8561ba36e9656268f332f3bf6716c1cf0c5840cbb44dcb215c17775` |
| `reference/screenshots/sc6mcl.jpg` | 18801 | `37700bc477dbda5aabca5212503880a84c0e8297cd0f325a6ea106b088c8c510` |
| `reference/screenshots/sc6mcm.jpg` | 14224 | `99084af6d603847904c2f6625645a371e4f251cb2aff7a45c091c649015e9147` |
| `reference/screenshots/yt.jpg` | 10290 | `f412e84020299c259bb4ff3da3b62adb0e6800ae569caedc58fb2e9547ab488c` |

## Variants (audit)

| File | Size (bytes) | SHA-256 | Notes |
| --- | --- | --- | --- |
| `reference/variants/Snake2EX.jar` | 43396 | `a0a3300e7048c8e05d7b2169ec1225391574100270091f188e69abc947257e40` | Byte-identical to original JAR |
| `reference/variants/phoneky_37152.jar` | 43396 | `a0a3300e7048c8e05d7b2169ec1225391574100270091f188e69abc947257e40` | Byte-identical to original (Phoneky `j4j37152`) |
| `reference/variants/phoneky_75679.jar` | 38961 | `d0eab4f421cbaf645127c495ecb4456a8fa431143f593c8ff97c5e5e3443056f` | Different build/repack (38,961 bytes) |
| `reference/variants/Snake3.jar` | 43565 | `2da6624f5bb6b81b27fc2339339c83c8ecefb121512af6d443f669ea32ac0606` | Different game (not Snake EX2) |

Conclusion: only `phoneky_75679.jar` is a different repack of Snake EX2; others are identical or a different title. No native 128×160 build of Snake EX2 exists in the collected set.

## Decompilation

- Tool: CFR 0.152
- CFR JAR (if available): `/tmp/opencode/cfr.jar` (SHA-256 `f686e8f3ded377d7bc87d216a90e9e9512df4156e75b06c655a16648ae8765b2`)
- Command (from JAR directory): `java -jar cfr.jar snakeex2.jar --outputdir decompiled` (exact flags used during extraction not fully recorded; CFR 0.152 was used)
- Decompiled sources live under `reference/decompiled/com/nokia/mid/appl/snake/` (see `reference/decompiled/summary.txt`).

Key engine files: `Snake.java`, `SnakeCanvas.java`, `b.java`, `a.java`, `c.java`, `GlobalDefs.java`, `ResourceDefs.java`, `SnakeTask.java`, `SnakeDevice.java`, `ToneDevice.java`.

## Original resource layout (from JAR)

- Asset blob: `d` (15,602 bytes). Contains sprites/tiles and possibly compressed/encoded graphics.
- Tones: `sounds.nok` (479 bytes).
- Languages: `lang.*` files.
- Icon: `icons/snicon.png`.
- Assets were extracted previously to `reference/assets/img0..img7.png`. Full decoding of `d` is handled by `tools/extract_snake_ex2.py` (A1).

## Original playfield/layout notes (for reference)

From decompilation/backlog analysis (original 128×128):
- Playfield: 21×18 cells (378 bytes/map). Target for derived 128×160: 21×23 cells (483 bytes/map).
- Cell size 6×6. Title art 128×128 at (0,16). HUD area at bottom in original 128×20.
- Derived 128×160 layout (see BACKLOG §1.5): 21×23 cells of 6×6 = 126×138 at (1,1), 1px borders; HUD shifted to (0,139), score at (39,139), bonus at (69,145), power-up icon y=142.

## Power-up semantics (G7, verified against b.java)

Source of truth: `b.java` `c()` (activate), `C()` (deactivate), `B()` (wall stamp), the food-eaten spawn block, the per-frame counter block, and the draw pass. `reference/powerups.txt` is generated from this same table by `tools/build_reference_index.py`.

| ID | Effect | Duration / counter | Icon |
| --- | --- | --- | --- |
| 1 | Speed up: cadence −2 frames/cell, clamp ≥1 | `ba=25+rand%50` movement steps | 0 |
| 2 | Slow down: cadence +2, clamp ≤10 | `ba=25+rand%50` movement steps | 1 |
| 3 | Wall ghost; body self-collision still kills | `ba=25+rand%50` movement steps | 2 |
| 4 | Wall stamp: `ua[9][9]` pattern at random 3×3; failure becomes type 1 | `ba=25+...`; on failure type 1 | 3 |
| 5 | Next three bonus foods get `+24` on their 25–74 countdown | 3 bonus spawns (`ba=3`) | 4 |
| 6 | Shrink one extra segment per step, floor length 3 | 3 movement steps | 5 |
| 7 | Grow one extra segment per step | 3 movement steps | 6 |
| 8 | Coin rush: one odd-indexed empty cell per **frame**; head/tongue collect +1. Clears slots/bonus/food; then removes coins one per frame and restores food | full add+remove scan (~2×cells frames) | 7 |
| 9 | Moves the **normal food** every `Ea=(15+rand%15)*((8-speed)>>1+1)` frames; HUD bonus digits show `Ea` | five normal foods | 8 |
| 10 | Lights out: `v=0x02020200+(10+rand%10)`; field toggles black on each byte expiry | `ba=25+rand%50` movement steps | 9 |

Details:
- Four slot records (`Ba[4][4]`), Extreme-only. Each empty slot independently rolls `(rand&0xF)==15` after a food is eaten; `id=1+rand%10`, slot `life=25+rand%50`, decremented every two frames. A slot at `life==0` becomes id 15; the draw pass clears id 15 (and the one-frame `Aa=15` marker) before the next engine tick.
- Pickup checks the head and tongue cells `(k,l)`/`(K,L)`; the original loops all four slots, so overlapping slots are collected in index order.
- The HUD draws the strip `SnakeCanvas.E` at `x=110-(Aa-1)*13` under a 13×13 clip, i.e. the selected icon always lands at `(110, 111)`; the derived build draws only that 13×13 icon at `(110, 142)`.
- Field sprites: slots use `SnakeCanvas.C` at `(x-1, y-1)`; coins and normal food use `SnakeCanvas.b[0][0]`.
- The spawn flash is `Snake.A(11)` (backlight), not `playSFX(11)` (which is the high-score sound in this port); the backlight is not modeled.
- The 21×23 derived field allows stamp y positions up to 19; the original 18-row check is `y <= 14`.
- **Rendering deviation:** the original sets `Ka` (full background redraw) when a wall stamp appears/expires and when coin rush starts/ends. The port instead repaints only the old/new 3×3 stamp cells and the per-frame coin/item cells, so there is no full-screen flash. Lights-out remains a genuine full-field repaint because every base cell changes colour. `test/host/test_playfield.cpp` asserts the no-full-refresh behaviour for these transitions.

Verification: `test/host/test_powerup.cpp` covers every effect/timer; `tools/oracle/build_oracle.py --pu-traces` generates 18 Extreme sandbox traces (power-up-seeking bot, no portals) and `test/host/test_powerup_oracle.cpp` compares 45,642 ticks of base + G7 state with 0 mismatches. The renderer's forced G7 layer cases are in `test/host/test_playfield.cpp`.

## Extreme campaign semantics (G8, verified against `b.java`)

The real Extreme mode is an **endless** level progression; it has no finite stage script or ending. `Snake.J` is the instructions/tutorial sequence (positive entries are text pages, `-1..-7` are scripted demo games offered from those pages, and `-8` is the standalone high-score demo). Those demos are handled with the tutorial work (U5), not as campaign progression.

State fields: level `x` (starts 1), level-start score `z`, portal state `Y` (0 none, 1 waiting, 2 shrinking), portal position `X,y` in field pixels, and maze `da`. The maze cycles `ca = {-1,0,1,2,3,4}`; `da = ca[(x-1) % 6]`.

Per tick, at the normal `c==0` frame:
1. If `Aa != 8` (not coin rush), `Y == 0`, and `score > z + speed*10 + level*20`, set `Y=1` and place the portal with the normal cell spawner. The comparison is strict; equality does not spawn.
2. If `Y == 1` and the **head** (not tongue cells) is exactly on the portal, set `Y=2`, set `ma=1`, then call `C()` (the normal power-up deactivate hook). A speed power-up's `C()` restores `na`, overriding `ma=1`; other effects keep the shrink cadence.

While shrinking, each movement tick drops exactly one tail segment and returns before movement, food or collision processing. At length 2 the next movement tick clears `Y`, calls the level-reset routine and returns.

Level reset:
- `z = score`, `level++`, load the next maze (`-1` means no walls).
- Length 6 at `(42,54)`, direction right, food at `(24,24)`, slots cleared, `Aa=0`, `ba=0`, `fa=0`, coins cleared, base `ma/La/Ma/Na`, full redraw.
- Original `a()` deliberately leaves `ga/Ga` (last bonus position), `Fa/ha/Ia` (bonus type/direction/frame), `Ea` (moving-food timer), `U/v` (lights state), `na` and `ka` (grow) untouched. The port preserves the same fields.

Implementation notes:
- Portal spawn uses `a()`'s exact retry/fallback semantics, including its cell-vs-pixel checks. In particular the duplicate portal check is `cell_col == X && cell_col == y`, and the food/slot/bonus comparisons also mix cell and pixel units. These quirks are intentional.
- Moving bonus food treats the portal as an obstacle (`b.java:1690`), so it bounces instead of crossing it.
- Wall-stamp placement checks the portal using the same cell-vs-pixel quirk.
- Coin rush (`Aa == 8`) suppresses portal spawning. Existing portals remain rendered while the snake shrinks.
- The original decrements the general power-up timer `ba` whenever `Aa` is not 5–9, **including `Aa == 0`**. This matters after an expired slot briefly sets `Aa=15` and the HUD pass clears it; the port matches that countdown.
- Rendering uses the 10×10 `SnakeCanvas.c` sprite at screen `(X-1, y-2)` (field origin `+1`, so `cell - 2`). Portal appearance and entry repaint only dirty cells; entry (`Y=1→2`) hides the head (original `!W || Y != 2 || n7 != A-1`) so the snake appears to enter the portal, and the renderer repaints the head cell locally. The renderer also supports local removal, but in the engine the portal only disappears at a level reset, which full-repaints. Level reset is the only full-field repaint.
- The derived 21×23 field keeps the original 2-cell margins; portal spawn rows are therefore `2..FIELD_ROWS-3` and the cell spawner uses `FIELD_ROWS-4` for its random range.

Verification:
- `test/host/test_extreme.cpp`: threshold equality/just-above, spawn bounds/fallback/wall/coin rejection, coin-rush suppression, head-only entry, speed/ghost/lights/stamp deactivate ordering, one-segment-per-tick shrink, level reset, all six mazes, preserved fields, moving-bonus bounce, and Classic gating.
- `test/host/test_extreme_oracle.cpp` + `tools/oracle/build_oracle.py --campaign-traces`: 9 forced-campaign traces, **180,000 ticks**, comparing base engine state, all snake segments, `x/z/Y/X/y`, maze and map bytes — 0 mismatches.
- The same parser also replays `--campaign-natural` (12 traces, **28,481 ticks**) generated with the power-up-seeking bot at `z=0`; these include natural portal spawns/entries, power-ups, shrinking and multiple level transitions — 0 mismatches.
- `test/host/test_playfield.cpp` compares portal appearance/move/entry/removal/edge/head-overlap against a fresh full redraw, checks that the head cell is portal-only during shrink, and verifies the incremental entry path erases the head; all renderer-level cases use local repaints.

Classic selection (G9):
- `game_init_classic()` accepts maze `-1` (No maze) or `0..4` (Box, Tunnel, Spiral,
  Blockade, Twisted) and speed `0..5`; it loads the derived 21×23 map into the caller's

  483-byte buffer and initialises a non-Extreme game. Invalid combinations fail before
  `game_t` is modified.
- `maze_name()` uses the original string-table names in PROGMEM (`strings.txt`
  67/58/69/68/57/70).
- `test/host/test_classic_select.cpp` checks all **6 selections (no maze + 5 mazes) × 6 speeds**: map bytes
  against `build/assets/mazes_21x23.bin`, no-maze tiles, cadence, score step, maze names
  and invalid input.
- `tools/oracle/build_oracle.py --classic-all` emits 36 original-engine traces; the
  classic replay reports **36 traces / 79,161 ticks, 0 mismatches**.
- Scope note: the original `c.java` draws nine speed-selector positions (`Snake.I=0..8`),
  while the project/backlog uses six (`0..5`). The engine formula already handles 0..8;
  D12 records six for U2 unless the selector is expanded.

Current ATmega644PA build: **RAM 1,938 / 4,096 B (47.3%), flash 48,946 / 65,536 B (74.7%, 16,590 B free)**.
The G9 `ATmega644PA_classic` preset environment was removed in U1; New game now starts from
saved EEPROM settings (default Classic/no maze/speed 2) through `game_init_classic()`.

Title/menu (U1):
- Cold boot calls `title_draw()` (original 128×128 art at y=16) and advances after 3 s
  idle or any button press. The surrounding bands use the S40 title-screen blue
  (`TITLE_BG_COLOR`); the 16 px band below the art contains two silver trophies and a
  centred `OPTIONS` button, approximating the supplied Nokia reference screenshot.
- `src/ui/menu.{h,cpp}` reconstructs the S40 `List` screen at 128×160: a 14 px
  decorative status bar (antenna, signal bars, battery, no-SIM mark, "Silent" and a static `18:52` clock, a transparent sprite extracted from `Nokia-GUI4.jpg` by `tools/gen_statusbar.py`), a 16 px
  `Snake EX2` title row with a dotted leader and list indicator `1`, four 20 px
  mixed-case rows (New game / High scores / Settings / Instructions) with dotted
  leaders, and a 20 px soft-key row with `Select`/`Exit`. The selected row is a
  full-width light-blue band with dark text, matching the reference screenshot.
  Up/Left selects the previous item, Down/Right the next (wrapping), Fire selects, and
  Pause returns to the title. With a saved game a "Continue" row is added on top. Selection moves repaint only the old and new rows;
  the full menu is drawn only on entry (or after returning from an info screen).
  The status-bar clock is decorative because the port has no RTC.
- `src/render/text.{h,cpp}` provides the two Series 40 fonts, generated by
  `tools/gen_fonts.py` from the labels of the six `Nokia-GUI*.jpg` screenshots: a proportional
  small font (`text_draw`/`text_width`, 13-row frame, A–Z a–z 0–9 and `. , - : /`; titles,
  side soft keys, credits) and the bold 15-row list font (`text_draw_big`; list rows and the
  centre soft key). Big-font glyphs of the screenshot labels are cut out of the images; the
  characters the screenshots lack (H M B T k w x z O, digits) and the whole small font are
  drawn in the tool in the same geometry (`--preview out.png` shows sample labels).
  `test/host/test_menu.cpp` verifies labels, wrapping, text metrics, lowercase glyphs,
  status/title/row/soft-key layout, row colours and that the selected row has no dotted
  leader. It also compares partial two-row navigation with a full redraw for adjacent and
  wrapping moves, and checks that navigation performs no `fill_screen()` call.
- `test/host/test_hud.cpp` verifies the title art and the new trophy/Options chrome.
- The old boot self-test is no longer run; its drivers were already hardware-verified and
  remain covered by the host tests.

Settings (U2):
- `src/ui/settings.{h,cpp}` draws a full 128×160 editor with four rows: Mode (Classic/
  Extreme), Maze (No maze / Box / Tunnel / Spiral / Blockade / Twisted), Speed (0–5),
  Sound (ON/OFF). Up/Down selects the row; Left/Right changes the value (toggles or
  cycles with clamping). Edits stay on screen only. The centred "Save" soft key (Fire) writes all
  of them to EEPROM via `eeprom_save_settings()` and returns to the main menu; "Back" (Pause)
  returns without saving. The screen shares the main menu's chrome (wallpaper, status bar, band).
  The maze list uses PROGMEM strings from `maze_name()`; a new `text_draw_p`/
  `text_width_p` pair in `render/text` reads them correctly without copying to RAM.
- `test/host/test_settings.cpp` verifies selection wrapping, value adjustments, clamps,
  maze cycling, and partial row repaint without `fill_screen()`.

High scores (U3):
- Navigation: main menu → "High scores" → a Classic/Extreme list (the main menu's list
  component with a "Back" soft key, `menu_open(MENU_LIST_SCORES)`) → the top-3 view.
  Pause/Fire in the view returns to the list; Pause in the list returns to the main menu.
- `src/ui/highscores.{h,cpp}` draws the view over the shared S40 chrome
  (`src/ui/chrome.{h,cpp}`: wallpaper, status bar, "Snake EX2" title row with the mode name
  at the right, "Back" soft key): three score frames with six-digit scores (dashes for
  empty slots) and the two medals beside the best. Classic uses EEPROM scores 0..2,
  Extreme 3..5 (`SCORES_PER_MODE` = 3; blocks at 32 and 48, CRCs at 44 and 60 — an older
  2-score layout fails its CRC and the table starts empty).
- Game over: when a game ends and the score beats the 3rd stored entry for that mode,
  it is saved straight away via `highscores_submit()` (no entry screen, no initials,
  like the original) and the lower scores are shifted down. The title screen
  then shows the final score, with two trophies for a new best.
- `test/host/test_highscores.cpp` verifies qualification logic, score formatting with
  dashes for empty slots, and full-screen rendering.

Saved game and game over (U4):
- There is no pause overlay. Back (Pause) during a game stores the game in EEPROM
  (`src/game/savegame.{h,cpp}`, offset 64: marker, `sizeof(game_t)`, payload length, CRC16,
  then `game_t` without its unused snake tail and the transient occupancy bitmap) and opens
  the main menu, where a "Continue" row now sits on top and is selected. A save is rejected
  if the marker, struct size or CRC differ (torn write, other firmware layout).
- Continue loads the snapshot (the maze buffer is reloaded from `game_t.maze`), consumes the
  save and redraws the board. The game stays frozen until Fire is pressed; that Fire is masked
  until it is released, so it does not shoot the tongue. Going Back again saves it anew.
  New game leaves an existing save alone; a game that ends is never saved.
- When a game ends, the title screen is shown with the final score (and two trophies for a
  new best) instead of a separate game-over screen.
- `test/host/test_savegame.cpp` covers the round trip, identical continued play, partial
  payloads, corruption and size-mismatch rejection; `test_menu.cpp` the "Continue" row.
- The Q5 stack readout of the debug build (`STACK_MEASURE`) is now drawn on the credits screen.

Credits (replaces the original tutorial):
- The main menu's "Instructions" item opens `src/ui/credits.{h,cpp}`: the menu wallpaper, status
  bar and title row, the author and repository link centred on the screen (small font, link
  wrapped at the slashes) and a "Back" soft key. Fire or Pause returns to the main menu.
  The tutorial pages and scripted demos were removed.
  `test/host/test_credits.cpp` checks the chrome and the centring.

## Reproduction (fresh checkout)

1. **Verify reference set**: `python3 tools/verify_reference.py` (checks `reference/MANIFEST.sha256`, file sizes/anchors, and embedded `d`/`sounds.nok` in `snakeex2.jar`).
2. **Extract raw JAR members** (regenerate `d`, `sounds.nok`, `lang.*`, `icons/snicon.png`, `MANIFEST.MF`): `python3 tools/extract_reference.py --jar reference/snakeex2.jar --out reference --force` (verifies JAR SHA and writes raw members).
3. **Decompile** (optional if already present): with CFR 0.152 available, `java -jar cfr.jar reference/snakeex2.jar --outputdir reference/decompiled` (may require adjusting paths/flags).
4. **Decode assets from `d`**: `python3 tools/extract_snake_ex2.py` (A1). This decodes sprites/tiles from the original `d` blob; the pre-extracted `assets/img0..img7.png` are provided for convenience.
5. **Capture screenshots**: obtain from original sources (IGDB) as done; stored in `reference/screenshots/`.
6. **Build reference index**: `python3 tools/build_reference_index.py` (generates `reference/strings.txt`, `mazes.txt`, `sprite_inventory.txt`, `powerups.txt`, `index.txt`).
7. **Capture visual references**: `python3 tools/capture_visual_references.py` (generates all files under `reference/screens/` — labeled extracts, maze renders, sprite sheets, and the 128×160 composite mockup).

## Verification

- Manifest of all reference files: `reference/MANIFEST.sha256` (generate/update with `cd reference && find . -type f ! -name MANIFEST.sha256 -print0 | sort -z | xargs -0 sha256sum > MANIFEST.sha256`).
- Automated check: `python3 tools/verify_reference.py` (exits non-zero on mismatch). The script verifies the manifest, key anchors, sizes, and that `d`/`sounds.nok` inside `snakeex2.jar` match the standalone copies.

## Visual reference catalog (`reference/screens/`)

Generated by `tools/capture_visual_references.py`. Naming convention:

| Category | Pattern | Count | Example |
|----------|---------|-------|---------|
| Title art | `title_128x128.png` | 1 | Original 128×128 title PNG |
| HUD bar | `hud_bar_128x20.png` | 1 | 128×20 score bar |
| Digits | `digits_77x9.png` | 1 | 11-glyph digit strip |
| Power-up strip | `powerup_strip_130x13.png` | 1 | 10 icons × 13×13 |
| Power-up icons | `powerup_icon_NN.png` | 10 | Individual 13×13 icons (01–10) |
| Wall strip | `wall_strip_120x6.png` | 1 | 20 tiles × 6×6 |
| Wall tiles | `wall_tile_NN.png` | 20 | Individual 6×6 wall tiles (00–19) |
| Maze renders | `maze_N_NAME_21x23.png` | 6 | 126×138 derived maze renders |
| Layout mockup | `layout_128x160_mockup.png` | 1 | Composite 128×160 with playfield, borders, HUD |
| B-sprite sheet | `sprite_sheet_B0_B41.png` | 1 | All 42 snake body sprites in grid |
| Body part ref | `body_part_reference.png` | 1 | Key body-part sprites over maze BG |
| Power-up spawn ref | `powerup_spawn_reference.png` | 1 | Icons + pickup item sprite |
| Key B-sprites | `sprite_B{idx}_{part}.png` | 14 | Individual head/tail/turn/eat sprites |
| Special sprites | `sprite_{name}_*.png` | 4 | c (10×10 ghost), C (8×8 pickup), d (19×21), D (32×18) |
| IGDB screenshots | `igdb_*.jpg` | 5 | Named copies of original IGDB screenshots |
| Misc extracts | `tongue_11x11.png`, `tutorial_arrows_11x11.png`, `vs_label_60x21.png` | 3 | Ancillary original art |

**Total: 73 files.** Regenerate with `python3 tools/capture_visual_references.py`.

## Legal note

The reference materials in `reference/` are derived from the original Nokia Snake EX2 MIDlet (copyright Nokia/author). They are included here for personal preservation and faithful reproduction on this project only. `reference/` is git-ignored. Do not redistribute these assets.
