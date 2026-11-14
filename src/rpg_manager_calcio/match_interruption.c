#include "match_interruption.h"
#include "match_stoppage.h"

bool rmc_match_interruption_record(
    const RmcWorldCore *world,
    RmcMatchState *state,
    uint32_t duration_seconds)
{
    if (world == NULL || state == NULL ||
        duration_seconds == 0 ||
        !rmc_match_state_valid(world, state) ||
        (state->mode != RMC_MATCH_RESTART_PENDING &&
         state->mode != RMC_MATCH_MANAGEMENT_WINDOW)) {
        return false;
    }

    uint32_t *accumulated = NULL;
    uint32_t period_start = 0;

    RmcMatchState next = *state;

    if (next.period == RMC_MATCH_FIRST_HALF) {
        accumulated = &next.first_half_stoppage_seconds;
    } else if (next.period == RMC_MATCH_SECOND_HALF) {
        accumulated = &next.second_half_stoppage_seconds;
        period_start = next.second_half_start_second;
    } else {
        return false;
    }

    uint32_t current_deadline;

    if (!rmc_stoppage_deadline(
            period_start,
            *accumulated,
            &current_deadline) ||
        next.elapsed_seconds > current_deadline) {
        return false;
    }

    if (!rmc_stoppage_record(
            accumulated,
            duration_seconds)) {
        return false;
    }

    if (!rmc_match_state_valid(world, &next)) {
        return false;
    }

    *state = next;
    return true;
}
