# Build, test and flash

## 1. Prerequisites

- **PlatformIO CLI** (`pio`) or the PlatformIO IDE extension.
- **Python 3** with [Pillow](https://python-pillow.org/) (`python3 -m pip install pillow`)
  for the asset pipeline and visual tests.
- A C++17 host compiler (`g++`) for the native tests.
- **USBasp** programmer and, on Windows, a libusbK/WinUSB driver (Zadig).
- `avrdude` for manual fuse/flash commands. PlatformIO installs its own copy when you
  upload; a system `avrdude` is convenient for fuse work.

The project is configured in `platformio.ini` as:

```ini
[env:ATmega644PA]
platform = atmelavr
board = ATmega644P
board_build.mcu = atmega644pa
board_build.f_cpu = 8000000L
framework = arduino
upload_protocol = usbasp
upload_flags = -e
```

`board = ATmega644P` supplies the 64 KB/4 KB memory layout; `board_build.mcu` selects the
644PA device. The Arduino/MightyCore core provides `millis()`, `delay()` and the AVR
`pgm_read_*` helpers; the game itself is bare-metal C/C++.

## 2. Build

```sh
# PlatformIO
pio run -e ATmega644PA

# or the Makefile wrapper
make firmware
```

The image is written to `.pio/build/ATmega644PA/firmware.hex` (and `.elf`). A successful
build prints the memory usage, for example:

```
RAM:   [=====     ]  46.3% (used 1896 bytes from 4096 bytes)
Flash: [=======   ]  71.1% (used 46580 bytes from 65536 bytes)
```

The checked-in `src/assets/assets_gen.{h,cpp}` are generated from `reference/d`, and
`src/assets/sounds_gen.{h,cpp}` from `reference/sounds.nok`; a clean checkout builds without
regenerating them. If you change the reference assets, regenerate:

```sh
make assets                    # extract + convert + generate images/mazes/sprites
python3 tools/gen_sounds.py    # decode reference/sounds.nok
make assets-test               # verify the golden manifests
```

Never hand-edit `assets_gen.*` or `sounds_gen.*`.

## 3. Host tests

```sh
make test            # asset golden test, visual diff, then the native suite
# or
sh test/host/run.sh
```

`make test` runs:

- `tools/test_assets.py` — regenerates the asset pipeline in a temp directory and compares
  the SHA-256/CRC32 golden manifests, maze bytes, sprite counts and `assets_gen.*`.
- `tools/test_visual.py` — builds the stub-LCD renderer and compares the 128×160 frame with
  the independent asset preview and the documented layout mockup.
- The native logic tests and oracle traces (movement, wrap, collision, food, score, portal,
  power-ups, EEPROM CRC, menus, high scores, pause, rendering).

The frame-budget check prints the pixel-count estimate at 4 MHz SPI:

```
frame_budget: max normal 12.75 ms, avg 18.72 ms, full-redraw 108.79 ms ...
```

Normal dirty-cell frames meet the 50 ms budget; full redraws are event-only (level start,
lights-out, death-blink transitions).

## 4. Flash with PlatformIO

```sh
pio run -e ATmega644PA -t upload
# or
make upload
```

PlatformIO auto-detects the USBasp. If more than one USB device is present, pass the port:

```sh
pio run -e ATmega644PA -t upload --upload-port usb
```

`upload_flags = -e` performs a chip erase before programming. With the recommended
`HFUSE=0xD7` (`EESAVE` enabled), the EEPROM settings and high scores survive the erase. If
you change the high fuse to `0xDF`/`0xFF` (EEPROM not retained), the erase will also clear
the EEPROM.

## 5. Flash manually with avrdude

```sh
avrdude -p m644p -c usbasp -P usb \
  -U flash:w:.pio/build/ATmega644PA/firmware.hex:i
```

To erase first (optional; `-e` is the chip erase):

```sh
avrdude -p m644p -c usbasp -P usb -e \
  -U flash:w:.pio/build/ATmega644PA/firmware.hex:i
```

If the target clock is wrong or the chip is new, add `-B 10` for a slow ISP clock:

```sh
avrdude -p m644p -c usbasp -P usb -B 10 \
  -U flash:w:.pio/build/ATmega644PA/firmware.hex:i
```

## 6. Fuses

> Use the explicit `avrdude` commands below. PlatformIO's dynamic fuse generator does not
> list the `atmega644pa` MCU (only `atmega644p`), so `pio run -t fuses` is not used here.

Read before writing:

```sh
avrdude -p m644p -c usbasp -P usb \
  -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h
```

Recommended values (see `docs/hardware.md` §5 for the bit meanings). The as-built board uses
the internal 8 MHz RC oscillator; use `0xF7` only if an external 8 MHz crystal is fitted.

```sh
# as-built internal 8 MHz RC
avrdude -p m644p -c usbasp -P usb \
  -U lfuse:w:0xE2:m -U hfuse:w:0xD7:m -U efuse:w:0xFD:m

# optional external 8 MHz crystal
avrdude -p m644p -c usbasp -P usb \
  -U lfuse:w:0xF7:m -U hfuse:w:0xD7:m -U efuse:w:0xFD:m
```

| Fuse | Value | Notes |
|------|-------|-------|
| LFUSE | `0xE2` | Internal 8 MHz RC, CKDIV8 off (as built) |
| LFUSE (external) | `0xF7` | 8 MHz external crystal, CKDIV8 off |
| HFUSE | `0xD7` | JTAG off, SPIEN on, EEPROM retained, no bootloader |
| EFUSE | `0xFD` | BOD 2.7 V (`0xFC` = 4.3 V, `0xFF` = off) |
| Lock | `0xFF` | Unlocked |

After writing, read them back and confirm `E2 D7 FD` (internal RC) or `F7 D7 FD` (external
crystal).

## 7. EEPROM backup and restore

Settings and high scores live in the ATmega644PA's 1 KB EEPROM. Back it up before a fuse or
chip-erase experiment:

```sh
avrdude -p m644p -c usbasp -P usb -U eeprom:r:eeprom.hex:i
avrdude -p m644p -c usbasp -P usb -U eeprom:w:eeprom.hex:i
```

The firmware validates the EEPROM with CRC-16/CCITT and falls back to defaults if the magic
or CRC is invalid, so a blank/corrupt EEPROM is safe.

## 8. Recovery

- **Wrong clock fuse (chip not detected):** feed a known clock into XTAL1 (e.g. 1–8 MHz
  square wave) or use `avrdude -B 10`; then restore the matching low fuse (`0xE2` for the
  internal 8 MHz RC, `0xF7` for an external 8 MHz crystal). If the chip still does not
  respond, use a high-voltage parallel programmer.
- **SPIEN disabled:** ISP programming is disabled. A high-voltage parallel programmer (or a
  fuse-reset programmer) is required to re-enable SPIEN. This is why the recommended
  `HFUSE=0xD7` keeps SPIEN enabled.
- **JTAG enabled and PC2–PC5 used as I/O:** the pins will not behave as GPIO. Disable JTAG
  by restoring `HFUSE=0xD7` over ISP; if the JTAG pins are not used, this is harmless.
- **Bad flash / boot loop:** erase and reflash with the commands above. The title screen
  appears in under 2 seconds; there is no bootloader and no serial console.
- **Display blank:** check the 3.3 V rail, backlight transistor, CS/DC/RST wiring and the
  `TFT_OFFSET_*`/`TFT_BGR`/`TFT_INVERT` settings in `src/config.h`.

## 9. Stack measurement (Q5)

The release build does not include the stack-painting code. To measure the high-water mark:

```sh
make stack-measure          # build with -DSTACK_MEASURE=1
make stack-measure-upload   # build and flash the debug image
```

Alternatively, set `#define STACK_MEASURE 1` in `src/config.h` and rebuild normally.

Play the longest game you care about (Extreme with a long snake, plus the menus), then
pause or die. The pause screen shows:

```
STACK FREE: NNNN
```

`NNNN` is the number of unused bytes between `__bss_end` and the deepest stack address
seen since boot. The Q5 acceptance threshold is **≥ 512 B**. A value close to the full
free-SRAM size means the stack never reached the painted area, so the reading is a lower
bound (still a pass if it is ≥ 512 B).

After measuring, restore the release image:

```sh
make firmware
make upload
```

The debug build adds about 376 B of flash and 19 B of RAM in the current image; the
release build is unchanged. The instrumentation is compiled out when `STACK_MEASURE` is 0
(the default in `src/config.h`).

## 10. SD card

There is **no SD card** in this build. All assets (mazes, sprites, fonts, sounds) are
compiled into flash, and settings/high scores are stored in the internal EEPROM. No card
formatting is required. The original backlog's SD-format step is obsolete for the current
hardware.
