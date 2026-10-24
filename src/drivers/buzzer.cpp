/**
 * buzzer.cpp — Buzzer Driver
 *
 * Active buzzer on PB0 via NPN low-side switch.
 * Non-blocking: starts a tone, then each buzzer_update() call
 * decrements the duration counter.
 *
 * Passive piezo support (Timer0 CTC) is stubbed — enable via
 * FEATURE_PASSIVE_BUZZER in config.h.
 *
 * BACKLOG.MD §5 D4.
 */
#include "drivers/buzzer.h"
#include "hal/gpio.h"
#include "config.h"
#include <Arduino.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

static bool s_muted = false;

/* Currently playing tone */
static struct {
    bool    active;
    uint8_t sound_id;
    uint8_t step;
    uint16_t duration;    // ms remaining
    uint16_t note_duration;
    uint16_t frequency;   // Hz (unused in active mode)
} s_tone;

/* ── Tone sequence table (simplified mapping) ────────────────────────────
 *
 * Each entry: {duration_ms, frequency_hz}
 * The active buzzer only uses duration; frequency is for passive mode.
 * Sequences are placeholders until sounds.nok is decoded (A1).
 * BACKLOG.MD §1.3.
 */
typedef struct {
    uint8_t duration_ms;
    uint16_t frequency_hz;
} tone_step_t;

typedef struct {
    uint8_t       step_count;
    tone_step_t   steps[4];  // max 4 notes per SFX
} tone_seq_t;

static const tone_seq_t PROGMEM s_sfx_table[] = {
    //  0: eat/collect
    {1, {{50,  880}}},
    //  1: speed-up
    {2, {{30, 1200}, {30, 1600}}},
    //  2: ghost
    {1, {{30,  660}}},
    //  3: wall-stamp
    {1, {{60,  440}}},
    //  4: shrink
    {1, {{30,  550}}},
    //  5: grow
    {1, {{30, 1100}}},
    //  6: moving-food
    {1, {{30,  990}}},
    //  7: bonus-boost
    {2, {{40, 1400}, {40, 1760}}},
    //  8: coin-rush
    {1, {{20, 1760}}},
    //  9: lights-out
    {1, {{40,  330}}},
    // 10: coin collected
    {1, {{10, 2000}}},
    // 11: death
    {3, {{80, 440}, {80, 330}, {80, 220}}},
    // 12: beep (synthetic)
    {1, {{100, 1760}}},
};

void buzzer_init(void) {
    s_tone.active = false;
    s_tone.duration = 0;
    s_tone.step = 0;
    s_muted = false;

#if FEATURE_PASSIVE_BUZZER
    // Timer0 CTC mode for square wave generation
    // Will be configured on first play
    TCCR0 = 0;  // stop timer
#endif
}

void buzzer_play(uint8_t sound_id) {
    if (sound_id >= 13 || s_muted) return;

    s_tone.active = true;
    s_tone.sound_id = sound_id;
    s_tone.step = 0;
    s_tone.duration = 0;

    tone_seq_t seq;
    memcpy_P(&seq, &s_sfx_table[sound_id], sizeof(tone_seq_t));

    if (seq.step_count > 0) {
        s_tone.note_duration = seq.steps[0].duration_ms;
        s_tone.duration = seq.steps[0].duration_ms;
        s_tone.frequency = seq.steps[0].frequency_hz;
    }

#if FEATURE_PASSIVE_BUZZER
    // Configure Timer0 for CTC square wave
    if (s_tone.frequency > 0) {
        // OCR0 = F_CPU / (2 * prescaler * freq) - 1
        // Use prescaler 8: 16M / 8 = 2M
        uint16_t ocr = (F_CPU / 8 / s_tone.frequency) - 1;
        if (ocr > 255) ocr = 255;
        OCR0 = (uint8_t)ocr;
        TCCR0 = (1 << WGM01) | (1 << CS01);  // CTC, prescaler 8
        TIMSK |= (1 << OCIE0);   // enable compare match interrupt
    }
#else
    // Active buzzer: just turn on
    buzzer_on();
#endif
}

void buzzer_update(void) {
    if (!s_tone.active || s_tone.duration == 0) return;

    // Decrement by frame period (assumes ~66ms per frame at 15fps)
    // For more precise timing, use millis() in a future timer HAL
    if (s_tone.duration > 16) {
        s_tone.duration -= 16;
    } else {
        s_tone.duration = 0;
    }

    if (s_tone.duration == 0) {
        // Advance to next note
        s_tone.step++;

        tone_seq_t seq;
        memcpy_P(&seq, &s_sfx_table[s_tone.sound_id], sizeof(tone_seq_t));

        if (s_tone.step < seq.step_count) {
            s_tone.note_duration = seq.steps[s_tone.step].duration_ms;
            s_tone.duration = seq.steps[s_tone.step].duration_ms;
            s_tone.frequency = seq.steps[s_tone.step].frequency_hz;

#if FEATURE_PASSIVE_BUZZER
            // Update frequency
            uint16_t ocr = (F_CPU / 8 / s_tone.frequency) - 1;
            if (ocr > 255) ocr = 255;
            OCR0 = (uint8_t)ocr;
#endif
        } else {
            // Sequence done
            buzzer_stop();
        }
    }
}

void buzzer_stop(void) {
    s_tone.active = false;
    s_tone.duration = 0;
    buzzer_off();

#if FEATURE_PASSIVE_BUZZER
    TCCR0 = 0;                  // stop timer
    TIMSK &= ~(1 << OCIE0);     // disable interrupt
#endif
}

bool buzzer_is_playing(void) {
    return s_tone.active;
}

void buzzer_mute(bool mute) {
    s_muted = mute;
    if (mute) buzzer_stop();
}

bool buzzer_is_muted(void) {
    return s_muted;
}

#if FEATURE_PASSIVE_BUZZER
/** Timer0 compare match: toggle buzzer pin for square wave. */
ISR(TIMER0_COMP_vect) {
    // Toggle PB0
    PINB |= (1 << PB0);  // writing 1 to PIN toggles the bit
}
#endif