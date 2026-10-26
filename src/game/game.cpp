#include "game/game.h"
#include "game/rng.h"
#include <avr/pgmspace.h>
#include <string.h>

#define W_PX FIELD_WIDTH_PX
#define H_PX FIELD_HEIGHT_PX

/* ── power-up tables ─────────────────────────────────────────────────────── */

/** Original ua[9][9]: 3x3 wall-stamp patterns, values are wall tile IDs. */
static const uint8_t STAMP_PATTERNS[9][9] PROGMEM = {
    {0, 0, 0, 4, 5, 6, 0, 0, 0},
    {0, 1, 0, 0, 2, 0, 0, 3, 0},
    {0, 0, 0, 0, 7, 6, 0, 3, 0},
    {0, 1, 0, 4,10, 0, 0, 0, 0},
    {0, 1, 0, 4,15, 6, 0, 3, 0},
    {0, 4, 8, 0, 0, 2, 0, 4,10},
    {0, 1, 0, 0, 2, 0, 4,12, 6},
    {1, 0, 1, 9, 5,10, 0, 0, 0},
    {0, 7, 6, 0, 2, 0, 4,10, 0},
};

/* ── helpers ─────────────────────────────────────────────────────────────── */

/** Java: Math.abs(random.nextInt()) % m  (abs(INT_MIN) stays negative, like Java). */
static int32_t rnd_abs_mod(int32_t m) {
    int32_t x = game_rand_int();
    if (x < 0) x = (int32_t)(0u - (uint32_t)x);
    return x % m;
}

static void wrap_x(int16_t *v) { if (*v <= 0) *v += W_PX; if (*v >= W_PX) *v -= W_PX; }
static void wrap_y(int16_t *v) { if (*v <= 0) *v += H_PX; if (*v >= H_PX) *v -= H_PX; }

/** Cells one and two ahead of the head, wrapped: the original's persistent k/l and K/L
 *  fields.  The engine updates them on movement ticks, but the pickup checks run on the
 *  independent C cadence, so they must stay valid between moves (b.java:1517, 1560). */
static void tongue_reach(const game_t *g, int16_t *t1x, int16_t *t1y,
                         int16_t *t2x, int16_t *t2y) {
    uint8_t hd = g->dir & 0x0F;
    int16_t x = g->qx, y = g->qy;
    switch (hd) {
        case DIR_UP:    y -= CELL_SIZE; break;
        case DIR_DOWN:  y += CELL_SIZE; break;
        case DIR_LEFT:  x -= CELL_SIZE; break;
        default:        x += CELL_SIZE; break;
    }
    wrap_x(&x); wrap_y(&y);
    *t1x = x; *t1y = y;
    switch (hd) {
        case DIR_UP:    y -= CELL_SIZE; break;
        case DIR_DOWN:  y += CELL_SIZE; break;
        case DIR_LEFT:  x -= CELL_SIZE; break;
        default:        x += CELL_SIZE; break;
    }
    wrap_x(&x); wrap_y(&y);
    *t2x = x; *t2y = y;
}

/** Step one cell back along direction d (inverse of the movement). */
static void step_back(int16_t *x, int16_t *y, uint8_t d) {
    switch (d & 0x0F) {
        case DIR_UP:    *y += CELL_SIZE; break;
        case DIR_DOWN:  *y -= CELL_SIZE; break;
        case DIR_LEFT:  *x += CELL_SIZE; break;
        case DIR_RIGHT: *x -= CELL_SIZE; break;
    }
    wrap_x(x);
    wrap_y(y);
}

/** Mark every segment cell in the occupancy bitmap (used only while spawning food). */
static void mark_snake(game_t *g) {
    memset(g->occ, 0, sizeof g->occ);
    int16_t x = g->qx, y = g->qy;
    for (int16_t i = (int16_t)g->len - 1; i >= 0; i--) {
        uint16_t idx = (uint16_t)(x / CELL_SIZE) + (uint16_t)(y / CELL_SIZE) * FIELD_COLS;
        g->occ[idx >> 3] |= (uint8_t)(1 << (idx & 7));
        step_back(&x, &y, g->a[i]);
    }
}

static bool occupied(const game_t *g, uint16_t idx) {
    return (g->occ[idx >> 3] >> (idx & 7)) & 1;
}

/** Maze tile at a cell, honouring an active wall stamp (which replaces the 3x3 block,
 *  including any original wall under a zero pattern cell). */
uint8_t game_tile_at(const game_t *g, uint8_t col, uint8_t row) {
    if (g->stamp_active &&
        col >= g->stamp_col && col < g->stamp_col + 3 &&
        row >= g->stamp_row && row < g->stamp_row + 3) {
        return pgm_read_byte(&STAMP_PATTERNS[g->stamp_pat][(row - g->stamp_row) * 3 + (col - g->stamp_col)]);
    }
    if (!g->map) return 0;
    return (uint8_t)(g->map[row * FIELD_COLS + col] & 0x0F);
}

bool game_has_coin(const game_t *g, uint8_t col, uint8_t row) {
    uint16_t idx = (uint16_t)(col + row * FIELD_COLS);
    return (g->coins[idx >> 4] >> (idx & 15)) & 1;
}

