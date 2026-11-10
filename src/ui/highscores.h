/**
 * highscores.h — High-score table and saving.
 *
 * Displays the top three scores per mode (Classic indices 0..2, Extreme 3..5).
 * On game over, a qualifying score is saved without any extra screen.
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

/** Show the top-3 table for the given mode (chosen in the Classic/Extreme list). */
void highscores_show(hs_mode_t mode);

/** Called from game over: saves the score if it qualifies (and plays the
 *  high-score sound). Returns true if the score was saved. */
bool highscores_submit(uint32_t score, bool extreme);

/** Repaint the complete 128×160 high-score view. */
void highscores_draw(hs_mode_t mode);

/** Check if a score would enter the top 3 of its mode. */
bool highscores_qualifies(uint32_t score, bool extreme);

#ifdef __cplusplus
}
#endif

#endif