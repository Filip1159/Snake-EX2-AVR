/**
 * st7735.h — ST7735 128×160 SPI LCD Driver
 */
#ifndef DRIVERS_ST7735_H
#define DRIVERS_ST7735_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void st7735_init(void);
void st7735_fill_screen(uint16_t color);
void st7735_set_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
void st7735_blit(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *buf);
void st7735_blit_masked(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                        const uint16_t *buf, const uint8_t *mask);
uint16_t st7735_draw_rle(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                         const uint8_t *data, uint16_t data_len);
/** Streamed pixel write: begin(window), pixels(color, repeat)..., end. Exactly w*h pixels. */
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void st7735_stream_pixels(uint16_t color, uint8_t count);
void st7735_stream_end(void);
void st7735_set_rotation(uint8_t rotation);
void st7735_display_on(bool on);
void st7735_backlight(bool on);

#ifdef __cplusplus
}
#endif

#endif