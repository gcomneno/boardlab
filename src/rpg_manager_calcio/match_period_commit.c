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

bool rmc_match_period_commit(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcMatchEventLog *log)
{
    if (world == NULL || state == NULL || log == NULL ||
        log->events == NULL ||
        (state->period != RMC_MATCH_FIRST_HALF &&
         state->period != RMC_MATCH_SECOND_HALF)) {
        return false;
    }

    const size_t required =
        state->period == RMC_MATCH_SECOND_HALF ? 2u : 1u;

    if (log->count > log->capacity ||
        log->capacity - log->count < required ||
        log->count > UINT64_MAX - required) {
        return false;
    }

    RmcMatchState next = *state;

    if (!rmc_match_sequence_apply(
            world, &next, RMC_SEQUENCE_END_PERIOD)) {
        return false;
    }

    const uint64_t first_id =
        (uint64_t)log->count + UINT64_C(1);

    const RmcMatchEvent first =
        period_event(state, first_id, RMC_EVENT_PERIOD_END, 0);

    const RmcMatchEvent second =
        period_event(
            state, first_id + UINT64_C(1),
            RMC_EVENT_MATCH_END, first_id);

    const RmcMatchEventLog original_log = *log;
    const RmcMatchEvent saved_first =
        log->events[log->count];

    RmcMatchEvent saved_second = {0};

    if (required == 2u) {
        saved_second = log->events[log->count + 1u];
    }

    if (!rmc_match_event_log_append(
            world, state, log, &first)) {
        return false;
    }

    if (required == 2u &&
        !rmc_match_event_log_append(
            world, state, log, &second)) {
        log->events[original_log.count] = saved_first;
        log->events[original_log.count + 1u] = saved_second;
        *log = original_log;
        return false;
    }

    *state = next;
    return true;
}
