/**
 * text.h — 5×7 bitmap text for menus and simple screens.
 *
 * The font is a compact ASCII subset: A–Z, a–z, 0–9, space, '-', '.' and
 * ':'. Each glyph occupies a 5×7 box; the horizontal advance is 5 + 1 pixels,
 * scaled. Lowercase glyphs use a 5-row x-height so mixed-case S40-style menus
 * can be rendered without changing the fixed-pitch layout used elsewhere.
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

/** Bold 12-row proportional font for the main-menu list and soft key (cap height 10,
 *  baseline below row 9). Only the glyphs the menu labels need exist (see
 *  tools/gen_bigfont.py); space is 4 px wide, other characters render as space. */
void text_draw_big(uint8_t x, uint8_t y, const char *txt, uint16_t color);

/** Pixel width of a string in the bold menu font. */
uint8_t text_width_big(const char *txt);

#ifdef __cplusplus
}
#endif

#endif
