#ifndef RMC_MATCH_PERIOD_COMMIT_H
#define RMC_MATCH_PERIOD_COMMIT_H

#include "match_event_log.h"
#include "match_sequence_apply.h"

#include <stdbool.h>

/*
 * M001-B — Atomic period completion, version 1.
 *
 * FIRST_HALF -> HALF_TIME:
 *   PERIOD_END
 *
 * SECOND_HALF -> FINISHED:
 *   PERIOD_END, MATCH_END
 *
 * MATCH_END references the immediately preceding
 * PERIOD_END event as its causal predecessor.
 *
 * The operation validates the state transition before
 * touching the event log and reserves all event slots.
 *
 * On failure, state, log metadata and event storage
 * remain unchanged.
 *
 * Preconditions:
 * - caller-owned event storage does not overlap state
 *   or log metadata;
 * - no concurrent access to state or event storage.
 *
 * No PRNG, heap allocation or time advancement.
 * Does not resolve gameplay actions.
 */
bool rmc_match_period_commit(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcMatchEventLog *log
);

#endif
