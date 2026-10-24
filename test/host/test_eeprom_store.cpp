#include "drivers/eeprom_store.h"
#include <stdio.h>
#include <string.h>
uint8_t mock_eeprom[1024];
unsigned mock_eeprom_writes;
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main() {
    /* CRC-16/CCITT-FALSE known answer */
    CHECK(eeprom_crc16((const uint8_t *)"123456789", 9) == 0x29B1);

    /* blank EEPROM (0xFF) -> defaults */
    memset(mock_eeprom, 0xFF, sizeof mock_eeprom);
    CHECK(!eeprom_store_init());
    CHECK(eeprom_get_settings()->speed == 2 && eeprom_get_settings()->maze == -1);
    CHECK(eeprom_get_score(0) == 0 && eeprom_get_score(3) == 0);
    CHECK(memcmp(mock_eeprom, "S2EX", 4) == 0 && mock_eeprom[4] == 1);

    /* second boot: intact, no writes */
    mock_eeprom_writes = 0;
    CHECK(eeprom_store_init());
    CHECK(mock_eeprom_writes == 0);

    /* settings + scores round trip */
    settings_t s = {MODE_EXTREME, 3, 5, false, true};
    eeprom_save_settings(&s);
    eeprom_set_score(0, 123456);
    eeprom_set_score(3, 0xDEADBEEF);
    eeprom_set_score(0, 100);               /* lower: ignored */
    CHECK(eeprom_get_score(0) == 123456);
    CHECK(eeprom_store_init());
    CHECK(eeprom_get_settings()->mode == MODE_EXTREME && eeprom_get_settings()->maze == 3 &&
          eeprom_get_settings()->speed == 5 && !eeprom_get_settings()->sound_on);
    CHECK(eeprom_get_score(0) == 123456 && eeprom_get_score(3) == 0xDEADBEEF);

    /* saving unchanged settings writes nothing */
    mock_eeprom_writes = 0;
    eeprom_save_settings(&s);
    CHECK(mock_eeprom_writes == 0);

    /* flip every single bit of the used area: must never hang, must detect */
    uint8_t good[1024];
    memcpy(good, mock_eeprom, sizeof good);
    int undetected = 0;
    for (int byte = 0; byte < 58; byte++) {
        if ((byte >= 5 && byte < 8) || byte == 13 || (byte >= 16 && byte < 32) || (byte >= 42 && byte < 48)) continue;  /* unused */
        for (int bit = 0; bit < 8; bit++) {
            memcpy(mock_eeprom, good, sizeof good);
            mock_eeprom[byte] ^= 1 << bit;
            if (eeprom_store_init()) undetected++;
            /* after a fallback the store must be self-consistent */
            CHECK(eeprom_store_init());
        }
    }
    CHECK(undetected == 0);

    /* corrupt only classic scores: settings + extreme survive */
    memcpy(mock_eeprom, good, sizeof good);
    mock_eeprom[33] ^= 0x10;
    CHECK(!eeprom_store_init());
    CHECK(eeprom_get_score(0) == 0 && eeprom_get_score(3) == 0xDEADBEEF);
    CHECK(eeprom_get_settings()->maze == 3);

    /* valid CRC but out-of-range value -> defaults */
    memcpy(mock_eeprom, good, sizeof good);
    mock_eeprom[10] = 9;                              /* speed 9 */
    uint16_t crc = eeprom_crc16(mock_eeprom + 8, 5);
    mock_eeprom[14] = (uint8_t)crc; mock_eeprom[15] = crc >> 8;
    CHECK(!eeprom_store_init());
    CHECK(eeprom_get_settings()->speed == 2);

    printf(fails ? "eeprom_store: %d FAILED\n" : "eeprom_store: all passed\n", fails);
    return fails != 0;
}
