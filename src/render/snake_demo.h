/** snake_demo.h — static snake exercising every turn/direction sprite. */
#ifndef RENDER_SNAKE_DEMO_H
#define RENDER_SNAKE_DEMO_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Draw a 67-segment snake on the current playfield. open = open mouth + food ahead; fat = every 7th segment holds food. */
void snake_demo_draw(bool open, bool fat);

#ifdef __cplusplus
}
#endif

#endif
