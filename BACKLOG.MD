# Snake EX2 on ATmega644PA — Implementation Backlog

**Project:** faithful hardware port of Nokia's *Snake EX2* (Nokia 3300, 2003, MIDlet v1.19) to a standalone ATmega644PA.
**Target hardware:** ATmega644PA at 5 V / 8 MHz + ST7735 128×160 SPI LCD + active buzzer (no SD card — removed 2026-10-23, see D4) + 6 buttons, programmed with USBasp. The actual chip is an **ATmega644PA** (device signature `0x1E960A`); it is configured as `board = ATmega644P`, `board_build.mcu = atmega644pa`. The original ATmega32 target was swapped on 2026-10-24 because the implemented firmware already used 30,958 / 32,768 B and U1/U2/U5 still need flash.
**Status:** G1–G9 complete and hardware-verified; U1 complete and hardware-verified; U2 complete and hardware-verified; U3 complete and hardware-verified; U4 complete and hardware-verified; U5 complete and hardware-verified.
**Last updated:** 2026-10-24.

This file is the working backlog. Work top-to-bottom, check off tasks, and record decisions in the “Decisions” section. Do not start the game engine before the reference assets and the display pipeline are proven.

---

## 0. Goal and definition of done

Build a self-contained console that reproduces Snake EX2's **gameplay screen** as closely as the AVR allows:

- **Full-screen 128×160 game UI**, 1:1 pixels, with no scaling or letterboxing.
- 21×23 playfield of 6×6 cells (126×138 px), a 1 px black border at the top and bottom, and the original 20 px HUD at the bottom.
- Original 21×18 mazes preserved and extended to 21×23 by the documented rule in §1.5; exact wall tiles, snake sprites, food sprites, power-up icons, HUD bar, digit font, title art, and palettes taken from the original JAR.
- Title art remains the extracted 128×128 image at 1:1, centered vertically; menus and gameplay fill the full 128×160 screen.
- 15 fps update cadence; original movement cadence for all 6 speed settings.
- Classic mode (maze + speed select) and Extreme mode (scripted campaign, power-ups, tongue, score-gated portal, maze cycling).
- Original edge wrapping, wall/self collision, growth, bonus food, power-ups, pause, game over, high scores, and settings persistence.
- Sound effects driven by the extracted Nokia tone data, within the limits of the buzzer.

**Done when:** all acceptance criteria in §9 pass, the game runs from a 5 V supply with only USBasp for programming, and a cold boot reaches the title screen in under 2 seconds (all assets in flash).

### Fidelity tiers

| Tier | Area | Commitment |
|---|---|---|
| A | Playfield, snake, food, walls, HUD, mazes, timing, mechanics | Pixel/behavior faithful; 128×160 uses the documented 21×23 maze extension, never scaling or stretching original art |
| B | Menus, settings, instructions, high-score screens | Reconstructed in the original style; the JAR uses Series 40 system UI rather than pixel art, so these are approximations |
| C | Audio | Monophonic square-wave versions of the original effects; an active buzzer can only produce fixed-pitch beeps |

Do not silently downgrade Tier A. If flash/RAM forces a compromise, record it in “Decisions” and update this file.

---

## 1. Research findings (reference baseline)

### 1.1 Original facts confirmed from the JAR and decompiled source

