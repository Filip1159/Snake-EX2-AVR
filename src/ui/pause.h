/**
 * pause.h — Pause overlay (BACKLOG U4).
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

#ifdef __cplusplus
}
#endif

#endif