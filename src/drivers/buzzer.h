/**
 * buzzer.h — Buzzer Driver
 *
 * Active buzzer mode: GPIO on/off with minimum pulse.
 * Passive piezo mode: Timer2 CTC square wave with the decoded Nokia tone table.
 * Non-blocking: start a sequence, service it from the main loop.
 *
 * Sound IDs match the original playSFX(0..12).
 * The sequences live in assets/sounds_gen.* (decoded from the original sounds.nok).
 */
#ifndef DRIVERS_BUZZER_H
#define DRIVERS_BUZZER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Sound IDs matching the original playSFX(0..12). */
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

/** Output hardware.  Active = fixed-pitch gated buzzer; passive = piezo
 *  driven by Timer2.  The default comes from FEATURE_PASSIVE_BUZZER. */
typedef enum {
    BUZZER_MODE_ACTIVE = 0,
    BUZZER_MODE_PASSIVE = 1
} buzzer_mode_t;

/** Initialize buzzer.  Stops any tone and selects the compile-time default mode. */
void buzzer_init(void);

/**
 * Play a sound effect (non-blocking).
 * Starts the first note and returns immediately.  A new call restarts the
 * monophonic channel (the original plays several Sound objects at once; the
 * AVR port deliberately has one channel — Tier C).
 * Call buzzer_update() each frame/loop to advance it.
 */
void buzzer_play(uint8_t sound_id);

/**
 * Update the buzzer state.  Call once per main-loop iteration.
 * Advances to the next note when the current note's duration has elapsed.
 */
void buzzer_update(void);

/** Immediately stop all sound. */
void buzzer_stop(void);

/** Query whether a tone sequence is currently playing. */
bool buzzer_is_playing(void);

/** Enable/disable sound output. */
void buzzer_mute(bool mute);
bool buzzer_is_muted(void);

/** Select active or passive output hardware at runtime. */
void buzzer_set_mode(buzzer_mode_t mode);
buzzer_mode_t buzzer_get_mode(void);

/** Current note frequency in Hz (0 = pause or stopped).  For tests/diagnostics. */
uint16_t buzzer_note_frequency(void);

/**
 * Timer2 compare value for a square wave of hz (0 = pause / out of range).
 * Uses the fixed prescaler and F_CPU; exposed for host tests.
 */
uint8_t buzzer_ocr_for_hz(uint16_t hz);

#ifdef __cplusplus
}
#endif

#endif
