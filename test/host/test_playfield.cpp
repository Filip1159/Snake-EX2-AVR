/* G5: the dirty-cell renderer must produce exactly the same pixels as a full redraw.
 * Drives real games (random input, restarts after death) across all mazes, and injects
 * item changes (food moves, bonus spawn/expire) to exercise the overlay paths. After every
 * tick the incremental framebuffer is compared with a fresh playfield_draw_all. */
#include "assets/maze.h"
#include "game/game.h"
#include "render/playfield.h"
#include "drivers/st7735.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint16_t fb[160][128];
static uint16_t dirty[160][128];
static uint8_t st_x, st_y, st_w, st_h, st_px;

extern "C" {
void st7735_fill_screen(uint16_t c) { for (auto &r : fb) for (auto &p : r) p = c; }
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) fb[y + j][x + i] = c;
}
void st7735_blit(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *b) {
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) fb[y + j][x + i] = b[j * w + i];
}
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t h) { st_x = x; st_y = y; st_w = w; st_h = h; st_px = 0; }
void st7735_stream_pixels(uint16_t c, uint8_t n) {
    while (n--) { fb[st_y + st_px / st_w][st_x + st_px % st_w] = c; st_px++; }
}
void st7735_stream_end(void) { if (st_px != st_w * st_h) { printf("stream wrote %d of %d px\n", st_px, st_w * st_h); exit(3); } }
}

/* deterministic pseudo-random input */
static uint32_t rs = 12345;
static uint32_t next(void) { rs = rs * 1103515245u + 12345u; return rs >> 16; }
extern "C" int32_t game_rand_int(void) { return (int32_t)(next() | ((uint32_t)next() << 16)); }

static uint8_t maze_map[MAZE_CELLS];

