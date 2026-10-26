/**
 * instructions.cpp — Tutorial/Instructions screen (BACKLOG U5).
 */
#include "ui/instructions.h"
#include "render/text.h"
#include "drivers/st7735.h"
#include "drivers/buttons.h"
#include "ui/menu.h"
#include "ui/highscores.h"
#include "drivers/eeprom_store.h"
#include "game/game.h"
#include "game/classic.h"
#include "game/rng.h"
#include "render/playfield.h"
#include "screens.h"
#include "gamestate.h"
#include <Arduino.h>
#include <avr/pgmspace.h>

#define FRAME_MS  66        /* 15 fps game tick */

/* Get tutorial text for a page index (0..11). Returns PROGMEM string. */
static const char *inst_get_text(uint8_t page) {
    switch (page) {
        case 0:  return PSTR("SNAKE EX2");
        case 1:  return PSTR("PRESS FIRE TO EXTEND THE TONGUE. THE TONGUE REACHES TWO CELLS AHEAD AND CAN GRAB FOOD FROM A DISTANCE.");
        case 2:  return PSTR("NOKIA GAMES SERVICE. DOWNLOAD NEW GAMES AND LEVELS.");
        case 3:  return PSTR("MAKE THE SNAKE GROW LONGER BY DIRECTING IT TO THE FOOD. EACH FOOD INCREASES YOUR LENGTH BY ONE.");
        case 4:  return PSTR("USE THE DIRECTION KEYS TO CHANGE THE SNAKE'S DIRECTION. YOU CANNOT TURN 180 DEGREES.");
        case 5:  return PSTR("AVOID HITTING WALLS AND YOUR OWN TAIL. COLLISION ENDS THE GAME.");
        case 6:  return PSTR("EAT THE BONUS FOOD BEFORE THE TIME EXPIRES. IT MOVES AND IS WORTH MORE POINTS.");
        case 7:  return PSTR("IN EXTREME MODE YOU ADVANCE WHEN YOU HAVE SCORED ENOUGH POINTS. A PORTAL APPEARS.");
        case 8:  return PSTR("ENTER THE PORTAL TO SHRINK THE SNAKE TO LENGTH TWO. THEN THE NEXT MAZE BEGINS.");
        case 9:  return PSTR("MAZES CYCLE: NO MAZE, BOX, TUNNEL, SPIRAL, BLOCKADE, TWISTED. THE CYCLE REPEATS ENDLESSLY.");
        case 10: return PSTR("POWER-UPS SPAWN WHEN YOU EAT FOOD. THEY HAVE VARIOUS EFFECTS: SPEED, GHOST, STAMP, COIN RUSH, LIGHTS OUT AND MORE.");
        case 11: return PSTR("COLLECT COINS DURING COIN RUSH FOR EXTRA POINTS. THE PORTAL IS HIDDEN DURING COIN RUSH.");
        default: return PSTR("SNAKE EX2");
    }
}

/* Demo menu text */
static const char *demo_get_name(uint8_t idx) {
    switch (idx) {
        case 0: return PSTR("TONGUE");
        case 1: return PSTR("WALLS");
        case 2: return PSTR("BONUS FOOD");
        case 3: return PSTR("SPEED UP");
        case 4: return PSTR("GHOST");
        case 5: return PSTR("COIN RUSH");
        case 6: return PSTR("LIGHTS OUT");
        case 7: return PSTR("HIGH SCORE");
        default: return PSTR("");
    }
}

static const int8_t demo_ids[8] = {
    DEMO_TONGUE, DEMO_WALLS, DEMO_BONUS_FOOD, DEMO_POWERUP_SPEED,
    DEMO_POWERUP_GHOST, DEMO_POWERUP_COIN, DEMO_POWERUP_LIGHTS, DEMO_HIGHSCORE
};

/* Demo loops bypass main.cpp, so service the game's SFX mask and the
 * non-blocking buzzer here (same rules as the main loop). */
static void demo_service_audio(void) {
    if (s_game.sfx_mask && eeprom_get_settings()->sound_on) {
        for (uint8_t id = 0; id < SFX_COUNT; id++) {
            if (s_game.sfx_mask & SFXBIT(id)) { buzzer_play(id); break; }
        }
    }
    if (buzzer_is_playing()) buzzer_update();
}

