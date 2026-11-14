#ifndef RMC_RNG_H
#define RMC_RNG_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint64_t state;
    uint64_t increment;
} RmcPcg32;

/*
 * PCG-XSH-RR 64/32.
 * initstate and initseq are unsigned 64-bit values.
 */
void rmc_pcg32_seed(
    RmcPcg32 *rng,
    uint64_t initstate,
    uint64_t initseq
);

uint32_t rmc_pcg32_next(RmcPcg32 *rng);

/*
 * Uniform draw in [0, bound).
 * Returns false for invalid arguments.
 */
bool rmc_pcg32_bounded(
    RmcPcg32 *rng,
    uint32_t bound,
    uint32_t *result
);

#endif
