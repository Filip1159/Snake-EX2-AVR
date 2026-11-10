/* Generated from the original game's sounds.nok.
 * Do not edit.  Nokia RTPL decoded per Smart Messaging §3.8; index 12 is
 * the synthetic 1760 Hz / 100 ms beep from ToneDevice.initialise().
 *
 * SOUND_NOTE_FREQ/MS are a flat note pool; SOUND_SEQ_OFFSET/LEN index it.
 * A frequency of 0 is a pause.  Durations are in milliseconds.
 */
#ifndef ASSETS_SOUNDS_GEN_H
#define ASSETS_SOUNDS_GEN_H

#include <stdint.h>

#define SOUND_BANK_COUNT 13
#define SOUND_NOTE_COUNT 150
#define SOUND_BEEP_INDEX 12

extern const uint16_t SOUND_NOTE_FREQ[SOUND_NOTE_COUNT];
extern const uint8_t  SOUND_NOTE_MS[SOUND_NOTE_COUNT];
extern const uint16_t SOUND_SEQ_OFFSET[SOUND_BANK_COUNT];
extern const uint8_t  SOUND_SEQ_LEN[SOUND_BANK_COUNT];

#endif /* ASSETS_SOUNDS_GEN_H */
