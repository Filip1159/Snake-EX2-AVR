#include "game/classic.h"
#include "assets/maze.h"
#include "config.h"

extern "C" bool game_init_classic(game_t *g, uint8_t speed, int8_t maze,
                                  uint8_t *map_buf) {
    if (!g || speed > SPEED_INDEX_MAX ||
        maze < MAZE_NONE || maze >= MAZE_COUNT) {
        return false;
    }

    if (maze == MAZE_NONE) {
        game_init(g, speed, MAZE_NONE, 0);
        return true;
    }

    if (!map_buf || maze_load((uint8_t)maze, map_buf) != 0) return false;

    game_init(g, speed, maze, map_buf);
    g->map_buf = map_buf;
    g->maze_loader = maze_load;
    return true;
}
