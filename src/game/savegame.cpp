/**
 * savegame.cpp — Saved game in EEPROM (see savegame.h).
 *
 * Layout at SAVE_OFF:  +0 marker (written last, cleared first)  +1 sizeof(game_t) (u16)
 *   +3 payload length (u16)  +5 CRC16 of the payload  +7 payload:
 *   [game_t before `a`] [a[0..len)] [game_t from `qx` up to `occ`]
 */
#include "game/savegame.h"
#include "drivers/eeprom_store.h"
#include <avr/eeprom.h>
#include <stddef.h>
#include <string.h>

#define SAVE_OFF    64
#define SAVE_MARK   0xA5
#define HDR_SIZE    7
#define PAY_OFF     (SAVE_OFF + HDR_SIZE)

#define PART_A      ((uint16_t)offsetof(game_t, a))                       /* config .. len */
#define PART_C_OFF  ((uint16_t)offsetof(game_t, qx))
#define PART_C      ((uint16_t)(offsetof(game_t, occ) - offsetof(game_t, qx)))

static uint16_t read16(uint16_t off) {
    uint8_t b[2];
    eeprom_read_block(b, (const void *)(uintptr_t)off, 2);
    return (uint16_t)(b[0] | (b[1] << 8));
}

static void write16(uint16_t off, uint16_t v) {
    uint8_t b[2] = { (uint8_t)v, (uint8_t)(v >> 8) };
    eeprom_update_block(b, (void *)(uintptr_t)off, 2);
}

static uint16_t crc_region(uint16_t off, uint16_t n) {
    uint8_t buf[32];
    uint16_t crc = 0xFFFF;
    while (n) {
        uint8_t k = n < sizeof buf ? (uint8_t)n : (uint8_t)sizeof buf;
        eeprom_read_block(buf, (const void *)(uintptr_t)off, k);
        crc = eeprom_crc16_update(crc, buf, k);
        off = (uint16_t)(off + k);
        n = (uint16_t)(n - k);
    }
    return crc;
}

/* Marker, size, length bounds and CRC; returns the payload length via *plen. */
static bool header_ok(uint16_t *plen) {
    if (eeprom_read_byte((const uint8_t *)SAVE_OFF) != SAVE_MARK) return false;
    if (read16(SAVE_OFF + 1) != sizeof(game_t)) return false;
    uint16_t n = read16(SAVE_OFF + 3);
    if (n < PART_A + PART_C || n > PART_A + GAME_MAX_LEN + PART_C) return false;
    if (crc_region(PAY_OFF, n) != read16(SAVE_OFF + 5)) return false;
    *plen = n;
    return true;
}

bool savegame_exists(void) {
    uint16_t n;
    return header_ok(&n);
}

/* Write `n` bytes at the payload cursor, folding them into the CRC. */
static void put(uint16_t *off, uint16_t *crc, const uint8_t *p, uint16_t n) {
    eeprom_update_block(p, (void *)(uintptr_t)*off, n);
    *crc = eeprom_crc16_update(*crc, p, n);
    *off = (uint16_t)(*off + n);
}

void savegame_save(const game_t *g) {
    uint16_t len = g->len > GAME_MAX_LEN ? GAME_MAX_LEN : g->len;
    const uint8_t *p = (const uint8_t *)g;

    eeprom_update_byte((uint8_t *)SAVE_OFF, 0);          /* invalid while being rewritten */
    uint16_t off = PAY_OFF, crc = 0xFFFF;
    put(&off, &crc, p, PART_A);
    put(&off, &crc, g->a, len);
    put(&off, &crc, p + PART_C_OFF, PART_C);
    write16(SAVE_OFF + 1, (uint16_t)sizeof(game_t));
    write16(SAVE_OFF + 3, (uint16_t)(off - PAY_OFF));
    write16(SAVE_OFF + 5, crc);
    eeprom_update_byte((uint8_t *)SAVE_OFF, SAVE_MARK);
}

bool savegame_load(game_t *g) {
    uint16_t plen;
    if (!header_ok(&plen)) return false;

    memset(g, 0, sizeof *g);
    uint8_t *p = (uint8_t *)g;
    uint16_t off = PAY_OFF;
    eeprom_read_block(p, (const void *)(uintptr_t)off, PART_A);
    off = (uint16_t)(off + PART_A);

    if (g->len < 1 || g->len > GAME_MAX_LEN || plen != PART_A + g->len + PART_C) return false;
    eeprom_read_block(g->a, (const void *)(uintptr_t)off, g->len);
    off = (uint16_t)(off + g->len);
    eeprom_read_block(p + PART_C_OFF, (const void *)(uintptr_t)off, PART_C);

    if (g->speed > SPEED_INDEX_MAX || g->maze < -1 || g->maze > 4) return false;
    g->map = NULL;
    g->map_buf = NULL;
    g->maze_loader = NULL;
    g->sfx_mask = 0;
    g->moved = g->tail_removed = g->food_eaten = g->bonus_eaten = false;
    return true;
}

void savegame_clear(void) {
    eeprom_update_byte((uint8_t *)SAVE_OFF, 0);
}
