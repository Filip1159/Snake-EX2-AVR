/**
 * instructions.h — Tutorial/Instructions screen (BACKLOG U5).
 * Port of the original Snake.J tutorial sequence with text pages and scripted demos.
 */
#ifndef UI_INSTRUCTIONS_H
#define UI_INSTRUCTIONS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Instruction page IDs (matching original Snake.J positive indices).
 * Negative IDs are demo games: -1..-7 are tutorial demos, -8 is high-score demo.
 */
typedef enum {
    INST_PAGE_TITLE      = 0,   // "Snake EX2"
    INST_PAGE_TONGUE     = 1,   // tongue hint
    INST_PAGE_NOKIA      = 6,   // Nokia games service hint
    INST_PAGE_GROW       = 11,  // make the snake grow
    INST_PAGE_CONTROLS   = 16,  // use keys to change direction
    INST_PAGE_AVOID      = 21,  // avoid walls and tail
    INST_PAGE_BONUS      = 26,  // eat bonus food before time expires
    INST_PAGE_EXTREME1   = 51,  // Extreme mode advance when scored enough
    INST_PAGE_EXTREME2   = 52,  // Extreme explanation continued
    INST_PAGE_EXTREME3   = 53,
    INST_PAGE_EXTREME4   = 54,
    INST_PAGE_EXTREME5   = 55,
    INST_PAGE_COUNT      = 13,  // number of text pages (indices above)
} inst_page_t;

/** Demo game IDs (negative Snake.J indices). */
typedef enum {
    DEMO_TONGUE        = -1,  // tongue demo
    DEMO_WALLS         = -2,  // walls demo
    DEMO_BONUS_FOOD    = -3,  // bonus food demo
    DEMO_POWERUP_SPEED = -4,  // speed power-up demo
    DEMO_POWERUP_GHOST = -5,  // ghost power-up demo
    DEMO_POWERUP_COIN  = -6,  // coin rush demo
    DEMO_POWERUP_LIGHTS= -7,  // lights-out demo
    DEMO_HIGHSCORE     = -8,  // high-score demo
} demo_id_t;

/** Initialize and show the instructions screen (starts at title page). */
void instructions_show(void);

/** Handle input while in instructions. Returns true to stay, false to exit to menu. */
bool instructions_handle_input(void);

/** Launch a scripted demo game by ID. Returns when demo ends. */
void instructions_launch_demo(int8_t demo_id);

#ifdef __cplusplus
}
#endif

#endif