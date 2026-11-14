#ifndef RMC_MATCH_CLOCK_H
#define RMC_MATCH_CLOCK_H

#include "match_state.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * M001-B — Regulation-time clock, version 1.
 *
 * This API advances game time only while the ball
 * is in play, during the first or second half.
 *
 * Stoppage-time policy and interruption accounting
 * will be introduced separately.
 *
 * A rejected advance leaves the state unchanged.
 * No PRNG is consumed and no events are emitted.
 */
bool rmc_match_clock_advance(
    const RmcWorldCore *world,
    RmcMatchState *state,
    uint32_t delta_seconds
);

#endif
