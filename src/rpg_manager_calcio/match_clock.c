#include "match_clock.h"
#include "match_stoppage.h"

#include <stdint.h>

bool rmc_match_clock_advance(
    const RmcWorldCore *world,
    RmcMatchState *state,
    uint32_t delta_seconds)
{
    if (world == NULL || state == NULL ||
        delta_seconds == 0 ||
        !rmc_match_state_valid(world, state) ||
        state->mode != RMC_MATCH_BALL_IN_PLAY) {
        return false;
    }

    uint32_t period_limit;

    if (state->period == RMC_MATCH_FIRST_HALF) {
        if (!rmc_stoppage_deadline(
                0,
                state->first_half_stoppage_seconds,
                &period_limit)) {
            return false;
        }
    } else if (state->period == RMC_MATCH_SECOND_HALF) {
        if (!rmc_stoppage_deadline(
                state->second_half_start_second,
                state->second_half_stoppage_seconds,
                &period_limit)) {
            return false;
        }
    } else {
        return false;
    }

    if (state->elapsed_seconds > period_limit ||
        delta_seconds >
            period_limit - state->elapsed_seconds) {
        return false;
    }

    RmcMatchState next = *state;

    next.elapsed_seconds += delta_seconds;

    if (next.possession.team_id.value != 0) {
        if (delta_seconds >
            UINT32_MAX - next.possession.elapsed_seconds) {
            return false;
        }

        next.possession.elapsed_seconds += delta_seconds;
    }

    if (!rmc_match_state_valid(world, &next)) {
        return false;
    }

    *state = next;
    return true;
}
