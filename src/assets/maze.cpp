#include "assets/maze.h"
#include "assets/assets_gen.h"

#define SRC_ROWS      18
#define INSERT_AFTER  9
#define INSERT_COUNT  5

/* Names exactly as in the original string table (strings 67, 58, 69, 68, 57, 70). */
static const char MAZE_NAMES[MAZE_COUNT + 1][9] PROGMEM = {
    "No maze",   /* -1 */
    "Box",       /*  0 */
    "Tunnel",    /*  1 */
    "Spiral",    /*  2 */
    "Blockade",  /*  3 */
    "Twisted",   /*  4 */
};

static uint8_t src_cell(uint8_t index, uint16_t k) {
    uint8_t b = pgm_read_byte(&MAZES_4BPP[index][k >> 1]);
    return (k & 1) ? (b & 0x0F) : (b >> 4);
}

const char *maze_name(int8_t maze) {
    if (maze < -1 || maze >= MAZE_COUNT) return 0;
    return MAZE_NAMES[maze + 1];
}

int8_t maze_load(uint8_t index, uint8_t *map) {
    if (index >= MAZE_COUNT) return -1;
    uint8_t *dst = map;
    for (uint8_t r = 0; r < SRC_ROWS; r++) {
        for (uint8_t c = 0; c < MAZE_COLS; c++) {
            *dst++ = src_cell(index, (uint16_t)r * MAZE_COLS + c);
        }
        if (r == INSERT_AFTER) {
            for (uint8_t i = 0; i < INSERT_COUNT; i++) {
                for (uint8_t c = 0; c < MAZE_COLS; c++) {
                    *dst++ = (index == 0) ? src_cell(index, MAZE_COLS + c) : 0;
                }
            }
        }
    }
    return 0;
}
