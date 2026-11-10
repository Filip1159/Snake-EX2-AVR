/**
 * text.h — Series 40 bitmap fonts for menus and screens (see tools/gen_fonts.py).
 *
 * Small font (text_draw*): proportional, A–Z, a–z, 0–9, space and `. , - : /`; a 13-row
 * frame whose top row is `y` (caps rows 1..9, baseline row 9, descenders to row 12), 1 px
 * between glyphs.  Unknown characters render as a 3 px space.  `scale` multiplies sizes.
 */
#ifndef RENDER_TEXT_H
#define RENDER_TEXT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Draw a NUL-terminated string at (x,y). Unknown characters render as spaces. */
void text_draw(uint8_t x, uint8_t y, const char *txt, uint8_t scale, uint16_t color);

/** Draw a NUL-terminated string from PROGMEM at (x,y). */
void text_draw_p(uint8_t x, uint8_t y, const char *txt_p, uint8_t scale, uint16_t color);

/** Pixel width of a string at the given scale (last inter-glyph gap excluded). */
uint8_t text_width(const char *txt, uint8_t scale);

/** Pixel width of a PROGMEM string at the given scale. */
uint8_t text_width_p(const char *txt_p, uint8_t scale);

/** Bold proportional list font for the menus and the centre soft key: a 15-row frame whose
 *  top row is `y` (caps rows 1..11, baseline row 11, descenders to row 14). Glyphs: the
 *  letters and digits the game's labels need (see tools/gen_fonts.py); space is 4 px wide,
 *  other characters render as space. */
void text_draw_big(uint8_t x, uint8_t y, const char *txt, uint16_t color);

/** Pixel width of a string in the bold menu font. */
uint8_t text_width_big(const char *txt);

#ifdef __cplusplus
}
#endif

#endif
