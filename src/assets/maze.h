/**
 * maze.h — Maze loader (BACKLOG §1.5, A3).
 * Expands an original 21x18 4-bit map from flash to the derived 21x23 byte map.
 */
#ifndef ASSETS_MAZE_H
#define ASSETS_MAZE_H

#include <stdint.h>

#include "assets/assets_gen.h"   /* MAZE_COUNT */

#define MAZE_COLS   21
#define MAZE_ROWS   23
#define MAZE_CELLS  (MAZE_COLS * MAZE_ROWS)   /* 483 */

/* Classic/Extreme maze IDs. The original also stores an unused L-shape at 5. */
#define MAZE_NONE      (-1)
#define MAZE_BOX        0
#define MAZE_TUNNEL     1
#define MAZE_SPIRAL     2
#define MAZE_BLOCKADE   3
#define MAZE_TWISTED    4

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load maze `index` (0 Box, 1 Tunnel, 2 Spiral, 3 Blockade, 4 Twisted) into
 * map[MAZE_CELLS]. Five blank rows are inserted after row 9; Box repeats its
 * side-wall row instead. Returns 0 on success, -1 if index is out of range.
 */
int8_t maze_load(uint8_t index, uint8_t *map);

/**
 * Return the flash string for a selection value (-1 "No maze", 0 Box, 1 Tunnel,
 * 2 Spiral, 3 Blockade, 4 Twisted), or NULL for an invalid value. On AVR the
 * pointer is a PROGMEM address; read it with pgm_read_byte().
 */
const char *maze_name(int8_t maze);

#ifdef __cplusplus
}
#endif

#endif
