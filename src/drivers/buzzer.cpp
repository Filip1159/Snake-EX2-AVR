/**
 * buzzer.cpp — Buzzer Driver
 *
 * Active buzzer on PD6 via an NPN low-side switch: the note sequence gates the
 * pin on/off.  Passive piezo on the same pin: Timer2 CTC toggles PD6 at the
 * decoded note frequency.
 *
 * The decoded Nokia RTPL table is in assets/sounds_gen.*.  Notes are timed
 * against millis(), so playback is non-blocking and does not drift with the
 * main-loop rate.  A new buzzer_play() call restarts the single channel.
 *
 * Timer choice: the Arduino core owns Timer0 for millis(), so the passive
 * output uses Timer2 (8-bit, prescaler 64).  Timer1 is reserved for the
 * 15 fps scheduler.
 */
#include "drivers/buzzer.h"
#include "assets/sounds_gen.h"
#include "hal/gpio.h"
#include "config.h"
#include <Arduino.h>
#include <avr/pgmspace.h>
#include <stdint.h>

#if defined(__AVR__)
#include <avr/io.h>
#include <avr/interrupt.h>
#endif

/* Timer2 CTC clock: F_CPU / 64.  Square-wave frequency = tick / (2*(OCR+1)). */
#define BUZZER_PRESCALER   64
#define BUZZER_TIMER_HZ    (F_CPU / BUZZER_PRESCALER)

static bool          s_muted;
static bool          s_active;
static buzzer_mode_t s_mode;
static uint8_t       s_sound;
static uint8_t       s_step;
static uint16_t      s_note_hz;
static uint32_t      s_next_ms;

/* ── Table helpers ──────────────────────────────────────────────────────── */

static uint16_t seq_offset(uint8_t id) {
    return pgm_read_word(&SOUND_SEQ_OFFSET[id]);
}

static uint8_t seq_len(uint8_t id) {
    return pgm_read_byte(&SOUND_SEQ_LEN[id]);
}

static uint16_t note_freq(uint16_t index) {
    return pgm_read_word(&SOUND_NOTE_FREQ[index]);
}

static uint8_t note_ms(uint16_t index) {
    return pgm_read_byte(&SOUND_NOTE_MS[index]);
}

/* ── Hardware ───────────────────────────────────────────────────────────── */

uint8_t buzzer_ocr_for_hz(uint16_t hz) {
    if (hz == 0) return 0;
    /* Round tick/(2*hz) to the nearest compare value.  Values above the
     * 8-bit Timer2 range are clamped; below the audible range is a pause. */
    uint32_t n = ((uint32_t)BUZZER_TIMER_HZ / 2u + hz / 2u) / hz;
    if (n < 2u) return 0;
    if (n > 256u) n = 256u;
    return (uint8_t)(n - 1u);
}

static void tone_start(uint16_t hz, uint8_t ms) {
    s_note_hz = hz;
    s_next_ms = millis() + ms;

#if defined(__AVR__)
    uint8_t ocr = (s_mode == BUZZER_MODE_PASSIVE) ? buzzer_ocr_for_hz(hz) : 0;
    if (ocr != 0) {
        /* Passive piezo: Timer2 CTC toggles PD6 in the ISR. */
        TCCR2A = (1 << WGM21);
        TCCR2B = (1 << CS22);           /* prescaler 64 */
        OCR2A  = ocr;
        TCNT2  = 0;
        TIMSK2 = (1 << OCIE2A);
        return;
    }
    /* Active buzzer, or a pause in passive mode: stop the timer and gate PD6. */
    TCCR2A = 0;
    TCCR2B = 0;
    TIMSK2 = 0;
#endif

    if (hz == 0) buzzer_off();
    else         buzzer_on();
}

/* ── Public API ─────────────────────────────────────────────────────────── */

void buzzer_init(void) {
    s_muted  = false;
    s_active = false;
    s_sound  = 0;
    s_step   = 0;
    s_note_hz = 0;
    s_next_ms = 0;
#if FEATURE_PASSIVE_BUZZER
    s_mode = BUZZER_MODE_PASSIVE;
#else
    s_mode = BUZZER_MODE_ACTIVE;
#endif
    buzzer_stop();
}

void buzzer_play(uint8_t sound_id) {
    if (sound_id >= SOUND_BANK_COUNT || s_muted) return;

    s_active = true;
    s_sound  = sound_id;
    s_step   = 0;

    uint16_t index = seq_offset(sound_id);
    if (seq_len(sound_id) == 0) {
        buzzer_stop();
        return;
    }
    tone_start(note_freq(index), note_ms(index));
}

void buzzer_update(void) {
    if (!s_active) return;
    /* Wrap-safe: advance only when the deadline has passed. */
    if ((int32_t)(millis() - s_next_ms) < 0) return;

    s_step++;
    if (s_step >= seq_len(s_sound)) {
        buzzer_stop();
        return;
    }

    uint16_t index = seq_offset(s_sound) + s_step;
    tone_start(note_freq(index), note_ms(index));
}

void buzzer_stop(void) {
    s_active  = false;
    s_step    = 0;
    s_note_hz = 0;
#if defined(__AVR__)
    TCCR2A = 0;
    TCCR2B = 0;
    TIMSK2 = 0;
#endif
    buzzer_off();
}

bool buzzer_is_playing(void) {
    return s_active;
}

void buzzer_mute(bool mute) {
    s_muted = mute;
    if (mute) buzzer_stop();
}

bool buzzer_is_muted(void) {
    return s_muted;
}

void buzzer_set_mode(buzzer_mode_t mode) {
    if (mode != s_mode) buzzer_stop();
    s_mode = mode;
}

buzzer_mode_t buzzer_get_mode(void) {
    return s_mode;
}

uint16_t buzzer_note_frequency(void) {
    return s_active ? s_note_hz : 0;
}

#if defined(__AVR__)
/** Timer2 compare match: toggle PD6 for the passive piezo square wave. */
ISR(TIMER2_COMPA_vect) {
    PORTD ^= (1 << PD6);
}
#endif