static bool coin_get(const game_t *g, uint16_t idx) {
    return (g->coins[idx >> 4] >> (idx & 15)) & 1;
}

static bool blocked(const game_t *g, int col, int row) {
    uint16_t idx = (uint16_t)(col + row * FIELD_COLS);
    /* Original a(): Ca[cell] != 0 rejects walls, snake bits and coin overlays. */
    return occupied(g, idx) || game_tile_at(g, (uint8_t)col, (uint8_t)row) != 0 ||
           coin_get(g, idx);
}

static void coin_set(game_t *g, uint16_t idx) {
    g->coins[idx >> 4] |= (uint16_t)(1u << (idx & 15));
}
static void coin_clear(game_t *g, uint16_t idx) {
    g->coins[idx >> 4] &= (uint16_t)~(1u << (idx & 15));
}
static void coin_collect(game_t *g, int16_t x, int16_t y) {
    uint16_t idx = (uint16_t)(game_col(x) + game_row(y) * FIELD_COLS);
    if (coin_get(g, idx)) {
        coin_clear(g, idx);
        g->score++;                    /* original ++H, not score_step */
    }
}

/* Original a(int[],int[]): choose a food cell. Result in spawn_x/y (px). Reproduces its quirks:
 * candidate is rejected when its column/row equals the previous spawn's, and it is compared
 * (cell vs pixel units) with the food/slot positions. */
static void spawn_cell(game_t *g) {
    int n3 = 0, n2, n;
    bool bl;
    do {
        bl = false;
        n2 = (int)rnd_abs_mod(FIELD_COLS - 4) + 2;
        n  = (int)rnd_abs_mod(FIELD_ROWS - 4) + 2;
        if (n2 == g->spawn_x / CELL_SIZE || n == g->spawn_y / CELL_SIZE) bl = true;
        if (n2 == g->food_x && n == g->food_y) bl = true;
        if (n2 == g->bonus_x && n == g->bonus_y) bl = true;
        if (g->extreme) {
            /* Original a() bug: the portal test compares the cell column with both
             * pixel coordinates (`n2 == X && n2 == y`), not (n2,n). Kept for fidelity. */
            if (n2 == g->portal_x && n2 == g->portal_y) bl = true;
            for (uint8_t i = 0; i < POWERUP_SLOTS; i++)
                if (g->pu[i].id != 0 && n2 == g->pu[i].x && n == g->pu[i].y) { bl = true; break; }
        }
        if (n2 >= FIELD_COLS || n >= FIELD_ROWS || blocked(g, n2, n)) bl = true;
    } while (bl && ++n3 < 8);

    if (n3 >= 8) {
        n2 = 2; n = 2; n3 = 0;
        do {
            bl = false;
            if (n2 == g->spawn_x / CELL_SIZE || n == g->spawn_y / CELL_SIZE) bl = true;
            if (n2 == g->food_x && n == g->food_y) bl = true;
            if (n2 == g->bonus_x && n == g->bonus_y) bl = true;
            if (g->extreme) {
                if (n2 == g->portal_x && n2 == g->portal_y) bl = true;
                for (uint8_t i = 0; i < POWERUP_SLOTS; i++)
                    if (g->pu[i].id != 0 && n2 == g->pu[i].x && n == g->pu[i].y) { bl = true; break; }
            }
            if (n2 < FIELD_COLS && n < FIELD_ROWS && blocked(g, n2, n)) bl = true;
            ++n3;
            if (++n2 <= FIELD_COLS - 4) continue;
            n2 = 2;
            if (++n <= FIELD_ROWS - 4) continue;
            n2 = FIELD_COLS - 4;
            n = FIELD_ROWS - 4;
            bl = false;
        } while (bl);
    }
    g->spawn_x = n2 * CELL_SIZE;
    g->spawn_y = n * CELL_SIZE;
}

/* Original a(): advance to the next Extreme maze. Score is preserved; z snapshots it,
 * x increments and the maze cycles -1,0,1,2,3,4. The snake and all field objects reset. */
static void next_level(game_t *g) {
    g->level_start_score = g->score;
    g->level = (uint16_t)(g->level + 1);

    uint8_t idx = (uint8_t)((g->level - 1u) % EXTREME_MAZE_COUNT);
    int8_t maze = (idx == 0) ? -1 : (int8_t)(idx - 1);
    g->maze = maze;
    if (maze < 0) {
        g->map = NULL;
    } else if (g->map_buf && g->maze_loader &&
               g->maze_loader((uint8_t)maze, g->map_buf) == 0) {
        g->map = g->map_buf;
    } else {
        g->map = NULL;
    }

    g->len = START_LENGTH;
    g->qx = START_HEAD_X;
    g->qy = START_HEAD_Y;
    g->dir = DIR_RIGHT | 0x20;          /* original j = 40 */
    for (uint8_t i = 0; i < START_LENGTH; i++) g->a[i] = g->dir;

    g->food_x = START_FOOD_X;
    g->food_y = START_FOOD_Y;
    g->bonus = 0;
    /* Original a() leaves ga/Ga/Fa/ha/Ia (and Ea, U/v, ka) untouched; they are
     * invisible while fa == 0 but must match the engine state exactly. */

    g->pu_active = 0;
    g->pu_timer = 0;
    for (uint8_t i = 0; i < POWERUP_SLOTS; i++) {
        g->pu[i].id = 0;
        g->pu[i].life = 0;
        g->pu[i].x = 0;
        g->pu[i].y = 0;
    }
    memset(g->coins, 0, sizeof g->coins);

    g->portal_state = 0;
    g->per_ma = g->per_La = g->per_Ma = g->per_Na =
        (uint8_t)(SPEED_FRAMES_BASE + (SPEED_OFFSET - g->speed));

    g->moved = false;
    g->tail_removed = false;
    g->food_eaten = false;
    g->bonus_eaten = false;
    g->shrink_removed = false;
    g->full_redraw = true;
}

