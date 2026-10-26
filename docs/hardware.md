# Hardware — as-built wiring, BOM, power and fuses

This is the as-built reference for the ATmega644PA port. It supersedes the original
ATmega32/16 MHz plan in `BACKLOG.MD` §2. The current build is:

- **MCU:** ATmega644PA-PU, DIP-40, 5 V, 8 MHz (internal RC as built; an external 8 MHz crystal is optional)
- **Display:** ST7735 128×160 SPI LCD (3.3 V logic)
- **Input:** six active-low tactile buttons on PA0–PA5
- **Sound:** one active buzzer on PB0 (passive piezo supported in firmware)
- **Storage:** none — all assets live in flash; there is no SD card
- **Programming:** USBasp through a 6-pin ISP header

## 1. Pin table (ATmega644PA DIP-40)

| Pin | Port | Signal | Direction | Notes |
|----:|------|--------|-----------|-------|
| 1 | PB0 | Buzzer | out | Active buzzer or passive piezo; drive through an NPN transistor |
| 3 | PB2 | LCD_RST | out | ST7735 reset (active low) |
| 4 | PB3 | LCD_DC | out | ST7735 data/command select |
| 5 | PB4 | LCD_CS | out | ST7735 chip select (active low) |
| 6 | PB5 | LCD_MOSI | out | Hardware SPI MOSI |
| 7 | PB6 | LCD_MISO | in | Not read by the write-only driver; optional |
| 8 | PB7 | LCD_SCK | out | Hardware SPI clock (F_CPU/2 = 4 MHz) |
| 9 | RESET | RESET | in | 10 kΩ to 5 V; ISP RESET; optional 100 nF to GND |
| 10 | VCC | 5 V | power | 100 nF decoupling |
| 11 | GND | GND | power | |
| 12 | XTAL2 | Crystal | — | Optional 8 MHz crystal; 22 pF to GND (not fitted on the internal-RC build) |
| 13 | XTAL1 | Crystal | — | Optional 8 MHz crystal; 22 pF to GND |
| 23 | PC1 | LCD_BL | out | Backlight enable; use a transistor, not the LED directly |
| 30 | AVCC | 5 V | power | 100 nF decoupling |
| 31 | GND | GND | power | |
| 32 | AREF | AREF | — | 100 nF to GND (ADC unused) |
| 35 | PA5 | BTN_PAUSE | in | Pause / back; internal pull-up, button to GND |
| 36 | PA4 | BTN_FIRE | in | Tongue / confirm; internal pull-up, button to GND |
| 37 | PA3 | BTN_RIGHT | in | Internal pull-up, button to GND |
| 38 | PA2 | BTN_LEFT | in | Internal pull-up, button to GND |
| 39 | PA1 | BTN_DOWN | in | Internal pull-up, button to GND |
| 40 | PA0 | BTN_UP | in | Internal pull-up, button to GND |

Spare pins: PB1 (2), PD0–PD7 (14–21), PC0 (22), PC2–PC5 (24–27), PC6/PC7 (28–29),
PA6/PA7 (33–34). **PC2–PC5 are JTAG pins**; the recommended high fuse disables JTAG so
they can be used as GPIO, but the firmware does not use them. Do not enable JTAG and then
try to use PC2–PC5 as I/O.

### ISP header (2×3, standard AVR)

| Pin | Signal | ATmega644PA pin |
|----:|--------|-----------------|
| 1 | MISO | 7 (PB6) |
| 2 | VCC | 5 V (target) |
| 3 | SCK | 8 (PB7) |
| 4 | MOSI | 6 (PB5) |
| 5 | RESET | 9 |
| 6 | GND | 11/31 |

If the board has its own 5 V supply, connect only GND, MISO, SCK, MOSI and RESET from the
USBasp. Do not connect USBasp VCC at the same time as the board supply unless the USBasp is
jumpered for 5 V and the board is not otherwise powered.

## 2. Bill of materials

| Qty | Part | Notes |
|----:|------|-------|
| 1 | ATmega644PA-PU | DIP-40, 64 KB flash, 4 KB SRAM, 1 KB EEPROM |
| 1 | 8 MHz crystal (optional) | Only for the external-clock configuration; HC-49S or similar |
| 2 | 22 pF ceramic (optional) | Crystal load caps to GND; not fitted on the internal-RC build |
| 1 | 10 kΩ resistor | RESET pull-up |
| 1 | 100 nF ceramic | RESET filter (optional) |
| 1 | 6-pin ISP header | 2×3, 2.54 mm |
| 1 | ST7735 128×160 SPI LCD | Verify 3.3 V logic; many modules have a 3.3 V regulator but no level shifters |
| 1 | 3.3 V LDO | AMS1117-3.3 or better, ≥300 mA; 10 µF + 100 nF |
| 1 | 5 V regulator or supply | 7805/AMS1117-5.0 or a clean 5 V 1 A supply |
| 1 | 74LVC245 or 74HC4050 | 5 V → 3.3 V level shifting for LCD CS/DC/RST/SCK/MOSI |
| 6 | Tactile buttons | Active low to GND |
| 6 | 100 nF ceramic | Optional button debounce, one per button |
| 1 | Active buzzer | Or a passive piezo; firmware supports both |
| 1 | NPN transistor | 2N2222/BC547 for the buzzer; 1 kΩ base resistor |
| 1 | 1N4148 diode | Flyback across a magnetic buzzer |
| 1 | NPN transistor | Backlight switch; 1 kΩ base resistor |
| — | 100 nF / 10 µF | Decoupling at VCC, AVCC, AREF, LCD and 3.3 V rail |
| 1 | 5 V 1 A supply | Or a regulated 5 V input |

## 3. Power and grounding

- **5 V rail:** ATmega644PA at 8 MHz draws roughly 10 mA; the LCD backlight and buzzer add
  tens of mA. A 1 A supply is ample. Put 100 nF at VCC (pin 10), AVCC (pin 30) and AREF
  (pin 32), plus a 10 µF bulk capacitor near the board.
- **3.3 V rail:** power the ST7735 from a separate 3.3 V LDO, not from the 5 V rail, unless
  the module explicitly includes a 3.3 V regulator. Add 100 nF at the display and 10 µF at
  the LDO output.
- **Common ground:** tie the 5 V and 3.3 V grounds together at the regulators. Keep the
  display and MCU ground returns short.
- **Backlight:** drive the module's `LED`/`BL` pin through an NPN transistor (PC1 → 1 kΩ
  base, emitter to GND, collector to the LED cathode; LED anode to 3.3 V through the
  module's resistor). Do not sink the backlight current directly into PC1.
- **Buzzer:** PB0 → 1 kΩ → NPN base; active buzzer between 5 V and the collector. Add a
  1N4148 across a magnetic buzzer (cathode to 5 V). A small piezo can be driven directly
  from PB0 with a series resistor, but the transistor is safer.
- **Buttons:** connect each PA0–PA5 pin to GND through a tactile switch. The firmware
  enables the internal pull-ups. Optional 100 nF from each pin to GND improves debounce;
  the firmware also debounces for 20 ms.

## 4. Level shifting (5 V MCU ↔ 3.3 V LCD)

The ST7735 is a 3.3 V device. The ATmega644PA runs at 5 V, so its outputs are 5 V:

- Use a **74LVC245** powered from 3.3 V (its inputs are 5 V tolerant) or a **74HC4050**
  for CS, DC, RST, SCK and MOSI. Direction is MCU → LCD.
- The LCD's MISO output is 3.3 V. The ATmega's 5 V VIH is about 3.0 V, so 3.3 V is usually
  recognized as high, but add a 1 kΩ series resistor and verify, or use a bidirectional
  level shifter. The current driver never reads MISO, so it may be left unconnected.
- Many "5 V compatible" ST7735 modules only regulate VCC; their logic pins remain 3.3 V.
  Check the module schematic or measure the idle levels before connecting 5 V logic.
- Never apply 5 V to a 3.3 V-only display module.

## 5. Clock and fuses

