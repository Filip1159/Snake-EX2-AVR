# Hardware

The KiCad project is in `snake-ex2-avr-kicad.zip`. The board is minimal: an ATmega644PA on the
internal 8 MHz RC oscillator, an 8-pin ST7735 module, six buttons, a buzzer and an ISP header.

## Pin table (ATmega644PA, DIP-40)

| Pin | Port | Signal | Notes |
|----:|------|--------|-------|
| 1 | PD6 | Buzzer | 47 Ω in series; active buzzer, or passive piezo (Timer2) |
| 3 | PB2 | LCD_RST | active low |
| 4 | PB3 | LCD_DC | |
| 5 | PB4 | LCD_CS | active low |
| 6 | PB5 | LCD_MOSI | hardware SPI, F_CPU/2 = 4 MHz |
| 7 | PB6 | MISO | ISP only; the LCD is write-only |
| 8 | PB7 | LCD_SCK | |
| 9 | RESET | RESET | ISP |
| 23 | PC1 | LCD_BL | backlight, driven directly from the pin |
| 35–40 | PA5–PA0 | Pause, Fire, Right, Left, Down, Up | internal pull-up, button to GND |

ISP header (USBasp): MISO = pin 7, SCK = 8, MOSI = 6, RESET = 9, plus VCC and GND.
PC2–PC5 are JTAG pins; the firmware disables JTAG at start-up and does not use them.

## Power

One Li-ion cell (4.2 V full, about 3.3 V empty) feeds the MCU and LCD module directly. The
brown-out detector is set to 2.7 V. The firmware measures the supply by converting the internal
1.1 V bandgap with VCC as the ADC reference (`src/drivers/battery.cpp`) and shows the result as
a 4-segment icon in the status bar, red below 20 %. The bandgap is only typically 1.1 V
(1.0–1.2 V over parts), so calibrate `BATTERY_BANDGAP_MV` in `src/config.h` against a
multimeter, and adjust `BATTERY_EMPTY_MV` / `BATTERY_FULL_MV` to taste.

## Fuses

| Fuse | Value | Meaning |
|------|-------|---------|
| LFUSE | `0xE2` | internal 8 MHz RC, CKDIV8 off (`0xF7` for an external 8 MHz crystal) |
| HFUSE | `0xD7` | JTAG off, SPIEN on, EEPROM preserved across chip erase |
| EFUSE | `0xFD` | brown-out at 2.7 V |

Write commands and recovery: [`build.md`](build.md).

## Display

The firmware assumes a "green tab" ST7735 whose visible area starts at GRAM (2,1), RGB order,
no inversion. If the picture is shifted or the colours are wrong, change `TFT_OFFSET_X/Y`,
`TFT_BGR` and `TFT_INVERT` in `src/config.h` ("red tab" modules need offset (0,0)).
