/* Host test: saved game in EEPROM (Continue). */
#include "game/savegame.h"
#include "game/game.h"
#include "drivers/eeprom_store.h"
#include <stdio.h>
#include <string.h>

uint8_t mock_eeprom[1024];
unsigned mock_eeprom_writes;
uint32_t mock_millis = 0;

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static game_t a, b;

int main() {
    memset(mock_eeprom, 0xFF, sizeof mock_eeprom);
    eeprom_store_init();
    CHECK(!savegame_exists());
    CHECK(!savegame_load(&b));

    /* Play a little Extreme so the state is non-trivial. */
    game_init_mode(&a, 3, -1, NULL, true);
    for (int i = 0; i < 400; i++) game_tick(&a, (i % 7 == 0) ? KEY_UP : (i % 11 == 0 ? KEY_LEFT : 0));
    a.score = 1234; a.pu[1].id = 5; a.pu[1].life = 9; a.coins[3] = 0xBEEF;

    savegame_save(&a);
    CHECK(savegame_exists());
    CHECK(savegame_load(&b));
    CHECK(b.len == a.len && memcmp(b.a, a.a, a.len) == 0);
    CHECK(b.qx == a.qx && b.qy == a.qy && b.dir == a.dir && b.score == 1234);
    CHECK(b.speed == 3 && b.extreme && b.maze == a.maze);
    CHECK(b.food_x == a.food_x && b.food_y == a.food_y && b.bonus == a.bonus);
    CHECK(b.pu[1].id == 5 && b.pu[1].life == 9 && b.coins[3] == 0xBEEF);
    CHECK(b.cnt_c == a.cnt_c && b.per_ma == a.per_ma && b.level == a.level);
    CHECK(b.map == NULL && b.map_buf == NULL && b.maze_loader == NULL);

    /* A reloaded game ticks identically to the original. */
    game_t c = a;
    for (int i = 0; i < 60; i++) {
        game_tick(&c, i % 5 == 0 ? KEY_RIGHT : 0);
        game_tick(&b, i % 5 == 0 ? KEY_RIGHT : 0);
    }
    CHECK(c.len == b.len && c.qx == b.qx && c.qy == b.qy && c.score == b.score);

    /* Saving again after the snake grew/shrank, with a shorter payload. */
    game_init_mode(&a, 1, -1, NULL, false);
    savegame_save(&a);
    CHECK(savegame_load(&b) && b.len == a.len && !b.extreme && b.speed == 1);

    /* Cleared / corrupted saves are rejected. */
    savegame_clear();
    CHECK(!savegame_exists());
    savegame_save(&a);
    CHECK(savegame_exists());
    mock_eeprom[64 + 7 + 3] ^= 0x40;                    /* flip a payload bit */
    CHECK(!savegame_exists() && !savegame_load(&b));
    savegame_save(&a);
    CHECK(savegame_exists());
    mock_eeprom[64 + 1] ^= 1;                           /* layout size mismatch */
    CHECK(!savegame_exists());

    /* Settings and scores in the other blocks are untouched. */
    eeprom_set_score(0, 777);
    savegame_save(&a);
    CHECK(eeprom_load_scores() && eeprom_get_score(0) == 777);

    printf(fails ? "savegame: %d FAILED\n" : "savegame: all passed\n", fails);
    return fails != 0;
}
