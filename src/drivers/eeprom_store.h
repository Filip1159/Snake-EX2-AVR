/**
 * eeprom_store.h — Settings and High-Score Persistence
 *
 * Stores settings + high scores in EEPROM with magic/version/CRC.
 * Falls back to defaults on corruption.
 *
 * BACKLOG.MD §3.3, §5 D5.
 */
#ifndef DRIVERS_EEPROM_STORE_H
#define DRIVERS_EEPROM_STORE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Settings struct ────────────────────────────────────────────────────── */

typedef enum {
    MODE_CLASSIC = 0,
    MODE_EXTREME = 1,
} game_mode_t;

typedef struct {
    game_mode_t mode;        // Classic or Extreme
    int8_t      maze;        // -1 (empty) .. 4
    uint8_t     speed;       // speedIndex 0..5
    bool        sound_on;    // sound enabled
    bool        backlight;   // backlight on/off
} settings_t;

/* ── High scores ────────────────────────────────────────────────────────── */

#define SCORES_PER_MODE   3   // top 3 per mode
#define HIGH_SCORE_COUNT  (2 * SCORES_PER_MODE)   // classic 0..2, extreme 3..5
#define CRC16_POLY        0x1021  // CCITT

/* ── API ────────────────────────────────────────────────────────────────── */

/**
 * Load settings and scores from EEPROM, validating magic/version/CRC16.
 * Anything invalid falls back to defaults (and is rewritten).
 * Returns true if everything loaded intact, false if any part was reset.
 */
bool eeprom_store_init(void);

/** Get a pointer to current settings. */
const settings_t *eeprom_get_settings(void);

/** Save settings to EEPROM (writes only if changed). */
void eeprom_save_settings(const settings_t *new_settings);

/** Reload high scores (CRC-checked per block). Returns false if any block was corrupt. */
bool eeprom_load_scores(void);

/** Save high scores to EEPROM. */
void eeprom_save_scores(void);

/** Get/set an individual score. index 0..5 (mode * SCORES_PER_MODE + rank). */
uint32_t eeprom_get_score(uint8_t index);
void eeprom_set_score(uint8_t index, uint32_t score);

/** Compute CRC16 over a buffer. Used for validation/testing. */
uint16_t eeprom_crc16(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif