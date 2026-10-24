/**
 * highscores.h — High-score screen and entry (BACKLOG U3).
 *
 * Displays two scores per mode (Classic indices 0/1, Extreme indices 2/3).
 * On game over, if the score qualifies, shows an entry screen where the
 * player can input initials (3 characters A-Z) and saves the score.
 */
#ifndef UI_HIGHSCORES_H
#define UI_HIGHSCORES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HS_MODE_CLASSIC = 0,
    HS_MODE_EXTREME,
    HS_MODE_COUNT
} hs_mode_t;

/** Show the high-score table for the given mode (from main menu). */
void highscores_show(hs_mode_t mode);

/** Called from game over when a new score qualifies.
 *  Returns true if the score was entered and saved. */
bool highscores_maybe_enter(uint32_t score, bool extreme);

/** Repaint the complete 128×160 high-score table screen. */
void highscores_draw(hs_mode_t mode);

/** Check if a score qualifies for the high-score table. */
bool highscores_qualifies(uint32_t score, bool extreme);

#ifdef __cplusplus
}
#endif

#endif