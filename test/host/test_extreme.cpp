/* Extreme campaign unit tests (G8): score-gated portal, shrink-to-2, level reset,
 * maze cycle, level-start score, coin-rush suppression and derived-field bounds. */
#include "game/game.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* Deterministic spawn: rand = 16 gives cell (18,18) in the 21x23 field. */
extern "C" int32_t game_rand_int(void) { return 16; }

static uint8_t maze_buf[FIELD_CELLS];
static uint8_t last_maze = 255;

static int8_t loader(uint8_t index, uint8_t *buf) {
    if (index >= 5) return -1;
    last_maze = index;
    memset(buf, 0, FIELD_CELLS);
    buf[0] = 1;                 /* one wall cell, enough to prove the map was replaced */
    buf[1] = (uint8_t)(index + 1);
    return 0;
}

static game_t g;

static void fresh(void) {
    game_init_mode(&g, 5, -1, nullptr, true);
    game_set_maze_source(&g, maze_buf, loader);
    last_maze = 255;
}

/* Force the portal eligibility check on this frame without moving the snake. */
static void check_frame(void) {
    g.cnt_c = (uint8_t)(g.per_La - 1);   /* increments to 0 */
    g.cnt_b = 1;                         /* increments to 2 (no movement) */
}

static void enter_portal(void) {
    CHECK(g.portal_state == 1);
    g.qx = g.portal_x;
    g.qy = g.portal_y;
    check_frame();
    game_tick(&g, 0);
}

