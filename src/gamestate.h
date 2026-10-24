/**
 * gamestate.h — Shared game state declarations for cross-module access.
 */
#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <stdint.h>
#include "game/game.h"
#include "render/playfield.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Game state defined in main.cpp */
extern uint8_t s_map[483];
extern game_t s_game;
extern playfield_cache_t s_cache;
extern bool s_paused;
extern bool s_over_shown;
extern bool s_hs_checked;
extern uint32_t s_next_tick;
extern uint32_t s_hud_score;
extern int16_t s_hud_bonus;
extern uint8_t s_hud_pu;

/* Functions defined in main.cpp */
extern void hud_refresh(void);
extern void hud_draw_bar(void);
extern void hud_draw_score(uint32_t);
extern void hud_draw_bonus(uint16_t);
extern void hud_draw_powerup(uint8_t);
extern bool playfield_update(const game_t *, playfield_cache_t *);
extern void playfield_draw_all(const game_t *, playfield_cache_t *);
extern void game_set_maze_source(game_t *, uint8_t *, int8_t (*)(uint8_t, uint8_t *));
extern int8_t maze_load(uint8_t index, uint8_t *buf);
extern void game_rand_seed(uint32_t);

#ifdef __cplusplus
}
#endif

#endif