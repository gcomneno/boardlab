#ifndef RMC_MATCH_SEQUENCE_H
#define RMC_MATCH_SEQUENCE_H

#include "match_state.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * M001-B — Sequence Policy v1.
 *
 * An action window permits at most 32 actions.
 *
 * This policy determines whether another action may
 * begin. It never resolves, interrupts or records
 * an action, and never mutates the match state.
 *
 * A future Action Engine must enforce the decision
 * before accepting the next candidate action.
 */

#define RMC_SEQUENCE_POLICY_VERSION UINT32_C(1)
#define RMC_POSSESSION_MAX_ACTIONS UINT32_C(32)

typedef enum {
    RMC_SEQUENCE_ALLOW_ACTION = 0,
    RMC_SEQUENCE_RESET_ACTION_WINDOW,
    RMC_SEQUENCE_END_PERIOD,
    RMC_SEQUENCE_DECISION_COUNT
} RmcSequenceDecision;

/*
 * Classify the next permitted sequence operation.
 *
 * Preconditions:
 * - valid world and match state;
 * - FIRST_HALF or SECOND_HALF;
 * - BALL_IN_PLAY mode.
 *
 * Precedence:
 * 1. Period deadline reached -> END_PERIOD.
 * 2. Action-window cap -> RESET_ACTION_WINDOW.
 * 3. Otherwise -> ALLOW_ACTION.
 *
 * RESET_ACTION_WINDOW requires an active possession.
 * END_PERIOD may occur with or without possession.
 *
 * On failure, decision remains unchanged.
 * No PRNG consumption, events or state mutation.
 */
bool rmc_match_sequence_decide(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    RmcSequenceDecision *decision
);

#endif
