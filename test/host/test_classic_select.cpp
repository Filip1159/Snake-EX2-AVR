/* G9: every Classic combination (-1..4 x speed 0..5) starts with the right map,
 * cadence and score step. Maze bytes are compared with the independently derived
 * build/assets/mazes_21x23.bin, not with the loader's own output. */
#include "assets/maze.h"
#include "game/classic.h"
#include "game/game.h"
#include "config.h"
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

extern "C" int32_t game_rand_int(void) { return (int32_t)0x12345678; }

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static std::vector<uint8_t> slurp(const char *p) {
    FILE *f = fopen(p, "rb");
    if (!f) { printf("cannot open %s\n", p); exit(2); }
    std::vector<uint8_t> v;
    int c;
    while ((c = fgetc(f)) != EOF) v.push_back((uint8_t)c);
    fclose(f);
    return v;
}

static void check_name(int8_t maze, const char *want) {
    const char *got = maze_name(maze);
    CHECK(got != 0);
    if (!got) return;
    size_t i = 0;
    for (; want[i]; i++) CHECK(pgm_read_byte(got + i) == (uint8_t)want[i]);
    CHECK(pgm_read_byte(got + i) == 0);
}

int main() {
    static const char *const names[] = {
        "No maze", "Box", "Tunnel", "Spiral", "Blockade", "Twisted"
    };
    for (int8_t m = MAZE_NONE; m <= MAZE_TWISTED; m++) check_name(m, names[m + 1]);
    CHECK(maze_name(-2) == 0);
    CHECK(maze_name(5) == 0);

    std::vector<uint8_t> derived = slurp("build/assets/mazes_21x23.bin");
    CHECK(derived.size() >= (size_t)MAZE_COUNT * MAZE_CELLS);

    for (int8_t maze = MAZE_NONE; maze <= MAZE_TWISTED; maze++) {
        for (uint8_t speed = SPEED_INDEX_MIN; speed <= SPEED_INDEX_MAX; speed++) {
            uint8_t map[MAZE_CELLS];
            memset(map, 0xA5, sizeof map);
            game_t g;
            memset(&g, 0x5A, sizeof g);

            bool ok = game_init_classic(&g, speed, maze,
                                        maze == MAZE_NONE ? nullptr : map);
            CHECK(ok);
            if (!ok) continue;

            CHECK(g.speed == speed);
            CHECK(g.maze == maze);
            CHECK(!g.extreme);
            CHECK(g.map == (maze == MAZE_NONE ? nullptr : map));
            CHECK(g.score_step == speed + 1);
            CHECK(g.per_ma == SPEED_FRAMES_BASE + (SPEED_OFFSET - speed));
            CHECK(g.per_La == g.per_ma && g.per_Ma == g.per_ma &&
                  g.per_Na == g.per_ma);
            CHECK(g.len == START_LENGTH);
            CHECK(g.qx == START_HEAD_X && g.qy == START_HEAD_Y);
            CHECK(g.level == 1 && g.level_start_score == 0);
            CHECK(g.portal_state == 0);

            if (maze == MAZE_NONE) {
                for (uint8_t row = 0; row < FIELD_ROWS; row++)
                    for (uint8_t col = 0; col < FIELD_COLS; col++)
                        CHECK(game_tile_at(&g, col, row) == 0);
            } else {
                const uint8_t *want = &derived[(size_t)maze * MAZE_CELLS];
                CHECK(memcmp(map, want, MAZE_CELLS) == 0);
                CHECK(g.map_buf == map);
                CHECK(g.maze_loader == maze_load);
            }

            /* One cadence window: no movement before per_ma, movement on it. */
            g.dir = DIR_RIGHT;
            for (uint16_t i = 0; i < g.len; i++) g.a[i] = DIR_RIGHT;
            g.input = DIR_RIGHT;
            g.food_x = 24; g.food_y = 24;
            int16_t x0 = g.qx;
            for (uint8_t i = 0; i + 1 < g.per_ma; i++) {
                game_tick(&g, 0);
                CHECK(g.qx == x0);
            }
            game_tick(&g, 0);
            CHECK(g.qx != x0);
        }
    }

    /* Invalid selection is rejected before game_t is initialised. */
    {
        game_t g;
        uint8_t map[MAZE_CELLS];
        memset(&g, 0x5A, sizeof g);
        CHECK(!game_init_classic(&g, SPEED_INDEX_MAX + 1, MAZE_NONE, nullptr));
        CHECK(!game_init_classic(&g, 0, MAZE_TWISTED + 1, map));
        CHECK(!game_init_classic(&g, 0, MAZE_BOX, nullptr));
        CHECK(g.speed == 0x5A);   /* unchanged */
    }

    if (fails) printf("classic_select: %d FAILED\n", fails);
    else printf("classic_select: all passed (6 mazes x 6 speeds, names, invalid input)\n");
    return fails != 0;
}
