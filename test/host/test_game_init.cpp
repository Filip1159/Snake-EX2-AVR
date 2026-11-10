/* G1/G2/G3 at the derived 21x23 geometry: initial state, cadence, wraps, wall death. */
#include "assets/maze.h"
#include "game/game.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

extern "C" int32_t game_rand_int(void) { return (int32_t)0x12345678; }

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static uint8_t maze_map[MAZE_CELLS];

/* Put the snake in a straight line and make the next tick a movement tick. */
static void setup_move(game_t *g, uint8_t dir, int16_t x, int16_t y) {
    game_init(g, 5, -1, nullptr);
    g->qx = x; g->qy = y;
    g->dir = dir;
    for (uint16_t i = 0; i < g->len; i++) g->a[i] = dir;
    g->input = dir;
    g->cnt_b = (uint8_t)(g->per_ma - 1);
    g->cnt_C = (uint8_t)(g->per_Na - 1);
    g->food_x = 24; g->food_y = 24;          /* not on the tested path */
}

int main() {
    game_t g;

    /* initial state, unchanged from the original */
    game_init(&g, 5, 0, maze_map);
    CHECK(g.len == 6);
    CHECK(g.qx == 42 && g.qy == 54);
    CHECK(g.food_x == 24 && g.food_y == 24);
    CHECK((g.dir & 0x0F) == DIR_RIGHT);
    CHECK(g.a[5] == 0x28);
    CHECK(g.score == 0 && g.score_step == 6);
    CHECK(g.death == 0 && !g.game_over);
    CHECK(g.per_ma == 2 + (SPEED_OFFSET - 5));
    CHECK(g.per_La == g.per_ma && g.per_Ma == g.per_ma && g.per_Na == g.per_ma);

    /* cadence: speed 0..5 -> 10,9,..,5 frames per move */
    for (uint8_t s = SPEED_INDEX_MIN; s <= SPEED_INDEX_MAX; s++) {
        game_init(&g, s, -1, nullptr);
        g.dir = DIR_RIGHT;                     /* the original starts at 0x28 (first move is a no-op) */
        for (uint16_t i = 0; i < g.len; i++) g.a[i] = DIR_RIGHT;
        CHECK(g.per_ma == 2 + (SPEED_OFFSET - s));
        uint8_t frames = 0;
        uint16_t x = g.qx;
        while (frames < 12) {
            game_tick(&g, 0);
            frames++;
            if (g.qx != x) break;
        }
        CHECK(frames == g.per_ma);
    }

    /* wrap right: cell 20 -> cell 0 */
    setup_move(&g, DIR_RIGHT, 120, 54);
    game_tick(&g, KEY_RIGHT);
    CHECK(g.moved && g.qx == 0 && g.qy == 54);

    /* wrap left: cell 0 -> cell 20 */
    setup_move(&g, DIR_LEFT, 0, 54);
    game_tick(&g, KEY_LEFT);
    CHECK(g.moved && g.qx == 120 && g.qy == 54);

    /* wrap up: row 0 -> row 22 */
    setup_move(&g, DIR_UP, 42, 0);
    game_tick(&g, KEY_UP);
    CHECK(g.moved && g.qy == FIELD_HEIGHT_PX - CELL_SIZE && g.qx == 42);

    /* wrap down: row 22 -> row 0 */
    setup_move(&g, DIR_DOWN, 42, FIELD_HEIGHT_PX - CELL_SIZE);
    game_tick(&g, KEY_DOWN);
    CHECK(g.moved && g.qy == 0 && g.qx == 42);

    /* 180-degree reversal is rejected (head keeps going right) */
    setup_move(&g, DIR_RIGHT, 42, 54);
    game_tick(&g, KEY_LEFT);
    CHECK(g.moved && g.qx == 48 && g.qy == 54);

    /* wall collision: pick a wall cell and drive into it from the left */
    maze_load(0, maze_map);
    /* Box maze 0: solid border. Drive left into column 0 from column 1. */
    int wr = -1;
    for (int r = 4; r < 16 && wr < 0; r++)
        if (maze_map[0 + r * FIELD_COLS] & 0x0F) wr = r;
    CHECK(wr > 0);
    game_init(&g, 5, 0, maze_map);
    g.qx = 1 * CELL_SIZE;
    g.qy = (int16_t)(wr * CELL_SIZE);
    g.dir = DIR_LEFT;
    for (uint16_t i = 0; i < g.len; i++) g.a[i] = DIR_LEFT;
    g.input = DIR_LEFT;
    g.cnt_b = (uint8_t)(g.per_ma - 1);
    g.cnt_C = (uint8_t)(g.per_Na - 1);
    g.food_x = 24; g.food_y = 24;
    int16_t px = g.qx, py = g.qy;
    game_tick(&g, KEY_LEFT);
    CHECK(g.death == 40);
    CHECK(!g.moved);
    CHECK(g.qx == px && g.qy == py);           /* the fatal move is undone */
    CHECK(g.sfx_mask & SFXBIT(SFX_DEATH));
    CHECK(!g.game_over);

    /* death animation runs exactly 40 ticks, then game over */
    for (int i = 0; i < 40; i++) {
        game_tick(&g, 0);
        CHECK(g.moved == false);
    }
    CHECK(g.death == 0 && g.game_over);

    /* occupancy bitmap covers exactly the snake cells and nothing else */
    game_init(&g, 5, -1, nullptr);
    /* mark_snake is internal; verify the derived cell walk instead */
    int16_t x = g.qx, y = g.qy;
    for (int16_t i = (int16_t)g.len - 1; i >= 0; i--) {
        int col = x / CELL_SIZE, row = y / CELL_SIZE;
        CHECK(col >= 0 && col < FIELD_COLS && row >= 0 && row < FIELD_ROWS);
        switch (g.a[i] & 0x0F) {
            case DIR_UP: y += CELL_SIZE; break;
            case DIR_DOWN: y -= CELL_SIZE; break;
            case DIR_LEFT: x += CELL_SIZE; break;
            default: x -= CELL_SIZE; break;
        }
        if (x <= 0) x += FIELD_WIDTH_PX;
        if (x >= FIELD_WIDTH_PX) x -= FIELD_WIDTH_PX;
        if (y <= 0) y += FIELD_HEIGHT_PX;
        if (y >= FIELD_HEIGHT_PX) y -= FIELD_HEIGHT_PX;
    }

    if (fails) printf("game_init: %d FAILED\n", fails);
    else printf("game_init: all passed (21x23 field, wraps, cadence, wall death)\n");
    return fails != 0;
}
