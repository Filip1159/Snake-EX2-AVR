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
    /* Paint every pixel exactly once: field background, then the black top
     * border, side borders and HUD area.  The previous full-screen clear was
     * immediately overwritten by the field fill, doubling the SPI traffic on
     * every full redraw (Q5). */
    st7735_fill_rect(FIELD_ORIGIN_X, FIELD_ORIGIN_Y, FIELD_WIDTH_PX, FIELD_HEIGHT_PX, COLOR_FIELD_BG);
    st7735_fill_rect(0, 0, TFT_WIDTH, FIELD_ORIGIN_Y, 0x0000);
    st7735_fill_rect(0, FIELD_ORIGIN_Y, FIELD_ORIGIN_X, FIELD_HEIGHT_PX, 0x0000);
    uint8_t right_x = (uint8_t)(FIELD_ORIGIN_X + FIELD_WIDTH_PX);
    st7735_fill_rect(right_x, FIELD_ORIGIN_Y, (uint8_t)(TFT_WIDTH - right_x),
                     FIELD_HEIGHT_PX, 0x0000);
    uint8_t hud_y = (uint8_t)(FIELD_ORIGIN_Y + FIELD_HEIGHT_PX);
    st7735_fill_rect(0, hud_y, TFT_WIDTH, (uint8_t)(TFT_HEIGHT - hud_y), 0x0000);
    for (uint8_t r = 0; r < MAZE_ROWS; r++) {
        for (uint8_t c = 0; c < MAZE_COLS; c++) {
            uint8_t t = map[r * MAZE_COLS + c] & 0x0F;
            if (t) field_draw_cell(c, r, t);
        }
    }
}
