/**
 * snake_sprites.h — Snake segment -> sprite mapping, transcribed from b.java draw()
 * (lines ~470-760). See docs/sprites.md.
 *
 * Direction bytes a[i] (original): low nibble = direction (DIR_*), 0x10 = segment holds
 * food (fat sprite), 0x20 / 0x40 = clockwise / counter-clockwise last turn (stripe phase).
 * a[0] is the tail, a[len-1] the head.
 */
#ifndef RENDER_SNAKE_SPRITES_H
#define RENDER_SNAKE_SPRITES_H

#include <stdint.h>
#include <stdbool.h>

/* SPRITES[] indices */
#define SPR_B(n)          (n)             /* B0..B41 */
#define SPR_FOOD          42              /* P0_0, 8x8, drawn at cell offset (-1,-1) */
#define SPR_BONUS_FIRST   43              /* P1_0.. : bonus food type Fa=1..7, frames */
#define SPR_POWERUP_ITEM  57              /* C, 8x8 at (-1,-1) */
#define SPR_MARKER        58              /* c, 10x10 at (-2,-2) */
#define SPR_TONGUE_R      59              /* f.png rotated 0deg   (points right) */
#define SPR_TONGUE_U      60              /* f.png rotated 90deg CCW (points up) */
#define SPR_TONGUE_L      61              /* f.png rotated 180deg */
#define SPR_TONGUE_D      62              /* f.png rotated 270deg CCW (points down) */

typedef struct {
    uint8_t id;
    int8_t  dx, dy;    /* pixel offset from the cell's top-left */
} spr_pick_t;

#ifdef __cplusplus
extern "C" {
#endif

/** Sprite index of bonus-food frame: type 0..7 (0 = normal food), frame within its group. */
uint8_t spr_food_frame(uint8_t type, uint8_t frame);

/** Direction byte for a new head direction `new_dir`, given the previous head byte. */
uint8_t snake_new_head_byte(uint8_t prev_head, uint8_t new_dir);

/** Body segment 0 < i < len-1: cur = a[i], next = a[i+1]. */
uint8_t snake_body_sprite(uint8_t cur, uint8_t next);

/** Tail (i = 0): cur = a[0], next = a[1]. */
uint8_t snake_tail_sprite(uint8_t cur, uint8_t next);

/** Head: cur = a[len-1]; `open` = food/coin in the next cell (11x11 open mouth). */
spr_pick_t snake_head_sprite(uint8_t cur, bool open);

/** Tongue sprite and offset from the top-left of the cell one ahead of the head.
 *  cur = head byte; its 0x20/0x40 turn flag shifts the up/down tongue by 1 px,
 *  exactly like the original (b.java tongue draw block). */
spr_pick_t snake_tongue_sprite(uint8_t cur);

#ifdef __cplusplus
}
#endif

#endif
