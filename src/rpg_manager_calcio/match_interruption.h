#ifndef RMC_MATCH_INTERRUPTION_H
#define RMC_MATCH_INTERRUPTION_H

#include "match_state.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * M001-B — Deterministic interruption accounting.
 *
 * Registers an interruption duration in the persistent
 * accumulator of the currently active match period.
 *
 * Preconditions:
 * - The source state is valid.
 * - The period is FIRST_HALF or SECOND_HALF.
 * - The mode is RESTART_PENDING or MANAGEMENT_WINDOW.
 * - Duration is positive and within the policy cap.
 * - The current clock is not beyond its authorized
 *   period deadline.
 *
 * Does not advance time, modify possession,
 * emit events or consume PRNG.
 *
 * On failure, the state remains unchanged.
 *
 * Event identity and duplicate-event prevention
 * belong to the later event-log component.
 */
bool rmc_match_interruption_record(
    const RmcWorldCore *world,
    RmcMatchState *state,
    uint32_t duration_seconds
);

#endif
