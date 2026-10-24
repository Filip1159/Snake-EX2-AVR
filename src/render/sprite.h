/**
 * sprite.h — Transparent sprite blit from flash (SPRITES[] in assets_gen.h).
 */
#ifndef RENDER_SPRITE_H
#define RENDER_SPRITE_H

#include <stdint.h>
#include "assets/assets_gen.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Draw sprite `id` with its top-left at screen (x,y); transparent pixels untouched;
 *  clipped to the current clip rectangle (default: whole 128x160 screen). */
void sprite_draw(int16_t x, int16_t y, uint8_t id);

/** Clip subsequent sprite draws to [x0,x1) x [y0,y1) (like the original's Graphics clip). */
void sprite_set_clip(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

/** Restore the default full-screen clip. */
void sprite_reset_clip(void);

#ifdef __cplusplus
}
#endif

#endif
