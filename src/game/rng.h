/** rng.h — random source for the game core. Platform provides game_rand_int(). */
#ifndef GAME_RNG_H
#define GAME_RNG_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/** Reseed the generator (0 is replaced by a fixed non-zero state). */
void game_rand_seed(uint32_t seed);

/** Equivalent of java.util.Random.nextInt(): uniformly distributed signed 32-bit value. */
int32_t game_rand_int(void);
#ifdef __cplusplus
}
#endif
#endif
