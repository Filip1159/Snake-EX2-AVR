/**
 * rng.cpp — xorshift32 random source for the AVR build.
 *
 * The game core only needs a cheap uniform 32-bit value (the original used
 * java.util.Random). Host tests substitute the exact Java LCG so oracle traces
 * compare bit-for-bit; on hardware play no trace parity is required.
 */
#include "game/rng.h"

static uint32_t s_state = 0x9E3779B9UL;

void game_rand_seed(uint32_t seed) {
    s_state = seed ? seed : 0x9E3779B9UL;   /* xorshift must not start at 0 */
}

int32_t game_rand_int(void) {
    uint32_t x = s_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_state = x;
    return (int32_t)x;
}
