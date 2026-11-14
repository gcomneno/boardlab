#include "match_period_commit.h"

#include <stdint.h>

static RmcMatchEvent period_event(
    const RmcMatchState *state,
    uint64_t id,
    RmcMatchEventType type,
    uint64_t cause_id)
{
    RmcMatchEvent event = {0};

    event.id = id;
    event.match_id = state->fixture.id;
    event.at_second = state->elapsed_seconds;
    event.type = type;
    event.cause_event_id = cause_id;

    return event;
}

bool rmc_match_period_plan(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    size_t preceding_event_count,
    RmcMatchPeriodPlan *destination)
{
    if (world == NULL || state == NULL || destination == NULL ||
        (state->period != RMC_MATCH_FIRST_HALF &&
         state->period != RMC_MATCH_SECOND_HALF)) {
        return false;
    }

    const size_t required =
        state->period == RMC_MATCH_SECOND_HALF ? 2u : 1u;
    if (preceding_event_count > SIZE_MAX - required ||
        preceding_event_count > UINT64_MAX - required) {
        return false;
    }

    RmcMatchPeriodPlan next = {0};
    next.next_state = *state;
    if (!rmc_match_sequence_apply(
            world, &next.next_state, RMC_SEQUENCE_END_PERIOD)) {
        return false;
    }

    const uint64_t first_id =
        (uint64_t)preceding_event_count + UINT64_C(1);
    next.event_count = required;
    next.events[0] =
        period_event(state, first_id, RMC_EVENT_PERIOD_END, 0);
    if (required == 2u) {
        next.events[1] = period_event(
            state, first_id + UINT64_C(1), RMC_EVENT_MATCH_END, first_id);
    }

    *destination = next;
    return true;
}

bool rmc_match_period_commit(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcMatchEventLog *log)
{
    if (state == NULL || log == NULL) {
        return false;
    }

    RmcMatchPeriodPlan plan;
    if (!rmc_match_period_plan(world, state, log->count, &plan) ||
        !rmc_match_event_log_append_batch(
            world, state, log, plan.events, plan.event_count)) {
        return false;
    }

    *state = plan.next_state;
    return true;
}
