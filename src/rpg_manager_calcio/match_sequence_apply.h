#ifndef RMC_MATCH_SEQUENCE_APPLY_H
#define RMC_MATCH_SEQUENCE_APPLY_H

#include "match_sequence.h"

#include <stdbool.h>

/*
 * M001-B — Atomic application of sequence boundaries.
 *
 * Applies a RESET_ACTION_WINDOW or END_PERIOD decision
 * only when the current sequence policy agrees.
 *
 * RESET_ACTION_WINDOW:
 * - resets only the bounded action counter;
 * - preserves possession, controller and BALL_IN_PLAY;
 * - does not invent a sporting restart or turnover.
 *
 * END_PERIOD:
 * - delegates to the qualified lifecycle transitions;
 * - advances no time;
 * - preserves the recorded stoppage accounting.
 *
 * ALLOW_ACTION is not a termination operation.
 *
 * On failure, match state remains unchanged.
 * No PRNG consumption or event emission.
 *
 * Structured termination events and explicit restart
 * semantics belong to subsequent increments.
 */
bool rmc_match_sequence_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcSequenceDecision requested
);

#endif
