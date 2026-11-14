#include "match_sequence_apply.h"
#include "match_transitions.h"


bool rmc_match_sequence_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcSequenceDecision requested)
{
    if (world == NULL || state == NULL ||
        requested == RMC_SEQUENCE_ALLOW_ACTION ||
        (unsigned)requested >= RMC_SEQUENCE_DECISION_COUNT) {
        return false;
    }

    RmcSequenceDecision actual;

    if (!rmc_match_sequence_decide(world, state, &actual) ||
        actual != requested) {
        return false;
    }

    RmcMatchState next = *state;

    if (actual == RMC_SEQUENCE_RESET_ACTION_WINDOW) {
        if (next.possession.team_id.value == 0) {
            return false;
        }

        next.possession.action_count = 0;

    } else if (actual == RMC_SEQUENCE_END_PERIOD) {
        RmcMatchTransition transition;

        if (next.period == RMC_MATCH_FIRST_HALF) {
            transition = RMC_TRANSITION_HALF_TIME;
        } else if (next.period == RMC_MATCH_SECOND_HALF) {
            transition = RMC_TRANSITION_FINISH;
        } else {
            return false;
        }

        if (!rmc_match_transition_apply(
                world,
                &next,
                transition,
                next.elapsed_seconds)) {
            return false;
        }
    } else {
        return false;
    }

    if (!rmc_match_state_valid(world, &next)) {
        return false;
    }

    *state = next;
    return true;
}
