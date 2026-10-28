/**
 * eeprom_store.cpp — Settings and High-Score Persistence
 *
 * Fixed layout (BACKLOG.MD §3.3), all multi-byte values little-endian:
 *   0    magic "S2EX"
 *   4    version
 *   8    settings, 5 bytes: mode, maze(int8), speed, sound, backlight
 *   14   settings CRC16 (over bytes 8..12)
 *   32   classic high scores (3 x uint32)
 *   44   classic CRC16 (over 32..43)
 *   48   extreme high scores (3 x uint32)
 *   60   extreme CRC16 (over 48..59)
 *   64   resume slot (unused; also self-test scratch)
 *
 * Each block has its own CRC so a write only touches one block.
 * BACKLOG.MD §5 D5.
 */
#include "drivers/eeprom_store.h"
#include "config.h"
#include <avr/eeprom.h>
#include <string.h>
#include <stdint.h>

#define EEP_MAGIC_OFF      0
#define EEP_VERSION_OFF    4
#define EEP_VERSION        1
#define EEP_SETTINGS_OFF   8
#define EEP_SETTINGS_SIZE  5
#define EEP_SETTINGS_CRC   14
#define EEP_CLASSIC_OFF    32
#define EEP_CLASSIC_CRC    44
#define EEP_EXTREME_OFF    48
#define EEP_EXTREME_CRC    60
#define EEP_SCORE_BLOCK    (4 * SCORES_PER_MODE)

static const uint8_t MAGIC[4] = {'S', '2', 'E', 'X'};

static settings_t s_settings;
static uint32_t   s_scores[HIGH_SCORE_COUNT];

static const settings_t DEFAULT_SETTINGS = {
    MODE_CLASSIC,
    -1,       // no maze
    2,        // speedIndex 2 (medium)
    true,     // sound on
    true,     // backlight on
};

uint16_t eeprom_crc16(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ CRC16_POLY)
                                 : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static void write_block(uint16_t off, uint16_t crc_off, const uint8_t *buf, uint8_t len) {
    uint16_t crc = eeprom_crc16(buf, len);
    uint8_t c[2] = {(uint8_t)crc, (uint8_t)(crc >> 8)};
    eeprom_update_block(buf, (void *)(uintptr_t)off, len);
    eeprom_update_block(c, (void *)(uintptr_t)crc_off, 2);
}

static bool read_block(uint16_t off, uint16_t crc_off, uint8_t *buf, uint8_t len) {
    uint8_t c[2];
    eeprom_read_block(buf, (const void *)(uintptr_t)off, len);
    eeprom_read_block(c, (const void *)(uintptr_t)crc_off, 2);
    return eeprom_crc16(buf, len) == (uint16_t)(c[0] | (c[1] << 8));
}

static void settings_to_bytes(const settings_t *s, uint8_t *b) {
    b[0] = (uint8_t)s->mode;
    b[1] = (uint8_t)s->maze;
    b[2] = s->speed;
    b[3] = s->sound_on ? 1 : 0;
    b[4] = s->backlight ? 1 : 0;
}

static bool settings_from_bytes(const uint8_t *b, settings_t *s) {
    int8_t maze = (int8_t)b[1];
    if (b[0] > 1 || maze < -1 || maze > 4 ||
        b[2] > SPEED_INDEX_MAX || b[3] > 1 || b[4] > 1) {
        return false;
    }
    s->mode = (game_mode_t)b[0];
    s->maze = maze;
    s->speed = b[2];
    s->sound_on = b[3];
    s->backlight = b[4];
    return true;
}

static void store_settings(void) {
    uint8_t b[EEP_SETTINGS_SIZE];
    settings_to_bytes(&s_settings, b);
    write_block(EEP_SETTINGS_OFF, EEP_SETTINGS_CRC, b, EEP_SETTINGS_SIZE);
}

static void scores_to_bytes(uint8_t blk, uint8_t *b) {
    for (uint8_t i = 0; i < SCORES_PER_MODE; i++) {
        uint32_t v = s_scores[blk * SCORES_PER_MODE + i];
        for (uint8_t k = 0; k < 4; k++) b[i * 4 + k] = (uint8_t)(v >> (8 * k));
    }
}

static void store_scores_block(uint8_t blk) {
    uint8_t b[EEP_SCORE_BLOCK];
    scores_to_bytes(blk, b);
    write_block(blk ? EEP_EXTREME_OFF : EEP_CLASSIC_OFF,
                blk ? EEP_EXTREME_CRC : EEP_CLASSIC_CRC, b, EEP_SCORE_BLOCK);
}

bool eeprom_load_scores(void) {
    bool ok = true;
    for (uint8_t blk = 0; blk < 2; blk++) {
        uint8_t b[EEP_SCORE_BLOCK];
        bool good = read_block(blk ? EEP_EXTREME_OFF : EEP_CLASSIC_OFF,
                               blk ? EEP_EXTREME_CRC : EEP_CLASSIC_CRC, b, EEP_SCORE_BLOCK);
        for (uint8_t i = 0; i < SCORES_PER_MODE; i++) {
            uint32_t v = 0;
            if (good) {
                for (uint8_t k = 0; k < 4; k++) v |= (uint32_t)b[i * 4 + k] << (8 * k);
            }
            s_scores[blk * SCORES_PER_MODE + i] = v;
        }
        if (!good) {
            ok = false;
            store_scores_block(blk);   // rewrite zeroed block with valid CRC
        }
    }
    return ok;
}

bool eeprom_store_init(void) {
    bool ok = true;

    uint8_t head[5];
    eeprom_read_block(head, (const void *)EEP_MAGIC_OFF, 4);
    head[4] = eeprom_read_byte((const uint8_t *)EEP_VERSION_OFF);
    bool header_ok = memcmp(head, MAGIC, 4) == 0 && head[4] == EEP_VERSION;

    if (!header_ok) {
        eeprom_update_block(MAGIC, (void *)EEP_MAGIC_OFF, 4);
        eeprom_update_byte((uint8_t *)EEP_VERSION_OFF, EEP_VERSION);
        ok = false;
    }

    uint8_t b[EEP_SETTINGS_SIZE];
    if (header_ok && read_block(EEP_SETTINGS_OFF, EEP_SETTINGS_CRC, b, EEP_SETTINGS_SIZE) &&
        settings_from_bytes(b, &s_settings)) {
        // loaded
    } else {
        s_settings = DEFAULT_SETTINGS;
        store_settings();
        ok = false;
    }

    if (header_ok) {
        if (!eeprom_load_scores()) ok = false;
    } else {
        memset(s_scores, 0, sizeof(s_scores));
        store_scores_block(0);
        store_scores_block(1);
    }
    return ok;
}

const settings_t *eeprom_get_settings(void) {
    return &s_settings;
}

void eeprom_save_settings(const settings_t *new_settings) {
    if (memcmp(&s_settings, new_settings, sizeof(settings_t)) != 0) {
        s_settings = *new_settings;
        store_settings();
    }
}

void eeprom_save_scores(void) {
    store_scores_block(0);
    store_scores_block(1);
}

uint32_t eeprom_get_score(uint8_t index) {
    if (index >= HIGH_SCORE_COUNT) return 0;
    return s_scores[index];
}

void eeprom_set_score(uint8_t index, uint32_t score) {
    if (index >= HIGH_SCORE_COUNT) return;
    if (score > s_scores[index]) {
        s_scores[index] = score;
        store_scores_block(index / SCORES_PER_MODE);
    }
}
