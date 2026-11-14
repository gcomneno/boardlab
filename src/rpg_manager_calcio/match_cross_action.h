#ifndef RMC_MATCH_CROSS_ACTION_H
#define RMC_MATCH_CROSS_ACTION_H

#include "match_event_log.h"
#include "match_probability.h"
#include "match_sequence.h"
#include "streams.h"

#include <stdbool.h>
#include <stdint.h>

#define RMC_CROSS_ACTION_VERSION UINT32_C(1)

typedef enum {
    RMC_CROSS_OUTCOME_COMPLETED = 401,
    RMC_CROSS_OUTCOME_INTERCEPTED = 402,
    RMC_CROSS_OUTCOME_LOOSE_BALL = 403
} RmcCrossOutcome;

typedef struct {
    RmcTeamId team_id;
    RmcPlayerId crosser;
    RmcPlayerId recipient;
    RmcPlayerId defender;
    int32_t context;
    uint32_t duration_seconds;
} RmcCrossAction;

typedef struct {
    uint32_t probability;
    uint32_t random_draw;
    RmcCrossOutcome outcome;
} RmcCrossResult;

/*
 * Resolve a cross from the current controller to a distinct on-field
 * teammate against an on-field opponent. Requires BALL_IN_PLAY, an
 * available sequence action, sector 2 or 3 and LEFT or RIGHT lane.
 * The ball destination is (source sector + 1, CENTRE); individual
 * player positions are not simulated.
 *
 * v1: A = floor((crosser.CROSS + crosser.VISIONE) / 2),
 * O = floor((defender.MARCATURA + defender.POSIZIONAMENTO) / 2).
 * P = frozen probability(A, O, context), context in [-20,20].
 * Exactly one bounded PCG32 draw in [0,9999]:
 * - draw < P: COMPLETED, recipient controls the destination;
 * - draw < min(P + 1000, 10000): LOOSE_BALL, no controller,
 *   previous possession team preserved, explicit recovery required;
 * - otherwise: INTERCEPTED, new possession for the defender,
 *   destination sector inverted (4 - destination), CENTRE lane,
 *   TRANSITION phase and new-possession counters zero.
 * The 1000-point band is an approved, uncalibrated candidate rule.
 * COMPLETED and LOOSE_BALL increment the action count and set phase
 * to ATTACKING_DEVELOPMENT in sector 3 or CHANCE in sector 4.
 *
 * A positive validated duration advances time. Exactly one ACTION
 * event records crosser and recipient, the cross outcome code and
 * resulting possession team (previous team for LOOSE_BALL).
 * No regulatory restart or separate interception recovery is emitted.
 *
 * State, PRNG, log metadata, storage and result commit atomically;
 * failure leaves them unchanged. No draw precedes preconditions.
 * Caller guarantees nonoverlapping writable objects and event storage,
 * and exclusive access throughout the operation.
 */
bool rmc_match_cross_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcCrossAction *action,
    RmcCrossResult *result
);

#endif
