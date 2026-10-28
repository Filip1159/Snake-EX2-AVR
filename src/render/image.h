/**
 * image.h — Flash RLE image drawing (BACKLOG A4). See tools/gen_assets.py for the format.
 */
#ifndef RENDER_IMAGE_H
#define RENDER_IMAGE_H

#include <stdint.h>
#include "assets/assets_gen.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Draw an image stored in flash (pointer to a PROGMEM img_t) with its top-left at (x,y). */
void img_draw(uint8_t x, uint8_t y, const img_t *img_P);

/** Draw only the sub-rectangle (rx,ry,rw,rh) of the image, with the image's top-left at
 *  (x,y) — i.e. the rectangle lands at (x+rx, y+ry). Used to restore a background. */
void img_draw_rect(uint8_t x, uint8_t y, const img_t *img_P,
                   uint8_t rx, uint8_t ry, uint8_t rw, uint8_t rh);

#ifdef __cplusplus
}
#endif

#endif
