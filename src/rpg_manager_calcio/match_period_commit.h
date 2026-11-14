#ifndef RMC_MATCH_PERIOD_COMMIT_H
#define RMC_MATCH_PERIOD_COMMIT_H

#include "match_event_log.h"
#include "match_sequence_apply.h"

#include <stdbool.h>

typedef struct {
    RmcMatchState next_state;
    RmcMatchEvent events[2];
    size_t event_count;
} RmcMatchPeriodPlan;

/*
 * Pure deadline closure plan for a validated active BALL_IN_PLAY state.
 * preceding_event_count includes any action events planned by the caller.
 * IDs follow that count; no log is read or written. At the deadline,
 * END_PERIOD takes precedence over the 32-action window, using the
 * existing sequence/lifecycle rules. Earlier/later clocks, illegal
 * modes and insufficient ID headroom are rejected.
 *
 * Produces HALF_TIME + PERIOD_END or FINISHED + PERIOD_END, MATCH_END.
 * No restarts, time advancement, heap or RNG. All outputs (including
 * unused event slots) remain unchanged on rejection.
 */
bool rmc_match_period_plan(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    size_t preceding_event_count,
    RmcMatchPeriodPlan *destination
);

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