void game_set_maze_source(game_t *g, uint8_t *buf,
                          int8_t (*loader)(uint8_t index, uint8_t *buf)) {
    g->map_buf = buf;
    g->maze_loader = loader;
}

/** Direction byte (with turn flags) for a new head direction; unchanged if `by` is not a plain direction. */
static uint8_t head_byte(uint8_t prev, uint8_t by) {
    uint8_t pd = prev & 0x0F;
    switch (by) {
        case DIR_UP:
            if (pd == DIR_UP) by |= prev & 0x60; else if (pd == DIR_LEFT) by |= 0x20; else if (pd == DIR_RIGHT) by |= 0x40;
            break;
        case DIR_DOWN:
            if (pd == DIR_DOWN) by |= prev & 0x60; else if (pd == DIR_LEFT) by |= 0x40; else if (pd == DIR_RIGHT) by |= 0x20;
            break;
        case DIR_LEFT:
            if (pd == DIR_UP) by |= 0x40; else if (pd == DIR_DOWN) by |= 0x20; else if (pd == DIR_LEFT) by |= prev & 0x60;
            break;
        case DIR_RIGHT:
            if (pd == DIR_UP) by |= 0x20; else if (pd == DIR_DOWN) by |= 0x40; else if (pd == DIR_RIGHT) by |= prev & 0x60;
            break;
    }
    return by;
}

/* ── power-ups (original B(), c(), C()) ─────────────────────────────────── */

/** Original B(): try up to five random placements of ua[pat] (3x3). Returns true on failure
 *  (the original's counter-intuitive convention); on success stores T/u/pattern and marks a
 *  full background repaint. Food/slot checks keep the original cell-vs-pixel comparisons. */
static bool wall_stamp_try(game_t *g) {
    int attempts = 0, x = 0, y = 0, pat = 0;
    bool bl;
    do {
        bl = false;
        pat = (int)rnd_abs_mod(9);
        x = (int)rnd_abs_mod(FIELD_COLS);
        y = (int)rnd_abs_mod(FIELD_ROWS);
        if (x < 2 || y < 2 || x > FIELD_COLS - 4 || y > FIELD_ROWS - 4) bl = true;
        if (!bl && game_tile_at(g, x, y) != 0) bl = true;
        for (int r = 0; r < 3 && !bl; r++) {
            for (int c = 0; c < 3; c++) {
                uint8_t v = pgm_read_byte(&STAMP_PATTERNS[pat][c + r * 3]);
                if (v == 0) continue;
                if (x + c == g->food_x && y + r == g->food_y) { bl = true; break; }
                /* Original B() compares the cell coordinate with the portal's pixel
                 * coordinates (n4+n2 == X && n3+n7 == y), another cell-vs-pixel bug. */
                if (x + c == g->portal_x && y + r == g->portal_y) { bl = true; break; }
                for (uint8_t n = 0; n < POWERUP_SLOTS; n++) {
                    if (g->pu[n].id != 0 && g->pu[n].x == x + c && g->pu[n].y == y + r) {
                        bl = true; break;
                    }
                }
                if (bl) break;
                if (game_tile_at(g, x + c, y + r) != 0) { bl = true; break; }
            }
        }
    } while (bl && ++attempts < 5);

    if (attempts < 5) {
        g->stamp_active = true;
        g->stamp_col = (uint8_t)x;
        g->stamp_row = (uint8_t)y;
        g->stamp_pat = (uint8_t)pat;
        /* The original sets Ka (full background redraw) here. The port repaints only
         * the 3x3 stamp block and leaves the rest of the screen untouched. */
    }
    return bl;
}

/** Original C(): undo the active effect (does not clear Aa itself). */
static void pu_deactivate(game_t *g) {
    switch (g->pu_active) {
        case 1:
        case 2:
            g->per_ma = g->pu_saved_period;
            g->per_Na = g->pu_saved_period;
            break;
        case 4:
            /* Repaint only the old 3x3 block; no full screen refresh. */
            g->stamp_active = false;
            break;
        case 10:
            g->lights_out = false;
            g->full_redraw = true;
            break;
        default:
            break;
    }
}

