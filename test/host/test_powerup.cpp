/* G7: power-ups — every type's effect, counter and expiry, plus slots, pickup, coin rush,
 * wall stamp, moving food and lights-out. Host-only, deterministic RNG. */
#include "game/game.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

static uint8_t map[FIELD_CELLS];
static game_t g;
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* Fixed random value. Low nibble 4 => no slot spawn (needs 15); &4 == 0 => bonus spawns. */
extern "C" int32_t game_rand_int(void) { return 16; }

static void reset(void) {
    game_init_mode(&g, 5, -1, NULL, true);
    g.level_start_score = 1000000000u;   /* unit sandbox: no campaign portals */
    g.food_x = g.food_y = -1;
    g.bonus = 0;
    g.len = 6;
    g.dir = DIR_RIGHT;
    for (uint8_t i = 0; i < g.len; i++) g.a[i] = DIR_RIGHT;
    g.qx = 60; g.qy = 54;
    g.cnt_b = g.cnt_B = g.cnt_c = g.cnt_C = 0;
    g.sfx_mask = 0;
    g.full_redraw = false;
}

/* Make the next frame both a movement tick and a food/power-up check tick. */
static void force_movement(void) {
    g.cnt_b = (uint8_t)(g.per_ma - 1);
    g.cnt_C = (uint8_t)(g.per_Na - 1);
}

static void food_ahead(void) {
    int16_t x = g.qx, y = g.qy;
    switch (g.dir & 0x0F) {
        case DIR_UP: y -= CELL_SIZE; break;
        case DIR_DOWN: y += CELL_SIZE; break;
        case DIR_LEFT: x -= CELL_SIZE; break;
        default: x += CELL_SIZE; break;
    }
    if (x <= 0) x += FIELD_WIDTH_PX;
    if (x >= FIELD_WIDTH_PX) x -= FIELD_WIDTH_PX;
    if (y <= 0) y += FIELD_HEIGHT_PX;
    if (y >= FIELD_HEIGHT_PX) y -= FIELD_HEIGHT_PX;
    g.food_x = x; g.food_y = y;
}

static void put_slot(uint8_t i, uint8_t id, uint8_t life, int16_t x, int16_t y) {
    g.pu[i].id = id; g.pu[i].life = life;
    g.pu[i].x = (uint8_t)x; g.pu[i].y = (uint8_t)y;
}

static void run_until_active(uint8_t active, int limit) {
    while (g.pu_active == active && limit-- > 0) game_tick(&g, 0);
}

/* ── types 1/2: speed and restore ───────────────────────────────────────── */
static void test_speed(void) {
    reset();
    g.per_ma = g.per_Na = 5;
    game_apply_powerup(&g, 1);
    CHECK(g.pu_active == 1);
    CHECK(g.pu_timer >= 25 && g.pu_timer <= 74);
    CHECK(g.per_ma == 3 && g.per_Na == 3);
    CHECK(g.pu_saved_period == 5);
    CHECK(g.sfx_mask & SFXBIT(SFX_SPEED_UP));
    run_until_active(1, 74 * 5 + 10);
    CHECK(g.pu_active == POWERUP_EXPIRED_ID);
    CHECK(g.per_ma == 5 && g.per_Na == 5);
    game_tick(&g, 0);                       /* draw-pass clear */
    CHECK(g.pu_active == 0);

    reset();
    g.per_ma = g.per_Na = 5;
    game_apply_powerup(&g, 2);
    CHECK(g.pu_active == 2);
    CHECK(g.per_ma == 7 && g.per_Na == 7);
    CHECK(g.sfx_mask & SFXBIT(SFX_SLOW_DOWN));
    run_until_active(2, 74 * 7 + 10);
    CHECK(g.pu_active == POWERUP_EXPIRED_ID);
    CHECK(g.per_ma == 5 && g.per_Na == 5);

    reset();
    g.per_ma = g.per_Na = 10;
    game_apply_powerup(&g, 2);
    CHECK(g.per_ma == 10);                  /* clamped */
    reset();
    g.per_ma = g.per_Na = 1;
    game_apply_powerup(&g, 1);
    CHECK(g.per_ma == 1);                   /* clamped */
}

