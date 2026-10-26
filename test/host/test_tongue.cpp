/* G6: tongue — reaches one and two cells ahead in both Classic and Extreme, retracts on the
 * next movement tick, and wraps with the field. Mirrors b.java: J==0 + K[4] -> J=-1, the
 * next movement tick sets J=1, and the food checks accept (k,l) and (K,L) while J>0.
 * Deviation: enabled in Classic too (the original only exposed it in Extreme). */
#include "assets/maze.h"
#include "game/game.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

extern "C" int32_t game_rand_int(void) { return (int32_t)0x12345678; }

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* Run frames with `keys` held until the next movement tick (the first one is the original's
 * no-op move, which still counts as a movement tick for the tongue). */
static void first_move(game_t *g, uint8_t keys) {
    g->moved = false;
    for (int i = 0; i < 20 && !g->moved; i++) game_tick(g, keys);
}

int main() {
    game_t g;

    /* one cell ahead: the tongue grabs it; the head itself does not move on the no-op tick */
    game_init_mode(&g, 5, -1, nullptr, true);
    g.food_x = (int16_t)(g.qx + CELL_SIZE); g.food_y = g.qy;
    int16_t fx = g.food_x, fy = g.food_y;
    first_move(&g, KEY_FIRE);
    CHECK(g.tongue == 1);
    CHECK(g.food_eaten);
    CHECK(g.eaten_x == fx && g.eaten_y == fy);
    CHECK(g.score == 6);
    CHECK(g.grow);
    CHECK(g.qx == 42 && g.qy == 54);

    /* the lash lasts the rest of the movement cadence, then retracts on the next move */
    for (int i = 0; i < 4; i++) { CHECK(g.tongue == 1); game_tick(&g, 0); }
    CHECK(g.tongue == 1);
    game_tick(&g, 0);
    CHECK(g.moved);
    CHECK(g.tongue == 0);

    /* two cells ahead */
    game_init_mode(&g, 5, -1, nullptr, true);
    g.food_x = (int16_t)(g.qx + 2 * CELL_SIZE); g.food_y = g.qy;
    first_move(&g, KEY_FIRE);
    CHECK(g.tongue == 1 && g.food_eaten && g.score == 6);

    /* three cells ahead: out of reach (the head does not move on the first tick) */
    game_init_mode(&g, 5, -1, nullptr, true);
    g.food_x = (int16_t)(g.qx + 3 * CELL_SIZE); g.food_y = g.qy;
    first_move(&g, KEY_FIRE);
    CHECK(g.tongue == 1);
    CHECK(!g.food_eaten && g.score == 0);

    /* behind the head: not eaten */
    game_init_mode(&g, 5, -1, nullptr, true);
    g.food_x = (int16_t)(g.qx - CELL_SIZE); g.food_y = g.qy;
    first_move(&g, KEY_FIRE);
    CHECK(!g.food_eaten);

    /* Classic: FIRE extends the tongue and grabs food from a distance. */
    game_init(&g, 5, -1, nullptr);
    g.food_x = (int16_t)(g.qx + CELL_SIZE); g.food_y = g.qy;
    first_move(&g, KEY_FIRE);
    CHECK(g.tongue == 1 && g.food_eaten && g.score == 6);

    /* Classic but no fire: no tongue, no grab */
    game_init(&g, 5, -1, nullptr);
    g.food_x = (int16_t)(g.qx + CELL_SIZE); g.food_y = g.qy;
    first_move(&g, 0);
    CHECK(g.tongue == 0 && !g.food_eaten);

    /* Extreme but no fire: no tongue, no grab */
    game_init_mode(&g, 5, -1, nullptr, true);
    g.food_x = (int16_t)(g.qx + CELL_SIZE); g.food_y = g.qy;
    first_move(&g, 0);
    CHECK(g.tongue == 0 && !g.food_eaten);

    /* one cell ahead across the right edge wraps to column 0 */
    game_init_mode(&g, 5, -1, nullptr, true);
    g.qx = FIELD_WIDTH_PX - CELL_SIZE; g.qy = 54;
    g.food_x = 0; g.food_y = 54;
    first_move(&g, KEY_FIRE);
    CHECK(g.tongue == 1 && g.food_eaten);
    CHECK(g.eaten_x == 0 && g.eaten_y == 54);

    /* two cells ahead across the wrap: last column -> column 1 */
    game_init_mode(&g, 5, -1, nullptr, true);
    g.qx = FIELD_WIDTH_PX - CELL_SIZE; g.qy = 54;
    g.food_x = CELL_SIZE; g.food_y = 54;
    first_move(&g, KEY_FIRE);
    CHECK(g.tongue == 1 && g.food_eaten);

    /* Extreme campaign regression: the portal shrink sets ma=1, so after next_level()
     * resets the periods the b and C counters are out of phase.  Pickup checks run on
     * the C tick even when it is not a movement tick; the tongue reach must still be
     * valid (original k/l and K/L are persistent fields, not locals reset each frame). */
    game_init_mode(&g, 5, -1, nullptr, true);
    g.cnt_b = 1;                                   /* -> 2, not a movement tick */
    g.cnt_C = (uint8_t)(g.per_Na - 1);             /* -> 0, a pickup tick */
    g.tongue = 1;
    int16_t tfx = (int16_t)(g.qx + CELL_SIZE);
    g.food_x = tfx; g.food_y = g.qy;
    g.food_eaten = false;
    g.score = 0;
    game_tick(&g, 0);
    CHECK(!g.moved);
    CHECK(g.food_eaten);
    CHECK(g.eaten_x == tfx && g.eaten_y == g.qy);
    CHECK(g.score == 6);

    /* Same cadence divergence for a field power-up: the tongue tip collects it. */
    game_init_mode(&g, 5, -1, nullptr, true);
    g.cnt_b = 1;
    g.cnt_C = (uint8_t)(g.per_Na - 1);
    g.tongue = 1;
    g.food_x = g.food_y = -1;
    g.pu[0].id = 3;
    g.pu[0].life = 50;
    g.pu[0].x = (uint8_t)(g.qx + CELL_SIZE);
    g.pu[0].y = (uint8_t)g.qy;
    game_tick(&g, 0);
    CHECK(!g.moved);
    CHECK(g.pu_active == 3);

    if (fails) { printf("tongue: %d FAILED\n", fails); return 1; }
    printf("tongue: all passed (1/2-cell reach, no 3rd cell, retract, wrap, Classic + Extreme)\n");
    return 0;
}
