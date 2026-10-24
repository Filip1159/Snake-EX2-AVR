/**
 * hud.h — HUD bar, score/bonus digits, power-up icon, title (BACKLOG A4).
 * Layout from b.java: HUD image at y=108 (derived 139); score digits at (6,114),
 * bonus digits at (69,114), power-up icon slot at (110,111). Derived = y + 31.
 */
#ifndef RENDER_HUD_H
#define RENDER_HUD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Draw the empty HUD bar at (0,HUD_Y). */
void hud_draw_bar(void);
/** 6-digit score (value mod 1,000,000, zero padded). */
void hud_draw_score(uint32_t score);
/** 4-digit bonus counter (value mod 10,000, zero padded). */
void hud_draw_bonus(uint16_t value);
/** Power-up icon for id 1..10, or 0 to restore the empty slot. */
void hud_draw_powerup(uint8_t id);

/** Title art (128x128) at (0,16), with the 16 rows above/below filled from its edge colours. */
void title_draw(void);

#ifdef __cplusplus
}
#endif

#endif
