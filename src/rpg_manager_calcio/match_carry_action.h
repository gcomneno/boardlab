#ifndef RMC_MATCH_CARRY_ACTION_H
#define RMC_MATCH_CARRY_ACTION_H

#include "match_event_log.h"
#include "match_probability.h"
#include "match_sequence.h"
#include "streams.h"

#include <stdbool.h>
#include <stdint.h>

#define RMC_CARRY_ACTION_VERSION UINT32_C(1)

typedef enum {
    RMC_CARRY_OUTCOME_ADVANCED = 101,
    RMC_CARRY_OUTCOME_CONTAINED = 102
} RmcCarryOutcome;

typedef struct {
    RmcTeamId team_id;
    RmcPlayerId carrier;
    RmcPlayerId opponent;
    int32_t context;
    uint32_t duration_seconds;
} RmcCarryAction;

typedef struct {
    uint32_t probability;
    uint32_t random_draw;
    RmcCarryOutcome outcome;
} RmcCarryResult;

/*
 * Carry the ball forward by one longitudinal sector.
 *
 * Requires:
 * - active possession and BALL_IN_PLAY;
 * - the carrier controlling the ball;
 * - an on-field opposing player;
 * - a current sector below the final sector;
 * - an available action window and valid duration.
 *
 * Candidate effectiveness model v1:
 * A = floor((carrier.CONTROLLO + carrier.VELOCITA) / 2)
 * O = floor((opponent.CONTRASTO +
 *            opponent.POSIZIONAMENTO) / 2)
 *
 * ADVANCED: sector + 1; carrier and lane unchanged.
 * CONTAINED: sector unchanged; carrier retains the ball.
 *
 * Emits one ACTION event with a carry-specific outcome
 * code distinct from the pass outcome codes.
 *
 * State, PRNG, event log and result commit atomically.
 * On failure all caller-owned data remains unchanged.
 *
 * Writable arguments and event storage must not overlap.
 * Caller provides exclusive access.
 */
bool rmc_match_carry_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcCarryAction *action,
    RmcCarryResult *result
);

#endif
