#include "match_sequence.h"
#include "match_stoppage.h"

bool rmc_match_sequence_decide(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    RmcSequenceDecision *decision)
{
    if (world == NULL || state == NULL ||
        decision == NULL ||
        !rmc_match_state_valid(world, state) ||
        state->mode != RMC_MATCH_BALL_IN_PLAY) {
        return false;
    }

    uint32_t deadline = 0;

    if (state->period == RMC_MATCH_FIRST_HALF) {
        if (!rmc_stoppage_deadline(
                0,
                state->first_half_stoppage_seconds,
                &deadline)) {
            return false;
        }
    } else if (state->period == RMC_MATCH_SECOND_HALF) {
        if (!rmc_stoppage_deadline(
                state->second_half_start_second,
                state->second_half_stoppage_seconds,
                &deadline)) {
            return false;
        }
    } else {
        return false;
    }

    if (state->elapsed_seconds > deadline) {
        return false;
    }

    RmcSequenceDecision next = RMC_SEQUENCE_ALLOW_ACTION;

    if (state->elapsed_seconds == deadline) {
        next = RMC_SEQUENCE_END_PERIOD;
    } else if (state->possession.team_id.value != 0 &&
               state->possession.action_count >=
                   RMC_POSSESSION_MAX_ACTIONS) {
        next = RMC_SEQUENCE_RESET_ACTION_WINDOW;
    }

    *decision = next;
    return true;
}
