#include "match_transitions.h"

#include <string.h>

bool rmc_match_transition_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcMatchTransition transition,
    uint32_t at_second)
{
    if (world == NULL || state == NULL ||
        (unsigned)transition >= RMC_TRANSITION_COUNT ||
        !rmc_match_state_valid(world, state) ||
        at_second != state->elapsed_seconds) {
        return false;
    }

    RmcMatchState next = *state;

    switch (transition) {
    case RMC_TRANSITION_START:
        if (state->period != RMC_MATCH_NOT_STARTED ||
            at_second != 0) {
            return false;
        }

        next.period = RMC_MATCH_FIRST_HALF;
        next.mode = RMC_MATCH_RESTART_PENDING;
        break;

    case RMC_TRANSITION_HALF_TIME:
        if (state->period != RMC_MATCH_FIRST_HALF ||
            at_second <
                UINT32_C(2700) +
                state->first_half_stoppage_seconds) {
            return false;
        }

        next.period = RMC_MATCH_HALF_TIME;
        next.second_half_start_second = at_second;
        next.mode = RMC_MATCH_PERIOD_ENDED;
        memset(&next.possession, 0, sizeof(next.possession));
        break;

    case RMC_TRANSITION_SECOND_HALF:
        if (state->period != RMC_MATCH_HALF_TIME) {
            return false;
        }

        next.period = RMC_MATCH_SECOND_HALF;
        next.mode = RMC_MATCH_RESTART_PENDING;
        break;

    case RMC_TRANSITION_FINISH:
        if (state->period != RMC_MATCH_SECOND_HALF ||
            at_second <
                state->second_half_start_second +
                UINT32_C(2700) +
                state->second_half_stoppage_seconds) {
            return false;
        }

        next.period = RMC_MATCH_FINISHED;
        next.mode = RMC_MATCH_PERIOD_ENDED;
        next.terminal_reason = RMC_MATCH_TERMINAL_COMPLETED;
        memset(&next.possession, 0, sizeof(next.possession));
        break;

    default:
        return false;
    }

    next.elapsed_seconds = at_second;

    if (!rmc_match_state_valid(world, &next)) {
        return false;
    }

    *state = next;
    return true;
}
