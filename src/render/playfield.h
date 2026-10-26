/**
 * playfield.h — Dirty-cell playfield renderer (BACKLOG G5).
 *
 * Draws the maze (base), food/bonus overlay and the snake. After each game_tick()
 * only the cells that changed are repainted: the new head, the vacated tail cell and
 * any item cell that appeared/disappeared, plus every segment sprite that can overlap
 * a repainted cell (so no ghost pixels and no full-screen redraw).
 *
 * Layer order matches the original: base -> food/bonus -> snake (head first).
 */
#ifndef RENDER_PLAYFIELD_H
#define RENDER_PLAYFIELD_H

#include <stdint.h>
#include <stdbool.h>
#include "game/game.h"

typedef struct {
    int16_t food_x, food_y;      /* last drawn positions, px */
    int16_t bonus_x, bonus_y;
    uint8_t bonus_type;
    uint8_t bonus_frame;
    bool    food_valid;
    bool    bonus_valid;
    bool    head_open;           /* head sprite variant (mouth open when an item is ahead) */
    bool    tongue;              /* tongue was drawn on the last movement tick */
    int16_t tongue_x, tongue_y;  /* head position when it was drawn */
    uint8_t tongue_dir;          /* head direction when it was drawn */
    /* power-up layer (G7) */
    uint8_t pu_id[POWERUP_SLOTS];   /* drawn ids 1..10, 0 = none */
    uint8_t pu_x[POWERUP_SLOTS], pu_y[POWERUP_SLOTS];
    bool    coin_rush;              /* coins were visible on the last pass */
    bool    lights_out;
    bool    stamp_active;
    uint8_t stamp_col, stamp_row, stamp_pat;
    /* Extreme portal (G8) */
    bool    portal_valid;
    int16_t portal_x, portal_y;
    uint8_t portal_state;           /* 1 waiting, 2 shrinking (head hidden) */
    /* Death blink (S3): original b.java draws the snake only while Ja/4 is even. */
    bool    death_active;
    bool    death_visible;
} playfield_cache_t;

#ifdef __cplusplus
extern "C" {
#endif

/** Full redraw: black screen, maze, items and snake. Resets `cache`. */
void playfield_draw_all(const game_t *g, playfield_cache_t *cache);

/** Incremental redraw after a game_tick(). Only changed cells are touched.
 *  Returns true only when the whole screen was repainted (lights-out toggle or an
 *  explicit full_redraw); the caller must then redraw the HUD bar and digits.
 *  Wall-stamp and coin-rush transitions are repainted as dirty cells. */
bool playfield_update(const game_t *g, playfield_cache_t *cache);

#ifdef __cplusplus
}
#endif

#endif
