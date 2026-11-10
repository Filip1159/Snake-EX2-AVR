# Build, test and flash

## Prerequisites

- [PlatformIO](https://platformio.org/) (`pio`)
- `avrdude` and a USBasp (on Windows, a libusbK/WinUSB driver via Zadig)
- For `make test`: `g++`

`platformio.ini` uses `board = ATmega644P` (memory layout) with `board_build.mcu = atmega644pa`
and `F_CPU = 8000000L`.

## Build and flash

```sh
pio run -e ATmega644PA              # or: make firmware
pio run -e ATmega644PA -t upload    # or: make upload
```

The upload chip-erases first (`-e`); with `HFUSE=0xD7` the EEPROM survives. Manual flashing:

```sh
avrdude -p m644p -c usbasp -P usb -U flash:w:.pio/build/ATmega644PA/firmware.hex:i
```

Add `-B 10` for a slow ISP clock if the chip is new or the clock fuse is wrong.

## Fuses

Read, then write (use `0xF7` for `LFUSE` only with an external 8 MHz crystal):

```sh
avrdude -p m644p -c usbasp -P usb -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h
make fuses     # writes E2 D7 FD
```

Meanings are in [`hardware.md`](hardware.md). PlatformIO's `fuses` target does not know the
`atmega644pa`, so use avrdude. A wrong clock fuse can be recovered by feeding a clock into
XTAL1 or with `-B 10`; disabling SPIEN needs a high-voltage programmer.

## EEPROM

Settings, high scores and the saved game live in the 1 KB EEPROM, protected by CRC-16; a blank
or corrupt EEPROM falls back to defaults. Back it up with
`avrdude -p m644p -c usbasp -P usb -U eeprom:r:eeprom.hex:i`.

## Tests

```sh
make test      # = sh test/host/run.sh
```

Builds and runs the native test programs (drivers, rendering, menus, game logic, save game)
against a stub display.

## Generated assets

`src/assets/*_gen.*`, `wallpaper`, `statusbar`, `trophy` and the font `.inc` files are generated
and checked in; never hand-edit them. The scripts that still ship are `tools/gen_wallpaper.py`,
`gen_statusbar.py`, `gen_fonts.py` and `gen_trophy.py` (they read the screenshots in
`tools/img/` and need `opencv-python-headless`, `numpy` and Pillow) and
`gen_sprite_vectors.py` (test vectors). The tools that extracted the original game's resources
are not part of the repository.

## Stack measurement

`make stack-measure-upload` builds and flashes a debug image whose credits screen
(menu → Instructions) shows `STACK FREE: NNNN`, the unused bytes between the heap end and the
deepest stack use. Anything ≥ 512 is fine. Restore the release image with `make upload`.
