/**
 * screen_game.cpp — The game: Classic or Extreme with the saved settings.
 *
 * Back (Pause) saves the game to EEPROM and returns to the menu; the menu's Continue
 * restores it and waits for Fire (which must not also shoot the tongue).
 */
#include <Arduino.h>
#include "app/screens.h"
#include "config.h"
#include "drivers/st7735.h"
#include "drivers/buzzer.h"
#include "drivers/eeprom_store.h"
#include "assets/maze.h"
#include "render/hud.h"
#include "render/playfield.h"
#include "game/game.h"
#include "game/classic.h"
#include "game/rng.h"
#include "game/savegame.h"

#define FRAME_MS  66        /* 15 fps game tick */

static uint8_t           s_map[MAZE_CELLS];
static game_t            s_game;
static playfield_cache_t s_cache;
static bool              s_wait_start;      /* restored game waits for Fire */
static bool              s_mask_fire;       /* ignore the Fire that started it until released */
static bool              s_over_shown;
static bool              s_hs_checked;      /* score saved for this game */
static uint32_t          s_next_tick;
static uint32_t          s_hud_score;
static int16_t           s_hud_bonus;
static uint8_t           s_hud_pu;
static bool              s_restored;        /* the game being entered was restored from EEPROM */

static void play_sfx(uint8_t id) {
    if (eeprom_get_settings()->sound_on) buzzer_play(id);
}

static void hud_refresh(void) {
    if (s_game.score != s_hud_score) { hud_draw_score(s_game.score); s_hud_score = s_game.score; }
    /* During type 9 the original shows Ea (time to the next food relocation) in the
     * bonus digits instead of fa. */
    int16_t v = (s_game.pu_active == 9) ? s_game.move_food_timer
                                        : (s_game.bonus > 0 ? s_game.bonus : 0);
    if (v != s_hud_bonus) { hud_draw_bonus((uint16_t)v); s_hud_bonus = v; }
    uint8_t id = (s_game.pu_active >= 1 && s_game.pu_active <= POWERUP_TYPE_COUNT)
                 ? s_game.pu_active : 0;
    if (id != s_hud_pu) { hud_draw_powerup(id); s_hud_pu = id; }
}

/* Draw the board and HUD of s_game (initialised by screen_game_new/_continue). */
static void enter(void) {
    game_set_maze_source(&s_game, s_map, maze_load);
    game_rand_seed(millis() * 2654435761u + 1u);
    playfield_draw_all(&s_game, &s_cache);
    hud_draw_bar();
    s_hud_score = 0xFFFFFFFFu;
    s_hud_bonus = -32768;
    s_hud_pu = 0xFF;
    hud_refresh();
    s_wait_start = s_restored;
    s_mask_fire = false;
    s_over_shown = false;
    s_hs_checked = false;
    s_next_tick = millis() + FRAME_MS;
}

void screen_game_new(void) {
    const settings_t *st = eeprom_get_settings();
    uint8_t speed = st->speed;
    int8_t  maze  = st->maze;

    if (speed > SPEED_INDEX_MAX) speed = 2;       // defensive: EEPROM already validates
    if (maze < MAZE_NONE || maze > MAZE_TWISTED) maze = MAZE_NONE;

    st7735_backlight(st->backlight);

    if (st->mode == MODE_EXTREME) {
        /* Extreme always starts its campaign on no-maze; the portal cycles mazes. */
        game_init_mode(&s_game, speed, MAZE_NONE, NULL, true);
    } else {
        if (!game_init_classic(&s_game, speed, maze, s_map)) {
            /* Never leave the player on a half-initialised board. */
            game_init_classic(&s_game, 2, MAZE_NONE, NULL);
        }
    }
    s_restored = false;
    screen_goto(&screen_game);
}

bool screen_game_continue(void) {
    bool ok = savegame_load(&s_game);
    if (ok && s_game.maze >= 0) {
        ok = maze_load((uint8_t)s_game.maze, s_map) == 0;
        if (ok) s_game.map = s_map;
    }
    savegame_clear();                   /* consumed: Back saves the game again */
    if (!ok) return false;

    st7735_backlight(eeprom_get_settings()->backlight);
    s_restored = true;
    screen_goto(&screen_game);
    return true;
}

static void on_button(uint8_t btn, btn_event_t ev) {
    if (ev != BTN_EVENT_PRESS) return;
    if (btn == BTN_PAUSE) {
        if (s_game.game_over) return;   /* the score is handled by the next tick */
        savegame_save(&s_game);
        screen_menu_open();             /* "Continue" now leads back here */
    } else if (btn == BTN_FIRE && s_wait_start) {
        /* The Fire that starts a restored game must not also shoot the tongue. */
        s_wait_start = false;
        s_mask_fire = true;
        s_next_tick = millis() + FRAME_MS;
    }
}

static void tick(uint32_t now, uint8_t raw) {
    if (s_wait_start || (int32_t)(now - s_next_tick) < 0) return;
    s_next_tick += FRAME_MS;
    if ((int32_t)(now - s_next_tick) > 0) s_next_tick = now + FRAME_MS;   /* don't burst after a stall */

    uint8_t keys = 0;
    if (raw & (1 << BTN_LEFT))  keys |= KEY_LEFT;
    if (raw & (1 << BTN_RIGHT)) keys |= KEY_RIGHT;
    if (raw & (1 << BTN_UP))    keys |= KEY_UP;
    if (raw & (1 << BTN_DOWN))  keys |= KEY_DOWN;
    if (raw & (1 << BTN_FIRE))  keys |= KEY_FIRE;
    if (s_mask_fire) {
        if (raw & (1 << BTN_FIRE)) keys &= (uint8_t)~KEY_FIRE;
        else s_mask_fire = false;
    }

    if (!s_game.game_over) {
        game_tick(&s_game, keys);
        bool full_redraw = playfield_update(&s_game, &s_cache);
        if (full_redraw) {
            /* playfield_draw_all() clears the whole screen (the original redraws the
             * background image), so repaint the HUD immediately. */
            hud_draw_bar();
            s_hud_score = 0xFFFFFFFFu;
            s_hud_bonus = -32768;
            s_hud_pu = 0xFF;
        }

        if (s_game.sfx_mask) {
            for (uint8_t id = 0; id < 16; id++) {
                if (s_game.sfx_mask & SFXBIT(id)) { play_sfx(id); break; }
            }
        }
        hud_refresh();
    } else if (!s_over_shown) {
        /* Game just ended: save the score, then the title screen with the score */
        s_over_shown = true;
        /* The table's first slot of the mode, read before it is saved. */
        bool new_best = s_game.score > eeprom_get_score(s_game.extreme ? SCORES_PER_MODE : 0);
        if (!s_hs_checked) {
            s_hs_checked = true;
            highscores_submit(s_game.score, s_game.extreme);
        }
        screen_title_open_score(s_game.score, new_best);
    }
}

const screen_t screen_game = { enter, on_button, tick, false };