The M2 D1 hardware note records the as-built board running from the **internal 8 MHz RC
oscillator**, with `CKDIV8` disabled so the CPU and SPI run at 8 MHz. An **external 8 MHz
crystal** is optional; it gives more accurate timing but is not required. The fuse values
are otherwise the same.

| Fuse | As-built value | Meaning |
|------|----------------|---------|
| LFUSE | `0xE2` | Internal 8 MHz RC, CKDIV8 disabled, slow-rising start-up |
| LFUSE (external option) | `0xF7` | External crystal 8–16 MHz, full-swing, fast rising, CKDIV8 disabled |
| HFUSE | `0xD7` | JTAG disabled, SPIEN enabled, EEPROM retained, no bootloader, watchdog not forced |
| EFUSE | `0xFD` | Brown-out detection at 2.7 V (`0xFC` = 4.3 V, `0xFF` = disabled) |
| Lock | `0xFF` | No lock bits |

`HFUSE=0xD7` keeps `EESAVE` enabled, so a chip erase (`avrdude -e`) preserves the EEPROM
settings and high scores. If you prefer a stronger brown-out threshold, use `EFUSE=0xFC`
(4.3 V); `0xFD` (2.7 V) gives more margin on a noisy 5 V rail.

Read the current values before writing anything:

```sh
avrdude -p m644p -c usbasp -P usb \
  -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h
```

If `LFUSE` reads `E2`, the chip is on the internal RC oscillator; keep `0xE2`. If it reads
`F7`, an external 8 MHz crystal is fitted; keep `0xF7`. Do **not** write `0xF7` unless a
crystal and its 22 pF load caps are actually fitted, and do not write `0xE2` if you intend
to use the crystal.

As-built internal-RC configuration:

```sh
avrdude -p m644p -c usbasp -P usb \
  -U lfuse:w:0xE2:m -U hfuse:w:0xD7:m -U efuse:w:0xFD:m
```

External 8 MHz crystal option:

```sh
avrdude -p m644p -c usbasp -P usb \
  -U lfuse:w:0xF7:m -U hfuse:w:0xD7:m -U efuse:w:0xFD:m
```

Then read them back and confirm `E2 D7 FD` (internal RC) or `F7 D7 FD` (external crystal).
`avrdude -p m644pa` may also work; `m644p` is the portable part name for both the 644P and
644PA.

> **Warning:** a wrong clock or SPIEN fuse can make the chip temporarily unprogrammable
> over ISP. Keep the ISP header populated and read the fuses before changing them. See
> `docs/build.md` for recovery options.

## 6. USBasp procedure

1. Connect the USBasp 10-pin header to the 6-pin ISP header with a standard adapter:
   MISO→MISO, MOSI→MOSI, SCK→SCK, RESET→RESET, GND→GND. Connect VCC only if the target is
   not separately powered.
2. Power the board from its 5 V supply (or let the USBasp supply 5 V if jumpered).
3. Verify the chip is detected:

   ```sh
   avrdude -p m644p -c usbasp -P usb -B 10
   ```

   `-B 10` uses a slow SCK and helps if the clock fuse is wrong.
4. Read the fuses, write the recommended values, then flash the firmware (see
   `docs/build.md`).
5. If avrdude cannot open the USBasp:
   - **Linux:** add a udev rule for the USBasp VID/PID (`16c0:05dc`), reload udev, and add
     your user to `plugdev`.
   - **Windows:** use Zadig to install libusbK/WinUSB for the USBasp device.
   - **macOS:** no driver is normally needed; check the USB cable and `system_profiler
     SPUSBDataType`.

## 7. Display configuration

The firmware assumes a common ST7735 128×160 panel with offsets `(0,0)`, RGB order and no
inversion. If the image is shifted or the colours are wrong, adjust the following in
`src/config.h` and rebuild:

```c
#define TFT_OFFSET_X  0
#define TFT_OFFSET_Y  0
#define TFT_BGR       0   // 1 = BGR
#define TFT_INVERT    0   // 1 = invert
```

The playfield origin is `(1,1)`, the HUD is at `y=139`, and the panel is driven at 4 MHz
SPI (F_CPU/2). See `docs/build.md` for the build/flash commands.
