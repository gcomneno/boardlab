#include "match_event_log.h"

#include <stdint.h>

static bool actor_on_field(
    const RmcMatchState *state,
    RmcPlayerId actor)
{
    if (actor.value == 0) {
        return true;
    }

    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (state->home_lineup.players[i].value == actor.value ||
            state->away_lineup.players[i].value == actor.value) {
            return true;
        }
    }

    return false;
}

bool rmc_match_event_log_init(
    const RmcMatchState *state,
    RmcMatchEvent *storage,
    size_t capacity,
    RmcMatchEventLog *destination)
{
    if (state == NULL || storage == NULL ||
        capacity == 0 || destination == NULL ||
        state->fixture.id.value == 0) {
        return false;
    }

    RmcMatchEventLog next = {
        .events = storage,
        .count = 0,
        .capacity = capacity,
        .match_id = state->fixture.id
    };

    *destination = next;
    return true;
}

bool rmc_match_event_log_append(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcMatchEvent *event)
{
    if (world == NULL || state == NULL ||
        log == NULL || event == NULL ||
        !rmc_match_state_valid(world, state) ||
        (state->period != RMC_MATCH_FIRST_HALF &&
         state->period != RMC_MATCH_SECOND_HALF) ||
        log->events == NULL ||
        log->capacity == 0 ||
        log->count >= log->capacity ||
        log->count >= UINT64_MAX ||
        log->match_id.value != state->fixture.id.value ||
        event->match_id.value != log->match_id.value ||
        event->id != (uint64_t)log->count + UINT64_C(1) ||
        (unsigned)event->type >= RMC_EVENT_TYPE_COUNT ||
        event->at_second > state->elapsed_seconds ||
        event->cause_event_id >= event->id ||
        !actor_on_field(state, event->primary_actor) ||
        !actor_on_field(state, event->secondary_actor)) {
        return false;
    }

    if (event->team_id.value != 0 &&
        event->team_id.value != state->fixture.home_team.value &&
        event->team_id.value != state->fixture.away_team.value) {
        return false;
    }

    if (log->count != 0 &&
        event->at_second <
            log->events[log->count - 1].at_second) {
        return false;
    }

    /*
     * All checks precede the only storage mutation.
     * No allocation and no downstream fallible operation.
     */
    log->events[log->count] = *event;
    ++log->count;

    return true;
}
