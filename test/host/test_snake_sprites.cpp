#include "render/snake_sprites.h"
#include "assets/assets_gen.h"
#include "config.h"
#include <stdio.h>
#include "sprite_vectors.inc"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

/* Firmware packs 4 bpp row-major, high nibble first (assets_gen.h). */
static uint8_t sprite_pixel(uint8_t id, uint8_t x, uint8_t y) {
    const spr_t &s = SPRITES[id];
    uint16_t k = (uint16_t)y * s.w + x;
    uint8_t b = pgm_read_byte(&s.pix[k >> 1]);
    return (k & 1) ? (uint8_t)(b & 0x0F) : (uint8_t)(b >> 4);
}

int main() {
    for (auto &v : BODY_V) {
        if (snake_body_sprite(v.cur, v.next) != v.body) { printf("body cur=%02X next=%02X got %d want %d\n", v.cur, v.next, snake_body_sprite(v.cur, v.next), v.body); fails++; }
        if (snake_tail_sprite(v.cur, v.next) != v.tail) { printf("tail cur=%02X next=%02X\n", v.cur, v.next); fails++; }
    }
    for (auto &v : HEAD_V) {
        spr_pick_t p = snake_head_sprite(v.cur, v.open);
        if (p.id != v.id || p.dx != v.dx || p.dy != v.dy) { printf("head cur=%02X open=%d\n", v.cur, v.open); fails++; }
        /* size check: closed head sprites are 9x9, open are 11x11 */
        uint8_t w = SPRITES[p.id].w; (void)w;
    }
    for (auto &v : TURN_V)
        if (snake_new_head_byte(v.prev, v.nd) != v.expect) { printf("turn prev=%02X nd=%d got %02X want %02X\n", v.prev, v.nd, snake_new_head_byte(v.prev, v.nd), v.expect); fails++; }

    /* sprite sizes match their role */
    for (int i = 0; i < 30; i++) CHECK(SPRITES[i].w == 6 && SPRITES[i].h == 6);
    for (int i = 30; i < 36; i++) CHECK(SPRITES[i].w == 9);
    for (int i = 36; i < 42; i++) CHECK(SPRITES[i].w == 11);
    CHECK(SPRITES[SPR_FOOD].w == 8 && SPRITES[SPR_POWERUP_ITEM].w == 8 && SPRITES[SPR_MARKER].w == 10);

    /* Decoded art pins the nibble order from SnakeCanvas.a(): low nibble first,
       then the same byte shifted right by 4. A high-first decode swaps every
       adjacent pixel pair and garbles heads/turns. */
    CHECK(sprite_pixel(0, 0, 2) == 2);   /* B0 body: green at x=0 */
    CHECK(sprite_pixel(0, 1, 2) == 3);   /* B0 body: dark green at x=1 */
    CHECK(sprite_pixel(0, 0, 3) == 4);   /* B0 body: white dash starts at x=0 */
    CHECK(sprite_pixel(30, 3, 4) == 4);  /* B30 closed head right: white eye */
    CHECK(sprite_pixel(36, 4, 4) == 4);  /* B36 open head right: white eye */

    /* tongue: f.png rotated per direction, offsets = b.java (k,l)+o/O */
    struct { uint8_t cur, id; int8_t dx, dy; } tv[] = {
        {DIR_RIGHT | 0x20, SPR_TONGUE_R,  3, -2},
        {DIR_LEFT  | 0x20, SPR_TONGUE_L, -6, -2},
        {DIR_UP    | 0x20, SPR_TONGUE_U, -1, -7},   /* 0x20 flag: o=-1 */
        {DIR_UP    | 0x40, SPR_TONGUE_U, -2, -7},   /* otherwise: o=-2 */
        {DIR_DOWN  | 0x20, SPR_TONGUE_D, -2,  1},   /* 0x20 flag: o=-2 */
        {DIR_DOWN  | 0x40, SPR_TONGUE_D, -1,  1},   /* otherwise: o=-1 */
    };
    for (auto &v : tv) {
        spr_pick_t t = snake_tongue_sprite(v.cur);
        CHECK(t.id == v.id && t.dx == v.dx && t.dy == v.dy);
    }
    CHECK(SPRITES[SPR_TONGUE_R].w == 11 && SPRITES[SPR_TONGUE_U].h == 11);
    CHECK(sprite_pixel(SPR_TONGUE_R, 0, 0) == 0);     /* corner transparent */
    CHECK(sprite_pixel(SPR_TONGUE_R, 0, 4) == 1);     /* black outline at the base */
    CHECK(sprite_pixel(SPR_TONGUE_R, 4, 5) == 2);     /* red tongue body */
    CHECK(spr_food_frame(0, 0) == 42 && spr_food_frame(3, 1) == 42 + 3 + 1 && spr_food_frame(7, 3) == 56);
    if (fails) printf("snake_sprites: %d FAILED\n", fails);
    else printf("snake_sprites: all passed (%d body/tail, %d head, %d turn vectors)\n", (int)(sizeof(BODY_V)/sizeof(BODY_V[0])), (int)(sizeof(HEAD_V)/sizeof(HEAD_V[0])), (int)(sizeof(TURN_V)/sizeof(TURN_V[0])));
    return fails != 0;
}
