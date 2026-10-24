#include "render/snake_sprites.h"
#include "config.h"

static const uint8_t GROUP_START[8] = {0, 1, 2, 3, 5, 7, 9, 11};
static const uint8_t GROUP_SIZE[8]  = {1, 1, 1, 2, 2, 2, 2, 4};

uint8_t spr_food_frame(uint8_t type, uint8_t frame) {
    if (type > 7) type = 0;
    if (frame >= GROUP_SIZE[type]) frame = 0;
    return SPR_FOOD + GROUP_START[type] + frame;
}

uint8_t snake_new_head_byte(uint8_t prev, uint8_t nd) {
    uint8_t pd = prev & 0x0F;
    uint8_t flag = 0;
    switch (nd) {
        case DIR_UP:    flag = pd == DIR_UP    ? (prev & 0x60) : pd == DIR_LEFT  ? 0x20 : pd == DIR_RIGHT ? 0x40 : 0; break;
        case DIR_DOWN:  flag = pd == DIR_DOWN  ? (prev & 0x60) : pd == DIR_LEFT  ? 0x40 : pd == DIR_RIGHT ? 0x20 : 0; break;
        case DIR_LEFT:  flag = pd == DIR_LEFT  ? (prev & 0x60) : pd == DIR_UP    ? 0x40 : pd == DIR_DOWN  ? 0x20 : 0; break;
        case DIR_RIGHT: flag = pd == DIR_RIGHT ? (prev & 0x60) : pd == DIR_UP    ? 0x20 : pd == DIR_DOWN  ? 0x40 : 0; break;
    }
    return nd | flag;
}

static uint8_t base_sprite(uint8_t dir) {
    switch (dir) {
        case DIR_UP:    return 2;
        case DIR_DOWN:  return 5;
        case DIR_LEFT:  return 1;
        default:        return 0;   /* DIR_RIGHT */
    }
}

static uint8_t apply_fat(uint8_t s) {
    switch (s) {
        case 7: return 19;
        case 6: return 18;
        case 9: return 21;
        case 8: return 20;
        case 2: return 14;
        case 5: return 17;
        case 1: return 13;
        case 0: return 12;
    }
    return s;
}

static uint8_t apply_ccw(uint8_t s) {
    switch (s) {
        case 26: return 27;
        case 29: return 28;
        case 19: return 23;
        case 18: return 22;
        case 14: return 15;
        case 17: return 16;
        case 7:  return 11;
        case 6:  return 10;
        case 2:  return 3;
        case 5:  return 4;
    }
    return s;
}

static uint8_t finish(uint8_t cur, uint8_t s) {
    if (cur & 0x10) s = apply_fat(s);
    if ((cur & 0x60) == 0x40) s = apply_ccw(s);
    return s;
}

uint8_t snake_body_sprite(uint8_t cur, uint8_t next) {
    uint8_t s = base_sprite(cur & 0x0F);
    uint8_t m = (cur | next) & 0x0F;
    uint8_t n = next & 0x0F;
    switch (m) {
        case 3:  s = (n == DIR_LEFT)  ? 7 : 8; break;
        case 9:  s = (n == DIR_RIGHT) ? 6 : 9; break;
        case 6:  s = (n == DIR_LEFT)  ? 9 : 6; break;
        case 12: s = (n == DIR_RIGHT) ? 8 : 7; break;
    }
    return finish(cur, s);
}

uint8_t snake_tail_sprite(uint8_t cur, uint8_t next) {
    uint8_t s;
    switch (next & 0x0F) {
        case DIR_UP:    s = 26; break;
        case DIR_DOWN:  s = 29; break;
        case DIR_LEFT:  s = 25; break;
        default:        s = 24; break;   /* DIR_RIGHT */
    }
    return finish(cur, s);
}

spr_pick_t snake_tongue_sprite(uint8_t cur) {
    /* b.java: j=1 m=90 o=-1 O=-7 (o-1 unless the 0x20 flag); j=4 m=270 o=-2 O=1
     * (o+1 unless 0x20); j=2 m=180 o=-6 O=-2; j=8 m=0 o=3 O=-2. Offsets are from
     * (k,l), the cell one ahead of the head. */
    bool cw = (cur & 0x60) == 0x20;
    spr_pick_t p;
    p.dy = -2;
    switch (cur & 0x0F) {
        case DIR_UP:
            p.id = SPR_TONGUE_U; p.dx = cw ? -1 : -2; p.dy = -7; break;
        case DIR_DOWN:
            p.id = SPR_TONGUE_D; p.dx = cw ? -2 : -1; p.dy = 1; break;
        case DIR_LEFT:
            p.id = SPR_TONGUE_L; p.dx = -6; break;
        default:   /* DIR_RIGHT */
            p.id = SPR_TONGUE_R; p.dx = 3; break;
    }
    return p;
}

spr_pick_t snake_head_sprite(uint8_t cur, bool open) {
    bool cw = (cur & 0x60) == 0x20;
    spr_pick_t p;
    switch (cur & 0x0F) {
        case DIR_UP:
            if (open) { p.id = cw ? 38 : 39; p.dx = cw ? -4 : -1; p.dy = -5; }
            else      { p.id = cw ? 32 : 33; p.dx = cw ? -3 :  0; p.dy = -3; }
            break;
        case DIR_DOWN:
            if (open) { p.id = cw ? 41 : 40; p.dx = cw ? -1 : -4; p.dy = 0; }
            else      { p.id = cw ? 35 : 34; p.dx = cw ?  0 : -3; p.dy = 0; }
            break;
        case DIR_LEFT:
            if (open) { p.id = 37; p.dx = -5; p.dy = -2; }
            else      { p.id = 31; p.dx = -3; p.dy = -1; }
            break;
        default:   /* DIR_RIGHT */
            if (open) { p.id = 36; p.dx = 0; p.dy = -2; }
            else      { p.id = 30; p.dx = 0; p.dy = -1; }
            break;
    }
    return p;
}
