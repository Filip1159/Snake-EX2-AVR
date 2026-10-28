/**
 * savegame.h — One saved game in EEPROM ("Continue" in the main menu).
 *
 * The game state is stored as a snapshot of game_t (without the transient occupancy
 * bitmap and the unused tail of the snake array), guarded by a marker, the struct size
 * and a CRC16, so a snapshot from another firmware layout or a torn write is never
 * restored. The snapshot holds no usable pointers: the caller re-attaches the maze
 * buffer after loading.
 */
#ifndef GAME_SAVEGAME_H
#define GAME_SAVEGAME_H

#include <stdbool.h>
#include "game/game.h"

#ifdef __cplusplus
extern "C" {
#endif

/** True when a complete, intact saved game is stored. */
bool savegame_exists(void);

/** Store `g` as the saved game (replaces the previous one). Takes a moment: EEPROM writes. */
void savegame_save(const game_t *g);

/** Restore the saved game into `g`. Pointer members (map, map_buf, maze_loader) are NULL
 *  afterwards. Returns false and leaves `g` unspecified when there is no valid save. */
bool savegame_load(game_t *g);

/** Forget the saved game. */
void savegame_clear(void);

#ifdef __cplusplus
}
#endif

#endif
