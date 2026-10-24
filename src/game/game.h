/**
 * game.h — Snake EX2 game core: movement, collision, food, score, tongue (BACKLOG G1-G6).
 *
 * Faithful port of the decompiled engine (class b, method b()); the original's field
 * names are noted in comments. Positions are pixels (multiples of CELL_SIZE) exactly like
 * the original so its quirks reproduce bit-for-bit. Verified against the original engine
 * run headless (tools/oracle, test/host/test_game_oracle.cpp).
 *
 * Classic selection (G9), Extreme tongue/power-ups/campaign (G6-G8). No hardware dependencies.
 */
#ifndef GAME_GAME_H
#define GAME_GAME_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "drivers/buzzer.h"

#define GAME_MAX_LEN  (FIELD_CELLS)

/* key bits passed to game_tick (original K[0..6]) */
#define KEY_LEFT   0x01   /* K0 */
#define KEY_RIGHT  0x02   /* K1 */
#define KEY_UP     0x04   /* K2 */
#define KEY_DOWN   0x08   /* K3 */
#define KEY_FIRE   0x10   /* K4 tongue (Extreme only) */
#define KEY_CCW    0x20   /* K5 relative turn, counter-clockwise */
#define KEY_CW     0x40   /* K6 relative turn, clockwise */

/* sound events requested during a tick (bit = playSFX id) */
#define SFXBIT(id) ((uint16_t)1 << (id))

/** One field power-up slot (original Ba[4][4]). Positions are pixels, exactly as stored
 *  by the original; id 0 = empty, 1..10 = item, 15 = consumed (cleared next tick). */
typedef struct {
    uint8_t id;       /* Ba[n][0] */
    uint8_t life;     /* Ba[n][1]: field lifetime in 2-frame ticks */
    uint8_t x, y;     /* Ba[n][2], Ba[n][3]: px */
} game_pu_slot_t;

typedef struct {
    /* configuration */
    uint8_t  speed;          /* Snake.I speedIndex 0..SPEED_INDEX_MAX */
    int8_t   maze;           /* da: -1 none, 0..4 */
    bool     extreme;        /* W: Extreme mode (tongue, power-ups, campaign) */

    /* snake: a[0] = tail ... a[len-1] = head (direction | 0x10 fat | 0x20/0x40 turn flags) */
    uint16_t len;            /* A */
    uint8_t  a[GAME_MAX_LEN + 1];
    int16_t  qx, qy;         /* Q, r: head position in px */
    uint8_t  dir;            /* j: current direction (0x28 until the first move) */
    uint8_t  input;          /* i: pending requested direction bits */
    int8_t   tongue;         /* J: 0 idle, -1 fire queued, >0 tongue out (this cadence) */
    bool     grow;           /* ka */

    /* frame counters and their periods */
    uint8_t  cnt_c, cnt_b, cnt_B, cnt_C;       /* c, b, B, C */
    uint8_t  per_La, per_ma, per_Ma, per_Na;   /* La, ma, Ma, Na */

    /* food */
    int16_t  food_x, food_y;     /* Da, ea (px, -1 = none) */
    int16_t  spawn_x, spawn_y;   /* Z, aa: last spawn result (px) */
    int16_t  bonus;              /* fa: bonus food countdown, 0 = none */
    int16_t  bonus_x, bonus_y;   /* ga, Ga */
    uint8_t  bonus_type;         /* Fa 1..7 */
    uint8_t  bonus_dir;          /* ha 1/2/4/8 */
    uint8_t  bonus_frame;        /* Ia: bonus sprite animation frame */

    /* power-ups (Extreme only; original Aa/ba/Ba/na/Ea/U/v) */
    uint8_t         pu_active;        /* Aa: 0 none, 1..10 active, 15 clear */
    int16_t         pu_timer;         /* ba: countdown / coin-rush scan index */
    uint8_t         pu_saved_period;  /* na: per_ma before a speed effect */
    game_pu_slot_t  pu[POWERUP_SLOTS];/* Ba[4][4] */
    uint16_t        coins[(FIELD_CELLS + 15) / 16]; /* 0x20 overlay bitmap */
    int16_t         move_food_timer;  /* Ea: type-9 timer in frames */
    bool            lights_out;       /* U */
    uint32_t        lights_timer;     /* v: packed lights-out countdown */
    bool            stamp_active;     /* type 4 wall block in effect */
    uint8_t         stamp_col, stamp_row; /* T, u */
    uint8_t         stamp_pat;        /* ua[stamp_pat][9] */

    /* renderer hints for this tick (original p/la/Ka erase + repaint flags) */
    bool     full_redraw;        /* Ka: base/items/coin/lights changed */
    int16_t  coin_added;         /* cell index of a coin placed this frame, -1 none */
    int16_t  coin_removed;       /* cell index of a coin erased this frame, -1 none */
    int16_t  shrink_x, shrink_y; /* tail cell removed by type 6 this tick */
    bool     shrink_removed;

    /* Extreme campaign (original x, z, Y, X, y) */
    uint16_t level;              /* x: 1-based level/maze number */
    uint32_t level_start_score;  /* z: score when the current level began */
    uint8_t  portal_state;       /* Y: 0 none, 1 waiting, 2 shrinking */
    int16_t  portal_x, portal_y; /* X, y: portal target in field px */
    uint8_t *map_buf;            /* mutable maze buffer used on level changes */
    int8_t (*maze_loader)(uint8_t index, uint8_t *buf); /* maze 0..4 loader */

    uint32_t score;              /* H */
    uint8_t  score_step;         /* oa = speed + 1 */
    uint8_t  death;              /* Ja: 40..1 dying, 0 alive */
    bool     game_over;

    /* outputs of the last tick */
    uint16_t sfx_mask;
    bool     moved;              /* head advanced this tick */
    bool     tail_removed;       /* tail cell freed this tick (not when growing) */
    int16_t  old_tail_x, old_tail_y;  /* px of the freed tail cell */
    bool     food_eaten, bonus_eaten;
    int16_t  eaten_x, eaten_y;   /* cell px of eaten normal food (for erase) */

    /* walls: low nibble = tile id, non-owning (maze buffer or NULL for no maze) */
    const uint8_t *map;
    /* transient snake occupancy bitmap for food spawning (1 bit per cell) */
    uint8_t  occ[(FIELD_CELLS + 7) / 8];
} game_t;

