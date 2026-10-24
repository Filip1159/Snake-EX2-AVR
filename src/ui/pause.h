/**
 * pause.h — Pause overlay and game-over screen (BACKLOG U4).
 */
#ifndef UI_PAUSE_H
#define UI_PAUSE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Draw the pause overlay (does not clear the screen). */
void pause_draw(void);

/** Handle input while paused. Returns the action chosen. */
typedef enum {
    PAUSE_ACTION_NONE = 0,
    PAUSE_ACTION_RESUME,
    PAUSE_ACTION_RESTART,
    PAUSE_ACTION_MENU,
} pause_action_t;

pause_action_t pause_handle_input(void);

/** Game-over screen with final score and options. */
void gameover_show(uint32_t score, bool extreme);

/** Handle input on game-over screen. Returns the action chosen. */
typedef enum {
    GO_ACTION_NONE = 0,
    GO_ACTION_RESTART,
    GO_ACTION_MENU,
} go_action_t;

go_action_t gameover_handle_input(void);

#ifdef __cplusplus
}
#endif

#endif