/* ── type 3: ghost through walls ────────────────────────────────────────── */
static void test_ghost(void) {
    reset();
    memset(map, 0, sizeof map);
    map[11 + 9 * FIELD_COLS] = 1;           /* directly ahead of the head */
    g.map = map;
    game_apply_powerup(&g, 3);
    CHECK(g.pu_active == 3);
    CHECK(g.sfx_mask & SFXBIT(SFX_GHOST));
    force_movement();
    game_tick(&g, 0);
    CHECK(g.death == 0);
    CHECK(game_col(g.qx) == 11 && game_row(g.qy) == 9);

    reset();
    g.map = map;                            /* same wall, no ghost */
    force_movement();
    game_tick(&g, 0);
    CHECK(g.death == 40);
}

/* ── type 4: wall stamp + restore ───────────────────────────────────────── */
static void test_wall_stamp(void) {
    reset();
    game_apply_powerup(&g, 4);
    CHECK(g.pu_active == 4);
    CHECK(g.stamp_active);
    CHECK(!g.full_redraw);                  /* stamp is a local 3x3 repaint */
    CHECK(g.stamp_pat == 7);                /* 16 % 9 */
    CHECK(g.stamp_col == 16 && g.stamp_row == 16);
    CHECK(g.sfx_mask & SFXBIT(SFX_WALL_STAMP));
    CHECK(game_tile_at(&g, 16, 16) == 1);
    CHECK(game_tile_at(&g, 16, 17) == 9);
    CHECK(game_tile_at(&g, 17, 17) == 5);
    CHECK(game_tile_at(&g, 18, 17) == 10);
    CHECK(game_tile_at(&g, 16, 18) == 0);

    /* expiry restores the original (empty) cells */
    g.pu_timer = 1;
    force_movement();
    game_tick(&g, 0);
    CHECK(g.pu_active == POWERUP_EXPIRED_ID);
    CHECK(!g.stamp_active);
    CHECK(!g.full_redraw);                  /* expiry is a local 3x3 repaint */
    CHECK(game_tile_at(&g, 17, 17) == 0);

    /* while active the stamp blocks movement */
    reset();
    memset(map, 0, sizeof map);
    g.map = map;
    game_apply_powerup(&g, 4);              /* at (16..18, 16..18) */
    g.qx = 15 * CELL_SIZE; g.qy = 17 * CELL_SIZE;
    g.dir = DIR_RIGHT;
    for (uint8_t i = 0; i < g.len; i++) g.a[i] = DIR_RIGHT;
    force_movement();
    game_tick(&g, 0);
    CHECK(g.death == 40);                   /* (16,17) is part of the stamp */
}

/* ── type 5: next three bonus foods +24 ─────────────────────────────────── */
static void test_bonus_boost(void) {
    reset();
    g.per_ma = g.per_Na = 1;
    game_apply_powerup(&g, 5);
    CHECK(g.pu_active == 5);
    CHECK(g.pu_timer == 3);
    CHECK(g.sfx_mask & SFXBIT(SFX_BONUS_BOOST));

    for (int i = 0; i < 3; i++) {
        g.bonus = 0;
        food_ahead();
        uint8_t before = (uint8_t)g.pu_timer;
        game_tick(&g, 0);
        CHECK(g.pu_timer == before - 1);
        CHECK(g.bonus >= 25 + 24);          /* 41 + 24 with this RNG */
        CHECK(g.bonus <= 74 + 24);
    }
    CHECK(g.pu_active == POWERUP_EXPIRED_ID);
}

/* ── type 6: shrink one segment per step, floor 3 ───────────────────────── */
static void test_shrink(void) {
    reset();
    g.per_ma = g.per_Na = 1;
    g.len = 8;
    for (uint8_t i = 0; i < g.len; i++) g.a[i] = DIR_RIGHT;
    game_apply_powerup(&g, 6);
    CHECK(g.pu_active == 6 && g.pu_timer == 3);
    CHECK(g.sfx_mask & SFXBIT(SFX_SHRINK));
    for (int i = 0; i < 3; i++) {
        uint8_t before = g.len;
        game_tick(&g, 0);
        CHECK(g.len == before - 1);
        CHECK(g.shrink_removed);
    }
    CHECK(g.pu_active == POWERUP_EXPIRED_ID);

    reset();
    g.per_ma = g.per_Na = 1;
    g.len = 3;
    for (uint8_t i = 0; i < g.len; i++) g.a[i] = DIR_RIGHT;
    game_apply_powerup(&g, 6);
    game_tick(&g, 0);
    CHECK(g.len == 3);
    CHECK(g.pu_active == POWERUP_EXPIRED_ID);
}