#ifdef __cplusplus
extern "C" {
#endif

/** Start a classic game. maze_map: FIELD_CELLS bytes or NULL for no maze (maze must then be -1). */
void game_init(game_t *g, uint8_t speed, int8_t maze, const uint8_t *maze_map);

/** Start a game in the given mode. Classic == game_init(); Extreme enables the tongue. */
void game_init_mode(game_t *g, uint8_t speed, int8_t maze, const uint8_t *maze_map, bool extreme);

/** Supply the mutable maze buffer and loader used when Extreme advances to a new maze.
 *  `loader(index, buf)` must fill FIELD_CELLS bytes for maze 0..4 and return 0 on success. */
void game_set_maze_source(game_t *g, uint8_t *buf,
                          int8_t (*loader)(uint8_t index, uint8_t *buf));

/** True while the Extreme portal is drawn (original Y == 1 or 2). */
static inline bool game_portal_visible(const game_t *g) {
    return g->extreme && (g->portal_state == 1 || g->portal_state == 2);
}

/** Advance one 15 fps frame. keys = KEY_* bits held during this frame. */
void game_tick(game_t *g, uint8_t keys);

/** Apply a power-up exactly as a pickup does (original C() then c()). Used by tests and,
 *  later, by scripted campaign stages. `id` 1..10. */
void game_apply_powerup(game_t *g, uint8_t id);

/** Cell index helpers (col,row from px). */
static inline uint8_t game_col(int16_t px) { return (uint8_t)((px / CELL_SIZE) % FIELD_COLS); }
static inline uint8_t game_row(int16_t py) { return (uint8_t)((py / CELL_SIZE) % FIELD_ROWS); }

/** Maze tile (0..15) at a cell, honouring an active wall-stamp block. */
uint8_t game_tile_at(const game_t *g, uint8_t col, uint8_t row);

/** Coin-rush overlay at a cell (original Ca[cell] & 0x20). */
bool game_has_coin(const game_t *g, uint8_t col, uint8_t row);

#ifdef __cplusplus
}
#endif

#endif
