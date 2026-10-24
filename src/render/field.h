/**
 * field.h — Playfield renderer (BACKLOG A3). 21x23 cells of 6x6 px at (1,1).
 */
#ifndef RENDER_FIELD_H
#define RENDER_FIELD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Draw one cell: tile 0 = background, 1..15 = wall tile. */
void field_draw_cell(uint8_t col, uint8_t row, uint8_t tile);

/** Black screen, background-coloured playfield, then all wall tiles of map[483]. */
void field_draw_map(const uint8_t *map);

#ifdef __cplusplus
}
#endif

#endif
