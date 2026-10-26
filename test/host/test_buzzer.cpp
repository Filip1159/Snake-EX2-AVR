/* Host test: decoded sound table and non-blocking buzzer driver. */
#include "drivers/buzzer.h"
#include "assets/sounds_gen.h"
#include "config.h"
#include "Arduino.h"
#include <avr/pgmspace.h>
#include <stdio.h>

uint32_t mock_millis;
uint8_t  mock_buzzer_pin;
uint8_t  mock_buttons;

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static uint16_t seq_offset(uint8_t id) { return pgm_read_word(&SOUND_SEQ_OFFSET[id]); }
static uint8_t  seq_len(uint8_t id)    { return pgm_read_byte(&SOUND_SEQ_LEN[id]); }
static uint16_t note_freq(uint16_t i)  { return pgm_read_word(&SOUND_NOTE_FREQ[i]); }
static uint8_t  note_ms(uint16_t i)    { return pgm_read_byte(&SOUND_NOTE_MS[i]); }

/* Play one sequence to completion, checking every note and the output pin. */
static void play_and_exhaust(uint8_t id) {
    uint16_t off = seq_offset(id);
    uint8_t  len = seq_len(id);

    buzzer_play(id);
    CHECK(buzzer_is_playing());
    CHECK(buzzer_note_frequency() == note_freq(off));

    for (uint8_t i = 0; i < len; i++) {
        uint16_t f = note_freq(off + i);
        CHECK(buzzer_is_playing());
        CHECK(buzzer_note_frequency() == f);
        CHECK(mock_buzzer_pin == (f != 0));
        mock_millis += note_ms(off + i);
        buzzer_update();
    }

    CHECK(!buzzer_is_playing());
    CHECK(mock_buzzer_pin == 0);
}

int main() {
    /* ── Decoded table ───────────────────────────────────────────────────── */
    CHECK(SOUND_BANK_COUNT == SFX_COUNT);
    CHECK(SOUND_NOTE_COUNT > 0);
    CHECK(SOUND_BEEP_INDEX == SFX_BEEP_INDEX);

    uint16_t expected_off = 0;
    for (uint8_t id = 0; id < SOUND_BANK_COUNT; id++) {
        uint8_t len = seq_len(id);
        CHECK(len > 0);
        CHECK(seq_offset(id) == expected_off);   /* sequences are contiguous */
        CHECK((uint32_t)seq_offset(id) + len <= SOUND_NOTE_COUNT);
        expected_off = (uint16_t)(expected_off + len);
    }
    CHECK(expected_off == SOUND_NOTE_COUNT);

    for (uint16_t i = 0; i < SOUND_NOTE_COUNT; i++) {
        uint16_t f = note_freq(i);
        CHECK(f == 0 || (f >= 100 && f <= 4200));   /* audible range */
        CHECK(note_ms(i) > 0 && note_ms(i) <= 250);
    }

    /* Index 12 is the synthetic beep from ToneDevice.initialise(). */
    {
        uint16_t i = seq_offset(SOUND_BEEP_INDEX);
        CHECK(seq_len(SOUND_BEEP_INDEX) == 1);
        CHECK(note_freq(i) == SFX_BEEP_FREQ_HZ);
        CHECK(note_ms(i) == SFX_BEEP_DURATION_MS);
    }

    /* ── Timer2 compare math (F_CPU/64 = 125 kHz tick) ───────────────────── */
    CHECK(buzzer_ocr_for_hz(0) == 0);
    CHECK(buzzer_ocr_for_hz(440) == 141);
    CHECK(buzzer_ocr_for_hz(1760) == 35);
    CHECK(buzzer_ocr_for_hz(2000) == 30);
    CHECK(buzzer_ocr_for_hz(100) == 255);    /* clamped to the 8-bit range */
    CHECK(buzzer_ocr_for_hz(62500) == 0);    /* above Timer2's reach */

    /* ── Active mode (compile-time default) ──────────────────────────────── */
    buzzer_init();
    CHECK(!buzzer_is_playing());
    CHECK(!buzzer_is_muted());
    CHECK(mock_buzzer_pin == 0);
#if FEATURE_PASSIVE_BUZZER
    CHECK(buzzer_get_mode() == BUZZER_MODE_PASSIVE);
#else
    CHECK(buzzer_get_mode() == BUZZER_MODE_ACTIVE);
#endif
    buzzer_set_mode(BUZZER_MODE_ACTIVE);

    /* ── Restart and invalid IDs ─────────────────────────────────────────── */
    buzzer_play(SFX_SPEED_UP);
    CHECK(buzzer_is_playing());
    CHECK(buzzer_note_frequency() == note_freq(seq_offset(SFX_SPEED_UP)));
    mock_millis += note_ms(seq_offset(SFX_SPEED_UP));
    buzzer_update();
    CHECK(buzzer_note_frequency() == note_freq(seq_offset(SFX_SPEED_UP) + 1));

    /* An invalid ID must not disturb the channel already playing. */
    uint16_t before = buzzer_note_frequency();
    buzzer_play(SOUND_BANK_COUNT);
    CHECK(buzzer_is_playing());
    CHECK(buzzer_note_frequency() == before);

    /* A new valid call restarts the monophonic channel (Tier C). */
    buzzer_play(SFX_GHOST);
    CHECK(buzzer_note_frequency() == note_freq(seq_offset(SFX_GHOST)));

    /* ── Mute gates playback ─────────────────────────────────────────────── */
    buzzer_mute(true);
    CHECK(buzzer_is_muted());
    CHECK(!buzzer_is_playing());
    CHECK(mock_buzzer_pin == 0);
    buzzer_play(SFX_SPEED_UP);
    CHECK(!buzzer_is_playing());
    buzzer_mute(false);
    CHECK(!buzzer_is_muted());
    buzzer_play(SFX_SPEED_UP);
    CHECK(buzzer_is_playing());
    buzzer_stop();
    CHECK(!buzzer_is_playing());
    CHECK(mock_buzzer_pin == 0);

    /* ── Passive mode / beep ─────────────────────────────────────────────── */
    buzzer_set_mode(BUZZER_MODE_PASSIVE);
    CHECK(buzzer_get_mode() == BUZZER_MODE_PASSIVE);
    CHECK(!buzzer_is_playing());             /* mode change stops the channel */
    buzzer_play(SFX_BEEP);
    CHECK(buzzer_is_playing());
    CHECK(buzzer_note_frequency() == SFX_BEEP_FREQ_HZ);
    mock_millis += SFX_BEEP_DURATION_MS;
    buzzer_update();
    CHECK(!buzzer_is_playing());
    buzzer_set_mode(BUZZER_MODE_ACTIVE);
    CHECK(buzzer_get_mode() == BUZZER_MODE_ACTIVE);

    /* ── millis() wraparound ─────────────────────────────────────────────── */
    mock_millis = 0xFFFFFFF0u;
    buzzer_play(SFX_BEEP);
    mock_millis += 50;
    buzzer_update();
    CHECK(buzzer_is_playing());
    mock_millis += 50;
    buzzer_update();
    CHECK(!buzzer_is_playing());

    /* ── Every playSFX(0..12) sequence ───────────────────────────────────── */
    for (uint8_t id = 0; id < SOUND_BANK_COUNT; id++) play_and_exhaust(id);

    printf(fails ? "buzzer: %d FAILED\n" : "buzzer: all passed\n", fails);
    return fails != 0;
}