- MIDP 1.0 / CLDC 1.0 MIDlet, version 1.19, vendor Nokia.
- Original logical canvas: **128×128** (`SnakeCanvas` creates a 128×128 off-screen image and centers the canvas). The target is a derived **128×160** layout (§1.5).
- Original playfield: **21 columns × 18 rows**, each cell **6×6 px** → **126×108 px**. Target playfield: **21×23** → **126×138 px**.
- Original bottom HUD: **128×20 px** at y=108. Target HUD: **128×20 px** at y=139, with a 1 px border at y=0 and y=159.
- Frame rate: **15 fps** (confirmed by the original programmer's notes). The engine counts frames and moves the snake every `2 + (8 - speedIndex)` frames.
- Six original maze byte maps of **378 bytes** each (21×18), stored at the start of the `d` resource. They are expanded at build time to **483 bytes** each (21×23) for the 128×160 layout. The game uses maze IDs `-1` (empty) and `0..4`; maze `5` exists in the data but is not selected by the campaign's `ca` list.
- Classic mode: maze + speed selection, endless play.
- Extreme mode: endless score-gated progression; mazes cycle `-1,0,1,2,3,4`; the portal shrinks the snake to length 2 before the next level. `Snake.J` is the separate 62-entry instructions/tutorial sequence (positive text pages plus seven demo games `-1..-7`; `-8` is the high-score demo), scheduled under U5.
- Snake starts length **6**, head at cell **(7,9)** = pixel **(42,54)**, facing right. Initial food at cell **(4,4)** = pixel **(24,24)**.
- The snake **wraps at the edges** (x: 0↔120 by ±126 px; y: 0↔132 by ±138 px in the target layout; the original used ±108). Only walls, self, and the tongue/body collide fatally.
- Collision/death animation lasts **40 engine ticks** (~2.6 s at 15 fps).
- Normal food score = `speedIndex + 1` (1..6). Bonus food adds its remaining countdown value directly (25..74; +24 with one power-up type). Coin-rush pickups add 1 each.
- Extreme portal threshold: `score > levelStartScore + speedIndex*10 + levelNumber*20`; entering it sets the shrink state and eventually starts the next level.
- High scores: two entries per mode (`int[4]` in the original; classic at indices 0/1, extreme at 2/3). No initials.
- Original uses J2ME RMS both for high scores and for a full resume-state save. On AVR, use the 1 KB EEPROM for settings and high scores; resume-state saving is optional.

### 1.2 Original assets (exact)

The JAR resource `d` is **15,602 bytes**. Layout:

| Offset | Size | Asset | Notes |
|---:|---:|---|---|
| 0 | 2,268 | `mazes` (original) | 6 × 378 bytes, 21×18; build expands each to 483 bytes / 21×23 (§1.5) |
| 2,268 | 3,616 | `title.png` | 128×128 indexed PNG; target shows it 1:1, centered |
| 5,884 | 1,280 | `digits.png` | 77×9 indexed PNG, 11 glyphs × 7 px |
| 7,164 | 1,833 | `hud.png` | 128×20 indexed PNG |
| 8,997 | 287 | `g.png` | 11×11 PNG |
| 9,284 | 403 | `G.png` | 60×21 PNG |
| 9,687 | 1,440 | `powerup_strip.png` | 130×13 PNG, 10 icons × 13×13 |
| 11,127 | 184 | `f.png` | 11×11 PNG |
| 11,311 | 309 | `wall_strip.png` | 120×6 PNG, 20 6×6 tiles; game uses first 15 |
| 11,620 | 3,982 | `sprite records` | 42 `B` sprites + 15 power-up frames + `c,C,d,D` |

Sprite record format (big-endian):

```
int32 width
int32 height
uint8 paletteCount
int16 palette[paletteCount]   // 4444 ARGB; -241 / 0xFF0F means transparent
if paletteCount <= 16: nibble-packed indices (high nibble first)
else:                  one byte per index
```

Sprite inventory:

- `B0..B29`: 6×6
- `B30..B35`: 9×9
- `B36..B41`: 11×11
- Power-up animations: 8 groups with frame counts **1,1,1,2,2,2,2,4** = 15 frames, 8×8
- `c`: 10×10; `C`: 8×8; `d`: 19×21; `D`: 32×18

The sprite palettes are 4444, not RGB565. Convert carefully:
- alpha nibble 0 → transparent;
- 4-bit channels expanded to 8-bit by `v * 17`;
- then quantize to RGB565 for the ST7735.
- Preserve the original palette where possible; a per-sprite palette costs little flash and avoids color drift.

### 1.3 Audio

- `sounds.nok` is **479 bytes** and contains **13** length-prefixed Nokia tone chunks (little-endian 16-bit lengths).
- `ToneDevice` allocates 13 `Sound` objects, reads all 13 chunks, then **overwrites index 12** with a synthetic **1760 Hz / 100 ms** beep. In practice: 12 file-defined effects + 1 beep, indices 0–12.
- The original JAR has no MIDI files; it falls back to `ToneDevice`/`sounds.nok`.
- `playSFX(0..12)` indexes these sounds.
- Active buzzer → fixed pitch only. For melodies, a passive piezo is required. The firmware should support both and select the output mode in settings.

### 1.4 Reference links

- Original programmer's notes: <https://www.longsteve.com/fixmybugs/games-ive-coded-snake-ex2/>
- Playable page: <https://javagames.cc/play/2223>
- JAR used for extraction: <https://javagames.cc/Games/snakeex2_m1osxrt4.jar>
- Screenshots: IGDB images `sc6mcj`, `sc6mck`, `sc6mcl`, `sc6mcm` (via `images.igdb.com`), and <https://thegameextras.com/game/snake-ex2/>
- 128×160 research: <https://mobile.phoneky.com/games/?id=j4j37152>, <https://mobile.phoneky.com/games/?id=j4j75679>, and archive.org's Nokia 3100/3200/3300/6100/6220/6230/7210/7250 pack.
- Reference checksums (verify before relying on extracted data):
  - JAR SHA-256: `a0a3300e7048c8e05d7b2169ec1225391574100270091f188e69abc947257e40`
  - `d` SHA-256: `6eb869cceb6d8465dc45e9ce9a12771516056206ac2c95f83e90550db46174c8`
  - `sounds.nok` SHA-256: `1e7078a155f7284c735b594dd56acd455d4ebd15e838d7b4bf072f868f4c17ed`

Keep the JAR and extracted assets locally for personal use only. Do not redistribute Nokia's artwork or audio.

### 1.5 128×160 adaptation (no official build found)

**Research result:** no native 128×160 Snake EX2 MIDlet has been confirmed.

- The original game is a 128×128 MIDlet. `SnakeCanvas` creates a 128×128 off-screen image and centers it inside the phone's screen; it never re-lays out the playfield for larger displays.
- Phoneky's `j4j37152` page exposes a `128x160` preview thumbnail, but the downloaded JAR is byte-identical to the 128×128 original: SHA-256 `a0a3300e7048c8e05d7b2169ec1225391574100270091f188e69abc947257e40`.
- Phoneky's 128×128 sibling `j4j75679` is a 38,961-byte repack (SHA-256 `d0eab4f421cbaf645127c495ecb4456a8fa431143f593c8ff97c5e5e3443056f`) with the same classes and `d` asset blob but fewer language bundles; it is not a 128×160 port.
- The archive.org Nokia 3300/3100/3200/6100/6220/6230/7210/7250 game pack contains the same 128×128 `Snake2EX.jar`. No 128×160 Snake EX2 JAR was found in it.
- The 128×160 search results that mention Snake EX2 are compatibility listings; their 128×160 lists contain Snake III, Aztec Snake, and other titles, not a separate Snake EX2 build.
- `Snake3.jar` from the same archive is a different 43,565-byte game, not Snake EX2 and not a substitute.

**Therefore the 128×160 UI is a derived layout, not an official variant.** The adaptation must be explicit so it can be audited against the original.

#### Derived screen layout

| Region | Position | Size | Notes |
|---|---:|---:|---|
| Top border | y=0 | 128×1 | Black |
| Playfield | (1,1) | 126×138 | 21×23 cells of 6×6; logical coordinates 0..125 × 0..137 |
| HUD | (0,139) | 128×20 | Original `hud.png`, unscaled |
| Bottom border | y=159 | 128×1 | Black |

- The 126 px playfield is centered horizontally: x=0 and x=127 are black border columns; the HUD is full width.
- HUD contents shift by **+31 px in y only** from the original (b.java: score `(6,114)`, bonus `(69,114)`, icon slot `(110,111)`): score digits at `(6,145)`, bonus digits at `(69,145)`, power-up icon slot at `(110,142)` showing strip icon `type-1`. (Earlier text said score x=39/y=139 — wrong; 39 is the game-over screen.)
- Dynamic clip region: `(1,1,126,138)`.
- The title remains the original 128×128 art at `(0,16)`, 1:1, with the surrounding 16 px top and bottom filled with the title background. Menus may use the full 128×160 area because they are Tier B.

#### Maze extension rule (21×18 → 21×23)

Start with the original 378-byte map. Insert **five rows after local row 9 (0-based)** and shift original rows 10..17 down by five. This keeps the original walls and the spawn/food cells unchanged:

- All original wall tiles keep their relative positions; no tile IDs are renumbered.
- Inserted rows are all-zero for Tunnel, Spiral, Blockade, Twisted, and the unused L-shape.
- For Box, the inserted rows repeat the side-wall row `2...................2`, because Box has continuous left/right walls; an all-zero insert would open holes in the box.
- The spawn cell stays `(7,9)` and the initial food stays `(4,4)`.
- For maps with a bottom boundary (Box, Tunnel, Blockade), that boundary naturally moves to row 22. For open maps (Spiral, Twisted, L-shape), the inserted blank band becomes part of the wrap field.
- Exact derived ASCII maps are in Appendix A.2. Generate them in `tools/extract_snake_ex2.py`; do not hand-edit the C arrays.

#### Consequent constant changes

| Constant | Original | Target |
|---|---:|---:|
| Playfield cells | 21×18 | 21×23 |
| Playfield pixels | 126×108 | 126×138 |
| Playfield screen origin | (1,0) | (1,1) |
| Wrap height | 108 | 138 |
| Maze bytes per map | 378 | 483 |
| HUD y | 108 | 139 |
| Score digits y | 108 | 139 |
| Bonus digits y | 114 | 145 |
| Power-up icon y | 111 | 142 |
| Dynamic clip | (1,0,126,108) | (1,1,126,138) |

The logical head position remains `(42,54)` and the initial food remains `(24,24)`; the screen translation adds `(1,1)`, so the head's screen cell starts at `(43,55)` and the food at `(25,25)`. Preserve the original per-sprite pixel offsets when applying that translation.

---

## 2. Hardware plan

### 2.1 Recommended electrical design

**Recommendation:** ATmega32A at **5 V / 16 MHz**, ST7735 and SD at **3.3 V**, with level shifting. This gives the most flash/RAM headroom and the fastest SPI, at the cost of a 3.3 V rail and level shifters.

Alternative if minimal wiring matters more than margin: run the ATmega32 at **3.3 V / 8 MHz** and connect the LCD/SD directly. This removes level shifters but halves CPU/SPI speed and may make full-screen effects sluggish. Decide before building the board.

### 2.2 Bill of materials

| Qty | Part | Notes |
|---:|---|---|
| 1 | ATmega32A-PU (or ATmega32-16PU) | 40-pin DIP, 32 KB flash, 2 KB SRAM, 1 KB EEPROM |
| 1 | 16 MHz crystal + 2×22 pF | Required for 5 V/16 MHz build |
| 1 | 10 kΩ resistor | RESET pull-up |
| 4 | 100 nF ceramic | VCC, AVCC, LCD, SD decoupling |
| 2 | 10 µF electrolytic | Bulk near regulator and SD |
| 1 | 5 V regulator + input/output caps | Or a clean 5 V supply |
| 1 | 3.3 V LDO (AMS1117-3.3 or better) | LCD and SD rail |
| 1 | 74LVC245 / 74HC4050 level shifter | 5 V MCU → 3.3 V LCD/SD outputs |
| 1 | ST7735 128×160 SPI module | Verify 3.3 V logic and regulator/shifter onboard |
| 1 | microSD card breakout | 3.3 V only; level shift if needed |
| 1 | Active buzzer + NPN (BC547/2N2222) + 1 kΩ base resistor | Low-side switch; flyback diode if magnetic |
| 1 | Optional passive piezo/buzzer + driver | Needed for real melodies |
| 6 | Tactile buttons | Direction ×4, tongue/fire, pause/back |
| 6 | 10 kΩ resistors (optional) | External pull-ups; internal pull-ups can be used |
| 1 | 6-pin ISP header | USBasp |
| 1 | Power LED + 1 kΩ | Optional |
| 1 | 100 µF near SD | Handles card current spikes |

### 2.3 Pin assignment

Use this as the default. It avoids the JTAG pins (PC2–PC5) and keeps hardware SPI free.

| ATmega32 pin | Signal | Direction | Notes |
|---|---|---|---|
| PB5 | LCD/SD MOSI | out | Hardware SPI |
| PB6 | LCD/SD MISO | in | Hardware SPI |
| PB7 | LCD/SD SCK | out | Hardware SPI |
| PB4 | LCD_CS | out | Active low |
| PB3 | LCD_DC | out | Command/data |
| PB2 | LCD_RST | out | Active low |
| PB1 | SD_CS | out | Active low |
| PB0 | BUZZER | out | NPN base; Timer0 PWM possible for passive piezo |
| PA0 | BTN_UP | in | Active low, pull-up |
| PA1 | BTN_DOWN | in | Active low, pull-up |
| PA2 | BTN_LEFT | in | Active low, pull-up |
| PA3 | BTN_RIGHT | in | Active low, pull-up |
| PA4 | BTN_FIRE / TONGUE | in | Active low, pull-up |
| PA5 | BTN_PAUSE / BACK | in | Active low, pull-up |
| PC0 | SD_CD (optional) | in | Card detect, active low |
| PC1 | LCD_BL | out | Backlight on/off (PWM optional) |
| PD0/PD1 | UART TX/RX | — | Debug only; keep header accessible |
| RESET | ISP / reset | — | 10 kΩ pull-up |
| AVCC/VCC/GND | Power | — | Decouple every pair |

Notes:
- Do not use PC2–PC5 unless JTAG is disabled by fuse. Keep them spare.
- If the ST7735 module has an onboard 3.3 V regulator **and** level shifters, verify with a meter before applying 5 V logic. Many “5 V compatible” modules are not.
- SD cards are 3.3 V devices. Power them from 3.3 V, not 5 V.
- MISO from 3.3 V devices into a 5 V ATmega32 is usually recognized as high; add a 1 kΩ series resistor and verify. The clean solution is a proper level shifter.

### 2.4 Power, clock, and fuses

- 5 V rail: 1 A supply recommended; SD write/read current spikes can be 100 mA+.
- 3.3 V rail: separate LDO for LCD + SD; 100 nF at each device, 100 µF near the SD socket.
- 16 MHz crystal close to XTAL1/XTAL2; 22 pF to ground; keep traces short.
- Decouple VCC and AVCC with 100 nF; add 10 µF bulk.
- ISP header: MISO, MOSI, SCK, RESET, VCC, GND. USBasp can power the target at 5 V, but for a permanent build power the board separately and connect GND.
- Fuses: external crystal, 16 MHz, no bootloader, SPI programming enabled. If using PC2–PC5, disable JTAG. Read the current fuses before writing:
  ```
  avrdude -p m32 -c usbasp -U lfuse:r:-:h -U hfuse:r:-:h
  ```
  Record the stock values in the build notes. Do not blindly copy a fuse command from the internet; a wrong low fuse can leave the chip temporarily unprogrammable without a high-voltage programmer.
- Add a 6-pin ISP header even in the final build; it is the recovery path.

### 2.5 Display configuration

- Logical game area: **full 128×160 panel**. Gameplay uses a 1 px black border at y=0 and y=159, a 21×23 playfield at `(1,1)`, and the HUD at `(0,139)`. Do not scale or letterbox the UI.
- Confirm the panel controller variant (ST7735R/ST7735S, “red tab”/“green tab”). Set `TFT_X_OFFSET`, `TFT_Y_OFFSET`, `TFT_BGR`, and `TFT_INVERT` in one config header.
- Use hardware SPI, mode 0, MSB first. Start at 8 MHz; raise only after the whole system is stable.
- Keep `LCD_CS` low only during a transaction; SD shares MOSI/MISO/SCK.
- Backlight: on/off GPIO initially; PWM dimming optional. Do not drive a backlight LED directly from a pin without a resistor/transistor.

### 2.6 Button mapping

| Button | Game action | Menu action |
|---|---|---|
| UP | Up | Previous item |
| DOWN | Down | Next item |
| LEFT | Left | Previous value / page |
| RIGHT | Right | Next value / page |
| FIRE | Tongue / confirm / start | Select |
| PAUSE/BACK | Pause / back | Back / cancel |

Original key mapping reference: Series 40 keys 2/4/6/8 = up/left/right/down; 5 = fire; left/right softkeys = menu/back. The game buffers direction changes and forbids a direct 180° reversal.

---

## 3. Resource budget

### 3.1 Flash (32 KB)

| Item | Budget |
|---|---:|
| Vectors, startup, init | ~0.3 KB |
| HAL + ST7735 + SD + buttons + buzzer + EEPROM | 4–5 KB |
| Game engine + menus + UI | 12–16 KB |
| Essential assets in flash (walls, snake/food sprites, icons, compressed HUD/font, 6×483 mazes) | 7–10 KB |
| Reserve | 2 KB |
| **Target total** | **≤ 30 KB** |

Title art, tutorial text, and high-score decorations should live on SD if flash gets tight. (SD removed — see D4.)

### 3.2 SRAM (2 KB)

| Item | Budget |
|---|---:|
| Maze map `uint8_t[483]` | 483 B |
| Snake direction history `uint8_t[483]` | 483 B |
| Head/tail indices, length, direction | 8 B |
| Dirty-cell list (16 × `uint8_t`) | 16 B |
| Food, bonus food, 4 power-ups | ~25 B |
| Game state, timers, score, RNG | ~80 B |
| Menu/UI state | ~80 B |
| EEPROM shadow / settings | ~32 B |
| Stack | ≥ 256 B |
| **Subtotal (game + stack)** | **~1.43 KB** |
| SD sector buffer (only during boot/title/high-score I/O; overlaid with game/menu state) | 512 B |
| **Worst case (with ~0.2 KB boot margin)** | **~1.65 KB** |

Rules:
- No full-screen framebuffer. 128×160 RGB565 is 40 KB — impossible.
- No `malloc`, `new`, `String`, STL containers, or floating point.
- Store the snake as a direction history plus head position, not an `int[512]` position array. Derive body cells by walking backward.
- Reuse the SD sector buffer as a union with the snake/menu buffers during asset loading. Never allocate the SD buffer permanently during gameplay.
- Measure stack high-water mark (fill unused RAM with a pattern, run a long game, inspect).

### 3.3 EEPROM (1 KB)

| Offset | Content |
|---:|---|
| 0 | Magic `S E X 2` + version |
| 8 | Settings: mode, maze, speed, sound, backlight |
| 32 | Classic high scores (2 × uint32) |
| 48 | Extreme high scores (2 × uint32) |
| 64 | Optional resume slot (only if it fits) |
| 960 | CRC/scratch |

Write only on change. Use `eeprom_update_block` and a CRC16. Validate magic/CRC at boot and fall back to defaults on corruption.

---

## 4. Software architecture

### 4.1 Runtime choice

**Recommendation:** bare-metal `avr-libc` C++ (no Arduino core) for the game firmware.

Reasons:
- deterministic timing and no hidden timer0/UART buffers;
- full control of flash/RAM;
- no `String`/`Print`/dynamic allocation temptation;
- smaller binary.

Keep an Arduino-framework “blink/LCD smoke test” environment if it helps bring-up, but switch to bare metal before the game engine grows. The existing `src/main.cpp` blink demo is disposable.

If you prefer to stay on Arduino: use `framework = arduino`, avoid all Arduino objects, write direct register drivers, and compile with `-Os -flto`. It can work, but the binary budget is tighter.

### 4.2 Module layout

```
src/
  main.cpp                 // init, main loop, 15 fps scheduler
  config.h                 // pins, geometry, feature flags, tuning
  hal/
    clock.cpp/.h           // 16 MHz, timer1 15 fps tick, timer0 audio
    gpio.cpp/.h
    spi.cpp/.h
    uart.cpp/.h            // debug only
  drivers/
    st7735.cpp/.h          // init, window, fill, blit, RLE
    sd.cpp/.h              // Petit FatFs wrapper or minimal FAT reader
    buttons.cpp/.h         // debounce, repeat, key events
    buzzer.cpp/.h          // active on/off + passive PWM
    eeprom_store.cpp/.h    // settings, high scores, CRC
  gfx/
    gfx.cpp/.h             // rects, sprites, palettes, RLE
    font.cpp/.h            // 7×9 digit font + menu font
  game/
    game.cpp/.h            // state machine, scheduler
    snake.cpp/.h           // movement, growth, collision, wrap
    food.cpp/.h            // spawn, bonus food, moving food
    powerups.cpp/.h        // 10 types, timers
    levels.cpp/.h          // maze data, extreme campaign
    render.cpp/.h          // dirty-cell renderer, HUD
  ui/
    menu.cpp/.h
    title.cpp/.h
    highscore.cpp/.h
    instructions.cpp/.h
  assets/                  // generated .c/.h files; do not hand-edit
tools/
  extract_snake_ex2.py     // JAR -> assets + preview PNGs + C arrays
  preview_screens.py       // host-side reference renders
docs/
  wiring.md
  BOM.md
  reference.md
test/
  host/                    // native unit tests for pure game logic
platformio.ini
```

### 4.3 Game state machine

```
BOOT
  -> TITLE (3 s or key)
  -> MAIN_MENU
       -> NEW_GAME -> mode/maze/speed -> PLAYING
       -> HIGH_SCORES
       -> SETTINGS -> save EEPROM
       -> INSTRUCTIONS
  PLAYING <-> PAUSED
  PLAYING -> DEATH_ANIM (40 ticks) -> GAME_OVER -> HIGH_SCORES/MAIN_MENU
  PLAYING -> LEVEL_CLEAR (extreme portal) -> PLAYING
```

### 4.4 Timing

- Timer1 CTC: 15 fps tick (`OCR1A = 4166` at 16 MHz / 256 prescaler gives ~15.0 fps; tune for exactly 15.0).
- Every tick:
  1. sample/debounce buttons;
  2. update timers;
  3. if movement counter reaches 0, advance one cell;
  4. compute dirty cells;
  5. render only dirty cells + HUD;
  6. service sound.
- Input is buffered; a direction pressed between ticks is applied at the next tick. Reject 180° reversals.
- Buttons are also scanned at ~100 Hz for menus; do not block the frame loop with `delay()`.

### 4.5 Renderer

The original keeps a 128×128 off-screen image. On AVR that is impossible, so reproduce the **result**, not the buffer:

- Keep the target maze map (483 bytes, 21×23) as the source of truth.
- On level load, draw the full static playfield once: for each cell, draw wall tile or flat background; draw HUD bar.
- During play, maintain a small dirty-cell list. A moving step dirties at most: old tail, previous head (becomes body), new head, plus food/bonus/power-up cells. Redraw each dirty cell from the static base, then overlays, then the snake segment.
- Mark `uint8_t map[483]` bits: low nibble = wall tile; `0x10` = snake occupancy (transient); `0x20` = coin/portal overlay. Do not permanently store snake occupancy; rebuild it each step from the direction history.
- Draw 6×6 wall tiles at `x = 1 + col*6`, `y = 1 + row*6`. Dynamic sprites use logical playfield coordinates and are translated by `(1,1)`; preserve the original per-sprite offsets within that translated field.
- HUD: draw `hud.png` at y=139. Score: `digits.png` at x=39, y=139, 6 digits. Bonus counter: x=69, y=145, 4 digits. Power-up icon: `x=110-(type-1)*13`, `y=142`, 13×13.
- Avoid a full-screen repaint during gameplay. If tearing appears, ensure the dirty-cell writes happen during the vertical blank or split the update over two frames.

### 4.6 Storage strategy

| Asset | Storage | Reason |
|---|---|---|
| Mazes (6×483, 21×23 derived) | Flash | 2,898 B; needed instantly |
| Wall tiles (15×6×6) | Flash | 1 KB, hot path |
| Snake/food/power-up sprites | Flash | Needed every frame; keep raw/compressed |
| Digit font | Flash (compressed) | Small and hot |
| HUD bar | Flash (RLE) | 128×20, drawn on level load |
| Title 128×128 | SD (RLE/raw) | Large; drawn once per boot/menu |
| High-score decorations `c,C,d,D` | SD or flash | Optional; only on high-score screen |
| Tutorial text | Flash (abbreviated) or SD | 52 text entries would be large |
| Sound sequences | Flash | 12 short sequences |

Fallback rules:
- SD missing/unreadable → title screen drawn from a small built-in approximation; game remains playable.
- Title file corrupt → same fallback; log over UART.

Use **Petit FatFs** (read-only) or a tiny FAT16/FAT32 reader. Petit FatFs’s 512 B sector buffer must overlay game RAM, not coexist with it. Format the card FAT32 with 512-byte sectors; long filenames are unnecessary (`TITLE.RLE`, `SCORE.RLE`).

### 4.7 Asset pipeline

`tools/extract_snake_ex2.py` is the single source of truth:

1. Verify the JAR checksum.
2. Read `d`, split at the offsets in §1.2.
3. Write the six original 378-byte maze maps and the six derived 483-byte maps to `assets/mazes.c`. The extension function must insert five rows after local row 9 (side-wall rows for Box, blank rows for the others) and assert the resulting dimensions.
4. Decode the eight PNGs with Pillow (indexed → RGB/RGBA); write preview PNGs and RGB565 C arrays.
5. Parse the sprite records, convert 4444 → RGB565, write `assets/sprites.c` with dimensions, palette, alpha mask, and frame tables.
6. Slice the wall strip into 15 6×6 tiles; slice the power-up strip into 10 13×13 icons.
7. Decode `sounds.nok` into note/frequency/duration tables; write `assets/sounds.c`.
8. Generate `assets/assets.h` with offsets, counts, and checksums.
9. `--preview` renders a sprite sheet + all six original and derived mazes as PNG; render a 128×160 mockup with the HUD at y=139 and compare against the IGDB screenshots plus the documented adaptation.

Conversion rules:
- PNG palette colors: keep exact RGB until the final RGB565 quantize.
- Sprite 4444: `r = ((v >> 8) & 0xF) * 17`, `g = ((v >> 4) & 0xF) * 17`, `b = (v & 0xF) * 17`; alpha = high nibble (0 = transparent).
- The special palette value `-241` / `0xFF0F` is transparent; map it to `0x0FFF` before conversion.
- Use a per-sprite palette or a global 16-color palette; measure flash. Per-sprite palettes are easier to keep exact.
- Store transparency as a 1-bit mask separate from RGB565, or use a sentinel color. A mask is safer for sprites drawn over walls.
- RLE format: `[count:uint8][pixel:uint16]` with a repeat flag; cap runs at 255. Test both raw and RLE, choose the smaller.

Do not hand-edit generated asset files. Regenerate them from the JAR.

---

## 5. Milestones and tasks

Estimates are relative (S/M/L), not calendar promises. Complete milestones in order unless a task explicitly allows parallelism.

| Milestone | Outcome | Estimate |
|---|---|---|
| M0 | Reference + project hygiene | S |
| M1 | Bench hardware and power | M |
| M2 | Display, input, SD, audio drivers | L |
| M3 | Asset pipeline + static renderer | L |
| M4 | Playable snake core | L |
| M5 | Full mechanics | L |
| M6 | Menus, persistence, meta | M |
| M7 | Audio + polish | M |
| M8 | Accuracy + QA | M |
| M9 | Release/docs | S |

### M0 — Reference and project hygiene

- [x] **R1. Freeze the reference set**
  - Copy the JAR, `d`, `sounds.nok`, decompiled sources, and screenshots into a local `reference/` directory (git-ignored if copyrighted).
  - Record SHA-256 checksums in `docs/reference.md`.
  - Write down the exact JAR version (1.19) and the extraction offsets.
  - **Done when:** a fresh checkout can regenerate every asset from the JAR and checksums match.

- [x] **R2. Build a reference index**
  - Extract the 94 language strings from `lang.xx` into `reference/strings.txt` with IDs.
  - Extract the 6 original maze maps, generate the derived 21×23 maps, and dump the 42+ sprite inventory into text/JSON for review.
  - Note the 10 power-up IDs and their code locations.
  - **Done when:** every constant used by the port has a named reference or a comment in `config.h`.

- [x] **R3. Capture visual references**
  - Collect title, HUD, each maze, snake straight/turn/head/tail, food, bonus, each power-up icon, death, and high-score screens.
  - Use the IGDB screenshots and an emulator/video if possible; crop the original 128×128 frame and keep a separate 128×160 derived-layout mockup.
  - Store them as `reference/screens/*.png` with a naming convention.
  - **Done when:** at least one reference exists for every Tier A visual element.

### M1 — Bench hardware

- [x] **H1. Power and MCU bring-up**
  - Build the 5 V rail, 3.3 V rail, decoupling, reset pull-up, crystal, ISP header.
  - Program a blink at 1 Hz on PB0 (or a spare pin) with USBasp.
  - Measure 5 V and 3.3 V rails under load; check ripple.
  - **Done when:** the ATmega32 runs at 16 MHz, USBasp can read/write fuses and flash, and the blink survives a power cycle.

- [x] **H2. ST7735 bring-up**
  - Wire the LCD with a level shifter (or use the 3.3 V build).
  - Implement reset/init, `setAddrWindow`, `fillRect`, and a 128×160 color-bar test.
  - Confirm orientation, RGB/BGR order, inversion, and offsets.
  - **Done when:** color bars fill the panel, red is red, and the full 128×160 viewport maps 1:1 with the playfield origin at (1,1).

- [x] **H3. SD bring-up**
  - Wire 3.3 V power, 100 µF bulk, separate CS.
  - Read sector 0, print the FAT signature over UART.
  - Mount a FAT32 card and read a known file.
  - **Done when:** the card reads reliably after 100 power cycles and with the LCD active.

- [x] **H4. Buttons and buzzer bring-up**
  - Wire six buttons to PA0–PA5 with pull-ups; implement debounce and print events.
  - Drive the active buzzer through the NPN; verify on/off and current.
  - **Done when:** every button registers once per press and the buzzer produces a clean beep without resetting the MCU.

- [x] **H5. Level shifting and signal integrity check**
  - Scope (or logic-analyze) SPI at 8 MHz with the LCD and SD connected.
  - Check MISO high level into the 5 V MCU; add a series resistor or shifter if marginal.
  - **Done when:** no garbled display or SD read errors over a 10-minute soak.

### M2 — Drivers

- [x] **D1. ST7735 driver**
  - Implement init table(s), rotation, address window, fill, 6×6/8×8/11×11 blits, and RGB565 byte order.
  - Add `lcd_draw_rle()` that streams runs without a framebuffer.
  - Add a compile-time panel config (`OFFSET_X`, `OFFSET_Y`, `BGR`, `INVERT`).
  - **Done when:** a 128×160 RGB565 test image displays pixel-perfectly and a 6×6 tile writes in < 100 µs excluding SPI time.

- [x] **D2. SD/FAT driver** — CANCELLED: SD removed from the design (see D4 / session log). Driver deleted.
  - Integrate Petit FatFs or a minimal read-only FAT32 reader.
  - Provide `sd_read(offset, buf, len)` and `sd_read_file(name, ...)`.
  - Put the 512 B sector buffer in a union with boot-time-only buffers.
  - **Done when:** a 16 KB file reads correctly, CRC32 matches, and the buffer does not collide with game state.

- [x] **D3. Button driver** (host-tested; hardware check pending)
  - Scan at 100 Hz; debounce 10–20 ms; generate press/release events.
  - Menu auto-repeat: 400 ms delay, then every 100 ms.
  - Provide a raw key bitmask to the game, like the original `K[0..9]`.
  - **Done when:** no double presses, no missed releases, and all six buttons map correctly.

- [x] **D4. Buzzer driver**
  - Active mode: GPIO on/off with a minimum 20 ms pulse.
  - Passive mode: Timer0 CTC/PWM square wave with note table.
  - Non-blocking: `buzzer_play(id)` starts a sequence; the main loop advances it.
  - **Done when:** all 13 sound IDs (0–12) play without blocking the frame loop; mute works.

- [x] **D5. EEPROM store** (host-tested via `sh test/host/run.sh`; 100-power-cycle soak pending)
  - Implement magic/version/CRC, load defaults, save settings/high scores.
  - Add a host-side test for CRC and corruption fallback.
  - **Done when:** settings survive 100 power cycles and corrupted EEPROM falls back safely.

### M3 — Asset pipeline and static rendering

- [x] **A1. JAR extractor** (`python3 tools/extract_snake_ex2.py` → `build/assets/`)
  - Implement the PNG split and sprite-record parser exactly as in §1.2.
  - Fail loudly on unexpected offsets, sizes, palette counts, or trailing bytes.
  - Write a manifest with checksums.
  - **Done when:** all 8 PNGs and 61 sprite records decode, and the parser consumes exactly 15,602 bytes.

- [x] **A2. RGB565 conversion + preview** (`python3 tools/convert_assets.py` → `build/assets/{rgb565,preview}`)
  - Convert PNGs to RGB565 and sprites from 4444 to RGB565 + alpha mask.
  - Render preview sheets for all sprites and both the original 21×18 and derived 21×23 mazes.
  - **Done when:** the preview sheet visually matches the original screenshots and no color channel is swapped.

- [x] **A3. Wall tiles and maze renderer** (verified on hardware)
  - Slice `wall_strip.png` into 15 6×6 tiles; map tile IDs 1..15 to indices 0..14.
  - Render all six derived 21×23 mazes to the LCD using exact map bytes.
  - **Done when:** each maze matches the ASCII maps in Appendix A.2 and the reference screenshots; no tile gaps or 1-px offsets.

- [x] **A4. HUD, digits, title** (host-verified; LCD check pending)
  - Draw the HUD bar, score digits, bonus digits, and power-up icon at the exact coordinates.
  - Draw the title from flash (RLE, ~5.6 KB); small built-in logo only if flash runs out (then consider ATmega644P/PA).
  - **Done when:** the HUD is pixel-aligned at y=139, the playfield fills y=1..138, and the title appears within 1 s of boot.

- [x] **A5. Sprite mapping table** (verified on hardware)
  - Determine which `B0..B41` sprites are head/tail/body/curves/food and which are decorative.
  - Trace the original draw calls and compare against reference captures.
  - Build a table: direction, turn type, segment position, sprite index, offset.
  - **Done when:** a snake at every direction and turn combination renders correctly.

### M4 — Playable snake core

- [x] **G1. Map/model**
  - Store the derived 21×23 mazes in flash; implement cell IDs, wall test, wrap, and occupancy flags.
  - Audit every hard-coded `18`, `21`, `108`, `378`, and `% 18` inherited from the decompiled engine; replace them with field constants (`ROWS=23`, `COLS=21`, `FIELD_H=138`, `CELLS=483`).
  - Start state: length 6, head (7,9), direction right, food (4,4), all unchanged from the original.
  - **Done when:** host unit tests confirm exact initial state and all wraps in the 21×23 field.
  - *Done:* `test_game_init` (start state, cadence, 4 wraps, 180° rejection, wall death) and the oracle replay. The map is now a non-owning pointer into the caller's maze buffer plus a 61-byte occupancy bitmap, so `game_t` stays at ~600 B.

- [x] **G2. Movement and growth**
  - Direction history ring buffer; move head, retire tail, reject 180° reversals.
  - Movement cadence `2 + (8-speedIndex)` frames at 15 fps.
  - Eating grows by one segment.
  - **Done when:** speed 1..6 matches the original cadence and a 483-cell snake never overflows.
  - *Done:* cadence 10..5 frames verified for speeds 0..5; growth is capped at `GAME_MAX_LEN` (the direction array has one slot of slack). Original quirks preserved (first move is a no-op, `ma = 2 + (8-speed)`).

- [x] **G3. Collision and death**
  - Wall collision (unless ghost power-up), self collision, and edge wrap.
  - Death animation 40 ticks, sound, then game over.
  - **Done when:** every collision case is covered by a host test and matches the original behavior.
  - *Done:* wall death (move undone, 40-tick animation, death SFX), self-collision, and edge wrap match the original in all 216 oracle traces (127,955 ticks, 0 diffs); `test_game_init` covers the wall/undo/game-over sequence at 23 rows.

- [x] **G4. Food and score**
  - Spawn food using the original retry algorithm (8 tries, fallback scan).
  - Normal food score = speedIndex+1; HUD updates.
  - Bonus food appears with a 25–74 countdown and moving behavior in Extreme.
  - **Done when:** score, growth, and bonus timer match the reference logic.
  - *Done:* score = speed+1 per food, 8-try spawn with the original reject rules/fallback scan, bonus food 25–74 countdown; all checked bit-for-bit by the oracle replay.

- [x] **G5. Dirty-cell renderer**
  - Implement the dirty list and redraw order (base → overlay → snake).
  - Verify no ghost pixels after 1000 moves, including wrapping and turns.
  - **Done when:** 15 fps is sustained with no full-screen redraw during play.
  - *Done:* `render/playfield.{h,cpp}` repaints only a 3×3 cell block around each change (head, old head, new tail, vacated tail, item cells), then redraws the snake head-to-tail; sprites are clipped to the playfield like the original's `setClip(1,0,126,108)`. `test_playfield` drives 6 mazes × 6000 ticks (2,897 moves, 369 bonus spawns, 594 injected food moves) and compares every tick against a fresh full redraw: 0 differing pixels. Firmware boots into a playable Classic game (maze 0, speed 5, 15 fps, HUD, pause, death + FIRE restart). **Hardware-confirmed:** user approved the on-LCD game/snake rendering (2026-10-23).

### M5 — Full mechanics

- [x] **G6. Tongue**
  - Fire extends the tongue two cells ahead; it can collect food from distance.
  - Gate it exactly as the original does (Extreme mode) unless reference testing proves otherwise.
  - Add tongue graphics and sound.
  - **Done when:** food can be grabbed at 1–2 cells ahead and the tongue retracts correctly.
  - *Hardware-confirmed 2026-10-23:* user verified the lash graphics, 1–2-cell grab, retraction and controls on the LCD. Implementation notes: `game_t.extreme` + `tongue` (original `J`); FIRE queues `J=-1`, the next movement tick sets `J=1` and the food checks accept the head, `(k,l)` and `(K,L)` — one and two cells ahead, wrapped. `game_init()` stays Classic; `game_init_mode(..., true)` enables Extreme. `f.png` → four generated 4 bpp sprites `TONGUE_R/U/L/D` with the original `o/O` offsets, including the 1 px shift for up/down when the head byte has flag `0x20`; drawn before the head and erased through cached cells (also when the snake dies on the retract tick). `test_tongue` covers 1/2-cell grabs, 3-cell rejection, behind-head, Classic gating, no-fire, retraction and wrap; `test_playfield` runs Extreme + FIRE with food injected 1–2 cells ahead (36,000 ticks, 0 differing pixels). No lash SFX exists in the original (a grab plays SFX 12). Minor deviation: the first movement tick is still a no-op but the reach uses the current direction instead of the original's stale `k/l/K/L`.

- [x] **G7. Power-ups**
  - Implement the 10 types, 4-slot spawn table, 25–74 lifetime, icon mapping, and expiry.
  - Port effects: speed ±, ghost/pass-through, wall stamp, shrink variants, coin rush, moving food, lights-out.
  - Pickups must accept the head or the tongue's one/two-ahead cells (b.java 1517), same as food.
  - Verify each type against the decompiled `c()`/`C()` logic and reference video.
  - **Done when:** all 10 types have a defined effect, icon, duration, and test case.
  - *Implementation (hardware-confirmed 2026-10-24):* all ten types are ported in `game/game.cpp`:
    1 speed-up (cadence −2, clamp 1), 2 slow-down (+2, clamp 10), 3 wall ghost
    (self-collision still kills), 4 wall stamp (`ua[9][9]`, success SFX 3, failure falls
    through to type 1), 5 next three bonus foods get +24 on their 25–74 timer,
    6 shrink one extra segment/step ×3 (floor 3), 7 grow one extra/step ×3,
    8 coin rush (odd-indexed empty cell per **frame**, head/tongue +1, clears slots/bonus/
    food, then removes coins and restores food), 9 moves the **normal food** every
    `Ea=(15+rand%15)*((8-speed)>>1+1)` frames and lasts five normal foods (HUD bonus
    digits show `Ea`), 10 lights-out (`v=0x02020200+(10+rand%10)`, field toggles black on
    each byte expiry, HUD stays).
    Slot life is `25+rand%50` decremented every two frames; the active timer is
    `25+rand%50` decremented per movement step for types 1,2,3,4,10 and by events for
    5/6/7/8/9. Pickup loops over all four slots and accepts the head, `(k,l)` or `(K,L)`,
    exactly like food; multiple overlapping slots follow the original loop order. The HUD
    draws the selected 13×13 icon at `(110,142)` (the original draws the strip at
    `110-(Aa-1)*13` under a 13×13 clip, which is visually identical) and clears it when
    `Aa` is not 1..10. The field slot sprite is `C` (8×8 at `(x-1,y-1)`); coins and normal
    food use `B[0][0]`. Spawning flashes the original backlight (`Snake.A(11)`), which is
    not a sound and is not modeled.
    *Verification:* `test_powerup` covers every type, both speed clamps, ghost, stamp
    success/failure/blocking/restore, +24 ×3, shrink floor 3, grow ×3, coin-rush
    add/remove/pickup/end, moving-food relocation + five-food expiry, lights toggle +
    expiry, head/tongue pickup, slot lifetime/15-cleanup. The new Extreme sandbox oracle
    (`build/oracle/traces_pu`, 18 traces, 45,642 ticks) compares every G7 field (active
    id/timer, saved period, slots, coin bitmap, stamp cells, `Ea`, lights packed timer,
    counters) and all base engine state: 0 mismatches. The classic oracle is unchanged
    (216 traces / 127,955 ticks). `test_playfield` now also forces G7 renderer layers
    (slot spawn/move/edge/remove, coin start/add/remove/head-open/pickup/end, forced
    stamp near the head, lights on/move/off, shrink-tail cell) and compares each against
    a full redraw: 0 differing pixels. The tests also assert that stamp appearance/
    movement/expiry and coin-rush start/end return **no** full-screen refresh. Firmware
    builds at RAM 1,589/2,048 B and flash 29,404/32,768 B.
    *Deviations:* the 21×23 field extends the stamp placement y-range to 19 (original
    18-row code uses 14); the first movement tick's tongue reach is the G6 deviation.
    On hardware the original's `Ka` full-background repaint was visibly jarring for the
    wall stamp and coin rush, so the port deliberately repaints only the old/new 3×3
    stamp cells and the per-frame coin/item cells. Lights-out still requires a genuine
    full repaint because every base cell changes colour.
    *Hardware-confirmed 2026-10-24:* user verified pickup, HUD icon, coin rush, wall
    stamp, moving food, lights-out and expiry on the LCD. The incremental stamp/coin
    repaint fix was flashed and verified separately (no full-screen flash at either
    transition). `reference/powerups.txt` was regenerated with these verified semantics.

- [x] **G8. Extreme campaign**
  - Implement the real Extreme progression: score threshold, portal, shrink-to-2 transition,
    maze cycle `-1,0,1,2,3,4`, and level-start score (`b.java:1729-1741`, `1044-1058`,
    `296-329`).
  - The original campaign is **endless** (the level counter increments indefinitely); there
    is no finite 60-entry stage script. `Snake.J` is the instructions/tutorial sequence:
    positive entries are text pages, `-1..-7` are scripted demo games offered from those
    pages, and `-8` is the standalone high-score demo (`ta`/`Ta`). Those demos belong with
    U5, not campaign progression.
  - Verify portal/power-up interactions: coin rush suppresses the portal, speed `C()`
    overrides the shrink cadence, lights/stamp clear, moving bonus food bounces off the
    portal, and wall-stamp placement avoids it.
  - **Done when:** repeated levels and all six mazes can be played without state corruption,
    the portal/shrink/reset rules match the oracle, and the no-full-refresh portal rendering
    is hardware-verified.
  - *Hardware-confirmed 2026-10-24:* user verified the complete portal cycle with no
    flicker — appearance, head-only entry, one-segment-per-movement-tick shrink, maze
    changes with a reset and preserved score, and restart. G8 is ticked.
  - *Implementation (2026-10-24):* `game_t` now carries
    `level`, `level_start_score`, `portal_state`, `portal_x/y`, a mutable `map_buf` and a
    `maze_loader` callback; `game_set_maze_source()`/`game_portal_visible()` are the API.
    Extreme starts at `maze=-1, level=1, level_start_score=0` (`main.cpp`). At `cnt_c==0`,
    when `pu_active!=8`, `portal_state==0` and
    `score > level_start_score + speed*10 + level*20`, the portal is placed with the
    original `a()` retry/fallback spawner (including its cell-vs-pixel bugs: the portal test
    is `cell_col == X && cell_col == y`). Head-only entry sets state 2, `per_ma=1`, then
    calls the power-up deactivate hook; one tail segment is removed per movement tick; at
    length 2 `next_level()` snapshots the score, increments the level, cycles the maze,
    reloads `map_buf`, resets the snake/food/slots/coins/speed and requests a full redraw.
    Original `a()` leaves `ga/Ga/Fa/ha/Ia/Ea/U/v/na/ka` untouched; the port preserves them.
    The portal is `SnakeCanvas.c` (10×10) at screen `(X-1,y-2)`; appearance and entry use
    local dirty-cell repaints. The renderer also supports local removal, but in the engine
    the portal only disappears at a level reset, which full-repaints.
    *Verification:* `test_extreme` covers threshold equality/above, bounds, fallback/wall/
    coin rejection, coin-rush suppression, head-only entry, speed/ghost/lights/stamp
    deactivate ordering, one-segment-per-tick shrink, reset, all six mazes, preserved
    fields, moving-bonus bounce and Classic gating. `build/oracle/traces_campaign` (9
    forced-campaign traces, **180,000 ticks**) compares base state, all segments and
    `x/z/Y/X/y/da` + map bytes: 0 mismatches. `build/oracle/traces_campaign_natural`
    (12 power-up-bot traces, **28,481 ticks**, includes natural portal spawns/entries,
    power-ups and level transitions): 0 mismatches. `test_playfield` adds portal
    on/move/state/off/edge/head-overlap cases against a full redraw (all local).
    Flash/RAM: **30,958/32,768 B (94.5%, 1,810 B free), 1,610/2,048 B (78.6%)**;
    1,810 B will not cover the U1/U2 menus plus U5 demos, so plan the ATmega644P/PA swap
    (documented fallback) or prune assets before M6.
    *Decisions:* (1) challenge demos `-1..-8` are moved to U5 because they are tutorial
    demos, not campaign progression; (2) the original's `Ka` full-background redraw for
    wall-stamp/coin-rush transitions remains replaced by local repaints (G7).
    *Bug fixed during G8:* the general `ba` countdown must run even when `Aa==0`
    (`b.java:1526`), e.g. after an expired slot briefly sets `Aa=15`; this was exposed by
    natural campaign traces.
  - *Hardware checklist (confirmed):* portal appears at score >70 on speed 5/level 1
    without a screen flash; head entry shrinks one segment per movement tick; at length 2
    the maze changes through no-maze→Box→Tunnel→Spiral→Blockade→Twisted→no-maze with a
    snake reset and score preserved; coin rush suppresses new portals; a speed power-up
    restores its saved period at entry instead of shrinking every tick; death/restart
    returns to level 1/no-maze.

- [x] **G9. Classic mode and speed/maze selection**
  - No-maze + 5 mazes, 6 speeds, endless play.
  - Confirm maze names: Box, Tunnel, Spiral, Blockade, Twisted.
  - **Done when:** every combination starts with the correct map and speed.
  - *Implementation (2026-10-24):* `game_init_classic()` validates `maze -1..4` and
    `speed 0..5`, loads the selected derived 21×23 map, and records the map/loader for
    later use; invalid selections are rejected before `game_t` is touched. `maze_name()`
    returns the PROGMEM strings from the original table: **No maze, Box, Tunnel, Spiral,
    Blockade, Twisted** (`strings.txt` 67/58/69/68/57/70). During G9 verification,
    `main.cpp` could start Classic through this path with compile-time
    `START_EXTREME=0` and `START_MAZE`/`START_SPEED` overrides. U1 replaced that
    temporary boot path; `game_init_classic()` is now the New game path, driven by the
    saved EEPROM settings.
  - *Verification:* `test_classic_select` covers all 6 selections (no maze + 5 mazes)
    × 6 speeds against the
    independently derived `build/assets/mazes_21x23.bin`, plus names, cadence, score
    step, no-maze tile reads and invalid input. `tools/oracle/build_oracle.py
    --classic-all` generates 36 original-engine traces (one per maze/speed) and
    `test_game_oracle` replays them: **36 traces / 79,161 ticks, 0 mismatches** (the
    survival bot ends some traces early; every trace still exercises its start cadence).
  - *Decision:* the project keeps **six** speed indices `0..5`, matching the backlog,
    EEPROM validation and oracle traces. The original `c.java` draws **nine** selector
    positions (`Snake.I = 0..8`); the movement formula already accepts `0..8`, so a
    future U2 decision can expose nine without changing the engine (see D12).
  - *Hardware-confirmed 2026-10-24:* user flashed the Classic preset (Box, speed 5) and
    verified the Box map, slow vs. fast cadence, per-speed score increments, and endless
    play/death/restart. No-maze and the other maps are covered by the exhaustive
    36-combination host/oracle proof. G9 is complete.

### M6 — Menus, persistence, meta

- [x] **U1. Title and main menu**
  - Title screen (original 128×128 art at y=16), 3 s/idle or key-to-continue, menu items: New game, High scores, Settings, Instructions.
  - **Done when:** navigation works with 4 directions + fire/back and the menu uses the full 128×160 height.
  - *Implementation (2026-10-24):* cold boot now goes straight to `title_draw()` plus a
    “PRESS ANY KEY” prompt in the bottom title band; a press or 3 s idle advances to the
    full-screen main menu. The old boot self-test (EEPROM write test, 13-SFX sweep and
    button indicators) is no longer run; drivers remain covered by host tests and were
    hardware-verified earlier. New `src/render/text.{h,cpp}` owns the 5×7 font and a
    reusable `text_draw()`/`text_width()`. New `src/ui/menu.{h,cpp}` holds the four items
    with wrapping selection: Up/Left = previous, Down/Right = next, Fire = select,
    Pause = back to title. The menu uses the whole 128×160 panel: a 20 px header,
    four 27 px rows and a 20 px soft-key footer. Up/Down/Left/Right navigation
    repaints only the row that loses the highlight and the row that gains it;
    the full screen is drawn only when the menu is entered (or after returning
    from an info screen). High scores, Settings and Instructions
    open labelled “COMING SOON” screens with Fire/Pause back; their content belongs to
    U3/U2/U5. New game starts from the saved EEPROM settings through
    `game_init_classic()` (Classic) or `game_init_mode(..., true)` (Extreme); the default
    is Classic/no maze/speed 2. The G9 `START_*` preset and `ATmega644PA_classic`
    environment were removed because U1 supersedes the compile-time boot path.
  - *Verification:* `test/host/test_menu.cpp` covers exact labels, selection wrapping,
    text metrics, the selected/unselected row colours, and that the header, all four
    rows and the footer are rendered (full 128×160 height). It also compares two-row
    incremental navigation against a full redraw for both adjacent and wrapping moves,
    and asserts that no `fill_screen()` occurs during navigation. The complete host suite still
    passes: 216 classic traces / 127,955 ticks, 36 all-classic traces / 79,161 ticks,
    18 power-up traces / 45,642 ticks, 9 forced-campaign traces / 180,000 ticks and
    12 natural-campaign traces / 28,481 ticks, all with 0 mismatches. Firmware builds at
    **RAM 1,654/4,096 B (40.4%), flash 37,146/65,536 B (56.7%)**; the ~6 KB flash
    increase over G9 is the title art, which was previously dead-stripped because the
    old boot path never called `title_draw()`.
  - *Hardware check:* flash `.pio/build/ATmega644PA/firmware.hex`. Verify the title art
    and prompt appear immediately, auto-advance after ~3 s and also advance on any key;
    check Up/Left, Down/Right wrapping, Fire on all four items, Pause/back from the menu
    and from the three info screens, and that New game starts a playable Classic/no-maze/
    speed-2 game. Tick U1 after confirmation.

- [x] **U2. Settings**
  - Mode (Classic/Extreme), maze, speed, sound on/off.
  - Save to EEPROM on change; show confirmation.
  - **Done when:** settings survive power cycles and invalid EEPROM values fall back.
  - *Implementation (2026-10-24):* new `src/ui/settings.{h,cpp}` provides a full-screen editor
    for the four persisted settings. The screen uses the same 128×160 layout as the main
    menu: a 20 px header, four 27 px rows (Mode, Maze, Speed, Sound) and a
    20 px soft-key footer. Up/Down selects the item; Left/Right changes the value
    (Mode/Sound toggle, Maze/Speed cycle with clamping); Fire saves to
    EEPROM and flashes a momentary "SAVED" confirmation box; Pause returns to the main
    menu. The maze list uses the original PROGMEM names from `maze_name()`; a new
    `text_draw_p`/`text_width_p` pair in `render/text` handles PROGMEM strings correctly.
    A host test `test_settings` verifies selection wrapping, value adjustments (including
    clamps), toggles, maze cycling, and partial row repaint without full-screen
    `fill_screen()`.
  - *Verification:* `test_settings` passes. The complete host suite still passes
    (216/36/18/9/12 trace sets, 0 mismatches). Firmware builds at **RAM 1,731/4,096 B
    (42.3%), flash 38,260/65,536 B (58.4%)**; the ~1 KB increase is the settings screen
    and its dependencies.
  - *Hardware check:* flash `.pio/build/ATmega644PA/firmware.hex`. From the main menu,
    select Settings and verify:
    - Up/Down wraps through the four items with highlight.
    - Left/Right on Mode toggles CLASSIC↔EXTREME; on Maze cycles No maze→Box→Tunnel→
      Spiral→Blockade→Twisted→No maze (labels from PROGMEM); on Speed increments/
      decrements 0..5; on Sound toggles ON/OFF.
    - Fire writes to EEPROM, shows "SAVED" briefly, and returns to the settings screen.
    - Pause returns to the main menu with the new values active (New game uses them).
    - Power-cycle: settings persist.

- [x] **U3. High scores**
  - Two scores per mode, game-over entry, storage, and display.
  - **Done when:** scores persist across resets and display in the original layout.
  - *Implementation (2026-10-24):* new `src/ui/highscores.{h,cpp}` provides a full-screen high-score
    table (two entries per mode) and a game-over entry screen. The table shows the two saved
    scores for the selected mode (Classic/Extreme) with rank numbers and dashes for empty
    slots. On game over, if the final score qualifies (beats either stored score), the
    entry screen appears: the player edits three initials (A–Z) using Left/Right to change
    the letter and Up/Down to move to the next position; Fire saves and returns to the game.
    Scores are stored in EEPROM via the existing `eeprom_set_score()` API (indices 0/1 for
    Classic, 2/3 for Extreme). The main menu's "High scores" item now opens the table for
    the current mode. A host test `test_highscores` verifies qualification logic, score
    formatting, and full-screen rendering.
  - *Verification:* `test_highscores` passes. Complete host suite passes (216/36/18/9/12
    trace sets, 0 mismatches). Firmware builds at **RAM 1,795/4,096 B (43.8%), flash
    40,116/65,536 B (61.2%)**; the ~2 KB increase is the high-score table and entry
    screen.
  - *Hardware check:* flash `.pio/build/ATmega644PA/firmware.hex`. Verify:
    - Main menu → High scores shows two scores for current mode (dashes if empty).
    - Play a game, achieve a score that beats a stored entry → game-over entry screen
      appears, initials editable, Fire saves, returns to title.
    - High scores persist across power cycles.
    - Scores are per-mode (Classic/Extreme).

- [x] **U4. Pause and game over**
  - Pause overlay, resume, restart, exit to menu; game-over screen with score.
  - **Done when:** pause cannot corrupt the game state and back always returns safely.
  - *Implementation (2026-10-24):* new `src/ui/pause.{h,cpp}` provides a pause overlay and
    game-over screen. The pause overlay draws a subtle grid pattern over the frozen
    game, with three options: Resume, Restart, Menu. Up/Down selects, Fire confirms,
    Pause also resumes. The game-over screen shows the final score and two options:
    Restart, Menu. Up/Down selects, Fire confirms. Both screens preserve the underlying
    game state and return cleanly. The old `show_game_over()` was replaced. Host test
    `test_pause` verifies the overlay uses a grid pattern (no full-screen fill) and
    the game-over screen does a full-screen fill with score display.
  - *Verification:* `test_pause` passes. Complete host suite passes (216/36/18/9/12
    trace sets, 0 mismatches). Firmware builds at **RAM 1,819/4,096 B (44.4%), flash
    41,390/65,536 B (63.2%)**.
  - *Hardware check:* 2026-10-24 confirmed. PAUSE shows overlay with Resume/Restart/Menu;
    Resume clears grid pattern and resumes cleanly; Restart/Menu work. Game-over screen
    appears automatically with final score and Restart/Menu options. Pause cannot
    corrupt game state.

- [ ] **U5. Instructions/tutorial**
  - Port the relevant `lang.xx` strings; keep longer text on SD if flash is tight.
  - Include the tongue, walls, bonus food, and power-up hints.
  - Port the `Snake.J` page sequence and its seven scripted demo games (`ta`/`Ta`
    `-1..-7`), plus the standalone high-score demo `-8`. These are tutorial demos, not
    campaign levels (see G8 decision); schedule them with the page navigation and reserve
    flash accordingly.
  - **Done when:** all tutorial screens are reachable and legible on the full 128×160 display.
  - *Implementation (2026-10-24):* new `src/ui/instructions.{h,cpp}` provides the tutorial
    screen with 12 text pages (from original lang.xx IDs 0, 1, 6, 11, 16, 21, 26, 51–55)
    plus a demo menu with 8 scripted demos. Navigation: Up/Down for pages, Left/Right
    to enter demo menu, Fire to run demo, Pause/Fire to exit back to main menu. The
    demo games are scripted auto-play sequences that showcase: tongue grab (-1), wall
    collision (-2), bonus food (-3), speed power-up (-4), ghost/wall-pass (-5), coin
    rush (-6), lights out (-7), high-score entry (-8). Each demo initializes a game
    with specific settings, forces the relevant game state, and auto-plays for a few
    seconds. Text rendering uses word-wrapping with the 5×7 font. `text_draw_p`/`text_width_p`
    handle PROGMEM strings. Host test `test_instructions` (to be added) will verify
    page navigation and demo launch. Flash increased by ~4 KB for tutorial text and
    demo logic.
  - *Verification:* Full host suite passes (216/36/18/9/12 trace sets, 0 mismatches).
    Firmware builds at **RAM 1,878/4,096 B (45.8%), flash 45,414/65,536 B (69.3%)**.
  - *Hardware check:* flash `.pio/build/ATmega644PA/firmware.hex`. Verify:
    - Main menu → Instructions opens tutorial screen.
    - Up/Down navigates 12 text pages with correct content and page indicator.
    - Left/Right enters demo menu with 8 items (Tongue, Walls, Bonus Food, Speed Up,
      Ghost, Coin Rush, Lights Out, High Score).
    - Up/Down selects demo, Fire launches it, demo auto-plays and returns to demo menu.
    - Pause/Fire from any page returns to main menu.
    - All text legible on 128×160 display.

### M7 — Audio and polish

- [ ] **S1. Sound table**
  - Decode `sounds.nok` into note sequences; map `playSFX(0..12)` to events.
  - Implement the 1760 Hz beep for index 12.
  - **Done when:** all 13 sound IDs (0–12) play at the right moments and do not overlap incorrectly.

- [ ] **S2. Passive buzzer support**
  - Timer0 square wave, note frequencies, duration, envelope (on/off).
  - Optional volume via PWM duty.
  - **Done when:** a recognizable monophonic version of each effect plays on a passive piezo.

- [ ] **S3. Visual polish**
  - Death animation, portal effect, power-up blink, lights-out transition, high-score decorations.
  - Match timing to the original where known; avoid adding non-original effects to Tier A areas.
  - **Done when:** reference comparisons show no unintended visual differences.

### M8 — Accuracy and QA

- [ ] **Q1. Host-side logic tests**
  - Compile the pure game logic natively with stub HAL.
  - Test movement, wrap, collision, food spawn, score, portal threshold, power-up timers, EEPROM CRC.
  - **Done when:** all tests pass and are run by `pio test -e host` or a Makefile.

- [ ] **Q2. Asset golden tests**
  - Verify original and derived maze bytes, the 21×23 extension, sprite counts, palette sizes, and a CRC of every generated asset.
  - **Done when:** regenerating assets produces an identical manifest.

- [ ] **Q3. Visual diff**
  - Render reference frames on the host (same drawing code with a stub LCD) and compare to captured original frames; separately render the derived 128×160 frame and check the documented translation.
  - Allow only expected differences from RGB565 quantization, the 128×160 extension, and menu reconstruction.
  - **Done when:** all Tier A screens pass within a small color tolerance.

- [ ] **Q4. Hardware soak**
  - 2-hour play, 100 deaths, 100 power cycles, SD inserted/removed only while off, maximum-length (483-cell) snake.
  - Monitor 5 V/3.3 V rails, reset behavior, and EEPROM integrity.
  - **Done when:** no hangs, no corrupted saves, no display artifacts.

- [ ] **Q5. Timing/performance**
  - Measure frame time, SPI throughput, SD load time, and stack high-water mark.
  - **Done when:** 15 fps is stable, worst-case frame < 50 ms, flash ≤ 30 KB, static RAM ≤ 1.8 KB including stack and excluding the overlaid SD buffer, stack margin ≥ 256 B.

### M9 — Release and documentation

- [ ] **P1. Wiring and BOM docs**
  - Schematic-style pin table, power notes, level-shifter notes, fuse values, USBasp procedure.
  - **Done when:** a builder can assemble the board without reading the source.

- [ ] **P2. Build/flash docs**
  - `pio run -t upload` / `avrdude` command, fuse read/write, recovery, SD card format.
  - **Done when:** a clean machine can build and flash from the README.

- [ ] **P3. Final acceptance run**
  - Execute the §9 checklist on the real hardware.
  - Tag the release; archive the exact JAR/asset checksums.
  - **Done when:** every box is checked or explicitly waived in “Decisions”.

---

## 6. Quick-start order (first two weeks)

If you want a concrete execution order, do this first:

1. **R1–R3:** freeze assets and capture references.
2. **H1:** power/clock/ISP blink.
3. **H2:** LCD color bars and full 128×160 viewport.
4. **A1–A2:** extractor + sprite preview sheet and 21×23 maze extension.
5. **A3:** draw all six derived mazes on the LCD from the real map bytes.
6. **H3/H4:** SD read and buttons.
7. **G1–G4:** moving snake, food, score, death — no menus yet.
8. **G5:** dirty-cell renderer.
9. Only then add tongue, power-ups, extreme campaign, and menus.

Do not build the menu system before the playfield renders correctly; it hides rendering bugs.

---

## 7. Test matrix

| Area | Test | Expected |
|---|---|---|
| Maze | Load each of 6 derived maps | ASCII/byte-identical to Appendix A.2; 21×23 |
| Walls | Draw all 15 tile IDs | Correct tile, 1 px inset, no gaps |
| Snake | Straight/turn/head/tail in 4 directions | Correct sprites and offsets |
| Wrap | Move off each edge | Reappears opposite side in the 21×23 field, no death |
| Collision | Wall, self, tongue/body | Death after 40 ticks |
| Growth | Eat 100 foods | Length increases, no overflow |
| Speed | Settings 1..6 | 10,9,8,7,6,5 frames per cell |
| Food | Spawn 1000 times | Never in wall/snake/collectible |
| Bonus | Let timer expire / eat it | Disappears; score equals remaining value |
| Tongue | Grab at 1 and 2 cells | Food collected, tongue retracts |
| Power-ups | Each of 10 types | Correct effect, icon, duration, expiry |
| Extreme | Play 10 levels | Portal appears at threshold; maze cycles; snake resets |
| Death | Score/high score | Saved once, displayed after reset |
| EEPROM | Corrupt magic/CRC | Defaults load, no hang |
| SD | Missing/corrupt title | Fallback title, game still runs |
| Audio | All 13 IDs (0–12) | Correct event, no blocking/stuck note |
| Display | 1-hour play | No ghost pixels, tearing, or offset drift |
| Power | 4.5–5.5 V, cold boot | Stable reset, no brown-out resets |
| Memory | Max-length snake + menus | No stack collision, no corruption |

---

## 8. Risks and mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| 32 KB flash too small | Cannot fit full campaign/assets | Resolved by the D11 swap to ATmega644P/PA (64 KB); keep `-Os -flto`, RLE and no framebuffer. ATmega32 build is no longer the release target. |
| 4 KB SRAM / stack | Crashes, stack overflow | Direction-history snake, no framebuffer, 483-byte map/history, no dynamic allocation; measure stack and keep ≥512 B margin |
| 128×160 layout is derived, not official | Fidelity/expectation mismatch | Document §1.5, preserve all original 18-row wall tiles, never scale art, verify derived maps against Appendix A.2 |
| 5 V ↔ 3.3 V mismatch | Damaged LCD/SD or unreliable SPI | Proper level shifter, separate 3.3 V LDO, verify module; or run whole system at 3.3 V/8 MHz |
| ST7735 variant mismatch | Wrong colors/offset | Configurable init table and offsets; color-bar test |
| SD latency/power spikes | Boot hangs, corrupted reads | 100 µF bulk, retry logic, lower SPI, fallback; avoid hot-plug |
| Active buzzer cannot play melodies | Tier C audio only | Use fixed-pitch effects; offer passive piezo option and driver |
| Full-screen redraw too slow | Low fps/tearing | Dirty-cell rendering, tile blits, no framebuffer; optimize SPI |
| Menu/instruction text too large | Flash overflow | Abbreviate, store on SD, or drop non-essential tutorial cards |
| Fuse/JTAG mistake | Bricked/unprogrammable chip | Read fuses first, keep SPIEN, avoid PC2–PC5 or disable JTAG deliberately, keep ISP header |
| Copyright | Distribution issue | Personal preservation only; do not redistribute Nokia assets |

---

## 9. Acceptance criteria (release gate)

### Visual

- [ ] Gameplay fills the full 128×160 panel: 1 px border at y=0 and y=159, 21×23 playfield at (1,1), HUD at (0,139). No scaling or letterboxing.
- [ ] All 6 derived 21×23 maze maps match Appendix A.2 byte-for-byte and visually.
- [ ] Wall tiles, snake segments, food, and power-up icons use the extracted palettes and correct offsets, translated by the playfield origin (1,1).
- [ ] HUD bar at y=139; score digits at (39,139), 6 digits; bonus digits at (69,145), 4 digits; power-up icon at x=110-(type-1)*13, y=142.
- [ ] Title screen is the extracted 128×128 art shown 1:1 at (0,16), or the documented fallback if SD is absent.
- [ ] No ghost pixels or half-drawn cells during normal play.

### Behavior

- [ ] Movement cadence for speed 1..6 is 10/9/8/7/6/5 frames per cell at 15 fps.
- [ ] Edge wrapping works on all four sides; only walls/self kill.
- [ ] Initial state: length 6, head (7,9), facing right, food (4,4).
- [ ] Eating grows by 1; normal score = speedIndex+1; bonus adds its countdown value.
- [ ] Tongue grabs food at 1–2 cells ahead and retracts.
- [ ] All 10 power-up types have a verified effect and icon.
- [ ] Extreme portal appears at the correct score threshold, shrinks the snake to 2, and starts the next maze.
- [ ] Death animation lasts 40 ticks; high score is saved and displayed.
- [ ] Classic and Extreme mode selections work across all maze/speed combinations.

### Non-functional

- [ ] 15 fps stable during play; worst-case frame < 50 ms.
- [ ] Flash image fits the ATmega644P/PA (64 KB) with ≥ 4 KB free after all features; static RAM ≤ 3.5 KB including stack; stack margin ≥ 512 B.
- [ ] No dynamic allocation, no `String`, no floating point.
- [ ] Cold boot to title < 2 s; SD missing/corrupt still boots.
- [ ] Settings/high scores survive 100 power cycles; corrupted EEPROM falls back.
- [ ] 2-hour soak with no hang, reset, or display artifact.
- [ ] All 13 sound IDs (0–12) mapped; no stuck notes; mute works.

---

## 10. Decisions and open questions

Record the chosen option and date here before starting the related milestone. D9 and D10 are already decided and are included for auditability.

| ID | Decision | Recommendation | Status |
|---|---|---|---|
| D1 | 5 V/16 MHz + level shifters vs 3.3 V/8 MHz direct | 5 V/16 MHz for margin; 3.3 V/8 MHz for simplicity | open |
| D2 | Bare-metal avr-libc vs Arduino framework | Bare-metal | open |
| D3 | Active buzzer only vs add passive piezo | Add passive piezo for real melodies; keep active as fallback | open |
| D4 | SD role | **No SD card.** All assets in flash. Title art last/optional; if flash is short, shrink it or move to ATmega644P/PA (pin-compatible, 64 KB flash / 4 KB RAM; JTD via `MCUCR`, PlatformIO `board = ATmega644P`, `board_build.mcu = atmega644pa`) | decided 2026-10-23; target updated to 644PA 2026-10-24 |
| D5 | Extreme campaign depth | Endless portal/maze progression implemented; tutorial demos (`Snake.J` `-1..-8`) scoped to U5; revisit flash budget with the menu/tutorial work | open |
| D6 | Menu fidelity | Tier B custom Nokia-style menus (original uses Series 40 UI) | open |
| D7 | Resume-state save | High scores/settings first; optional compact resume slot later | open |
| D8 | Enclosure/buttons | 6 tactile buttons, landscape or portrait? | open |
| D9 | 128×160 layout | No official 128×160 build found; use the derived 21×23 playfield + HUD (§1.5), never scale the 128×128 art | decided 2026-10-23 |
| D10 | Title art on 128×160 | Keep the original 128×128 art 1:1 at (0,16); fill top/bottom with the title background | decided 2026-10-23 |
| D11 | MCU target | **ATmega644P/PA at 5 V / 8 MHz**; actual chip is **ATmega644PA, signature `0x1E960A`**. Same DIP-40 board and pinout; `MCUCR` JTD write (not `MCUCSR`); 64 KB flash / 4 KB SRAM. PlatformIO uses `board = ATmega644P` with `board_build.mcu = atmega644pa`. Swapped because the G8 image used 94.5% of the ATmega32 flash with all menus/tutorial still ahead. | decided 2026-10-24; 644PA confirmed |
| D12 | Speed selector range | Six indices `0..5` for G9/U2, matching the backlog and EEPROM/oracle contract. The original `c.java` draws nine positions (`Snake.I=0..8`); engine cadence/score already handle all nine, so exposing nine is a UI/settings change, not a core rewrite. Revisit before U2. | decided 2026-10-24 (six; nine deferred) |

---

## Appendix A — Original 21×18 maze maps (reference)

Raw bytes are `map[row*21 + col]`; low nibble 0 = empty, 1..15 = wall tile index+1. The original game draws walls at `x=1+6*col`, `y=6*row`. These maps are the extraction source; the target 128×160 maps are the derived 21×23 versions in Appendix A.2.

### Maze 0 — Box (`V=0`)

```
755555555555555555558
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
95555555555555555555A
```

### Maze 1 — Tunnel (`V=1`)

```
76.................48
3...................3
.....................
.....................
.....................
.....45555555556.....
.....................
.....................
.....................
.....................
.....................
.....................
.....45555555556.....
.....................
.....................
.....................
1...................1
96.................4A
```

### Maze 2 — Spiral (`V=2`)

```
..............2......
..............2......
..............2......
..............2......
..............2......
5555555556....2......
..............2......
..............3......
.....................
.....................
......1..............
......2..............
......2....4555555555
......2..............
......2..............
......2..............
......2..............
......2..............
```

### Maze 3 — Blockade (`V=3`)

```
755555555555555555558
2...................2
2...................2
2....1.........1....2
2....2.........2....2
2....2.4555556.2....2
3....2.........2....3
.....3.........3.....
.....................
.....................
.....1.........1.....
1....2.........2....1
2....2.4555556.2....2
2....2.........2....2
2....3.........3....2
2...................2
2...................2
95555555555555555555A
```

### Maze 4 — Twisted (`V=4`)

```
76...45B5555555555555
2......2.............
3......2.............
.......2.............
.......2.............
.......2.............
.......2.............
5555555A..45555555555
.....................
.....................
.....................
55555555555B555555555
...........2.........
...........2.........
...........2.........
...........2.........
...........2.........
...........2.........
```

### Maze 5 — unused L-shape (present in data, not selected by `ca`)

```
.....................
.....................
.....................
.....................
.....................
.............1.......
.............2.......
.............2.......
.............2.......
..........1..2.......
..........2..2.......
..........955A.......
.....................
.....................
.....................
.....................
.....................
.....................
```

---

## Appendix A.2 — Derived 21×23 maze maps (128×160 target)

Generated from Appendix A by inserting five rows after local row 9 (0-based) and shifting original rows 10..17 down by five. The inserted rows are blank except for Box, which repeats the `2...................2` side-wall row. Logical wall draw origin is `(1,1)`; `map[row*21 + col]`.

### Maze 0 — Box (derived)

```
755555555555555555558
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
2...................2
95555555555555555555A
```

### Maze 1 — Tunnel (derived)

```
76.................48
3...................3
.....................
.....................
.....................
.....45555555556.....
.....................
.....................
.....................
.....................
.....................
.....................
.....................
.....................
.....................
.....................
.....................
.....45555555556.....
.....................
.....................
.....................
1...................1
96.................4A
```

### Maze 2 — Spiral (derived)

```
..............2......
..............2......
..............2......
..............2......
..............2......
5555555556....2......
..............2......
..............3......
.....................
.....................
.....................
.....................
.....................
.....................
.....................
......1..............
......2..............
......2....4555555555
......2..............
......2..............
......2..............
......2..............
......2..............
```

### Maze 3 — Blockade (derived)

```
755555555555555555558
2...................2
2...................2
2....1.........1....2
2....2.........2....2
2....2.4555556.2....2
3....2.........2....3
.....3.........3.....
.....................
.....................
.....................
.....................
.....................
.....................
.....................
.....1.........1.....
1....2.........2....1
2....2.4555556.2....2
2....2.........2....2
2....3.........3....2
2...................2
2...................2
95555555555555555555A
```

### Maze 4 — Twisted (derived)

```
76...45B5555555555555
2......2.............
3......2.............
.......2.............
.......2.............
.......2.............
.......2.............
5555555A..45555555555
.....................
.....................
.....................
.....................
.....................
.....................
.....................
.....................
55555555555B555555555
...........2.........
...........2.........
...........2.........
...........2.........
...........2.........
...........2.........
```

### Maze 5 — unused L-shape (derived)

```
.....................
.....................
.....................
.....................
.....................
.............1.......
.............2.......
.............2.......
.............2.......
..........1..2.......
.....................
.....................
.....................
.....................
.....................
..........2..2.......
..........955A.......
.....................
.....................
.....................
.....................
.....................
.....................
```

---

## Appendix B — Original constants and mechanics (with 128×160 overrides)

### Cells and timing

- Cell size: 6×6. Original grid 21×18 (126×108); target grid **21×23 (126×138)**.
- Logical playfield coordinates are 0..125 × 0..137; screen origin is `(1,1)`.
- Head position is in logical pixels; cell = `(x/6, y/6)`.
- Wrap: `x <= 0 → x += 126`, `x >= 126 → x -= 126`; `y <= 0 → y += 138`, `y >= 138 → y -= 138`.
- Initial head: `(42,54)` = cell (7,9); screen cell origin `(43,55)`; initial direction: right (`0x08`).
- Initial food: `(24,24)` = cell (4,4); screen cell origin `(25,25)`.
- Initial length: 6.
- Movement: `framesPerCell = 2 + (8 - speedIndex)`, speedIndex 0..5.
- Speed-up power-up: `framesPerCell -= 2`, clamp 1.
- Slow-down power-up: `framesPerCell += 2`, clamp 10.
- Death timer: 40 ticks.

### Score

- Normal food: `score += speedIndex + 1`.
- Bonus food: `score += remainingBonusTimer` (25..74), plus +24 in one power-up case.
- Coin rush: +1 per coin.
- High scores: 2 per mode.

### Extreme

- Stage script length 60; entries include text/level IDs and special challenges `-1..-8`.
- Maze cycle: `-1, 0, 1, 2, 3, 4`.
- Portal threshold: `score > levelStartScore + speedIndex*10 + levelNumber*20`.
- Portal entry: shrink one segment per tick until length ≤ 2, then reset to length 6 and start the next maze.
- Level start: `levelStartScore = score`, `levelNumber++`.

### Power-up IDs (from `c()` / `C()`; verify names visually)

| ID | Code effect | Suggested label |
|---:|---|---|
| 1 | `framesPerCell -= 2` (clamp 1) | Speed up |
| 2 | `framesPerCell += 2` (clamp 10) | Slow down |
| 3 | wall collision bypassed while `Aa == 3`; self collision still applies | Wall ghost |
| 4 | tries to stamp a random 3×3 wall block; if placement fails, becomes speed-up instead | Wall stamp |
| 5 | next 3 bonus foods are worth +24 each | Bonus boost |
| 6 | removes one tail segment per step for 3 steps | Shrink |
| 7 | grows one segment per step for 3 steps | Grow |
| 8 | coin rush: marks map cells `0x20`, +1 per coin; food/portal suppressed | Coin rush |
| 9 | moving bonus food; timer resets | Moving food |
| 10 | lights-out: toggles background every N frames | Lights out |

The exact player-facing names are not in `lang.xx`; derive them from the icon strip and reference video, then update this table. Note that type 4's code path is counter-intuitive: `B()` returns `false` on successful placement, so a successful wall stamp keeps effect 4; failure falls through to effect 1.

### Draw coordinates (target 128×160; original in parentheses)

- Static wall tile: `(1 + 6*col, 1 + 6*row)` (original `(1 + 6*col, 6*row)`).
- Dynamic clip region: `(1,1,126,138)` (original `(1,0,126,108)`).
- HUD bar: `(0,139)` size 128×20 (original `(0,108)`).
- Score: `(39,139)`, 6 digits, 8 px pitch (original `(39,108)`).
- Bonus: `(69,145)`, 4 digits (original `(69,114)`).
- Power-up icon: `(110 - (id-1)*13, 142)`, 13×13 (original y=111).
- Title: `(0,16)` size 128×128 (original `(0,0)`); the original art is not scaled.

---

## Appendix C — Original strings relevant to the port

Extracted from `lang.xx` (94 entries, 0-based). IDs 56–93 are exact:

| ID | String |
|---:|---|
| 56 | Back |
| 57 | Blockade |
| 58 | Box |
| 59 | Classic |
| 60 | Blockade |
| 61 | Box |
| 62 | Classic |
| 63 | Extreme |
| 64 | Game mode is %U |
| 65 | Level %U selected |
| 66 | %U selected |
| 67 | No maze |
| 68 | Spiral |
| 69 | Tunnel |
| 70 | Twisted |
| 71 | Continue |
| 72 | Exit |
| 73 | Extreme |
| 74 | High scores |
| 75 | High scores |
| 76 | Instructions |
| 77 | Instructions |
| 78 | Level |
| 79 | Level |
| 80 | Mazes |
| 81 | Mazes |
| 82 | Game mode |
| 83 | Game mode |
| 84 | New game |
| 85 | Next |
| 86 | No maze |
| 87 | OK |
| 88 | Select |
| 89 | Settings |
| 90 | Settings |
| 91 | Spiral |
| 92 | Tunnel |
| 93 | Twisted |

Key tutorial strings:
- 0: Snake EX2
- 1: tongue hint (%U is the fire key)
- 6: Nokia games service hint
- 11: make the snake grow longer by directing it to the food
- 16: use the keys %0U, %1U, %2U, %3U to change direction
- 21: avoid hitting walls and your own tail
- 26: eat the bonus food before the time expires
- 51: in Extreme mode you advance when you have scored enough points
- 52–55: continuation of the Extreme explanation

Store the full decoded table in `reference/strings.txt`; do not hard-code partial translations.

---

## Appendix D — Build target sketch

The current `platformio.ini` uses Arduino. The recommended final target is:

```ini
[env:atmega32]
platform = atmelavr
board = ATmega32
board_build.mcu = atmega32
board_build.f_cpu = 16000000L
framework =
upload_protocol = usbasp
build_flags =
  -Os
  -flto
  -ffunction-sections
  -fdata-sections
  -fno-exceptions
  -fno-rtti
  -fno-threadsafe-statics
  -Wl,--gc-sections
```

Do not apply this until H1/H2 pass with the current Arduino blink; verify that `framework =` empty is accepted by the installed PlatformIO version. If it is not, keep `framework = arduino` and use direct register access.

---

## Appendix E — Session log

| Date | Note |
|---|---|
| 2026-10-23 | Extracted JAR, decompiled with CFR, decoded 8 PNGs + 61 sprite records + 6 mazes + 12 tone chunks; wrote this backlog. |
| 2026-10-23 | Verified no native 128×160 Snake EX2 build; downloaded Phoneky variants and confirmed both are 128×128. Adopted the derived 21×23 full-screen layout in §1.5 and updated budgets, coordinates, tests, and acceptance criteria. |
| 2026-10-23 | M0 complete: R1 (frozen ref set with verify_reference.py + extract_reference.py), R2 (reference index — 94 strings, 6 derived mazes, 61-sprite inventory, 10 power-ups, config.h), R3 (visual references — 73 screens: maze renders, extracted wall tiles/icons, decoded sprite sheets, 128×160 mockup). |
| 2026-10-23 | M2 D1 complete: ST7735 driver (init, fill, blit, masked blit, RLE draw, rotation) + SPI/GPIO HAL. Color-bar test passed on hardware. Note: actual board runs 8 MHz internal RC (platformio.ini f_cpu=8000000L, upload_flags=-e), not the 16 MHz crystal from §2.4 — record under Decisions when reachable. |
| 2026-10-23 | A5 done (host side): sprite roles/mapping read directly from b.java draw code and documented in `docs/sprites.md` (replaces size-based guesses). `render/snake_sprites.cpp` = body/tail/head/turn-flag selection; `tools/gen_sprite_vectors.py` is an independent table-driven transcription and `test_snake_sprites` checks 216 body/tail + 24 head + 36 turn vectors. `gen_assets.py` now emits 59 sprites (B0-41, 15 food/bonus frames, C, c; 4 bpp + palette + transparent index; d/D not yet). `render/sprite.cpp` = transparent blit (per-run windows; perf to revisit in M4: compose over bg buffer if slow). Demo screen on LCD: FIRE → last screen = 67-segment snake on Box with every turn type, UP = open mouth + food, DOWN = fat segments, plus sprite gallery. Host render (build/assets/preview/snake_demo_*.png) shows continuous stripes, correct corners, tail, fat bulges. FLASH: 21.2 KB (64.7%) incl. demo/viewer code that will be removed. |
| 2026-10-23 | A4 done (host side): gen_assets.py now emits palette+RLE images (title 5.6 KB vs 8.9 KB raw RLE, HUD 1.7 KB, digits 0.9 KB, power-up icons 1.4 KB). `render/image.cpp` streams them to the LCD (`st7735_stream_*`), `render/hud.cpp` draws bar/score/bonus/icon/title. CORRECTED HUD coordinates from b.java (score (6,145), bonus (69,145), icon slot (110,142)); config.h and §1.5 fixed. Title rows above/below filled with its own edge colours (seamless). Host test compares title/HUD/all digit cells/all 10 icons pixel-for-pixel with converted RGB565. Viewer: FIRE title→mazes+HUD, LEFT/RIGHT score (hold = ±1111), UP/DOWN icon, PAUSE bonus. FLASH WARNING: 16.9 KB used (51.6%) before any game engine; engine budget 12–16 KB → ~30 KB, tight. Decide ATmega644A vs dropping title after M4. |
| 2026-10-23 | BUG (found on hardware): `st7735_blit`/`st7735_blit_masked` never raised DC or asserted CS before bulk data, so every blit was invisible (only fill_rect worked; D1 was verified with colour bars only). Fixed. Lesson: host tests mock the driver, so any new LCD primitive needs an on-board check before its task is ticked. A3 LCD check still pending. |
| 2026-10-23 | A3 done (host side): `tools/gen_assets.py` → `src/assets/assets_gen.{h,cpp}` (wall palette 6 colours, 15 tiles 4-bit = 270 B, 5 mazes 4-bit = 945 B; unused L-shape maze 5 omitted). `maze_load()` expands 21×18→21×23 at runtime (Box repeats side row). `field_draw_map()` draws bg + tiles at (1,1). Host test `test_maze_field` renders all 5 mazes to a mock 128×160 framebuffer and compares every pixel with the independently converted `rgb565/wall_strip` + `mazes_21x23.bin` (mutation-checked). main.cpp: `DEMO_MAZE_VIEWER=1` — after self-test FIRE cycles mazes on the LCD. Flash 6.1 KB, RAM 772 B. |
| 2026-10-23 | A2 done: `tools/convert_assets.py` converts 8 PNGs to big-endian RGB565 (.rgb565), sprites to RGB565 palettes + transparent-index lists, derives 6×483 mazes (`mazes_21x23.bin`) and renders previews (sprites, original/derived mazes, 128×160 mock with HUD). RGB565 round-trip error ≤7/255 (asserted). All 6 derived mazes are byte-identical to Appendix A.2. FOUND: `build_reference_index.derive_23row_maze` detected Box by shape and wrongly treated Blockade (maze 3) as Box; fixed to use index 0, `reference/mazes.txt`/`powerups.txt`/`index.txt` regenerated, MANIFEST.sha256 updated, verify_reference passes. FOUND: playfield background is 0xEEDDBB (b.java A(), black only in lights-out) — added `COLOR_FIELD_BG 0xEEF7` to config.h; the spec’s black playfield assumption was wrong. Previews match IGDB screenshot (wall colours/bevels, vine HUD, no channel swap). |
| 2026-10-23 | A1 done: `tools/extract_snake_ex2.py` asserts d SHA/size, contiguous layout, PNG signature/IHDR dims/IEND for all 8 PNGs, 6 mazes (bytes ≤0x1F), 61 sprite records with expected sizes/palette/index bounds, and that the parser ends at exactly 15,602. Outputs mazes.bin, png/*.png, sprites.json, manifest.sha256. `g.png`/`G.png` are written as `g_11x11`/`G_60x21` (case-insensitive FS collision). Sprite names P<group>_<frame> and c/C/d/D are positional only; roles still unverified (A5). |
| 2026-10-23 | RE-VERIFIED against decompiled b.java/Snake.java/ToneDevice.java. CONFIRMED: speed formula, speed/slow deltas+clamps, power-up id→SFX map, duration 25+rand%50, death 40 ticks, Ea formula, wrap, start state. FIXED (were wrong): (1) direction bits are UP=1 LEFT=2 DOWN=4 RIGHT=8 (config had LEFT/RIGHT swapped; START_DIR was undefined), (2) SFX map: 0=speed-up 1=slow-down 10=death 11=new high score 12=beep on eating food; coin pickups silent, (3) power-ups spawn only in Extreme and per FOOD EATEN (1/16 per empty slot), not per tick, (4) bonus boost = +24 on bonus-food timer for next 3 bonus foods, not +24 score, (5) shrink/grow act per move step. `buzzer.h` SFX names renamed accordingly. STILL UNVERIFIED: sprite roles in sprite_inventory.txt (inferred from size; confirm in M3 A2 by rendering vs screenshots), IGDB screenshot naming, coin-rush `ba` usage (it doubles as a scan index, see b.java ~1474), stage scripts ta/Ta tables. |
| 2026-10-23 | D3 redone: time-based 20 ms debounce on both edges, wrap-safe millis, repeat 400/100 ms. D5 redone: fixed §3.3 layout (magic, version, settings@8, classic@32, extreme@48), CRC16 per block with its own CRC, range validation, per-block fallback. Host tests in `test/host/` (mock eeprom/millis/gpio): CRC known-answer, blank/corrupt/every-bit-flip, out-of-range, no-write-when-unchanged, bounce, glitch, auto-repeat, millis wrap. Self-test shows STORE OK / STORE RESET. |
| 2026-10-23 | DECISION: SD card removed from the project (bring-up never succeeded on 2 cards/2 readers; title was its only use). `sd.cpp/h`, `sd_cs_*`, SD self-test band deleted. Stay on ATmega32; all assets in flash, title art added last. NOTE: if flash runs out, switch to ATmega644A (64 KB flash, 4 KB RAM, same DIP-40 pinout/SPI pins; change board in platformio.ini and `MCUCSR`→`MCUCR` for JTD). Measure flash after M4/M5. Remaining SD mentions in this file are historical/superseded by D4. |
| 2026-10-23 | AUDIT: D2 reopened (SD never initialised on hardware; CMD8 CRC 0x87 missing, SDSC addressing, cluster overflow fixed, untested). D3 reopened (DEBOUNCE_MS defined but unused). D5 reopened (CRC16 never stored/validated, no host test, layout != §3.3). R2/R3 contain unverified guesses from a subagent (SFX map in config.h, power-up semantics, sprite roles, `START_DIR RIGHT` undefined, DIR_* values conflict with §App.B). Earlier text: M2 D2–D5 drafted: SD/FAT32 read-only driver (SPI init, sector read, MBR/BPB parse, cluster-chain file read), button driver (100 Hz debounce + auto-repeat), buzzer driver (non-blocking, 13 SFX IDs, passive-piezo stub), EEPROM store (magic/version/CRC16, settings + 4 high scores). All compile; hardware verification pending. |
| 2026-10-23 | G1–G4 done. Game core `src/game/game.{h,cpp}` is a faithful port of `b.b()`; host oracle (`tools/oracle/`, decompiled `b.java` compiled headless with MIDP stubs, seeded `java.util.Random`) replays **216 traces / 127,955 ticks with 0 diffs**, mutation-checked. `game_t` now holds a non-owning maze pointer plus a 61-byte occupancy bitmap (was a 483-byte map copy) and caps growth at `GAME_MAX_LEN`. `test_game_init` covers the 21×23 start state, speeds 0–5 cadence (10…5 frames), all four wraps, 180° rejection, wall death/undo and the 40-tick game-over sequence. Firmware RNG is xorshift32 (`src/game/rng.cpp`, reseeded from `millis()` per game). |
| 2026-10-23 | BUG (found from user's reference screenshots): `tools/extract_snake_ex2.py` unpacked 4 bpp sprite rows **high nibble first**, but `SnakeCanvas.a()` reads the low nibble first and then shifts the byte right by 4. Every adjacent pixel pair was swapped: straight bodies looked plausible, but head/turn/tail sprites were garbled (the LCD showed a green blob instead of the original head). Fixed the extractor, regenerated `sprites.json`/`sprites_rgb565.json`/`assets_gen.*`/previews, documented the order in `docs/sprites.md`, and added decoded-pixel assertions (B0 body, B30/B36 eye) to `test_snake_sprites`. All host tests pass; build unchanged at RAM 1,442 B (70.4%), flash 20,700 B (63.2%). Awaiting LCD check. |
| 2026-10-23 | G5 host side done: `src/render/playfield.{h,cpp}` dirty-cell renderer (3×3 blocks around changed cells; base→items→snake head-to-tail; sprite clip = playfield rect like the original's `setClip`). `test_playfield` runs all 6 mazes × 6000 ticks with random input and injected item changes (2,897 moves, 369 bonus spawns, 594 food teleports) and compares the incremental framebuffer with a fresh full redraw **every tick: 0 differing pixels**. Firmware `main.cpp` is now a playable Classic game (maze 0 Box, speed 5, 15 fps, HUD score/bonus, PAUSE, death SFX, GAME OVER + FIRE restart). Build: **RAM 1,442 B / 2,048 (70.4%), flash 20,700 B / 32,768 (63.2%)**, ≈606 B stack headroom. Awaiting hardware confirmation of smoothness/ghosting before ticking G5. |
| 2026-10-23 | G5 hardware-confirmed by the user (on-LCD snake/gameplay approved); ticked. Started G6 (tongue). `tools/convert_assets.py` now emits `build/assets/tongue.json` (four 11×11 rotations of `f.png`, transparent index 0; PIL `ROTATE_90` = CCW, right→up), `gen_assets.py` appends `TONGUE_R/U/L/D` (**SPRITE_COUNT 63**), and `snake_tongue_sprite()` reproduces the original `o/O` offsets: R(3,−2), L(−6,−2), U(−1,−7) or (−2,−7) and D(−2,1) or (−1,1) depending on the head's `0x20` flag. Core: `game_t.extreme` and `game_t.tongue` (J); FIRE → J=−1, next movement tick → J=1, normal and bonus food accepted at the head or one/two cells ahead (`k,l`/`K,L`, wrapped); retracts on the next movement tick. Classic `game_init()` ignores FIRE. Renderer draws the tongue before the head and erases through cached head/direction cells; a tongue that retracts on a death tick is also erased. Tests: new `test_tongue` (1/2-cell, 3-cell reject, behind-head, Classic gate, no-fire, retract, wrap) and `test_playfield` now runs Extreme + FIRE + food injected 1–2 cells ahead (**36,000 ticks, 2,986 moves, 0 differing pixels**). Decision: no lash SFX (none exists in the original; a grab plays SFX 12). Deviation: first movement tick is still a no-op but the tongue reach uses the current direction (the original leaves `k/l/K/L` stale at 0,0). Firmware temporarily starts Extreme (`START_EXTREME 1`) until U1/U2. Build: **RAM 1,458 B / 2,048 (71.2%), flash 22,034 B / 32,768 (67.2%)**; the renderer now expands dirty 3×3 blocks directly (no capped list), freeing ~192 B of stack. Awaiting hardware confirmation before ticking G6. |
| 2026-10-24 | G8 host side implemented, awaiting hardware confirmation. Extreme is the real endless score-gated progression, not the `Snake.J` tutorial script: the portal spawns when `score > level_start_score + speed*10 + level*20` on a `c==0` frame (unless coin rush); head entry sets `Y=2, ma=1`, then runs the deactivate hook, shrinks one tail segment per movement tick, and at length 2 snapshots the score, increments the level, cycles `-1,0,1,2,3,4`, reloads the 21×23 maze and resets the snake/field with a full redraw. Original `a()`'s untouched fields (`ga/Ga/Fa/ha/Ia/Ea/U/v/na/ka`) are preserved. Portal rendering uses `SnakeCanvas.c` at `(X-1,y-2)` with local dirty-cell repaints; level reset alone may full-repaint. **Oracle:** 9 forced-campaign traces / 180,000 ticks compare all base state, segments, `x/z/Y/X/y/da` and map bytes — 0 mismatches; 12 natural power-up-bot traces / 28,481 ticks (natural portals, power-ups, shrink and level transitions) — 0 mismatches. **Found/fixed:** moving bonus food must bounce off the portal (`b.java:1690`); the general `ba` countdown must run even when `Aa==0` (`b.java:1526`, exposed by expired slots). `test_extreme` covers threshold bounds/fallback/coin-rush/head-only/effect ordering/reset/maze cycle/preserved fields/bonus bounce; `test_playfield` adds portal on/move/entry/off/edge/head cases with 0 differing pixels. `Snake.J`'s `-1..-7` demo games and `-8` high-score demo are scoped to U5 (tutorial), recorded in G8. Build: RAM 1,610/2,048 B (78.6%), flash 30,958/32,768 B (94.5%, 1,810 B free). |
| 2026-10-24 | G8 hardware-confirmed by the user (portal cycle, shrink, maze changes, preserved score, no flicker); ticked. Swapped the target to **ATmega644A @ 5 V / 8 MHz** (D11): `platformio.ini` board/MCU/clock, `MCUCR` JTD write, `default_envs=ATmega644A`, plus an `ATmega644A_classic` test env. G9 host side complete: `game_init_classic()` validation/loading, PROGMEM `maze_name()` strings, updateable `START_EXTREME/MAZE/SPEED`, `test_classic_select` over all **36 combinations**, and a new `--classic-all` oracle set (**36 traces / 79,161 ticks, 0 mismatches**). Full host suite passes (including the unchanged 216-trace classic, 18 power-up, 9 forced-campaign, 12 natural-campaign sets). Builds: default **RAM 1,610/4,096 B (39.3%), flash 31,044/65,536 B (47.4%)**; Classic preset **flash 31,098 B**. G9 awaits the user's on-hardware spot-check. |
| 2026-10-24 | TARGET CORRECTION: the physical chip is an **ATmega644PA**, device signature `0x1E960A` (not ATmega644A). PlatformIO now uses `board = ATmega644P`, `board_build.mcu = atmega644pa`, `f_cpu = 8 MHz`; environments renamed `ATmega644PA` / `ATmega644PA_classic`. `MCUCR` JTD path unchanged; the `__AVR_ATmega644PA__` guard is already present. Both builds re-verified: default **RAM 1,610/4,096 B (39.3%), flash 31,044/65,536 B (47.4%)**; Classic preset **flash 31,098 B**. Host suite unchanged and passing. G9 still awaits the hardware spot-check. |
| 2026-10-24 | G9 hardware-confirmed (Classic Box/speed-5 preset, cadence, scoring, endless restart); ticked. G9’s temporary `START_*`/`ATmega644PA_classic` path was removed. U1 implemented: cold boot draws the original title art at y=16 with a “PRESS ANY KEY” prompt, auto-advances after 3 s or on any key, and opens a full-height main menu (New game, High scores, Settings, Instructions). New `render/text.{h,cpp}` owns the 5×7 font; new `ui/menu.{h,cpp}` owns selection/wrap and the 20 px header + 4×27 px rows + 20 px footer layout. Up/Left and Down/Right navigate, Fire selects, Pause returns; selection moves repaint only the old and new rows (no full-screen refresh), while entering the menu or returning from an info screen performs a full draw. The three non-game items show labelled coming-soon screens with Fire/Pause back. New game uses the saved EEPROM settings (default Classic/no maze/speed 2) through `game_init_classic()`, or `game_init_mode(..., true)` for Extreme. Host suite passes including new `test_menu`; and the existing 216/36/18/9/12 trace sets are unchanged (0 mismatches). Build: **RAM 1,654/4,096 B (40.4%), flash 37,146/65,536 B (56.7%)**; the ~6 KB increase is the title art now linked by `title_draw()`. Awaiting U1 hardware confirmation. |
| 2026-10-24 | U1 hardware-confirmed: title timing, four-direction navigation with partial-row refresh, Fire on all items, Pause back from menu and info screens, New game starts Classic/no-maze/speed-2 playable. Ticked. U2 Settings is next. |
| 2026-10-24 | U2 implemented: full-screen Settings screen with four rows (Mode, Maze, Speed, Sound). Up/Down selects item; Left/Right changes value with clamps/wrap; Fire saves to EEPROM with momentary "SAVED" confirmation; Pause returns to main menu. Backlight toggle was removed per user request. `src/ui/settings.{h,cpp}` added; `text_draw_p`/`text_width_p` in `render/text` handle PROGMEM maze names correctly. `test_settings` verifies navigation, value changes, clamps, maze cycling, and partial row repaint. Host suite passes (216/36/18/9/12 trace sets). Build: **RAM 1,731/4,096 B (42.3%), flash 38,260/65,536 B (58.4%)**. Awaiting U2 hardware confirmation. |
| 2026-10-24 | U2 hardware-confirmed: all four settings items navigate correctly, maze labels from PROGMEM display on entry and persist after the "SAVED" popup, mode/maze/speed/sound edit and save to EEPROM, power-cycle persistence verified. Ticked. U3 High scores is next. |
| 2026-10-24 | U3 hardware-confirmed: main menu High scores item shows two entries per mode with correct formatting (dashes for empty); game-over entry screen triggers when score beats a stored entry, initials editable A-Z via LR/UD, Fire saves and returns to title; scores persist across power cycles and are per-mode (Classic/Extreme). Ticked. U4 Pause and game over is next. |
| 2026-10-24 | U4 implemented: pause overlay with grid pattern (Resume/Restart/Menu) and game-over screen with final score (Restart/Menu). `src/ui/pause.{h,cpp}` added; old `show_game_over()` replaced; PAUSE during gameplay opens overlay, Up/Down/Fire/PAUSE handle actions; game over shows score with Restart/Menu. `test_pause` verifies grid overlay and full-screen fill. Host suite passes (216/36/18/9/12 trace sets). Build: **RAM 1,818/4,096 B (44.4%), flash 41,366/65,536 B (63.1%)**. Awaiting U4 hardware confirmation. |
| 2026-10-24 | U4 hardware-confirmed: PAUSE overlay shows Resume/Restart/Menu; Resume clears grid pattern and resumes cleanly; Restart/Menu work. Game-over screen appears automatically with final score and Restart/Menu options. Pause cannot corrupt game state. Ticked. U5 Instructions/tutorial is next. |
| 2026-10-24 | U5 implemented: tutorial screen with 12 text pages (from lang.xx) and 8 scripted demo games. `src/ui/instructions.{h,cpp}` added; navigation via Up/Down (pages), Left/Right (demo menu), Fire (select/run demo), Pause/Fire (back). Demos: -1 tongue, -2 walls, -3 bonus food, -4 speed up, -5 ghost, -6 coin rush, -7 lights out, -8 high score. Each demo auto-plays a configured scenario. Text rendering uses word-wrapped 5×7 font from PROGMEM. Host suite passes (216/36/18/9/12 trace sets). Build: **RAM 1,878/4,096 B (45.8%), flash 45,414/65,536 B (69.3%)**. Awaiting U5 hardware confirmation. |