/* is this cell occupied by the snake? */
static bool occupied(const game_t *g, uint8_t col, uint8_t row) {
    int16_t x = g->qx, y = g->qy;
    for (int16_t i = (int16_t)g->len - 1; i >= 0; i--) {
        if (game_col(x) == col && game_row(y) == row) return true;
        switch (g->a[i] & 0x0F) {
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
    return false;
}

static int coin_count_for_test(const game_t *g) {
    int n = 0;
    for (int i = 0; i < FIELD_CELLS; i++) n += (g->coins[i >> 4] >> (i & 15)) & 1;
    return n;
}

/* Run one incremental update and compare it with a fresh full redraw. */
static int render_check_ex(game_t *g, playfield_cache_t *c, const char *label, bool expect_full) {
    bool full = playfield_update(g, c);
    if (full != expect_full) {
        printf("FAIL render %s: full-screen refresh=%d, expected %d\n",
               label, (int)full, (int)expect_full);
        return 1;
    }
    memcpy(dirty, fb, sizeof fb);
    playfield_cache_t c2;
    playfield_draw_all(g, &c2);
    if (memcmp(dirty, fb, sizeof fb) != 0) {
        int bad = 0, bx = -1, by = -1;
        for (int y = 0; y < 160; y++) for (int x = 0; x < 128; x++)
            if (dirty[y][x] != fb[y][x]) { if (bad++ == 0) { bx = x; by = y; } }
        printf("FAIL render %s: %d px differ, first (%d,%d) pu=%u lights=%d stamp=%d coins=%d\n",
               label, bad, bx, by, g->pu_active, (int)g->lights_out, (int)g->stamp_active,
               coin_count_for_test(g));
        return 1;
    }
    return 0;
}

static int render_check(game_t *g, playfield_cache_t *c, const char *label) {
    return render_check_ex(g, c, label, false);
}

static void open_game(game_t *g, playfield_cache_t *c) {
    game_init_mode(g, 5, -1, nullptr, true);
    g->level_start_score = 1000000000u;   /* renderer sandbox: no campaign portals */
    g->food_x = g->food_y = -1;
    g->bonus = 0;
    g->len = 6;
    g->dir = DIR_RIGHT;
    for (uint8_t i = 0; i < g->len; i++) g->a[i] = DIR_RIGHT;
    g->qx = 60; g->qy = 54;
    g->full_redraw = false;
    playfield_draw_all(g, c);
}

/* G7 layers: slot sprites, coin bitmap (including edge overhang), coin pickup, wall stamp
 * base overlay near the head, lights-out black base, and the shrink-tail erase branch. */
static int test_g7_layers(void) {
    static game_t g;
    playfield_cache_t cache;
    int fails = 0;
    open_game(&g, &cache);

    g.pu[0].id = 1; g.pu[0].life = 50; g.pu[0].x = 60; g.pu[0].y = 60;
    fails += render_check(&g, &cache, "slot spawn");
    g.pu[0].x = 72;
    fails += render_check(&g, &cache, "slot move");
    g.pu[0].x = 0; g.pu[0].y = 0;
    fails += render_check(&g, &cache, "slot edge 0,0");
    g.pu[0].x = (FIELD_COLS - 1) * CELL_SIZE; g.pu[0].y = (FIELD_ROWS - 1) * CELL_SIZE;
    fails += render_check(&g, &cache, "slot edge last");
    g.pu[0].id = 0;
    fails += render_check(&g, &cache, "slot remove");

    /* Coin rush start, add at both edges, add under the head, remove, and finish. */
    g.pu_active = 8;
    g.full_redraw = false;
    memset(g.coins, 0, sizeof g.coins);
    g.coin_added = g.coin_removed = -1;
    fails += render_check(&g, &cache, "coin start");
    g.coins[0] |= 1;
    g.coin_added = 0;
    fails += render_check(&g, &cache, "coin 0,0");
    int last = (FIELD_COLS - 1) + (FIELD_ROWS - 1) * FIELD_COLS;
    g.coins[last >> 4] |= (uint16_t)(1u << (last & 15));
    g.coin_added = (int16_t)last;
    fails += render_check(&g, &cache, "coin last");
    int16_t ax = g.qx + CELL_SIZE;
    if (ax >= FIELD_WIDTH_PX) ax -= FIELD_WIDTH_PX;
    int ahead = game_col(ax) + game_row(g.qy) * FIELD_COLS;
    g.coins[ahead >> 4] |= (uint16_t)(1u << (ahead & 15));
    g.coin_added = (int16_t)ahead;
    fails += render_check(&g, &cache, "coin head open");
    g.coins[0] &= (uint16_t)~1u;
    g.coin_added = -1;
    g.coin_removed = 0;
    fails += render_check(&g, &cache, "coin removed");
    /* Remove the remaining coins the way the real scan does (one cell per frame). */
    g.coins[last >> 4] &= (uint16_t)~(1u << (last & 15));
    g.coin_removed = (int16_t)last;
    fails += render_check(&g, &cache, "coin end last");
    g.coins[ahead >> 4] &= (uint16_t)~(1u << (ahead & 15));
    g.coin_removed = (int16_t)ahead;
    fails += render_check(&g, &cache, "coin end ahead");
    /* The bitmap is empty now; ending the rush must not full-refresh. */
    g.pu_active = 0;
    g.full_redraw = false;
    g.coin_added = g.coin_removed = -1;
    fails += render_check(&g, &cache, "coin end");

    /* A real coin pickup on a movement tick: the head erases the coin under it. */
    g.pu_active = 8;
    g.full_redraw = false;
    memset(g.coins, 0, sizeof g.coins);
    g.coin_added = g.coin_removed = -1;
    fails += render_check(&g, &cache, "coin restart");
    ax = g.qx + CELL_SIZE;
    if (ax >= FIELD_WIDTH_PX) ax -= FIELD_WIDTH_PX;
    ahead = game_col(ax) + game_row(g.qy) * FIELD_COLS;
    g.coins[ahead >> 4] |= (uint16_t)(1u << (ahead & 15));
    g.pu_timer = 1;
    g.per_ma = g.per_Na = 1;
    g.cnt_b = g.cnt_C = 0;
    game_tick(&g, 0);
    fails += render_check(&g, &cache, "coin pickup");
    /* The scan added a coin at odd index 1 on that tick; remove it explicitly. */
    g.coins[1 >> 4] &= (uint16_t)~(1u << (1 & 15));
    g.coin_removed = 1;
    g.coin_added = -1;
    g.pu_active = 0;
    g.full_redraw = false;
    fails += render_check(&g, &cache, "coin end 2");

    /* Forced wall stamp at (12,9), then move the head next to it so the dirty cells
     * include stamped wall tiles; finally expire it. None of these may full-refresh. */
    open_game(&g, &cache);
    g.stamp_active = true; g.stamp_col = 12; g.stamp_row = 9; g.stamp_pat = 4;
    g.full_redraw = false;
    fails += render_check(&g, &cache, "stamp on");
    g.stamp_col = 13;
    fails += render_check(&g, &cache, "stamp move");
    g.stamp_col = 12;
    fails += render_check(&g, &cache, "stamp move back");
    g.per_ma = g.per_Na = 1;
    g.cnt_b = g.cnt_C = 0;
    game_tick(&g, 0);
    fails += render_check(&g, &cache, "stamp near move");
    g.stamp_active = false;
    g.full_redraw = false;
    fails += render_check(&g, &cache, "stamp off");

    /* Lights out still needs a genuine full background repaint. */
    g.lights_out = true;
    g.full_redraw = true;
    fails += render_check_ex(&g, &cache, "lights on", true);
    g.per_ma = g.per_Na = 1;
    g.cnt_b = g.cnt_C = 0;
    game_tick(&g, 0);
    fails += render_check(&g, &cache, "lights move");
    g.lights_out = false;
    g.full_redraw = true;
    fails += render_check_ex(&g, &cache, "lights off", true);

    /* Shrink's extra tail erase (renderer branch). */
    g.full_redraw = false;
    g.shrink_removed = true;
    g.shrink_x = 12; g.shrink_y = 54;
    fails += render_check(&g, &cache, "shrink cell");
    g.shrink_removed = false;

    return fails;
}

/* G8 portal: appearance, movement of the (test-only) target, entry state change, removal,
 * edge clipping and overlap with the head. All transitions must stay local. */
static int test_g8_portal(void) {
    static game_t g;
    playfield_cache_t cache;
    int fails = 0;
    open_game(&g, &cache);

    g.portal_state = 1;
    g.portal_x = 60; g.portal_y = 60;
    g.full_redraw = false;
    fails += render_check(&g, &cache, "portal on");

    g.portal_x = 72;                     /* not done by the engine, but covers old+new cells */
    fails += render_check(&g, &cache, "portal move");

    g.portal_state = 2;                  /* entry: same sprite, same pixels */
    fails += render_check(&g, &cache, "portal shrink state");

    g.portal_state = 0;
    g.full_redraw = false;
    fails += render_check(&g, &cache, "portal off");

    /* Sprite overhang at both playfield edges. */
    g.portal_state = 1;
    g.portal_x = 2 * CELL_SIZE; g.portal_y = 2 * CELL_SIZE;
    fails += render_check(&g, &cache, "portal edge first");
    g.portal_x = (FIELD_COLS - 1) * CELL_SIZE;
    g.portal_y = (FIELD_ROWS - 1) * CELL_SIZE;
    fails += render_check(&g, &cache, "portal edge last");

    /* Portal under the head (10x10 sprite surrounds the 6x6 head). */
    g.portal_x = g.qx; g.portal_y = g.qy;
    fails += render_check(&g, &cache, "portal under head");

    g.portal_state = 0;
    g.full_redraw = false;
    fails += render_check(&g, &cache, "portal off after head");
    return fails;
}

int main() {
    int fails = 0, ticks = 0, moves = 0, bonus_spawns = 0, food_moves = 0;
    fails += test_g7_layers();
    fails += test_g8_portal();
    for (int m = -1; m < 5; m++) {
        if (m >= 0) maze_load((uint8_t)m, maze_map);
        static game_t g;
        playfield_cache_t cache;
        game_init_mode(&g, 5, (int8_t)m, m >= 0 ? maze_map : nullptr, true);  /* Extreme: tongue */
        g.level_start_score = 1000000000u;   /* renderer sandbox: no campaign portals */
        playfield_draw_all(&g, &cache);

        for (int t = 0; t < 6000; t++) {
            if (g.game_over) {
                game_init_mode(&g, 5, (int8_t)m, m >= 0 ? maze_map : nullptr, true);
                g.level_start_score = 1000000000u;
                playfield_draw_all(&g, &cache);
                bonus_spawns = bonus_spawns; /* keep going */
            }

            /* inject item changes to cover spawn/expire/erase paths the random game rarely hits */
            if (t % 97 == 0 && g.bonus <= 0) {
                for (int tries = 0; tries < 50; tries++) {
                    uint8_t c = (uint8_t)(2 + next() % (FIELD_COLS - 4));
                    uint8_t r = (uint8_t)(2 + next() % (FIELD_ROWS - 4));
                    uint16_t idx = (uint16_t)(c + r * FIELD_COLS);
                    if ((g.map && g.map[idx]) || occupied(&g, c, r)) continue;
                    if (c * CELL_SIZE == g.food_x && r * CELL_SIZE == g.food_y) continue;
                    g.bonus = 30;
                    g.bonus_x = (int16_t)(c * CELL_SIZE);
                    g.bonus_y = (int16_t)(r * CELL_SIZE);
                    g.bonus_type = (uint8_t)(1 + (t / 97) % 7);
                    g.bonus_dir = 1;
                    bonus_spawns++;
                    break;
                }
            }
            if (t % 61 == 0) {           /* move food to a free cell (renderer must follow it) */
                for (int tries = 0; tries < 50; tries++) {
                    uint8_t c = (uint8_t)(2 + next() % (FIELD_COLS - 4));
                    uint8_t r = (uint8_t)(2 + next() % (FIELD_ROWS - 4));
                    uint16_t idx = (uint16_t)(c + r * FIELD_COLS);
                    if ((g.map && g.map[idx]) || occupied(&g, c, r)) continue;
                    if (c * CELL_SIZE == g.bonus_x && r * CELL_SIZE == g.bonus_y) continue;
                    g.food_x = (int16_t)(c * CELL_SIZE);
                    g.food_y = (int16_t)(r * CELL_SIZE);
                    food_moves++;
                    break;
                }
            }
            /* put food 1-2 cells ahead so the tongue picks it up (distance grab + erase) */
            if (t % 53 == 0) {
                int k = 1 + (t / 53) % 2;
                int16_t fx = g.qx, fy = g.qy;
                switch (g.dir & 0x0F) {
                    case DIR_UP: fy -= k * CELL_SIZE; break;
                    case DIR_DOWN: fy += k * CELL_SIZE; break;
                    case DIR_LEFT: fx -= k * CELL_SIZE; break;
                    default: fx += k * CELL_SIZE; break;
                }
                if (fx <= 0) fx += FIELD_WIDTH_PX;
                if (fx >= FIELD_WIDTH_PX) fx -= FIELD_WIDTH_PX;
                if (fy <= 0) fy += FIELD_HEIGHT_PX;
                if (fy >= FIELD_HEIGHT_PX) fy -= FIELD_HEIGHT_PX;
                uint8_t c = (uint8_t)(fx / CELL_SIZE), r = (uint8_t)(fy / CELL_SIZE);
                uint16_t idx = (uint16_t)(c + r * FIELD_COLS);
                if (!(g.map && g.map[idx]) && !occupied(&g, c, r) &&
                    !(c * CELL_SIZE == g.bonus_x && r * CELL_SIZE == g.bonus_y)) {
                    g.food_x = fx; g.food_y = fy;
                    food_moves++;
                }
            }

            uint32_t r = next();
            uint8_t keys = 0;
            switch (r % 6) {                       /* mostly straight, some turns */
                case 0: keys = KEY_LEFT; break;
                case 1: keys = KEY_RIGHT; break;
                case 2: keys = KEY_UP; break;
                case 3: keys = KEY_DOWN; break;
                case 4: keys = (r & 0x100) ? KEY_CW : KEY_CCW; break;
                default: break;
            }
            if (t % 3 == 0) keys |= KEY_FIRE;   /* lash often (Extreme) */
            game_tick(&g, keys);
            playfield_update(&g, &cache);
            ticks++;
            if (g.moved) moves++;

            memcpy(dirty, fb, sizeof fb);
            playfield_cache_t c2;
            playfield_draw_all(&g, &c2);
            if (memcmp(dirty, fb, sizeof fb) != 0) {
                int bad = 0, bx = -1, by = -1;
                for (int y = 0; y < 160; y++) for (int x = 0; x < 128; x++)
                    if (dirty[y][x] != fb[y][x]) { if (bad++ == 0) { bx = x; by = y; } }
                int nc = 0;
                for (int i = 0; i < FIELD_CELLS; i++) nc += (g.coins[i >> 4] >> (i & 15)) & 1;
                printf("FAIL maze %d tick %d (moved=%d len=%u tail=%d bonus=%d food=%d,%d tongue=%d head=%d,%d dir=%u): %d px differ, first (%d,%d)\n",
                       m, t, (int)g.moved, g.len, (int)g.tail_removed, g.bonus, g.food_x, g.food_y,
                       (int)g.tongue, g.qx, g.qy, g.dir, bad, bx, by);
                printf("  pu=%u timer=%d lights=%d stamp=%d coins=%d slots=%u/%u/%u/%u added=%d removed=%d shrink=%d\n",
                       g.pu_active, g.pu_timer, (int)g.lights_out, (int)g.stamp_active, nc,
                       g.pu[0].id, g.pu[1].id, g.pu[2].id, g.pu[3].id,
                       g.coin_added, g.coin_removed, (int)g.shrink_removed);
                fails++;
                break;
            }
        }
    }
    if (fails) printf("playfield: %d FAILED\n", fails);
    else printf("playfield: all passed (%d ticks, %d moves, %d bonus spawns, %d food moves)\n",
                ticks, moves, bonus_spawns, food_moves);
    return fails != 0;
}