/** Original c(): apply the effect of g->pu_active and start its counter. */
static void pu_activate(game_t *g) {
    g->pu_saved_period = g->per_ma;
    g->pu_timer = (int16_t)(POWERUP_DURATION_BASE + rnd_abs_mod(POWERUP_DURATION_RAND));

    switch (g->pu_active) {
        case 3:
            g->sfx_mask |= SFXBIT(SFX_GHOST);
            break;

        case 4:
            if (!wall_stamp_try(g)) {
                g->sfx_mask |= SFXBIT(SFX_WALL_STAMP);
                break;
            }
            g->pu_active = 1;
            /* fall through */

        case 1: {
            g->sfx_mask |= SFXBIT(SFX_SPEED_UP);
            int v = (int)g->per_ma - SPEED_UP_DELTA;
            if (v < SPEED_MIN_FRAMES) v = SPEED_MIN_FRAMES;
            g->per_ma = (uint8_t)v;
            g->per_Na = (uint8_t)v;
            break;
        }

        case 8:
            g->sfx_mask |= SFXBIT(SFX_COIN_RUSH);
            g->pu_timer = 1;
            g->bonus = 0;
            for (uint8_t i = 0; i < POWERUP_SLOTS; i++) {
                g->pu[i].id = POWERUP_EXPIRED_ID;
                g->pu[i].life = 0;
            }
            g->food_x = -1;
            g->food_y = -1;
            /* No Ka: the renderer erases the old food/bonus/slots and the per-frame
             * coin scan repaints individual cells. */
            break;

        case 9:
            g->sfx_mask |= SFXBIT(SFX_MOVING_FOOD);
            g->move_food_timer = (int16_t)((MOVING_FOOD_TIMER_MIN + rnd_abs_mod(15)) *
                                           (((8 - g->speed) >> 1) + 1));
            g->pu_timer = 5;
            break;

        case 10:
            g->sfx_mask |= SFXBIT(SFX_LIGHTS_OUT);
            g->lights_out = true;
            g->lights_timer = 0x02020200u + (uint32_t)(10 + rnd_abs_mod(10));
            g->full_redraw = true;
            break;

        case 5:
            g->sfx_mask |= SFXBIT(SFX_BONUS_BOOST);
            g->pu_timer = 3;
            break;

        case 6:
            g->sfx_mask |= SFXBIT(SFX_SHRINK);
            g->pu_timer = 3;
            break;

        case 7:
            g->sfx_mask |= SFXBIT(SFX_GROW);
            g->pu_timer = 3;
            break;

        case 2: {
            g->sfx_mask |= SFXBIT(SFX_SLOW_DOWN);
            int v = (int)g->per_ma + SLOW_DOWN_DELTA;
            if (v > SPEED_MAX_FRAMES) v = SPEED_MAX_FRAMES;
            g->per_ma = (uint8_t)v;
            g->per_Na = (uint8_t)v;
            break;
        }

        default:
            break;
    }
}

void game_apply_powerup(game_t *g, uint8_t id) {
    if (id < 1 || id > POWERUP_TYPE_COUNT) return;
    pu_deactivate(g);
    g->pu_active = id;
    pu_activate(g);
}

/* ── init ────────────────────────────────────────────────────────────────── */

void game_init(game_t *g, uint8_t speed, int8_t maze, const uint8_t *maze_map) {
    game_init_mode(g, speed, maze, maze_map, false);
}

void game_init_mode(game_t *g, uint8_t speed, int8_t maze, const uint8_t *maze_map, bool extreme) {
    memset(g, 0, sizeof(*g));
    g->speed = speed;
    g->extreme = extreme;
    g->tongue = 0;
    g->maze = maze;
    g->len = START_LENGTH;
    g->qx = START_HEAD_X;
    g->qy = START_HEAD_Y;
    g->dir = DIR_RIGHT | 0x20;          /* original j = 40 */
    for (uint8_t i = 0; i < START_LENGTH; i++) g->a[i] = g->dir;
    g->food_x = START_FOOD_X;
    g->food_y = START_FOOD_Y;
    g->bonus_x = g->bonus_y = 0;
    g->per_ma = g->per_La = g->per_Ma = g->per_Na = (uint8_t)(SPEED_FRAMES_BASE + (SPEED_OFFSET - speed));
    g->score_step = (uint8_t)(speed + 1);
    g->level = 1;
    g->level_start_score = 0;
    g->portal_state = 0;
    g->map = (maze >= 0) ? maze_map : NULL;
    g->coin_added = -1;
    g->coin_removed = -1;
}

/* ── one frame ───────────────────────────────────────────────────────────── */

