/**
 * main.cpp — Snake EX2 AVR application.
 *
 * Boot flow (BACKLOG U1):
 *
 *   TITLE  full-screen title art, 3 s or key; after a game it shows the final
 *          score (plus two trophies for a new best) and waits for a key
 *   MENU   New game / High scores / Settings / Instructions (full 128×160)
 *   INFO   placeholder screen for the menu items owned by U2/U3/U5
 *   GAME   Classic or Extreme, using the saved EEPROM settings
 *
 * The old hardware self-test is no longer run at boot: the drivers are covered
 * by the host tests and have been hardware-verified. Keeping boot on the title
 * also satisfies the cold-boot <2 s requirement.
 */

#include <Arduino.h>
#include <avr/io.h>

#include "config.h"
#include "hal/gpio.h"
#include "drivers/st7735.h"
#include "drivers/buttons.h"
#include "drivers/buzzer.h"
#include "drivers/eeprom_store.h"
#include "drivers/stack_measure.h"
#include "assets/maze.h"
#include "render/hud.h"
#include "render/playfield.h"
#include "render/text.h"
#include "ui/menu.h"
#include "ui/settings.h"
#include "ui/highscores.h"
#include "ui/pause.h"
#include "ui/instructions.h"
#include "screens.h"
#include "gamestate.h"
#include "game/game.h"
#include "game/classic.h"
#include "game/rng.h"

#define C_WHITE   0xFFFF
#define C_BLACK   0x0000
#define C_YELLOW  0xFFE0

#define FRAME_MS  66        /* 15 fps game tick */
#define TITLE_MS  3000      /* auto-advance from the title screen */

static screen_t          s_screen;
static uint32_t          s_title_deadline;
static bool              s_title_timed;

static uint8_t           s_map[MAZE_CELLS];
/* Game state (also accessed from instructions.cpp via gamestate.h) */
game_t            s_game;
playfield_cache_t s_cache;
bool              s_paused;
bool              s_over_shown;
bool              s_hs_checked;      /* high-score entry handled for this game */
uint32_t          s_next_tick;
uint32_t          s_hud_score;
int16_t           s_hud_bonus;
uint8_t           s_hud_pu;

/* ── Small helpers ──────────────────────────────────────────────────────── */

static uint8_t centered_x(const char *txt, uint8_t scale) {
    uint8_t w = text_width(txt, scale);
    return (w >= 128) ? 0 : (uint8_t)((128 - w) / 2);
}

static void play_sfx(uint8_t id) {
    if (eeprom_get_settings()->sound_on) buzzer_play(id);
}

/* ── Screens ────────────────────────────────────────────────────────────── */

static void enter_title(void) {
    title_draw();
    s_title_deadline = millis() + TITLE_MS;
    s_title_timed = true;
    s_screen = SCREEN_TITLE;
}

/* Title screen shown after a game: stays until a key is pressed. */
static void enter_title_score(uint32_t score, bool new_best) {
    title_draw_score(score, new_best);
    s_title_timed = false;
    s_screen = SCREEN_TITLE;
}

static void enter_menu(void) {
    menu_init();
    menu_draw();
    s_screen = SCREEN_MENU;
}

static void show_menu(void) {
    menu_draw();
    s_screen = SCREEN_MENU;
}

static void menu_navigate(int8_t delta) {
    menu_item_t previous = menu_selected();
    menu_move(delta);
    menu_item_t current = menu_selected();
    if (current == previous) return;

    /* Normal navigation repaints only the row that lost the highlight and the
     * row that gained it; header, footer and the other rows stay untouched. */
    menu_draw_item(previous);
    menu_draw_item(current);
}

