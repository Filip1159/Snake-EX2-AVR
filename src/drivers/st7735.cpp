/**
 * st7735.cpp — ST7735 128×160 SPI LCD Driver
 */
#include "drivers/st7735.h"
#include "hal/spi.h"
#include "hal/gpio.h"
#include "config.h"
#include <Arduino.h>
#include <avr/io.h>

#ifndef TFT_OFFSET_X
#define TFT_OFFSET_X 0
#endif
#ifndef TFT_OFFSET_Y
#define TFT_OFFSET_Y 0
#endif
#ifndef TFT_BGR
#define TFT_BGR 0
#endif
#ifndef TFT_INVERT
#define TFT_INVERT 0
#endif

static uint8_t s_madctl = 0;

static void write_cmd(uint8_t cmd) {
    lcd_dc_low();
    lcd_cs_low();
    spi_transfer(cmd);
    lcd_cs_high();
}

static void write_data(uint8_t data) {
    lcd_dc_high();
    lcd_cs_low();
    spi_transfer(data);
    lcd_cs_high();
}

static void write_data_bulk(const uint8_t *data, uint16_t len) {
    lcd_dc_high();
    lcd_cs_low();
    spi_write(data, len);
    lcd_cs_high();
}

static void hw_reset(void) {
    lcd_rst_high();
    _delay_us(50);
    lcd_rst_low();
    _delay_us(50);
    lcd_rst_high();
    _delay_ms(120);
}

/* Init sequence: {cmd, len, data[len]} — cmd=0 means delay by len ms */
static const uint8_t INIT_SEQ[] = {
    0x01, 0,                                    // SWRESET
    0x00, 150,                                  // delay 150 ms
    0x11, 0,                                    // SLPOUT
    0x00, 120,                                  // delay 120 ms
    0xB1, 3,  0x01, 0x2C, 0x2D,                // FRMCTR1
    0xB2, 3,  0x01, 0x2C, 0x2D,                // FRMCTR2
    0xB3, 6,  0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D,  // FRMCTR3
    0xB4, 1,  0x07,                             // INVCTR
    0xC0, 3,  0xA2, 0x02, 0x84,                // PWCTR1
    0xC1, 1,  0xC5,                             // PWCTR2
    0xC2, 2,  0x0A, 0x00,                      // PWCTR3
    0xC3, 2,  0x8A, 0x2A,                      // PWCTR4
    0xC4, 2,  0x8A, 0xEE,                      // PWCTR5
    0xC5, 1,  0x0E,                             // VMCTR1
    0xE0, 16, 0x02,0x1C,0x07,0x12,0x37,0x32,0x29,0x2D,
              0x29,0x25,0x2B,0x39,0x00,0x01,0x03,0x10,  // GMCTRP1
    0xE1, 16, 0x03,0x1D,0x07,0x06,0x2E,0x2C,0x29,0x2D,
              0x2E,0x2E,0x37,0x3F,0x00,0x00,0x02,0x10,  // GMCTRN1
    0x3A, 1,  0x05,                             // COLMOD: RGB565
    0x13, 0,                                    // NORON
    0x00, 10,                                   // delay 10 ms
    0x29, 0,                                    // DISPON
    0x00, 100,                                  // delay 100 ms
};

void st7735_init(void) {
    spi_init();
    hw_reset();

    const uint8_t *seq = INIT_SEQ;
    const uint8_t *end = seq + sizeof(INIT_SEQ);

    while (seq < end) {
        uint8_t cmd = *seq++;
        uint8_t len = *seq++;

        if (cmd == 0x00) {
            delay(len);
            continue;
        }

        write_cmd(cmd);
        if (len > 0) {
            write_data_bulk(seq, len);
            seq += len;
        }
    }

    st7735_set_rotation(0);
    st7735_fill_screen(0x0000);
}

void st7735_set_rotation(uint8_t rotation) {
    switch (rotation) {
        case 0: s_madctl = 0x00; break;
        case 1: s_madctl = 0x60; break;
        case 2: s_madctl = 0xC0; break;
        case 3: s_madctl = 0xA0; break;
    }
    if (TFT_BGR)    s_madctl |= 0x08;
    if (TFT_INVERT) s_madctl |= 0x04;

    write_cmd(0x36);  // MADCTL
    write_data(s_madctl);
    write_cmd(TFT_INVERT ? 0x21 : 0x20);  // INVON / INVOFF
}

void st7735_display_on(bool on) {
    write_cmd(on ? 0x29 : 0x28);
}

void st7735_backlight(bool on) {
    if (on) lcd_bl_on(); else lcd_bl_off();
}