/* ── type 7: grow one segment per step ──────────────────────────────────── */
static void test_grow(void) {
    reset();
    g.per_ma = g.per_Na = 1;
    game_apply_powerup(&g, 7);
    CHECK(g.pu_active == 7 && g.pu_timer == 3);
    CHECK(g.sfx_mask & SFXBIT(SFX_GROW));
    for (int i = 0; i < 3; i++) {
        uint8_t before = g.len;
        game_tick(&g, 0);
        CHECK(g.len == before + 1);
    }
    CHECK(g.pu_active == POWERUP_EXPIRED_ID);
}

/* ── type 8: coin rush ──────────────────────────────────────────────────── */
static void test_coin_rush(void) {
    reset();
    g.per_ma = g.per_Na = 1;
    put_slot(0, 2, 9, 24, 24);
    game_apply_powerup(&g, 8);
    CHECK(g.pu_active == 8 && g.pu_timer == 1);
    CHECK(!g.full_redraw);                  /* coin rush is per-cell repaint */
    CHECK(g.food_x == -1 && g.food_y == -1);
    CHECK(g.bonus == 0);
    CHECK(g.pu[0].id == POWERUP_EXPIRED_ID);
    CHECK(g.sfx_mask & SFXBIT(SFX_COIN_RUSH));

    for (int i = 0; i < 6; i++) game_tick(&g, 0);
    int coins = 0;
    for (int i = 0; i < FIELD_CELLS; i++) coins += (g.coins[i >> 4] >> (i & 15)) & 1;
    CHECK(coins > 0);

    /* coin at the cell the head will enter: +1 point and removed */
    int16_t x = g.qx, y = g.qy;
    switch (g.dir & 0x0F) {
        case DIR_UP: y -= CELL_SIZE; break;
        case DIR_DOWN: y += CELL_SIZE; break;
        case DIR_LEFT: x -= CELL_SIZE; break;
        default: x += CELL_SIZE; break;
    }
    if (x <= 0) x += FIELD_WIDTH_PX;
    if (x >= FIELD_WIDTH_PX) x -= FIELD_WIDTH_PX;
    if (y <= 0) y += FIELD_HEIGHT_PX;
    if (y >= FIELD_HEIGHT_PX) y -= FIELD_HEIGHT_PX;
    uint16_t idx = (uint16_t)(game_col(x) + game_row(y) * FIELD_COLS);
    g.coins[idx >> 4] |= (uint16_t)(1u << (idx & 15));
    g.pu_timer = 1;                         /* stay in the add phase */
    uint32_t score = g.score;
    game_tick(&g, 0);
    CHECK(g.score == score + 1);
    CHECK(!game_has_coin(&g, game_col(x), game_row(y)));

    int guard = 2 * FIELD_CELLS + 64;
    while (g.pu_active == 8 && guard-- > 0) game_tick(&g, 0);
    CHECK(g.pu_active == POWERUP_EXPIRED_ID);
    CHECK(!g.full_redraw);                  /* end is per-cell repaint */
    CHECK(g.food_x >= 0 && g.food_y >= 0);
    coins = 0;
    for (int i = 0; i < FIELD_CELLS; i++) coins += (g.coins[i >> 4] >> (i & 15)) & 1;
    CHECK(coins == 0);

    /* The tongue also collects coins two cells ahead. */
    reset();
    g.per_ma = g.per_Na = 1;
    game_apply_powerup(&g, 8);
    g.pu_timer = 1;
    int16_t tx = (int16_t)(g.qx + 2 * CELL_SIZE);
    if (tx >= FIELD_WIDTH_PX) tx -= FIELD_WIDTH_PX;
    uint16_t tidx = (uint16_t)(game_col(tx) + game_row(g.qy) * FIELD_COLS);
    g.coins[tidx >> 4] |= (uint16_t)(1u << (tidx & 15));
    g.tongue = -1;
    uint32_t s0 = g.score;
    game_tick(&g, 0);
    CHECK(g.score == s0 + 1);
    CHECK(!game_has_coin(&g, game_col(tx), game_row(g.qy)));
}

