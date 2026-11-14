#include "rng.h"

#include <stddef.h>

#define RMC_PCG32_MULT UINT64_C(6364136223846793005)

uint32_t rmc_pcg32_next(RmcPcg32 *rng)
{
    if (rng == NULL) {
        return 0;
    }

    const uint64_t oldstate = rng->state;

    rng->state =
        oldstate * RMC_PCG32_MULT + rng->increment;

    const uint32_t xorshifted =
        (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);

    const uint32_t rotation =
        (uint32_t)(oldstate >> 59u);

    return (xorshifted >> rotation) |
           (xorshifted << ((0u - rotation) & 31u));
}

void rmc_pcg32_seed(
    RmcPcg32 *rng,
    uint64_t initstate,
    uint64_t initseq)
{
    if (rng == NULL) {
        return;
    }

    rng->state = UINT64_C(0);
    rng->increment = (initseq << 1u) | UINT64_C(1);

    (void)rmc_pcg32_next(rng);
    rng->state += initstate;
    (void)rmc_pcg32_next(rng);
}

bool rmc_pcg32_bounded(
    RmcPcg32 *rng,
    uint32_t bound,
    uint32_t *result)
{
    if (rng == NULL || result == NULL || bound == 0u) {
        return false;
    }

    const uint32_t threshold =
        (uint32_t)(0u - bound) % bound;

    for (;;) {
        const uint32_t value = rmc_pcg32_next(rng);

        if (value >= threshold) {
            *result = value % bound;
            return true;
        }
    }
}
