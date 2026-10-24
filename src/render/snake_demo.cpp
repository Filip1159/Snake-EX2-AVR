#include "render/snake_demo.h"
#include "render/snake_sprites.h"
#include "render/sprite.h"
#include "config.h"
#include <string.h>

/* Path from the tail, in cells; starts at (2,2); verified collision-free and inside the Box maze. */
static const char PATH[] =
    "RRRRRRRR" "DDD" "LLLL" "DDD" "RRRRRRRRRR" "UUU" "RR" "DDDDDDDDD" "LLLLLL" "UUU" "LLLL" "DDDDDD" "RRRRR";
#define PATH_LEN (sizeof(PATH) - 1)
#define SEGS     (PATH_LEN + 1)

static uint8_t dir_of(char c) {
    switch (c) {
        case 'U': return DIR_UP;
        case 'D': return DIR_DOWN;
        case 'L': return DIR_LEFT;
        default:  return DIR_RIGHT;
    }
}

static void cell_xy(uint8_t c, uint8_t r, int16_t *x, int16_t *y) {
    *x = FIELD_ORIGIN_X + c * CELL_SIZE;
    *y = FIELD_ORIGIN_Y + r * CELL_SIZE;
}

void snake_demo_draw(bool open, bool fat) {
    uint8_t a[SEGS];
    uint8_t col[SEGS], row[SEGS];

    a[0] = DIR_RIGHT | 0x20;            /* original start byte 0x28 */
    col[0] = 2; row[0] = 2;
    for (uint8_t i = 1; i < SEGS; i++) {
        a[i] = snake_new_head_byte(a[i - 1], dir_of(PATH[i - 1]));
        col[i] = col[i - 1]; row[i] = row[i - 1];
        switch (a[i] & 0x0F) {
            case DIR_UP:    row[i]--; break;
            case DIR_DOWN:  row[i]++; break;
            case DIR_LEFT:  col[i]--; break;
            default:        col[i]++; break;
        }
        if (fat && i % 7 == 0 && i < SEGS - 1) a[i] |= 0x10;
    }

    int16_t x, y;
    uint8_t h = SEGS - 1;
    if (open) {                         /* food in the cell ahead of the head */
        cell_xy(col[h] + 1, row[h], &x, &y);
        sprite_draw(x - 1, y - 1, SPR_FOOD);
    }
    /* head first, then toward the tail, like b.java */
    for (int8_t i = h; i >= 0; i--) {
        cell_xy(col[i], row[i], &x, &y);
        if (i == h) {
            spr_pick_t p = snake_head_sprite(a[i], open);
            sprite_draw(x + p.dx, y + p.dy, p.id);
        } else if (i == 0) {
            sprite_draw(x, y, snake_tail_sprite(a[0], a[1]));
        } else {
            sprite_draw(x, y, snake_body_sprite(a[i], a[i + 1]));
        }
    }

    /* sprite gallery: normal food, bonus food frames, power-up item, marker (row 20) */
    int16_t gx = FIELD_ORIGIN_X + 7 * CELL_SIZE;
    int16_t gy = FIELD_ORIGIN_Y + 20 * CELL_SIZE;
    for (uint8_t id = SPR_FOOD; id <= SPR_MARKER; id++) {
        sprite_draw(gx, gy, id);
        gx += 10;
        if (gx > 120 - 10) { gx = FIELD_ORIGIN_X + 7 * CELL_SIZE; gy += 10; }
    }
}
