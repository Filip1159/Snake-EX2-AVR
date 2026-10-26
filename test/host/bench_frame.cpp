/* Q5 host frame-budget check: counts the pixels written per game tick and
 * converts them to the ST7735 SPI lower bound at F_CPU/2 = 4 MHz (4 us/pixel).
 * Normal dirty-cell frames must stay under the 50 ms budget; event-only full
 * redraws are reported separately. */
#include "assets/maze.h"
#include "game/game.h"
#include "render/playfield.h"
#include "render/hud.h"
#include "drivers/st7735.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

static uint32_t px_written;
static uint8_t st_w, st_px;

extern "C" {
void st7735_fill_screen(uint16_t c) { (void)c; px_written += 128u * 160u; }
void st7735_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t c) {
    (void)x; (void)y; (void)c; px_written += (uint32_t)w * h;
}
void st7735_blit(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint16_t *b) {
    (void)x; (void)y; (void)b; px_written += (uint32_t)w * h;
}
void st7735_stream_begin(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    (void)x; (void)y; (void)h; st_w = w; st_px = 0;
}
void st7735_stream_pixels(uint16_t c, uint8_t n) {
    (void)c; px_written += n; st_px += n;
}
void st7735_stream_end(void) { (void)st_w; (void)st_px; }
}

/* deterministic pseudo-random input */
static uint32_t rs = 0x12345678u;
static uint32_t next(void) { rs = rs * 1103515245u + 12345u; return rs >> 16; }
extern "C" int32_t game_rand_int(void) { return (int32_t)(next() | ((uint32_t)next() << 16)); }

int main(void) {
    static game_t g;
    playfield_cache_t cache;
    uint8_t map[MAZE_CELLS];
    if (maze_load(MAZE_BOX, map) != 0) return 1;

    game_init_mode(&g, 5, MAZE_BOX, map, true);
    game_set_maze_source(&g, map, maze_load);
    px_written = 0;
    playfield_draw_all(&g, &cache);
    hud_draw_bar();
    uint32_t full_px = px_written;

    uint32_t max_normal = 0, max_full = 0, total = 0, ticks = 0, fulls = 0;
    for (int t = 0; t < 12000; t++) {
        uint8_t keys = 0;
        uint32_t r = next();
        if (r & 1) keys |= KEY_UP;
        if (r & 2) keys |= KEY_DOWN;
        if (r & 4) keys |= KEY_LEFT;
        if (r & 8) keys |= KEY_RIGHT;

        px_written = 0;
        game_tick(&g, keys);
        bool full = playfield_update(&g, &cache);
        if (full) {
            hud_draw_bar();
            fulls++;
        }
        /* Worst-case HUD refresh: redraw both counters and the icon every tick. */
        hud_draw_score(g.score);
        hud_draw_bonus((uint16_t)(g.bonus > 0 ? g.bonus : 0));
        hud_draw_powerup(g.pu_active);
        if (full) {
            if (px_written > max_full) max_full = px_written;
        } else {
            if (px_written > max_normal) max_normal = px_written;
        }
        total += px_written;
        ticks++;

        if (g.game_over) {
            game_init_mode(&g, 5, MAZE_BOX, map, true);
            game_set_maze_source(&g, map, maze_load);
            px_written = 0;
            playfield_draw_all(&g, &cache);
            hud_draw_bar();
            if (px_written > max_full) max_full = px_written;
            fulls++;
        }
    }

    /* 16 bits/pixel at 4 MHz = 4 us/pixel = 0.004 ms/pixel. */
    double normal_ms = max_normal * 0.004;
    double full_ms = max_full * 0.004;
    double avg_ms = (double)total / ticks * 0.004;
    printf("frame_budget: max normal %.2f ms, avg %.2f ms, full-redraw %.2f ms "
           "(%u full events), initial full %.2f ms\n",
           normal_ms, avg_ms, full_ms, fulls, full_px * 0.004);

    if (normal_ms >= 50.0) {
        printf("FAIL frame_budget: normal frame %.2f ms >= 50 ms\n", normal_ms);
        return 1;
    }
    printf("frame_budget: all passed\n");
    return 0;
}
