#ifndef RMC_MATCH_DRIBBLE_ACTION_H
#define RMC_MATCH_DRIBBLE_ACTION_H

#include "match_event_log.h"
#include "match_probability.h"
#include "match_sequence.h"
#include "streams.h"

#include <stdbool.h>
#include <stdint.h>

#define RMC_DRIBBLE_ACTION_VERSION UINT32_C(1)

typedef enum {
    RMC_DRIBBLE_OUTCOME_SUCCESS = 201,
    RMC_DRIBBLE_OUTCOME_CONTAINED = 202
} RmcDribbleOutcome;

typedef struct {
    RmcTeamId team_id;
    RmcPlayerId dribbler;
    RmcPlayerId opponent;
    int32_t context;
    uint32_t duration_seconds;
} RmcDribbleAction;

typedef struct {
    uint32_t probability;
    uint32_t random_draw;
    RmcDribbleOutcome outcome;
} RmcDribbleResult;

/*
 * Attempt to dribble forward by one longitudinal sector.
 *
 * Requires:
 * - active possession and BALL_IN_PLAY;
 * - the dribbler controlling the ball;
 * - an on-field opposing player;
 * - a current sector below the final sector;
 * - an available action window and valid duration.
 *
 * Candidate effectiveness model v1:
 * A = floor((dribbler.DRIBBLING + dribbler.VELOCITA) / 2)
 * O = floor((opponent.CONTRASTO +
 *            opponent.POSIZIONAMENTO) / 2)
 *
 * SUCCESS: sector + 1; controller, team and lane unchanged.
 * CONTAINED: sector unchanged; dribbler retains the ball.
 *
 * Emits one ACTION event with a dribble-specific outcome
 * code distinct from the pass and carry outcome codes.
 *
 * State, PRNG, event log and result commit atomically.
 * On failure all caller-owned data remains unchanged.
 *
 * Writable arguments and event storage must not overlap.
 * Caller provides exclusive access.
 */
bool rmc_match_dribble_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcDribbleAction *action,
    RmcDribbleResult *result
);

#endif