int main(void) {
    /* 1. Threshold is strictly greater; equal score does not spawn. */
    fresh();
    CHECK(g.level == 1 && g.level_start_score == 0 && g.portal_state == 0);
    g.score = 70;                        /* 0 + 5*10 + 1*20 */
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 0);
    g.score = 71;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 1);
    CHECK(g.portal_x == 108 && g.portal_y == 108);   /* rand 16 -> cell (18,18) */
    CHECK(g.portal_x / CELL_SIZE >= 2 && g.portal_x / CELL_SIZE <= FIELD_COLS - 3);
    CHECK(g.portal_y / CELL_SIZE >= 2 && g.portal_y / CELL_SIZE <= FIELD_ROWS - 3);

    /* 2. A waiting portal is not replaced while the head is elsewhere. */
    g.qx = 42; g.qy = 54;
    g.score = 100000;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 1 && g.portal_x == 108 && g.portal_y == 108);

    /* 3. Tongue cells do not enter a portal; only the head does. */
    g.tongue = 1;
    g.dir = DIR_RIGHT;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 1);
    g.tongue = 0;

    /* 4. Entry sets Y=2 and ma=1 (no speed power-up active). */
    g.score = 71;
    enter_portal();
    CHECK(g.portal_state == 2);
    CHECK(g.per_ma == 1);

    /* 5. Shrink one segment per movement tick, then reset to level 2 / maze 0. */
    CHECK(g.len == START_LENGTH);
    int guard = 20;
    while (g.portal_state == 2 && guard-- > 0) game_tick(&g, 0);
    CHECK(guard > 0);
    CHECK(g.level == 2);
    CHECK(g.level_start_score == 71);
    CHECK(g.maze == 0);
    CHECK(last_maze == 0);
    CHECK(g.map == maze_buf && maze_buf[1] == 1);
    CHECK(g.len == START_LENGTH);
    CHECK(g.qx == START_HEAD_X && g.qy == START_HEAD_Y);
    CHECK(g.food_x == START_FOOD_X && g.food_y == START_FOOD_Y);
    CHECK(g.portal_state == 0);
    CHECK(g.full_redraw);
    CHECK(g.per_ma == (uint8_t)(SPEED_FRAMES_BASE + SPEED_OFFSET - 5));

    /* 6. Threshold grows with the new level: 71 + 50 + 2*20 = 161. */
    g.score = 161;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 0);
    g.score = 162;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 1);

    /* 7. Portal entry runs `ma=1` and then the power-up's deactivate hook. A speed
     * power-up restores its saved period; other effects leave ma=1. */
    fresh();
    g.score = 71;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 1);
    g.pu_active = 1;
    g.pu_saved_period = 5;
    g.per_ma = 3;
    g.per_Na = 3;
    enter_portal();
    CHECK(g.portal_state == 2 && g.per_ma == 5 && g.per_Na == 5);
    CHECK(g.pu_active == 1);                 /* original C() does not clear Aa */

    fresh();
    g.score = 71;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 1);
    g.pu_active = 3;                         /* ghost: C() default, ma stays 1 */
    g.per_ma = 7;
    g.per_Na = 7;
    enter_portal();
    CHECK(g.portal_state == 2 && g.per_ma == 1 && g.per_Na == 7);

    fresh();
    g.score = 71;
    check_frame();
    game_tick(&g, 0);
    g.pu_active = 10;
    g.lights_out = true;
    g.full_redraw = false;
    enter_portal();
    CHECK(g.portal_state == 2 && !g.lights_out && g.full_redraw);

    /* A wall stamp is cleared locally, not with the original's full background redraw. */
    fresh();
    g.score = 71;
    check_frame();
    game_tick(&g, 0);
    g.pu_active = 4;
    g.stamp_active = true;
    g.stamp_col = 3; g.stamp_row = 3;
    g.full_redraw = false;
    enter_portal();
    CHECK(g.portal_state == 2 && !g.stamp_active && !g.full_redraw);

    /* 8. Coin rush suppresses portal spawning. */
    fresh();
    g.score = 100000;
    g.pu_active = 8;
    g.pu_timer = 1;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 0);
    g.pu_active = 0;

    /* 9. Maze cycle -1,0,1,2,3,4 then -1; level-start score snapshots the score.
     * Also check the fields original a() deliberately leaves alone. */
    fresh();
    int8_t expected[8] = {-1, 0, 1, 2, 3, 4, -1, 0};
    CHECK(g.maze == expected[0]);
    g.bonus_x = 30; g.bonus_y = 36; g.bonus_type = 5;
    g.move_food_timer = 7;
    g.grow = true;
    for (int i = 1; i < 8; i++) {
        g.score = 1000u + (uint32_t)i;
        g.portal_state = 2;
        g.len = 2;
        g.cnt_b = (uint8_t)(g.per_ma - 1);  /* increments to 0 -> shrink block */
        game_tick(&g, 0);
        CHECK(g.level == (uint16_t)(i + 1));
        CHECK(g.maze == expected[i]);
        CHECK(g.portal_state == 0);
        if (g.maze < 0) {
            CHECK(g.map == nullptr);
        } else {
            CHECK(g.map == maze_buf && last_maze == (uint8_t)g.maze);
        }
        CHECK(g.level_start_score == 1000u + (uint32_t)i);
    }
    CHECK(g.bonus_x == 30 && g.bonus_y == 36 && g.bonus_type == 5);
    CHECK(g.move_food_timer == 7);
    CHECK(g.grow);

    /* 10. Spawn rejects a wall under the fixed candidate (18,18), then falls back.
     * The original's fallback post-increments before exiting, so for 21 columns the
     * selected cell is (3,2), one past the first free scan cell. A coin under the
     * candidate is rejected the same way. */
    fresh();
    memset(maze_buf, 0, sizeof maze_buf);
    maze_buf[18 + 18 * FIELD_COLS] = 1;
    g.maze = 0;
    g.map = maze_buf;
    g.score = 100000;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 1);
    CHECK(g.portal_x == 3 * CELL_SIZE && g.portal_y == 2 * CELL_SIZE);

    fresh();
    uint16_t coin_idx = (uint16_t)(18 + 18 * FIELD_COLS);
    g.coins[coin_idx >> 4] |= (uint16_t)(1u << (coin_idx & 15));
    g.score = 100000;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 1);
    CHECK(g.portal_x == 3 * CELL_SIZE && g.portal_y == 2 * CELL_SIZE);

    /* 11. Moving bonus food bounces off the portal target (original b.java:1690). */
    fresh();
    g.portal_state = 1;
    g.portal_x = 12; g.portal_y = 30;
    g.bonus = 10;
    g.bonus_type = 3;
    g.bonus_x = 6; g.bonus_y = 30;
    g.bonus_dir = 8;                     /* moving right into the portal */
    g.cnt_B = (uint8_t)(g.per_Ma - 1);
    g.cnt_b = 1;                         /* do not move the snake */
    game_tick(&g, 0);
    CHECK(g.bonus_x == 6 && g.bonus_y == 30);
    CHECK(g.bonus_dir == 2);             /* reversed to left */

    /* 12. Classic never opens a portal. */
    game_init(&g, 5, -1, nullptr);
    g.score = 100000;
    check_frame();
    game_tick(&g, 0);
    CHECK(g.portal_state == 0);

    printf("extreme: %s (%d failures)\n", fails ? "FAILED" : "all passed", fails);
    return fails != 0;
}