/* ── type 9: moving normal food, lasts five foods ───────────────────────── */
static void test_moving_food(void) {
    reset();
    g.per_ma = g.per_Na = 5;
    game_apply_powerup(&g, 9);
    CHECK(g.pu_active == 9 && g.pu_timer == 5);
    CHECK(g.move_food_timer == 32);         /* (15+16%15) * ((8-5)>>1 + 1) */
    CHECK(g.sfx_mask & SFXBIT(SFX_MOVING_FOOD));

    int16_t fx = g.food_x, fy = g.food_y;
    g.move_food_timer = 1;
    g.cnt_c = (uint8_t)(g.per_La - 1);      /* next frame is even */
    game_tick(&g, 0);
    CHECK(g.food_x != fx || g.food_y != fy);
    CHECK(g.move_food_timer == 32);

    /* five normal foods expire the effect */
    reset();
    g.per_ma = g.per_Na = 1;
    game_apply_powerup(&g, 9);
    g.move_food_timer = 1000;
    for (int i = 0; i < 5; i++) {
        g.move_food_timer = 1000;
        food_ahead();
        uint8_t before = (uint8_t)g.pu_timer;
        game_tick(&g, 0);
        CHECK(g.pu_timer == before - 1);
    }
    CHECK(g.pu_active == POWERUP_EXPIRED_ID);
}

/* ── type 10: lights out ────────────────────────────────────────────────── */
static void test_lights_out(void) {
    reset();
    g.per_ma = g.per_Na = 5;
    game_apply_powerup(&g, 10);
    CHECK(g.pu_active == 10);
    CHECK(g.lights_out);
    CHECK((g.lights_timer & 0xFF) == 16);   /* 10 + 16%10 */
    CHECK(g.sfx_mask & SFXBIT(SFX_LIGHTS_OUT));

    bool u = g.lights_out;
    int i;
    for (i = 0; i < 20 && g.lights_out == u; i++) game_tick(&g, 0);
    CHECK(i < 20);                          /* toggled on schedule */
    CHECK(g.full_redraw);

    /* expiry turns the lights back on */
    g.pu_timer = 1;
    force_movement();
    game_tick(&g, 0);
    CHECK(g.pu_active == POWERUP_EXPIRED_ID);
    CHECK(!g.lights_out);
}

/* ── slots, pickup (head + tongue), lifetime, consumed-id cleanup ───────── */
static void test_slots(void) {
    reset();
    g.per_ma = g.per_Na = 1;
    put_slot(0, 2, 10, g.qx + CELL_SIZE, g.qy);
    game_tick(&g, 0);
    CHECK(g.pu_active == 2);
    CHECK(g.pu[0].id == POWERUP_EXPIRED_ID);
    CHECK(g.per_ma == 3);                   /* slow down from the pickup */
    game_tick(&g, 0);
    CHECK(g.pu[0].id == 0);                 /* cleared at the next tick */

    reset();
    g.per_ma = g.per_Na = 1;
    g.tongue = -1;                          /* lash this tick */
    put_slot(0, 3, 10, g.qx + 2 * CELL_SIZE, g.qy);
    game_tick(&g, 0);
    CHECK(g.pu_active == 3);

    reset();
    g.per_La = 5;
    put_slot(0, 3, 1, 24, 24);
    g.cnt_c = (uint8_t)(g.per_La - 1);      /* lifetime ticks on even frames */
    game_tick(&g, 0);
    CHECK(g.pu[0].id == POWERUP_EXPIRED_ID);
    game_tick(&g, 0);
    CHECK(g.pu[0].id == 0);
}

/* The original decrements ba even when Aa == 0 (b.java:1526), e.g. after an expired
 * slot briefly set Aa=15 and the HUD draw pass cleared it. */
static void test_zero_active_timer(void) {
    reset();
    g.pu_active = 0;
    g.pu_timer = 2;
    g.per_Na = 5;
    g.cnt_b = 1;
    g.cnt_C = 4;
    game_tick(&g, 0);
    CHECK(g.pu_active == 0 && g.pu_timer == 1);
    g.cnt_b = 1;
    g.cnt_C = 4;
    game_tick(&g, 0);
    CHECK(g.pu_active == POWERUP_EXPIRED_ID && g.pu_timer == 0);
    game_tick(&g, 0);
    CHECK(g.pu_active == 0);
}

int main(void) {
    test_speed();
    test_ghost();
    test_wall_stamp();
    test_bonus_boost();
    test_shrink();
    test_grow();
    test_coin_rush();
    test_moving_food();
    test_lights_out();
    test_zero_active_timer();
    test_slots();
    if (fails) printf("powerup: %d FAILED\n", fails);
    else printf("powerup: all passed (10 types, slots, pickup, coin rush, stamp, timers)\n");
    return fails != 0;
}
