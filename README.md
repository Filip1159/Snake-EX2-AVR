# Snake EX2 — ATmega644PA hardware port

A faithful standalone port of Nokia's **Snake EX2** (Nokia 3300, 2003, MIDlet v1.19) to an
ATmega644PA, an ST7735 128×160 SPI LCD and six buttons. The original Java game logic,
mazes, sprites, HUD, sounds, menus and Extreme-mode progression were reconstructed from the
decompiled MIDlet and a frozen copy of its `d` resource; the port keeps the original 15 fps
movement cadence, edge wrapping, power-ups, portal/shrink cycle, high-score table and sound
effects.

![Target](https://img.shields.io/badge/MCU-ATmega644PA%20%40%208%20MHz-blue)
![Display](https://img.shields.io/badge/display-ST7735%20128%C3%97160-green)

## Hardware

| Part | Notes |
|------|-------|
| ATmega644PA-PU | DIP-40, 5 V, 8 MHz (internal RC as built; external crystal optional), 64 KB flash / 4 KB SRAM / 1 KB EEPROM |
| ST7735 128×160 SPI LCD | 3.3 V logic; level-shift the 5 V MCU outputs |
| 6 tactile buttons | PA0–PA5, active low, internal pull-ups |
| Active buzzer or passive piezo | PB0; passive mode uses Timer2 |
| USBasp | 6-pin ISP header |

Full pin table, BOM, power/level-shifting notes, fuse values and USBasp wiring:
[`docs/hardware.md`](docs/hardware.md).

## Quick start

```sh
# Build the firmware
pio run -e ATmega644PA        # or: make firmware

# Run the host tests (assets, visual diff, logic/oracle suite)
make test

# Flash with USBasp
pio run -e ATmega644PA -t upload   # or: make upload
```

A clean checkout builds without regenerating assets: `src/assets/assets_gen.{h,cpp}` (from
`reference/d`) and `sounds_gen.{h,cpp}` (from `reference/sounds.nok`) are checked in. To
regenerate them:

```sh
make assets                    # images/mazes/sprites from reference/d
python3 tools/gen_sounds.py    # sound tables from reference/sounds.nok
make assets-test
```

Build, flash, fuse, EEPROM and recovery details:
[`docs/build.md`](docs/build.md).

## Controls

| Button | Game | Menus |
|--------|------|-------|
| Up / Down / Left / Right | Steer the snake | Navigate |
| Fire | Tongue / confirm | Select / confirm |
| Pause | Pause / back | Back |

The game starts at the title screen, then the main menu. Classic mode lets you choose a maze
and speed; the tongue works in both modes. Extreme mode is the endless score-gated campaign
with power-ups, the portal and maze cycling. Pause offers Resume, Restart and Menu. The
high-score table stores two scores for each mode (four entries total) in EEPROM.

## Buzzer mode

The default is an **active buzzer** (`FEATURE_PASSIVE_BUZZER 0` in `src/config.h`). If you
wire a passive piezo, either set the flag to `1` and rebuild, or call
`buzzer_set_mode(BUZZER_MODE_PASSIVE)` at runtime. The driver plays all 13 original sound
sequences non-blocking; the settings menu can mute sound.

## Repository layout

| Path | Contents |
|------|----------|
| `src/game/` | Original game logic: movement, collision, food, power-ups, Extreme/portal |
| `src/render/` | Dirty-cell playfield renderer, sprites, HUD, high scores, menus |
| `src/drivers/` | ST7735, buttons, buzzer, EEPROM |
| `src/assets/` | Generated maze/sprite/image/sound tables (do not hand-edit `*_gen.*`) |
| `src/ui/` | Title, menu, settings, pause, credits, high scores |
| `tools/` | Asset extractor/converter/generator, golden and visual tests |
| `test/host/` | Native test programs and stub HAL/Arduino layer |
| `reference/` | Frozen original `d` resource, decompiled classes, screenshots |
| `docs/` | Hardware/build reference, sprite notes, original reference notes |
| `BACKLOG.MD` | Full implementation plan, decisions, status and session log |

## Status

G1–G9, U1–U5, S1–S2, Q1–Q3 and P1–P2 are complete; S3 is implemented and host-verified,
with the hardware check pending. Q5's host/static frame analysis and stack instrumentation are
complete; the on-hardware stack reading is still to be measured. Q4 (soak test) and P3 (final
acceptance) require the real board. See `BACKLOG.MD` for the detailed status and the remaining
hardware checklist.

## Notes

- No SD card is used; all assets are in flash and settings/high scores are in EEPROM.
- The firmware targets 15 fps; normal dirty-cell frames are under 50 ms. Full redraws
  (level start, lights-out, death-blink transitions) are event-only and take about 109 ms at
  the 4 MHz SPI clock.
- For the Q5 stack check, `make stack-measure` builds a debug image that shows
  `STACK FREE: NNNN` on the pause screen; see `docs/build.md` §9.
- The original game's assets and decompiled code are included for preservation and
  interoperability research. Snake EX2 is a Nokia trademark; this is a non-commercial
  fan port.
