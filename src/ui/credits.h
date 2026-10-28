/**
 * credits.h — Credits screen shown by the main menu's "Instructions" item.
 */
#ifndef UI_CREDITS_H
#define UI_CREDITS_H

#ifdef __cplusplus
extern "C" {
#endif

/** Repaint the complete 128×160 screen: menu wallpaper, status bar, title, author and
 *  repository link centred, "Back" soft key. */
void credits_draw(void);

#ifdef __cplusplus
}
#endif

#endif
