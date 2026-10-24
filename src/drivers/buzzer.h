/**
 * buzzer.h — Buzzer Driver
 *
 * Active buzzer mode: GPIO on/off with minimum pulse.
 * Passive piezo mode: Timer0 CTC square wave with note table.
 * Non-blocking: start a tone, service it in the main loop.
 *
 * BACKLOG.MD §1.3, §5 D4.
 */
#ifndef DRIVERS_BUZZER_H
#define DRIVERS_BUZZER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Sound IDs matching the original playSFX(0..12). BACKLOG.MD §1.3. */
#define SFX_SPEED_UP       0
#define SFX_SLOW_DOWN      1
#define SFX_GHOST          2
#define SFX_WALL_STAMP     3
#define SFX_SHRINK         4
#define SFX_GROW           5
#define SFX_MOVING_FOOD    6
#define SFX_BONUS_BOOST    7
#define SFX_COIN_RUSH      8
#define SFX_LIGHTS_OUT     9
#define SFX_DEATH         10
#define SFX_HIGH_SCORE    11
#define SFX_BEEP          12

/** Initialize buzzer. Configures Timer0 for passive mode if enabled. */
void buzzer_init(void);

/**
 * Play a sound effect (non-blocking).
 * Starts the tone and returns immediately.
 * Call buzzer_update() each frame to advance it.
 */
void buzzer_play(uint8_t sound_id);

/**
 * Update the buzzer state. Call once per frame.
 * Handles tone duration, note transitions, and auto-off.
 */
void buzzer_update(void);

/** Immediately stop all sound. */
void buzzer_stop(void);

/** Query whether a tone sequence is currently playing. */
bool buzzer_is_playing(void);

/** Enable/disable sound output. */
void buzzer_mute(bool mute);
bool buzzer_is_muted(void);

#ifdef __cplusplus
}
#endif

#endif