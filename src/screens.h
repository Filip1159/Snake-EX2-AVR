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
    SCREEN_INFO,
    SCREEN_SETTINGS,
    SCREEN_HIGHSCORES,
    SCREEN_INSTRUCTIONS,
    SCREEN_GAME,
} screen_t;

extern screen_t s_screen;

#ifdef __cplusplus
}
#endif

#endif