void st7735_set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    x0 += TFT_OFFSET_X; x1 += TFT_OFFSET_X;
    y0 += TFT_OFFSET_Y; y1 += TFT_OFFSET_Y;

    write_cmd(0x2A);  // CASET
    write_data(0x00); write_data(x0);
    write_data(0x00); write_data(x1);

    write_cmd(0x2B);  // RASET
    write_data(0x00); write_data(y0);
    write_data(0x00); write_data(y1);

    write_cmd(0x2C);  // RAMWR
}

void st7735_fill_screen(uint16_t color) {
    st7735_fill_rect(0, 0, 128, 160, color);
}

void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color) {
    if (w == 0 || h == 0) return;

    st7735_set_window(x, y, x + w - 1, y + h - 1);

    uint32_t count = (uint32_t)w * h;
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);

    lcd_dc_high();
    lcd_cs_low();

    while (count--) {
        SPDR = hi; while (!(SPSR & (1 << SPIF)));
        SPDR = lo; while (!(SPSR & (1 << SPIF)));
    }
    lcd_cs_high();
}

void st7735_blit(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *buf) {
    if (w == 0 || h == 0) return;
    st7735_set_window(x, y, x + w - 1, y + h - 1);
    uint32_t count = (uint32_t)w * h;
    lcd_dc_high();
    lcd_cs_low();
    spi_write16_bulk(buf, (uint16_t)count);
    lcd_cs_high();
}

void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    st7735_set_window(x, y, x + w - 1, y + h - 1);
    lcd_dc_high();
    lcd_cs_low();
}

void st7735_stream_pixels(uint16_t color, uint8_t count) {
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);
    while (count--) {
        SPDR = hi; while (!(SPSR & (1 << SPIF)));
        SPDR = lo; while (!(SPSR & (1 << SPIF)));
    }
}

void st7735_stream_end(void) {
    lcd_cs_high();
}

void st7735_blit_masked(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                        const uint16_t *buf, const uint8_t *mask) {
    if (w == 0 || h == 0) return;

    uint8_t mask_bytes_per_row = (w + 7) / 8;

    for (uint8_t row = 0; row < h; row++) {
        const uint8_t *row_mask = mask + row * mask_bytes_per_row;
        const uint16_t *row_buf = buf + ((uint32_t)row * w);

        uint8_t col = 0;
        while (col < w) {
            // Skip transparent pixels
            while (col < w) {
                uint8_t byte_idx = col / 8;
                uint8_t bit_idx  = 7 - (col % 8);
                if (row_mask[byte_idx] & (1 << bit_idx)) break;
                col++;
            }
            if (col >= w) break;

            // Find run of visible pixels
            uint8_t run_start = col;
            while (col < w) {
                uint8_t byte_idx = col / 8;
                uint8_t bit_idx  = 7 - (col % 8);
                if (!(row_mask[byte_idx] & (1 << bit_idx))) break;
                col++;
            }
            uint8_t run_len = col - run_start;

            st7735_set_window(x + run_start, y + row,
                              x + run_start + run_len - 1, y + row);
            lcd_dc_high();
            lcd_cs_low();
            spi_write16_bulk(row_buf + run_start, run_len);
            lcd_cs_high();
        }
    }
}

uint16_t st7735_draw_rle(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                         const uint8_t *data, uint16_t data_len) {
    if (w == 0 || h == 0 || data_len == 0) return 0;

    uint32_t total = (uint32_t)w * h;
    uint32_t drawn = 0;
    uint16_t consumed = 0;

    st7735_set_window(x, y, x + w - 1, y + h - 1);
    lcd_dc_high();
    lcd_cs_low();

    while (drawn < total && consumed < data_len) {
        uint8_t cb = data[consumed++];

        if (cb & 0x80) {
            // Repeat run
            uint8_t cnt = (cb & 0x7F) + 1;
            if (consumed + 1 >= data_len) break;
            uint16_t color = ((uint16_t)data[consumed] << 8) | data[consumed + 1];
            consumed += 2;

            if (drawn + cnt > total) cnt = (uint8_t)(total - drawn);

            uint8_t hi = (uint8_t)(color >> 8);
            uint8_t lo = (uint8_t)(color & 0xFF);
            while (cnt--) {
                SPDR = hi; while (!(SPSR & (1 << SPIF)));
                SPDR = lo; while (!(SPSR & (1 << SPIF)));
            }
            drawn += (cb & 0x7F) + 1;
        } else {
            // Literal run
            uint8_t cnt = cb + 1;
            if (drawn + cnt > total) cnt = (uint8_t)(total - drawn);

            uint16_t bytes = cnt * 2;
            if (consumed + bytes > data_len) {
                bytes = data_len - consumed;
                cnt = bytes / 2;
            }

            for (uint8_t i = 0; i < cnt; i++) {
                SPDR = data[consumed++]; while (!(SPSR & (1 << SPIF)));
                SPDR = data[consumed++]; while (!(SPSR & (1 << SPIF)));
            }
            drawn += cnt;
        }
    }
    lcd_cs_high();
    return consumed;
}