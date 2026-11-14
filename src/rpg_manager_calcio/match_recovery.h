#ifndef RMC_MATCH_RECOVERY_H
#define RMC_MATCH_RECOVERY_H

#include "match_pass_action.h"

#include <stdbool.h>
#include <stdint.h>

#define RMC_RECOVERY_VERSION UINT32_C(1)

typedef enum {
    RMC_RECOVERY_RETAINED = 1,
    RMC_RECOVERY_TURNOVER = 2
} RmcRecoveryOutcome;

typedef struct {
    /* Incumbent: player of the current possession team. */
    RmcPlayerId incumbent;

    /* Challenger: player of the opposing team. */
    RmcPlayerId challenger;

    int32_t context;
    uint32_t duration_seconds;
} RmcRecoveryAction;

typedef struct {
    uint32_t probability;
    uint32_t random_draw;
    RmcRecoveryOutcome outcome;
    RmcPlayerId winner;
} RmcRecoveryResult;

/*
 * Resolve a loose ball originating from a pass, tackle, cross or Shot v1.
 *
 * Requires a live possession with no controller and
 * the immediately preceding event to be ACTION with
 * PASS_OUTCOME_LOOSE_BALL, TACKLE_OUTCOME_LOOSE_BALL or
 * CROSS_OUTCOME_LOOSE_BALL for the
 * suspended possession team and current match, with a coherent
 * consecutive ID and nondecreasing timestamp. Other ACTION outcomes
 * are ineligible. Also accepts a final SHOT with RMC_SHOT_SAVED_REBOUND,
 * RMC_SHOT_BLOCKED or RMC_SHOT_WOODWORK, referencing the adjacent validated
 * ACTION / RMC_SHOT_CHANCE with matching shooting team, shooter and payload.
 * Its timestamp must belong to the current period and its duration must fit
 * the suspended possession clock.
 * The loose-ball zone must match the Shot v1 destination (source zone for a
 * block; sector 4, CENTRE and CHANCE phase for rebound/woodwork). Source phase
 * on a block is not recorded in the payload and cannot be checked.
 * Each cause can be resolved only once; RECOVERY references the final SHOT,
 * never its chance event. The complete prefix is revalidated before sampling.
 * The sequence action window must allow another action; resetting
 * that window preserves the loose ball and its latest event cause.
 *
 * Both contenders must be on field for opposite teams. Uses exactly one
 * bounded draw from the existing match PCG32 stream.
 *
 * v1 effectiveness:
 * A = floor((incumbent.CONTRASTO +
 *            incumbent.POSIZIONAMENTO) / 2)
 * O = floor((challenger.CONTRASTO +
 *            challenger.POSIZIONAMENTO) / 2)
 *
 * Incumbent wins when draw < P(A-O+C).
 *
 * Retained possession: winner becomes controller,
 * existing possession continues and action count grows.
 *
 * Turnover: creates a fresh possession for the
 * challenger; longitudinal sector is inverted,
 * lane is retained and phase becomes TRANSITION.
 *
 * Emits one causally linked RECOVERY event.
 *
 * On failure state, RNG, log, storage and result are
 * unchanged. Writable arguments must not overlap.
 * Caller provides exclusive access. Prefix validation uses temporary heap
 * storage; allocation failure rejects atomically. Historical events must
 * satisfy the logger's current-lineup requirements.
 */
bool rmc_match_recovery_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcRecoveryAction *action,
    RmcRecoveryResult *result
);

#endif