/* delay() busy-waits without servicing the buzzer; keep notes advancing. */
static void demo_delay(uint32_t ms) {
    uint32_t start = millis();
    while ((uint32_t)(millis() - start) < ms) {
        if (buzzer_is_playing()) buzzer_update();
        delay(2);
    }
}

/* ── State ────────────────────────────────────────────────────────────────── */

static int8_t s_inst_page;       // 0..11 = text page, 12 = demo menu
static bool   s_in_demo_menu;    // true when showing demo list
static int8_t s_demo_selected;   // 0..7

/* Forward declaration */
static void inst_draw_demo_menu(void);

/* ── Helpers ──────────────────────────────────────────────────────────────── */

static void inst_draw_page(int8_t page) {
    st7735_fill_screen(0x0000);

    if (page == 12) {
        inst_draw_demo_menu();
        return;
    }

    /* Text page */
    const char *text = inst_get_text(page);

    /* Draw title bar.  The page indicator sits at the right of the title so it
     * cannot overlap the navigation hint at the bottom of the page. */
    text_draw(4, 4, "INSTRUCTIONS", 1, 0xFFFF);
    {
        char pg_str[8];
        pg_str[0] = '0' + (page + 1) / 10;
        pg_str[1] = '0' + (page + 1) % 10;
        pg_str[2] = '/';
        pg_str[3] = '1';
        pg_str[4] = '2';
        pg_str[5] = '\0';
        uint8_t pg_w = text_width(pg_str, 1);
        text_draw((uint8_t)(128 - pg_w - 4), 4, pg_str, 1, 0x8410);
    }
    st7735_fill_rect(0, 14, 128, 1, 0x4208);

    /* Word-wrap and draw text */
    uint8_t x = 4;
    uint8_t y = 20;
    const uint8_t line_h = 8;
    const uint8_t max_w = 120;

    char word[32];
    uint8_t word_len = 0;
    const char *p = text;

    while (true) {
        char c = pgm_read_byte(p++);
        if (c == '\0' || c == ' ' || c == '\n') {
            if (word_len > 0) {
                word[word_len] = '\0';
                uint8_t w = text_width(word, 1);
                if (x + w > max_w) {
                    x = 4;
                    y += line_h;
                }
                text_draw(x, y, word, 1, 0xFFFF);
                x += w + 4; // word spacing
                word_len = 0;
            }
            if (c == '\n') {
                x = 4;
                y += line_h;
            }
            if (c == '\0') break;
        } else {
            if (word_len < 31) word[word_len++] = c;
        }
    }

    /* Navigation hint */
    text_draw(4, 148, "UD: PAGE  LR: DEMOS", 1, 0x4208);
}

static void inst_draw_demo_menu(void) {
    st7735_fill_screen(0x0000);

    text_draw_p(32, 4, PSTR("DEMOS"), 1, 0xFFFF);

    for (int8_t i = 0; i < 8; i++) {
        const char *name = demo_get_name(i);
        uint8_t y = 24 + i * 16;
        bool sel = (i == s_demo_selected);
        uint16_t color = sel ? 0xFFFF : 0x8410;
        text_draw_p(12, y, name, 1, color);
        if (sel) {
            st7735_fill_rect(4, y + 2, 3, 3, 0xFFFF);
        }
    }
    /* Hint */
    text_draw_p(12, 144, PSTR("FIRE: RUN  PAUSE: BACK"), 1, 0x4208);
}

/* ── Demo game implementations ────────────────────────────────────────────── */

