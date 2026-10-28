/**
 * screens.h — Shared screen state enum for all UI modules.
 */
#ifndef SCREENS_H
#define SCREENS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCREEN_TITLE = 0,
    SCREEN_MENU,
    SCREEN_INFO,              /* credits */
    SCREEN_SETTINGS,
    SCREEN_HIGHSCORES_MENU,   /* Classic / Extreme list */
    SCREEN_HIGHSCORES,        /* the top-3 view */
    SCREEN_GAME,
} screen_t;

extern screen_t s_screen;

#ifdef __cplusplus
}
#endif

#endif