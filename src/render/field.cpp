#include "render/field.h"
#include "assets/assets_gen.h"
#include "assets/maze.h"
#include "drivers/st7735.h"
#include "config.h"

void field_draw_cell(uint8_t col, uint8_t row, uint8_t tile) {
    uint8_t x = FIELD_ORIGIN_X + col * CELL_SIZE;
    uint8_t y = FIELD_ORIGIN_Y + row * CELL_SIZE;
    tile &= 0x0F;
    if (tile == 0) {
        st7735_fill_rect(x, y, CELL_SIZE, CELL_SIZE, COLOR_FIELD_BG);
        return;
    }
    uint16_t buf[CELL_SIZE * CELL_SIZE];
    const uint8_t *src = WALL_TILES[tile - 1];
    for (uint8_t i = 0; i < WALL_TILE_BYTES; i++) {
        uint8_t b = pgm_read_byte(&src[i]);
        buf[2 * i]     = pgm_read_word(&WALL_PALETTE[b >> 4]);
        buf[2 * i + 1] = pgm_read_word(&WALL_PALETTE[b & 0x0F]);
    }
    st7735_blit(x, y, CELL_SIZE, CELL_SIZE, buf);
}

void field_draw_map(const uint8_t *map) {
    st7735_fill_screen(0x0000);
    st7735_fill_rect(FIELD_ORIGIN_X, FIELD_ORIGIN_Y, FIELD_WIDTH_PX, FIELD_HEIGHT_PX, COLOR_FIELD_BG);
    for (uint8_t r = 0; r < MAZE_ROWS; r++) {
        for (uint8_t c = 0; c < MAZE_COLS; c++) {
            uint8_t t = map[r * MAZE_COLS + c] & 0x0F;
            if (t) field_draw_cell(c, r, t);
        }
    }
}