static void demo_run_tongue(void) {
    /* Demo -1: Tongue. Classic mode, Box maze, slow speed, food placed 2 cells ahead. */
    game_init_classic(&s_game, 1, 0, s_map); // Box maze, speed 1 (slow)

    /* Place food 2 cells ahead of head */
    s_game.food_x = s_game.qx + 12; // 2 cells = 12 pixels
    s_game.food_y = s_game.qy;
    if (s_game.food_x >= 126) s_game.food_x -= 126;

    game_set_maze_source(&s_game, s_map, maze_load);
    game_rand_seed(millis() * 2654435761u + 1u);
    playfield_draw_all(&s_game, &s_cache);
    hud_draw_bar();
    s_hud_score = 0xFFFFFFFFu;
    s_hud_bonus = -32768;
    s_hud_pu = 0xFF;
    hud_refresh();

    /* Auto-play: wait a moment, then press FIRE to extend the tongue. */
    demo_delay(1500);
    bool fired = false;

    /* Run until food eaten or timeout */
    uint32_t start = millis();
    while (millis() - start < 10000) {
        uint8_t keys = 0;
        if (!fired) {
            keys = KEY_FIRE;
            fired = true;
        }
        game_tick(&s_game, keys);
        playfield_update(&s_game, &s_cache);
        hud_refresh();
        demo_service_audio();
        if (s_game.len > 6) break; // Food eaten
        demo_delay(FRAME_MS);
    }
    demo_delay(1000);
}

static void demo_run_walls(void) {
    /* Demo -2: Walls. Classic mode, Box maze, show wall collision. */
    game_init_classic(&s_game, 2, 0, s_map);
    s_game.extreme = false;

    game_set_maze_source(&s_game, s_map, maze_load);
    game_rand_seed(millis() * 2654435761u + 1u);
    playfield_draw_all(&s_game, &s_cache);
    hud_draw_bar();
    s_hud_score = 0xFFFFFFFFu;
    s_hud_bonus = -32768;
    s_hud_pu = 0xFF;
    hud_refresh();

    /* Auto-play: move toward wall */
    uint32_t start = millis();
    while (millis() - start < 8000) {
        uint8_t keys = 0;
        game_tick(&s_game, keys);
        playfield_update(&s_game, &s_cache);
        hud_refresh();
        demo_service_audio();
        if (s_game.game_over) break;
        demo_delay(FRAME_MS);
    }
    demo_delay(1000);
}

static void demo_run_bonus_food(void) {
    /* Demo -3: Bonus food. Extreme mode, show bonus food spawn and countdown. */
    game_init_mode(&s_game, 2, 0, s_map, true); // Box, speed 2, extreme
    s_game.extreme = true;

    game_set_maze_source(&s_game, s_map, maze_load);
    game_rand_seed(millis() * 2654435761u + 1u);
    playfield_draw_all(&s_game, &s_cache);
    hud_draw_bar();
    s_hud_score = 0xFFFFFFFFu;
    s_hud_bonus = -32768;
    s_hud_pu = 0xFF;
    hud_refresh();

    /* Force bonus food spawn */
    s_game.bonus = 50;
    s_game.bonus_x = 10;
    s_game.bonus_y = 10;
    s_game.bonus_type = 1;
    s_game.bonus_dir = 1;
    s_game.bonus_frame = 0;

    uint32_t start = millis();
    while (millis() - start < 10000) {
        uint8_t keys = 0;
        game_tick(&s_game, keys);
        playfield_update(&s_game, &s_cache);
        hud_refresh();
        demo_service_audio();
        if (s_game.bonus == 0) break;
        demo_delay(FRAME_MS);
    }
    demo_delay(1000);
}

static void demo_run_speed(void) {
    /* Demo -4: Speed power-up. */
    game_init_mode(&s_game, 3, 0, s_map, true);
    s_game.extreme = true;

    game_set_maze_source(&s_game, s_map, maze_load);
    game_rand_seed(millis() * 2654435761u + 1u);
    playfield_draw_all(&s_game, &s_cache);
    hud_draw_bar();
    s_hud_score = 0xFFFFFFFFu;
    s_hud_bonus = -32768;
    s_hud_pu = 0xFF;
    hud_refresh();

    /* Force speed power-up in slot 0 */
    s_game.pu[0].id = 1; // Speed up
    s_game.pu[0].life = 100;
    s_game.pu_active = 1;
    s_game.pu_timer = 50;
    s_game.pu_saved_period = s_game.per_ma;
    s_game.per_ma = (s_game.per_ma > 2) ? s_game.per_ma - 2 : 1;

    uint32_t start = millis();
    while (millis() - start < 8000) {
        uint8_t keys = 0;
        game_tick(&s_game, keys);
        playfield_update(&s_game, &s_cache);
        hud_refresh();
        demo_service_audio();
        demo_delay(FRAME_MS);
    }
    demo_delay(1000);
}

