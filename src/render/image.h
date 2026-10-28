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

/** Draw only image rows [row0, row0+rows) with their top-left at (x,y). */
void img_draw_rows(uint8_t x, uint8_t y, const img_t *img_P, uint8_t row0, uint8_t rows);

#ifdef __cplusplus
}
#endif

#endif
