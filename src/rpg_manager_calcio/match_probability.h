#ifndef RMC_MATCH_PROBABILITY_H
#define RMC_MATCH_PROBABILITY_H

#include <stdbool.h>
#include <stdint.h>

#define RMC_LOGISTIC_TABLE_VERSION UINT32_C(1)
#define RMC_LOGISTIC_TABLE_SIZE 201
#define RMC_PROBABILITY_SCALE UINT32_C(10000)

/*
 * Frozen M001 logistic probability model.
 *
 * D = clamp(A - O + C, -100, 100).
 * P = round_half_up(10000 / (1 + exp(-D / 16))).
 *
 * Values are canonical integers; runtime uses lookup only.
 *
 * Valid contextual operands:
 * A, O: [0,100]
 * C: [-20,20]
 *
 * Output is unchanged on failure.
 */
bool rmc_match_probability(
    int32_t attack,
    int32_t opposition,
    int32_t context,
    uint32_t *probability
);

/* Direct lookup for a valid D in [-100,100]. */
bool rmc_match_logistic_lookup(
    int32_t difference,
    uint32_t *probability
);

#endif
