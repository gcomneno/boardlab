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
 * Resolve a loose ball originating from a failed pass.
 *
 * Requires a live possession with no controller and
 * the immediately preceding event to be ACTION with
 * PASS_OUTCOME_LOOSE_BALL for the same team.
 *
 * Both contenders must be on field for opposite teams.
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
 * Caller provides exclusive access.
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