static void demo_run_ghost(void) {
    /* Demo -5: Ghost (wall pass-through) power-up. */
    game_init_mode(&s_game, 2, 0, s_map, true);
    s_game.extreme = true;

    game_set_maze_source(&s_game, s_map, maze_load);
    game_rand_seed(millis() * 2654435761u + 1u);
    playfield_draw_all(&s_game, &s_cache);
    hud_draw_bar();
    s_hud_score = 0xFFFFFFFFu;
    s_hud_bonus = -32768;
    s_hud_pu = 0xFF;
    hud_refresh();

    /* Force ghost power-up */
    s_game.pu[0].id = 3; // Wall ghost
    s_game.pu[0].life = 100;
    s_game.pu_active = 3;
    s_game.pu_timer = 80;

    uint32_t start = millis();
    while (millis() - start < 8000) {
        uint8_t keys = 0;
        game_tick(&s_game, keys);
        playfield_update(&s_game, &s_cache);
        hud_refresh();
        demo_service_audio();
        demo_delay(FRAME_MS);
    }
    demo_delay(1000);
}

static void demo_run_coin_rush(void) {
    /* Demo -6: Coin rush. */
    game_init_mode(&s_game, 2, 0, s_map, true);
    s_game.extreme = true;

    game_set_maze_source(&s_game, s_map, maze_load);
    game_rand_seed(millis() * 2654435761u + 1u);
    playfield_draw_all(&s_game, &s_cache);
    hud_draw_bar();
    s_hud_score = 0xFFFFFFFFu;
    s_hud_bonus = -32768;
    s_hud_pu = 0xFF;
    hud_refresh();

    /* Force coin rush */
    s_game.pu[0].id = 8; // Coin rush
    s_game.pu[0].life = 100;
    s_game.pu_active = 8;
    s_game.pu_timer = 60;

    /* Spawn some coins */
    for (int i = 0; i < 20; i++) {
        uint8_t idx = game_rand_int() % 483;
        if (idx < 483 && s_game.map_buf[idx] == 0) s_game.map_buf[idx] |= 0x20;
    }

    uint32_t start = millis();
    while (millis() - start < 8000) {
        uint8_t keys = 0;
        game_tick(&s_game, keys);
        playfield_update(&s_game, &s_cache);
        hud_refresh();
        demo_service_audio();
        demo_delay(FRAME_MS);
    }
    demo_delay(1000);
}

static void demo_run_lights_out(void) {
    /* Demo -7: Lights out. */
    game_init_mode(&s_game, 2, 0, s_map, true);
    s_game.extreme = true;

    game_set_maze_source(&s_game, s_map, maze_load);
    game_rand_seed(millis() * 2654435761u + 1u);
    playfield_draw_all(&s_game, &s_cache);
    hud_draw_bar();
    s_hud_score = 0xFFFFFFFFu;
    s_hud_bonus = -32768;
    s_hud_pu = 0xFF;
    hud_refresh();

    /* Force lights out */
    s_game.pu[0].id = 10; // Lights out
    s_game.pu[0].life = 100;
    s_game.pu_active = 10;
    s_game.pu_timer = 100;
    s_game.lights_timer = 0x02020200u | (uint32_t)(10 + (game_rand_int() % 10));

    uint32_t start = millis();
    while (millis() - start < 8000) {
        uint8_t keys = 0;
        game_tick(&s_game, keys);
        playfield_update(&s_game, &s_cache);
        hud_refresh();
        demo_service_audio();
        demo_delay(FRAME_MS);
    }
    demo_delay(1000);
}

