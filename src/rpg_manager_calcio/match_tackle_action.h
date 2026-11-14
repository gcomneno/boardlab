#ifndef RMC_MATCH_TACKLE_ACTION_H
#define RMC_MATCH_TACKLE_ACTION_H

#include "match_event_log.h"
#include "match_probability.h"
#include "match_sequence.h"
#include "streams.h"

#include <stdbool.h>
#include <stdint.h>

#define RMC_TACKLE_ACTION_VERSION UINT32_C(1)

typedef enum {
    RMC_TACKLE_OUTCOME_RETAINED = 301,
    RMC_TACKLE_OUTCOME_WON = 302,
    RMC_TACKLE_OUTCOME_LOOSE_BALL = 303
} RmcTackleOutcome;

typedef struct {
    /* Team of the defender initiating the tackle. */
    RmcTeamId team_id;
    RmcPlayerId carrier;
    RmcPlayerId defender;
    int32_t context;
    uint32_t duration_seconds;
} RmcTackleAction;

typedef struct {
    uint32_t probability;
    uint32_t random_draw;
    RmcTackleOutcome outcome;
} RmcTackleResult;

/*
 * Resolve a defender-initiated tackle against the current controller.
 * Both players must be on field for opposing match teams, with an
 * active possession, BALL_IN_PLAY and an available action window.
 *
 * Candidate effectiveness model v1:
 * D = floor((defender.CONTRASTO + defender.POSIZIONAMENTO) / 2)
 * A = floor((carrier.CONTROLLO + carrier.FORZA) / 2)
 * P = frozen probability(D, A, context), context in [-20,20].
 * Exactly one bounded PCG32 draw in [0,9999]:
 * - draw < P: WON, fresh possession for the defender, counters zero,
 *   sector inverted (4 - old sector), same lane, TRANSITION phase;
 * - draw < min(P + 1000, 10000): LOOSE_BALL, no controller;
 * - otherwise: RETAINED, carrier still controls the ball.
 * The 1000-point band is an approved candidate rule, not calibrated.
 * RETAINED and LOOSE_BALL preserve possession context and zone,
 * incrementing the action count. Only explicit recovery resolves a
 * loose ball; no regulatory restart is generated.
 *
 * A positive validated duration advances time. One ACTION event uses
 * defender as primary actor and carrier as secondary actor in every
 * outcome. Its team is the resulting possession team (the previous
 * team for RETAINED and LOOSE_BALL, the defender's team for WON).
 *
 * State, PRNG, event log, storage and result commit atomically.
 * On failure all caller-owned data remains unchanged.
 * Writable arguments and event storage must not overlap.
 * Caller provides exclusive access.
 */
bool rmc_match_tackle_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcTackleAction *action,
    RmcTackleResult *result
);

#endif