static void draw_info_screen(menu_item_t item) {
    const char *label = menu_item_label(item);
    const char *body  = "COMING SOON";
    const char *back  = "PAUSE BACK";

    st7735_fill_screen(C_WHITE);

    st7735_fill_rect(0, 0, 128, 20, C_BLACK);
    text_draw(centered_x(label, 1), 7, label, 1, C_WHITE);

    text_draw(centered_x(body, 1), 72, body, 1, C_BLACK);

    st7735_fill_rect(0, 140, 128, 20, C_BLACK);
    {
        uint8_t w = text_width(back, 1);
        text_draw((uint8_t)(128 - w - 5), 146, back, 1, C_WHITE);
    }
}

static void enter_info(menu_item_t item) {
    draw_info_screen(item);
    s_screen = SCREEN_INFO;
}

static void enter_highscores(void) {
    /* Called from menu: show high scores for the current game mode */
    hs_mode_t mode = (eeprom_get_settings()->mode == MODE_EXTREME)
                     ? HS_MODE_EXTREME : HS_MODE_CLASSIC;
    highscores_show(mode);
    s_screen = SCREEN_HIGHSCORES;
}

static void enter_settings(void) {
    settings_init();
    settings_draw();
    s_screen = SCREEN_SETTINGS;
}

static void enter_instructions(void) {
    instructions_show();
    s_screen = SCREEN_INSTRUCTIONS;
}

/* ── Game ───────────────────────────────────────────────────────────────── */

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

static void start_game_from_settings(void) {
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

    game_set_maze_source(&s_game, s_map, maze_load);
    game_rand_seed(millis() * 2654435761u + 1u);
    playfield_draw_all(&s_game, &s_cache);
    hud_draw_bar();
    s_hud_score = 0xFFFFFFFFu;
    s_hud_bonus = -32768;
    s_hud_pu = 0xFF;
    hud_refresh();
    s_paused = false;
    s_over_shown = false;
    s_hs_checked = false;
    s_next_tick = millis() + FRAME_MS;
    s_screen = SCREEN_GAME;
}

/* ── Entry points ───────────────────────────────────────────────────────── */

void setup() {
#if STACK_MEASURE
    /* Paint free SRAM before any driver/game work; the pause/game-over screens
     * report the low-water mark.  Release builds compile this out. */
    stack_measure_begin();
#endif

    // Disable JTAG to free PORTC (PC2–PC5). ATmega644P/PA moves JTD to MCUCR;
    // the ATmega32 fallback used MCUCSR.
#if defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644__) || \
    defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
    MCUCR = (1 << JTD);
    MCUCR = (1 << JTD);
#else
    MCUCSR = (1 << JTD);
    MCUCSR = (1 << JTD);
#endif

    gpio_init();
    st7735_init();
    buttons_init();
    buzzer_init();
    eeprom_store_init();                 // writes/repairs defaults on first boot

    st7735_backlight(eeprom_get_settings()->backlight);
    enter_title();
}

