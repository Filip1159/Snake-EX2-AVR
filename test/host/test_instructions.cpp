/* Host regression test: instructions input is driven by the event already
 * dequeued by main.cpp.  The old handler drained buttons_get_event() itself,
 * which consumed and discarded every event and left the tutorial stuck. */
#include "ui/instructions.h"
#include "ui/highscores.h"
#include "render/text.h"
#include "render/playfield.h"
#include "drivers/st7735.h"
#include "drivers/buzzer.h"
#include "drivers/eeprom_store.h"
#include "game/classic.h"
#include "game/game.h"
#include "game/rng.h"
#include "screens.h"
#include "gamestate.h"
#include "Arduino.h"
#include <stdio.h>

/* ── Shared state normally owned by main.cpp ─────────────────────────────── */
screen_t          s_screen;
uint8_t           s_map[483];
game_t            s_game;
playfield_cache_t s_cache;
uint32_t          s_hud_score;
int16_t           s_hud_bonus;
uint8_t           s_hud_pu;
uint32_t          mock_millis;

/* ── Hardware/render/game stubs ──────────────────────────────────────────── */
static int fill_screen_calls;
static int get_event_calls;

static settings_t mock_settings;

extern "C" {
const settings_t *eeprom_get_settings(void) { return &mock_settings; }

void st7735_fill_screen(uint16_t c) { (void)c; fill_screen_calls++; }
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    (void)x; (void)y; (void)w; (void)h; (void)c;
}

void text_draw(uint8_t x, uint8_t y, const char *txt, uint8_t scale, uint16_t color) {
    (void)x; (void)y; (void)txt; (void)scale; (void)color;
}
void text_draw_p(uint8_t x, uint8_t y, const char *txt, uint8_t scale, uint16_t color) {
    (void)x; (void)y; (void)txt; (void)scale; (void)color;
}
uint8_t text_width(const char *txt, uint8_t scale) { (void)txt; (void)scale; return 0; }
uint8_t text_width_p(const char *txt, uint8_t scale) { (void)txt; (void)scale; return 0; }

void buzzer_play(uint8_t id) { (void)id; }
void buzzer_update(void) {}
bool buzzer_is_playing(void) { return false; }

uint8_t buttons_poll(void) { return 0; }
btn_event_t buttons_get_event(uint8_t *idx) {
    (void)idx;
    get_event_calls++;
    return BTN_EVENT_NONE;
}
bool buttons_is_pressed(uint8_t btn) { (void)btn; return false; }

bool game_init_classic(game_t *g, uint8_t speed, int8_t maze, uint8_t *map_buf) {
    (void)g; (void)speed; (void)maze; (void)map_buf; return true;
}
void game_init_mode(game_t *g, uint8_t speed, int8_t maze,
                    const uint8_t *maze_map, bool extreme) {
    (void)g; (void)speed; (void)maze; (void)maze_map; (void)extreme;
}
void game_set_maze_source(game_t *g, uint8_t *buf,
                          int8_t (*loader)(uint8_t, uint8_t *)) {
    (void)g; (void)buf; (void)loader;
}
void game_tick(game_t *g, uint8_t keys) { (void)g; (void)keys; }
void game_rand_seed(uint32_t seed) { (void)seed; }
int32_t game_rand_int(void) { return 0; }
int8_t maze_load(uint8_t index, uint8_t *buf) { (void)index; (void)buf; return 0; }

void playfield_draw_all(const game_t *g, playfield_cache_t *cache) {
    (void)g; (void)cache;
}
bool playfield_update(const game_t *g, playfield_cache_t *cache) {
    (void)g; (void)cache; return false;
}
void hud_draw_bar(void) {}
void hud_refresh(void) {}

bool highscores_qualifies(uint32_t score, bool extreme) {
    (void)score; (void)extreme; return false;
}
bool highscores_submit(uint32_t score, bool extreme) {
    (void)score; (void)extreme; return false;
}
}

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main() {
    instructions_show();
    CHECK(fill_screen_calls == 1);
    CHECK(get_event_calls == 0);

    /* Releases are ignored. */
    CHECK(instructions_handle_input(BTN_EVENT_RELEASE, BTN_FIRE));
    CHECK(fill_screen_calls == 1);

    /* Down advances a page and repaints it. */
    CHECK(instructions_handle_input(BTN_EVENT_PRESS, BTN_DOWN));
    CHECK(fill_screen_calls == 2);

    /* Auto-repeat also advances, matching the menu behaviour. */
    CHECK(instructions_handle_input(BTN_EVENT_REPEAT, BTN_DOWN));
    CHECK(fill_screen_calls == 3);

    /* Right opens the demo menu. */
    CHECK(instructions_handle_input(BTN_EVENT_PRESS, BTN_RIGHT));
    CHECK(fill_screen_calls == 4);

    /* Demo-menu navigation repaints the list. */
    CHECK(instructions_handle_input(BTN_EVENT_PRESS, BTN_DOWN));
    CHECK(fill_screen_calls == 5);

    /* Pause leaves the demo menu and returns to the last text page. */
    CHECK(instructions_handle_input(BTN_EVENT_PRESS, BTN_PAUSE));
    CHECK(fill_screen_calls == 6);

    /* Fire-repeat must not exit; a real press exits to the menu. */
    CHECK(instructions_handle_input(BTN_EVENT_REPEAT, BTN_FIRE));
    CHECK(fill_screen_calls == 6);
    CHECK(!instructions_handle_input(BTN_EVENT_PRESS, BTN_FIRE));

    /* The handler must never consume the queue itself: main.cpp already
     * dequeued this event.  This is the regression that froze the screen. */
    CHECK(get_event_calls == 0);

    printf(fails ? "instructions: %d FAILED\n" : "instructions: all passed\n", fails);
    return fails != 0;
}
