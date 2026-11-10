/**
 * screen.h — The application's screens and the dispatcher that runs the current one.
 *
 * Every screen is a `screen_t`: how to show it, what to do with a button event and what to
 * do on every loop pass.  Screens change with screen_goto(); the dispatcher takes care of
 * the shared display chrome (a screen that does not draw on the Series 40 wallpaper
 * declares `wallpaper = false`, so the next wallpaper screen repaints everything).
 */
#ifndef APP_SCREEN_H
#define APP_SCREEN_H

#include <stdint.h>
#include <stdbool.h>
#include "drivers/buttons.h"

typedef struct {
    void (*enter)(void);                                  /* paint the screen */
    void (*on_button)(uint8_t btn, btn_event_t ev);       /* one queued button event */
    void (*tick)(uint32_t now, uint8_t raw);              /* every loop pass; may be NULL */
    bool wallpaper;                                       /* draws on the shared S40 chrome */
} screen_t;

/** Leave the current screen and show `next`. */
void screen_goto(const screen_t *next);

/** The screen being shown (NULL before the first screen_goto). */
const screen_t *screen_current(void);

/** Run the current screen's handlers (called from loop()). */
void screen_on_button(uint8_t btn, btn_event_t ev);
void screen_tick(uint32_t now, uint8_t raw);

#endif