void game_tick(game_t *g, uint8_t keys) {
    g->sfx_mask = 0;
    g->moved = g->tail_removed = g->food_eaten = g->bonus_eaten = false;
    g->full_redraw = false;
    g->coin_added = -1;
    g->coin_removed = -1;
    g->shrink_removed = false;

    /* The original clears consumed slot ids and the 15 ("erase HUD icon") marker during
     * the draw pass that follows every tick. Do it at the start of the next tick instead. */
    if (g->pu_active == POWERUP_EXPIRED_ID) g->pu_active = 0;
    for (uint8_t i = 0; i < POWERUP_SLOTS; i++)
        if (g->pu[i].id == POWERUP_EXPIRED_ID) g->pu[i].id = 0;

    if (g->death > 0) {
        if (--g->death == 0) g->game_over = true;
        return;
    }

    wrap_x(&g->qx);
    wrap_y(&g->qy);

    g->cnt_c = (uint8_t)((g->cnt_c + 1) % g->per_La);
    g->cnt_b = (uint8_t)((g->cnt_b + 1) % g->per_ma);
    g->cnt_B = (uint8_t)((g->cnt_B + 1) % g->per_Ma);
    g->cnt_C = (uint8_t)((g->cnt_C + 1) % g->per_Na);

    if (g->bonus > 0 && g->cnt_c % 2 == 0) {
        --g->bonus;
    }
    if (g->extreme && g->cnt_c % 2 == 0) {
        for (uint8_t i = 0; i < POWERUP_SLOTS; i++) {
            if (g->pu[i].life > 0) {
                if (--g->pu[i].life == 0) g->pu[i].id = POWERUP_EXPIRED_ID;
            }
        }
    }

    /* input accumulation (original i) */
    uint8_t n4 = g->input;
    if (keys & KEY_LEFT)       { n4 |= DIR_LEFT;  n4 &= (uint8_t)~DIR_RIGHT; }
    else if (keys & KEY_RIGHT) { n4 |= DIR_RIGHT; n4 &= (uint8_t)~DIR_LEFT; }
    if (keys & KEY_UP)         { n4 |= DIR_UP;    n4 &= (uint8_t)~DIR_DOWN; }
    else if (keys & KEY_DOWN)  { n4 |= DIR_DOWN;  n4 &= (uint8_t)~DIR_UP; }
    if (keys & KEY_CCW)        { n4 |= 0x40; n4 &= (uint8_t)~0x20; }
    else if (keys & KEY_CW)    { n4 |= 0x20; n4 &= (uint8_t)~0x40; }
    g->input = n4;

    /* tongue request (original: W && K[4] && J == 0 -> J = -1). Checked every frame;
     * the lash itself starts on the next movement tick. Enabled in both Classic and
     * Extreme (user request; the original only exposed it in Extreme). */
    if ((keys & KEY_FIRE) && g->tongue == 0) g->tongue = -1;

    /* one/two cells beyond the head, valid while the tongue is out (original k,l / K,L) */
    int16_t t1x = 0, t1y = 0, t2x = 0, t2y = 0;
    /* The pickup checks below run on the C cadence, which can differ from the movement
     * cadence (portal shrink sets ma=1).  Keep the reach valid on non-movement ticks;
     * a movement tick refreshes it after the head/direction change. */
    tongue_reach(g, &t1x, &t1y, &t2x, &t2y);

    if (g->cnt_b == 0) {
        if (g->tongue > 0) --g->tongue;

        /* Portal shrink (original: W && Y == 2). One tail segment per movement tick;
         * when only two are left, start the next maze. No movement/food/collision. */
        if (g->extreme && g->portal_state == 2) {
            if (g->len > PORTAL_SHRINK_MIN_LENGTH) {
                int16_t tx = g->qx, ty = g->qy;
                for (int16_t i = (int16_t)g->len - 1; i > 0; i--) step_back(&tx, &ty, g->a[i]);
                g->old_tail_x = tx;
                g->old_tail_y = ty;
                g->tail_removed = true;
                memmove(&g->a[0], &g->a[1], (size_t)(g->len - 1));
                g->len--;
            } else {
                g->portal_state = 0;
                next_level(g);
            }
            return;
        }

        /* choose direction */
        int I;
        if (g->input != 0) {
            I = (int8_t)(g->input & ~g->dir);
            switch (I) {
                default: I = (int8_t)g->input; break;
                case 32:
                    switch (g->dir) {
                        case DIR_UP: I = DIR_RIGHT; break;
                        case DIR_LEFT: I = DIR_UP; break;
                        case DIR_DOWN: I = DIR_LEFT; break;
                        case DIR_RIGHT: I = DIR_DOWN; break;
                    }
                    break;
                case 64:
                    switch (g->dir) {
                        case DIR_UP: I = DIR_LEFT; break;
                        case DIR_LEFT: I = DIR_DOWN; break;
                        case DIR_DOWN: I = DIR_RIGHT; break;
                        case DIR_RIGHT: I = DIR_UP; break;
                    }
                    break;
            }
            g->input = 0;
        } else {
            I = (int8_t)g->dir;
        }
        int t = (int8_t)(I & ~g->dir);
        uint8_t by = (g->dir & 5) != 0 ? (uint8_t)(t & 0xA) : (uint8_t)(t & 5);
        if (by == 0) by = g->dir;

        /* move the head; (n3,n2) = cell ahead of the new head */
        int16_t n3 = g->qx, n2 = g->qy;
        switch (by) {
            case DIR_UP:    g->qy -= CELL_SIZE; n2 = g->qy - CELL_SIZE; break;
            case DIR_DOWN:  g->qy += CELL_SIZE; n2 = g->qy + CELL_SIZE; break;
            case DIR_LEFT:  g->qx -= CELL_SIZE; n3 = g->qx - CELL_SIZE; break;
            case DIR_RIGHT: g->qx += CELL_SIZE; n3 = g->qx + CELL_SIZE; break;
        }
        wrap_x(&g->qx); wrap_y(&g->qy);
        wrap_x(&n3);    wrap_y(&n2);

        /* self collision (as in the original: walk the body from the new head, compare with n3,n2) */
        {
            int16_t x = g->qx, y = g->qy;
            for (int16_t i = (int16_t)g->len - 1; i > 0; i--) {
                step_back(&x, &y, g->a[i]);
                if (x == n3 && y == n2) { g->death = 40; break; }
            }
        }
        /* wall collision (type 3 passes through walls; self collision still applies) */
        if (g->death == 0 && g->pu_active != 3 &&
            game_tile_at(g, game_col(g->qx), game_row(g->qy)) > 0) {
            g->death = 40;
        }
        if (g->death == 40) {
            switch (by) {                       /* undo the move */
                case DIR_UP:    g->qy += CELL_SIZE; break;
                case DIR_DOWN:  g->qy -= CELL_SIZE; break;
                case DIR_LEFT:  g->qx += CELL_SIZE; break;
                case DIR_RIGHT: g->qx -= CELL_SIZE; break;
            }
            wrap_x(&g->qx); wrap_y(&g->qy);
            g->sfx_mask |= SFXBIT(SFX_DEATH);
            return;
        }

        /* type 6: remove one tail segment per movement step, at most three steps. This runs
         * before the normal append/drop, so a normal step loses one segment net (the old
         * tail plus the one the step would have dropped); a grow step stays the same length. */
        if (g->extreme && g->pu_active == 6) {
            if (g->len > 3) {
                int16_t tx = g->qx, ty = g->qy;
                for (int16_t i = (int16_t)g->len - 1; i > 0; i--) step_back(&tx, &ty, g->a[i]);
                g->shrink_x = tx; g->shrink_y = ty;
                g->shrink_removed = true;
                memmove(&g->a[0], &g->a[1], (size_t)(g->len - 1));
                g->len--;
            } else {
                g->pu_timer = 1;
            }
            if (--g->pu_timer == 0) {
                pu_deactivate(g);
                g->pu_active = POWERUP_EXPIRED_ID;
            }
        }
        /* type 7: force one grow per movement step, at most three steps */
        if (g->extreme && g->pu_active == 7 && !g->grow) {
            g->grow = true;
            if (--g->pu_timer == 0) {
                pu_deactivate(g);
                g->pu_active = POWERUP_EXPIRED_ID;
            }
        }

        by = head_byte(g->a[g->len - 1], by);

        /* append head, drop tail unless growing */
        if (g->grow && g->len < GAME_MAX_LEN) {
            g->a[g->len - 1] |= 0x10;
            g->a[g->len++] = by;
            g->grow = false;
        } else {
            g->old_tail_x = 0; g->old_tail_y = 0;
            {   /* px of the tail cell before it is dropped */
                int16_t x = g->qx, y = g->qy;
                /* the new head is segment `len` (not yet stored); walk uses the new head's own direction first */
                step_back(&x, &y, by);
                for (int16_t i = (int16_t)g->len - 1; i > 0; i--) step_back(&x, &y, g->a[i]);
                g->old_tail_x = x; g->old_tail_y = y;
            }
            g->a[g->len] = by;
            memmove(&g->a[0], &g->a[1], g->len);
            g->tail_removed = true;
            g->grow = false;
        }
        g->moved = true;
        g->dir = by & 0x0F;
        if (g->tongue < 0) g->tongue = 1;
        /* The head and direction just changed; refresh k/l and K/L for this tick's
         * bonus/pickup checks. */
        tongue_reach(g, &t1x, &t1y, &t2x, &t2y);

        /* bonus food eaten by the new head or by the tongue (1-2 cells ahead) */
        bool bonus_hit = g->bonus > 0 &&
            ((g->qx == g->bonus_x && g->qy == g->bonus_y) ||
             (g->tongue > 0 && ((t1x == g->bonus_x && t1y == g->bonus_y) ||
                                (t2x == g->bonus_x && t2y == g->bonus_y))));
        if (bonus_hit) {
            g->score += (uint32_t)g->bonus;
            g->bonus = 0;
            g->grow = true;
            g->sfx_mask |= SFXBIT(SFX_BEEP);
            g->bonus_eaten = true;
        }
    }

    /* ── coin rush: one coin marked/erased per frame (original Aa==8 block) ── */
    if (g->extreme && g->pu_active == 8) {
        mark_snake(g);
        if (g->pu_timer > 0 && g->pu_timer < FIELD_CELLS) {
            while (g->pu_timer < FIELD_CELLS) {
                uint16_t idx = (uint16_t)g->pu_timer;
                uint8_t col = (uint8_t)(idx % FIELD_COLS);
                uint8_t row = (uint8_t)(idx / FIELD_COLS);
                if (game_tile_at(g, col, row) == 0 && !occupied(g, idx) && !coin_get(g, idx)) break;
                g->pu_timer += 2;
            }
            if (g->pu_timer < FIELD_CELLS) {
                coin_set(g, (uint16_t)g->pu_timer);
                g->coin_added = g->pu_timer;
            }
        } else if (g->pu_timer >= FIELD_CELLS) {
            g->pu_timer = -1;
        } else if (g->pu_timer < 0) {
            int idx = -g->pu_timer;
            if (idx > 0 && idx < FIELD_CELLS && coin_get(g, (uint16_t)idx)) {
                coin_clear(g, (uint16_t)idx);
                g->coin_removed = (int16_t)idx;
            }
            g->pu_timer -= 2;
            if (g->pu_timer <= -FIELD_CELLS) {
                pu_deactivate(g);
                g->pu_active = POWERUP_EXPIRED_ID;
                /* The scan has already removed every coin; only the restored food
                 * cell needs repainting (handled by the incremental renderer). */
                mark_snake(g);
                spawn_cell(g);
                g->food_x = g->spawn_x;
                g->food_y = g->spawn_y;
            }
        }
    }

    /* ── lights out: toggle the field background on the packed countdown ── */
    if (g->extreme && g->pu_active == 10) {
        uint32_t n = (g->lights_timer & 0xFFu) - 1u;
        if (n == 0) {
            g->lights_out = !g->lights_out;
            g->lights_timer >>= 8;
            g->full_redraw = true;
        } else {
            g->lights_timer = (g->lights_timer & 0xFFFFFF00u) + n;
        }
        if (g->lights_timer == 0) {
            g->lights_timer = 0x02020200u + (uint32_t)(10 + rnd_abs_mod(10));
        }
    }

    if (g->cnt_C == 0) {
        /* power-up pickup by the head or the tongue (original b.java:1517) */
        if (g->extreme) {
            for (uint8_t i = 0; i < POWERUP_SLOTS; i++) {
                if (g->pu[i].id != 0 &&
                    ((g->qx == g->pu[i].x && g->qy == g->pu[i].y) ||
                     (g->tongue > 0 &&
                      ((t1x == g->pu[i].x && t1y == g->pu[i].y) ||
                       (t2x == g->pu[i].x && t2y == g->pu[i].y))))) {
                    pu_deactivate(g);
                    g->pu_active = g->pu[i].id;
                    g->pu[i].id = POWERUP_EXPIRED_ID;
                    g->pu[i].life = 0;
                    pu_activate(g);
                }
            }
            /* General effect countdown; types 5..9 run their own counters. The original
             * also counts down when Aa == 0 (e.g. after an expired slot set Aa=15 and the
             * next draw pass cleared it), so do not require a live effect here. */
            uint8_t a = g->pu_active;
            if (a != 5 && a != 6 && a != 7 && a != 8 && a != 9 &&
                g->pu_timer > 0 && --g->pu_timer == 0) {
                pu_deactivate(g);
                g->pu_active = POWERUP_EXPIRED_ID;
            }
        }

        /* coin rush pickups: head and tongue cells, +1 each (no sound) */
        if (g->extreme && g->pu_active == 8) {
            coin_collect(g, g->qx, g->qy);
            if (g->tongue > 0) {
                coin_collect(g, t1x, t1y);
                coin_collect(g, t2x, t2y);
            }
        }

        /* normal food eaten by the head or by the tongue (1-2 cells ahead) */
        bool food_hit = g->pu_active != 8 &&
                        ((g->qx == g->food_x && g->qy == g->food_y) ||
                         (g->tongue > 0 && ((t1x == g->food_x && t1y == g->food_y) ||
                                            (t2x == g->food_x && t2y == g->food_y))));
        if (food_hit) {
            g->grow = true;
            g->sfx_mask |= SFXBIT(SFX_BEEP);
            g->food_eaten = true;
            g->eaten_x = g->food_x; g->eaten_y = g->food_y;
            g->score += g->score_step;

            /* type 9 expires after five normal foods; the timer is reset even on expiry
             * (original consumes this RNG before choosing the replacement food). */
            if (g->extreme && g->pu_active == 9) {
                if (--g->pu_timer == 0) {
                    pu_deactivate(g);
                    g->pu_active = POWERUP_EXPIRED_ID;
                }
                g->move_food_timer = (int16_t)((MOVING_FOOD_TIMER_MIN + rnd_abs_mod(15)) *
                                               (((8 - g->speed) >> 1) + 1));
            }

            mark_snake(g);
            spawn_cell(g);
            g->food_x = g->spawn_x;
            g->food_y = g->spawn_y;

            if (g->bonus <= 0 && g->pu_active != 9 && (game_rand_int() & 4) == 0) {
                g->bonus = (int16_t)(25 + rnd_abs_mod(50));
                if (g->pu_active == 5) {
                    g->bonus = (int16_t)(g->bonus + BONUS_BOOST_EXTRA);
                    if (--g->pu_timer == 0) {
                        pu_deactivate(g);
                        g->pu_active = POWERUP_EXPIRED_ID;
                    }
                }
                g->bonus_type = (uint8_t)(1 + rnd_abs_mod(7));
                spawn_cell(g);
                g->bonus_x = g->spawn_x;
                g->bonus_y = g->spawn_y;
                uint8_t h = (uint8_t)rnd_abs_mod(4);
                g->bonus_dir = (h == 0) ? 1 : (h == 1) ? 2 : (h == 2) ? 4 : 8;
                g->bonus_frame = 0;
            }

            /* power-ups: each empty slot gets an independent 1/16 roll */
            if (g->extreme) {
                mark_snake(g);
                for (uint8_t i = 0; i < POWERUP_SLOTS; i++) {
                    if (g->pu[i].id != 0) continue;          /* short-circuit: no RNG */
                    if ((game_rand_int() & 0xF) != 15) continue;
                    g->pu[i].life = (uint8_t)(POWERUP_DURATION_BASE + rnd_abs_mod(POWERUP_DURATION_RAND));
                    g->pu[i].id = (uint8_t)(1 + rnd_abs_mod(POWERUP_TYPE_COUNT));
                    bool clash;
                    do {
                        spawn_cell(g);
                        clash = false;
                        for (uint8_t j = 0; j < POWERUP_SLOTS; j++) {
                            if (j != i && g->pu[j].x == g->spawn_x && g->pu[j].y == g->spawn_y) {
                                clash = true; break;
                            }
                        }
                    } while (clash);
                    g->pu[i].x = (uint8_t)g->spawn_x;
                    g->pu[i].y = (uint8_t)g->spawn_y;
                    /* The original flashes the backlight here (Snake.A(11)); it is not a sound. */
                }
            }
        }
    }

    /* ── bonus food movement (original B==0 block; Extreme, types 3..7) ── */
    if (g->extreme && g->cnt_B == 0 && g->bonus > 0 &&
        g->bonus_type != 0 && g->bonus_type != 1 && g->bonus_type != 2) {
        int16_t nx = g->bonus_x, ny = g->bonus_y;
        switch (g->bonus_dir) {
            case 1: ny -= CELL_SIZE; break;
            case 4: ny += CELL_SIZE; break;
            case 2: nx -= CELL_SIZE; break;
            case 8: nx += CELL_SIZE; break;
        }
        wrap_x(&nx); wrap_y(&ny);
        uint8_t nc = game_col(nx), nr = game_row(ny);
        mark_snake(g);
        uint16_t idx = (uint16_t)(nc + nr * FIELD_COLS);
        /* Original b.java:1684-1692: Ca (walls/snakes/coins), normal food, and the
         * portal target all bounce moving bonus food. */
        bool hit = game_tile_at(g, nc, nr) != 0 || occupied(g, idx) || coin_get(g, idx) ||
                   (nx == g->food_x && ny == g->food_y) ||
                   (nx == g->portal_x && ny == g->portal_y);
        if (!hit) {
            for (uint8_t i = 0; i < POWERUP_SLOTS; i++) {
                if (g->pu[i].id != 0 && g->pu[i].x == nx && g->pu[i].y == ny) {
                    hit = true; break;
                }
            }
        }
        if (hit) {
            g->bonus_dir = (uint8_t)(g->bonus_dir << 2);
            if (g->bonus_dir > 8) g->bonus_dir = (uint8_t)(g->bonus_dir >> 4);
        } else {
            g->bonus_x = nx;
            g->bonus_y = ny;
        }
        if (g->bonus_type >= 3 && g->bonus_type <= 6)
            g->bonus_frame = (uint8_t)((g->bonus_frame + 1) % 2);
        else if (g->bonus_type == 7)
            g->bonus_frame = (uint8_t)((g->bonus_frame + 1) % 4);
    }

    /* ── moving food (type 9): the normal food relocates on a frame timer ── */
    if (g->extreme && g->pu_active == 9 && g->move_food_timer > 0 &&
        g->cnt_c % 2 == 0 && --g->move_food_timer == 0) {
        mark_snake(g);
        spawn_cell(g);
        g->food_x = g->spawn_x;
        g->food_y = g->spawn_y;
        g->move_food_timer = (int16_t)((MOVING_FOOD_TIMER_MIN + rnd_abs_mod(15)) *
                                       (((8 - g->speed) >> 1) + 1));
    }

    /* ── Extreme portal: score-gated spawn, then shrink-to-2 level transition ── */
    if (g->extreme && !g->game_over && g->cnt_c == 0) {
        uint32_t threshold = g->level_start_score +
                             (uint32_t)g->speed * PORTAL_SPEED_MULT +
                             (uint32_t)g->level * PORTAL_LEVEL_MULT;
        if (g->pu_active != 8 && g->portal_state == 0 && g->score > threshold) {
            mark_snake(g);               /* original Ca snake bits are still set here */
            spawn_cell(g);
            g->portal_state = 1;
            g->portal_x = g->spawn_x;
            g->portal_y = g->spawn_y;
        }
        if (g->portal_state == 1 && g->qx == g->portal_x && g->qy == g->portal_y) {
            /* Original order: Y=2, ma=1, then C(). A speed power-up's C() restores
             * its saved period, so the shrink can run at that cadence instead. */
            g->portal_state = 2;
            g->per_ma = 1;
            pu_deactivate(g);
        }
    }
}