void loop() {
    uint8_t raw = buttons_poll();

    uint8_t btn;
    btn_event_t ev;
    while ((ev = buttons_get_event(&btn)) != BTN_EVENT_NONE) {
        switch (s_screen) {
        case SCREEN_TITLE:
            if (ev == BTN_EVENT_PRESS) enter_menu();
            break;

        case SCREEN_MENU:
            if (ev == BTN_EVENT_PRESS || ev == BTN_EVENT_REPEAT) {
                if (btn == BTN_UP || btn == BTN_LEFT) {
                    menu_navigate(-1);
                } else if (btn == BTN_DOWN || btn == BTN_RIGHT) {
                    menu_navigate(1);
                } else if (ev == BTN_EVENT_PRESS && btn == BTN_FIRE) {
                    menu_item_t item = menu_selected();
                    if (item == MENU_ITEM_NEW_GAME) start_game_from_settings();
                    else if (item == MENU_ITEM_SETTINGS) enter_settings();
                    else if (item == MENU_ITEM_HIGH_SCORES) enter_highscores();
                    else if (item == MENU_ITEM_INSTRUCTIONS) enter_instructions();
                    else enter_info(item);
                } else if (ev == BTN_EVENT_PRESS && btn == BTN_PAUSE) {
                    enter_title();
                }
            }
            break;

        case SCREEN_INFO:
            if (ev == BTN_EVENT_PRESS && (btn == BTN_FIRE || btn == BTN_PAUSE)) {
                show_menu();
            }
            break;

        case SCREEN_SETTINGS:
            if (ev == BTN_EVENT_PRESS || ev == BTN_EVENT_REPEAT) {
                if (btn == BTN_UP || btn == BTN_DOWN) {
                    setting_item_t prev = settings_selected();
                    settings_move(btn == BTN_UP ? -1 : 1);
                    settings_draw_value(prev);
                    settings_draw_value(settings_selected());
                } else if (btn == BTN_LEFT || btn == BTN_RIGHT) {
                    settings_adjust(btn == BTN_LEFT ? -1 : 1);
                    settings_draw_value(settings_selected());
                } else if (ev == BTN_EVENT_PRESS && btn == BTN_FIRE) {
                    settings_save();
                } else if (ev == BTN_EVENT_PRESS && btn == BTN_PAUSE) {
                    show_menu();
                }
            }
            break;

        case SCREEN_HIGHSCORES:
            if (ev == BTN_EVENT_PRESS && (btn == BTN_FIRE || btn == BTN_PAUSE)) {
                show_menu();
            }
            break;

        case SCREEN_INSTRUCTIONS:
            if (ev == BTN_EVENT_PRESS || ev == BTN_EVENT_REPEAT) {
                if (!instructions_handle_input(ev, btn)) {
                    show_menu();
                }
            }
            break;

        case SCREEN_GAME:
            if (ev == BTN_EVENT_PRESS) {
                if (btn == BTN_PAUSE) {
                    /* Enter pause overlay */
                    pause_draw();
                    while (true) {
                        pause_action_t pa = pause_handle_input();
                        if (pa == PAUSE_ACTION_RESUME) {
                            s_paused = false;
                            /* Full redraw to clear the pause grid overlay */
                            playfield_draw_all(&s_game, &s_cache);
                            hud_draw_bar();
                            s_hud_score = 0xFFFFFFFFu;
                            s_hud_bonus = -32768;
                            s_hud_pu = 0xFF;
                            hud_refresh();
                            break;
                        } else if (pa == PAUSE_ACTION_RESTART) {
                            start_game_from_settings();
                            break;
                        } else if (pa == PAUSE_ACTION_MENU) {
                            s_paused = false;
                            show_menu();
                            break;
                        }
                    }
                }
            }
            break;
        }
    }

    uint32_t now = millis();

    if (s_screen == SCREEN_TITLE && s_title_timed && (int32_t)(now - s_title_deadline) >= 0) {
        enter_menu();
    }

    if (s_screen == SCREEN_GAME && !s_paused && (int32_t)(now - s_next_tick) >= 0) {
        s_next_tick += FRAME_MS;
        if ((int32_t)(now - s_next_tick) > 0) s_next_tick = now + FRAME_MS;   /* don't burst after a stall */

        uint8_t keys = 0;
        if (raw & (1 << BTN_LEFT))  keys |= KEY_LEFT;
        if (raw & (1 << BTN_RIGHT)) keys |= KEY_RIGHT;
        if (raw & (1 << BTN_UP))    keys |= KEY_UP;
        if (raw & (1 << BTN_DOWN))  keys |= KEY_DOWN;
        if (raw & (1 << BTN_FIRE))  keys |= KEY_FIRE;

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
            /* Game just ended: show game-over screen and handle input */
            s_over_shown = true;
            /* The table's first slot of the mode, read before an entry can change it. */
            bool new_best = s_game.score > eeprom_get_score(s_game.extreme ? 2 : 0);
            if (!s_hs_checked && highscores_qualifies(s_game.score, s_game.extreme)) {
                s_hs_checked = true;
                highscores_maybe_enter(s_game.score, s_game.extreme);
            }
            enter_title_score(s_game.score, new_best);
        }
    }

    if (buzzer_is_playing()) buzzer_update();
    delay(2);
}
