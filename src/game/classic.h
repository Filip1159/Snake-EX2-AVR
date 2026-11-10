/**
 * classic.h — Validated Classic-mode start.
 *
 * Classic accepts no maze (-1) or one of the five derived mazes (0..4), and the
 * six speed indices 0..5. The caller owns the 483-byte map buffer used by mazes
 * 0..4; no-maze ignores it and starts with a NULL map.
 */
#ifndef GAME_CLASSIC_H
#define GAME_CLASSIC_H

#include <stdint.h>
#include "game/game.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Load the selected maze and initialise a non-Extreme game.
 * Returns false without initialising `g` when speed/maze are out of range or
 * when a maze is selected but `map_buf` is NULL/loading fails.
 */
bool game_init_classic(game_t *g, uint8_t speed, int8_t maze, uint8_t *map_buf);

#ifdef __cplusplus
}
#endif

#endif