static void demo_run_highscore(void) {
    /* Demo -8: High score demo. Play a game and show score entry. */
    game_init_classic(&s_game, 5, -1, NULL); // No maze, fast
    s_game.extreme = false;

    game_set_maze_source(&s_game, s_map, maze_load);
    game_rand_seed(millis() * 2654435761u + 1u);
    playfield_draw_all(&s_game, &s_cache);
    hud_draw_bar();
    s_hud_score = 0xFFFFFFFFu;
    s_hud_bonus = -32768;
    s_hud_pu = 0xFF;
    hud_refresh();

    /* Auto-play for a bit to get a score */
    uint32_t start = millis();
    while (millis() - start < 15000) {
        uint8_t keys = 0;
        game_tick(&s_game, keys);
        playfield_update(&s_game, &s_cache);
        hud_refresh();
        demo_service_audio();
        if (s_game.game_over) break;
        demo_delay(FRAME_MS);
    }

    if (s_game.game_over && highscores_qualifies(s_game.score, s_game.extreme)) {
        highscores_maybe_enter(s_game.score, s_game.extreme);
    }
    demo_delay(2000);
}

/* ── Public API ───────────────────────────────────────────────────────────── */

void instructions_show(void) {
    s_inst_page = 0;
    s_in_demo_menu = false;
    s_demo_selected = 0;
    inst_draw_page(0);
}

bool instructions_handle_input(btn_event_t ev, uint8_t btn) {
    /* main.cpp owns the event queue and passes the event it already dequeued.
     * Do not read buttons_get_event() here: the first event has already been
     * consumed by the main loop, so an internal drain loop would lose every
     * press and leave the tutorial screen permanently stuck. */
    if (ev != BTN_EVENT_PRESS && ev != BTN_EVENT_REPEAT) return true;

    if (s_in_demo_menu) {
        if (btn == BTN_UP) {
            if (s_demo_selected > 0) s_demo_selected--;
            inst_draw_demo_menu();
        } else if (btn == BTN_DOWN) {
            if (s_demo_selected < 7) s_demo_selected++;
            inst_draw_demo_menu();
        } else if (ev == BTN_EVENT_PRESS && btn == BTN_FIRE) {
            instructions_launch_demo(demo_ids[s_demo_selected]);
            inst_draw_demo_menu();
        } else if (ev == BTN_EVENT_PRESS &&
                   (btn == BTN_PAUSE || btn == BTN_LEFT || btn == BTN_RIGHT)) {
            s_in_demo_menu = false;
            s_inst_page = 11; // Back to last text page (Extreme5)
            inst_draw_page(s_inst_page);
        }
    } else {
        if (btn == BTN_UP) {
            if (s_inst_page > 0) s_inst_page--;
            inst_draw_page(s_inst_page);
        } else if (btn == BTN_DOWN) {
            if (s_inst_page < 11) s_inst_page++;
            inst_draw_page(s_inst_page);
        } else if (btn == BTN_LEFT) {
            s_in_demo_menu = true;
            s_demo_selected = 0;
            inst_draw_demo_menu();
        } else if (btn == BTN_RIGHT) {
            s_in_demo_menu = true;
            s_demo_selected = 0;
            inst_draw_demo_menu();
        } else if (ev == BTN_EVENT_PRESS && (btn == BTN_FIRE || btn == BTN_PAUSE)) {
            return false; // Exit to menu
        }
    }
    return true;
}

void instructions_launch_demo(int8_t demo_id) {
    /* Run the demo */
    s_screen = SCREEN_GAME;
    switch (demo_id) {
        case DEMO_TONGUE:        demo_run_tongue(); break;
        case DEMO_WALLS:         demo_run_walls(); break;
        case DEMO_BONUS_FOOD:    demo_run_bonus_food(); break;
        case DEMO_POWERUP_SPEED: demo_run_speed(); break;
        case DEMO_POWERUP_GHOST: demo_run_ghost(); break;
        case DEMO_POWERUP_COIN:  demo_run_coin_rush(); break;
        case DEMO_POWERUP_LIGHTS: demo_run_lights_out(); break;
        case DEMO_HIGHSCORE:     demo_run_highscore(); break;
    }

    /* Restore and redraw instructions */
    s_screen = SCREEN_INSTRUCTIONS;
    if (s_in_demo_menu) {
        inst_draw_demo_menu();
    } else {
        inst_draw_page(s_inst_page);
    }
}