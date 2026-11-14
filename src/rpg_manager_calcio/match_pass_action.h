#ifndef RMC_MATCH_PASS_ACTION_H
#define RMC_MATCH_PASS_ACTION_H

#include "match_event_log.h"
#include "match_probability.h"
#include "match_sequence.h"
#include "streams.h"

#include <stdbool.h>
#include <stdint.h>

#define RMC_PASS_ACTION_VERSION UINT32_C(1)

typedef enum {
    RMC_PASS_OUTCOME_COMPLETED = 1,
    RMC_PASS_OUTCOME_LOOSE_BALL = 2
} RmcPassOutcome;

typedef struct {
    RmcTeamId team_id;
    RmcPlayerId passer;
    RmcPlayerId recipient;
    RmcPlayerId opponent;

    /* Context modifier in [-20, 20]. */
    int32_t context;

    /* Positive action duration in match seconds. */
    uint32_t duration_seconds;
} RmcPassAction;

typedef struct {
    uint32_t probability;
    uint32_t random_draw;
    RmcPassOutcome outcome;
} RmcPassResult;

/*
 * Resolve one pass using the qualified match PRNG,
 * integer probability model and append-only event log.
 *
 * v1 effectiveness:
 * A = floor((passer.PASSAGGIO + passer.VISIONE) / 2)
 * O = floor((opponent.MARCATURA +
 *            opponent.POSIZIONAMENTO) / 2)
 * C = action.context
 *
 * On success, recipient controls the ball.
 * On failure, the ball becomes unassigned while
 * the possession context awaits explicit resolution.
 *
 * The action increments the possession action count,
 * advances match time and emits one ACTION event.
 *
 * On failure, state, PRNG, event log, storage and
 * result remain unchanged.
 *
 * Caller guarantees nonoverlapping writable objects
 * and exclusive access during the operation.
 */
bool rmc_match_pass_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcPassAction *action,
    RmcPassResult *result
);

#endif
