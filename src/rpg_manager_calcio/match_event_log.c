#include "match_event_log.h"

#include <stdint.h>
#include <string.h>

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

static bool neutral_restart(const RmcShotEventPayload *shot)
{
    return shot->restart_kind == RMC_RESTART_NONE &&
           shot->restart_awarded_team.value == 0 &&
           shot->restart_zone.sector == 0 &&
           shot->restart_zone.lane == RMC_LANE_LEFT &&
           shot->restart_disposition == RMC_RESTART_DISPOSITION_NONE;
}

static bool neutral_shot(const RmcShotEventPayload *shot)
{
    return shot->model_version == 0 && shot->chance_quality == 0 &&
           shot->source_zone.sector == 0 &&
           shot->source_zone.lane == RMC_LANE_LEFT &&
           shot->context == 0 && shot->duration_seconds == 0 &&
           shot->defender_id.value == 0 && shot->goalkeeper_id.value == 0 &&
           shot->applied_talent_mask == 0 &&
           shot->trajectory == RMC_SHOT_TRAJECTORY_NONE &&
           neutral_restart(shot);
}

static const RmcPlayer *find_player(
    const RmcWorldCore *world, RmcPlayerId id)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
            if (world->rosters[i].players[j].id.value == id.value) {
                return &world->rosters[i].players[j];
            }
        }
    }
    return NULL;
}

static bool in_lineup(
    const RmcLineup *lineup, RmcPlayerId id, bool goalkeeper)
{
    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (lineup->players[i].value == id.value) {
            return (lineup->slots[i] == RMC_SLOT_POR) == goalkeeper;
        }
    }
    return false;
}

/* Fields fixed by the chance event; compare values, never padding. */
static bool same_chance(
    const RmcMatchEvent *a, const RmcMatchEvent *b)
{
    return a->team_id.value == b->team_id.value &&
           a->primary_actor.value == b->primary_actor.value &&
           a->secondary_actor.value == b->secondary_actor.value &&
           a->at_second == b->at_second &&
           a->shot.model_version == b->shot.model_version &&
           a->shot.chance_quality == b->shot.chance_quality &&
           a->shot.source_zone.sector == b->shot.source_zone.sector &&
           a->shot.source_zone.lane == b->shot.source_zone.lane &&
           a->shot.context == b->shot.context &&
           a->shot.duration_seconds == b->shot.duration_seconds &&
           a->shot.defender_id.value == b->shot.defender_id.value &&
           a->shot.goalkeeper_id.value == b->shot.goalkeeper_id.value &&
           a->shot.applied_talent_mask == b->shot.applied_talent_mask;
}

static bool shot_valid(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    const RmcMatchEvent *event,
    const RmcMatchEvent *cause)
{
    const RmcShotEventPayload *shot = &event->shot;
    const bool shot_code = event->outcome_code >= RMC_SHOT_CHANCE &&
                           event->outcome_code <= RMC_SHOT_WOODWORK;

    if (shot->model_version == 0) {
        return !shot_code && neutral_shot(shot);
    }
    if (shot->model_version != RMC_SHOT_MODEL_VERSION || !shot_code ||
        shot->chance_quality > 100 ||
        shot->source_zone.sector < 2 || shot->source_zone.sector > 4 ||
        (unsigned)shot->source_zone.lane >= RMC_LANE_COUNT ||
        shot->context < -20 || shot->context > 20 ||
        shot->duration_seconds == 0 ||
        shot->duration_seconds > event->at_second ||
        event->secondary_actor.value != shot->goalkeeper_id.value) {
        return false;
    }

    const uint64_t period_start = state->period == RMC_MATCH_FIRST_HALF ?
        0 : state->second_half_start_second;
    const uint64_t deadline = period_start + UINT64_C(2700) +
        (state->period == RMC_MATCH_FIRST_HALF ?
         state->first_half_stoppage_seconds : state->second_half_stoppage_seconds);
    if (event->at_second > deadline || event->at_second < period_start ||
        shot->duration_seconds > event->at_second - period_start) {
        return false;
    }

    const RmcLineup *attack;
    const RmcLineup *defence;
    RmcTeamId opponent;
    if (event->team_id.value == state->fixture.home_team.value) {
        attack = &state->home_lineup;
        defence = &state->away_lineup;
        opponent = state->fixture.away_team;
    } else if (event->team_id.value == state->fixture.away_team.value) {
        attack = &state->away_lineup;
        defence = &state->home_lineup;
        opponent = state->fixture.home_team;
    } else {
        return false;
    }

    const RmcPlayer *shooter = find_player(world, event->primary_actor);
    const RmcPlayer *defender = find_player(world, shot->defender_id);
    const RmcPlayer *keeper = find_player(world, shot->goalkeeper_id);
    if (shooter == NULL || defender == NULL || keeper == NULL ||
        shooter->department == RMC_DEPARTMENT_GOALKEEPER ||
        defender->department == RMC_DEPARTMENT_GOALKEEPER ||
        keeper->department != RMC_DEPARTMENT_GOALKEEPER ||
        !in_lineup(attack, event->primary_actor, false) ||
        !in_lineup(defence, shot->defender_id, false) ||
        !in_lineup(defence, shot->goalkeeper_id, true)) {
        return false;
    }

    const uint32_t composure = UINT32_C(1) << RMC_TALENT_COMPOSURE;
    const bool composure_active =
        (shooter->talents.consolidated_mask & composure) != 0 &&
        shot->source_zone.sector == 4 && shot->chance_quality >= 60;
    if (shot->applied_talent_mask != (composure_active ? composure : 0)) {
        return false;
    }

    if (event->type == RMC_EVENT_ACTION) {
        return event->outcome_code == RMC_SHOT_CHANCE &&
               shot->trajectory == RMC_SHOT_TRAJECTORY_NONE &&
               neutral_restart(shot);
    }

    if ((event->type != RMC_EVENT_SHOT && event->type != RMC_EVENT_GOAL) ||
        event->outcome_code == RMC_SHOT_CHANCE || cause == NULL ||
        cause->id != event->id - UINT64_C(1) ||
        !same_chance(event, cause)) {
        return false;
    }
    if (event->type == RMC_EVENT_GOAL) {
        return event->outcome_code == RMC_SHOT_GOAL &&
               cause->type == RMC_EVENT_SHOT &&
               cause->outcome_code == RMC_SHOT_GOAL &&
               shot->trajectory == cause->shot.trajectory &&
               shot->restart_kind == cause->shot.restart_kind &&
               shot->restart_awarded_team.value ==
                   cause->shot.restart_awarded_team.value &&
               shot->restart_zone.sector == cause->shot.restart_zone.sector &&
               shot->restart_zone.lane == cause->shot.restart_zone.lane &&
               shot->restart_disposition == cause->shot.restart_disposition;
    }
    if (cause->type != RMC_EVENT_ACTION ||
        cause->outcome_code != RMC_SHOT_CHANCE) {
        return false;
    }

    switch (event->outcome_code) {
    case RMC_SHOT_GOAL:
    case RMC_SHOT_SAVED_HELD:
    case RMC_SHOT_SAVED_REBOUND:
        if (shot->trajectory != RMC_SHOT_TRAJECTORY_ON_TARGET) {
            return false;
        }
        break;
    case RMC_SHOT_WIDE:
        if (shot->trajectory != RMC_SHOT_TRAJECTORY_WIDE) {
            return false;
        }
        break;
    case RMC_SHOT_WOODWORK:
        if (shot->trajectory != RMC_SHOT_TRAJECTORY_WOODWORK) {
            return false;
        }
        break;
    case RMC_SHOT_BLOCKED:
        if (shot->trajectory < RMC_SHOT_TRAJECTORY_ON_TARGET ||
            shot->trajectory > RMC_SHOT_TRAJECTORY_WOODWORK) {
            return false;
        }
        break;
    default:
        return false;
    }

    if (event->outcome_code == RMC_SHOT_GOAL ||
        event->outcome_code == RMC_SHOT_WIDE) {
        const bool goal = event->outcome_code == RMC_SHOT_GOAL;
        return shot->restart_kind ==
                   (goal ? RMC_RESTART_KICKOFF : RMC_RESTART_GOAL_KICK) &&
               shot->restart_awarded_team.value == opponent.value &&
               shot->restart_zone.sector == (goal ? 2 : 0) &&
               shot->restart_zone.lane == RMC_LANE_CENTRE &&
               shot->restart_disposition ==
                   (event->at_second == deadline ?
                    RMC_RESTART_SUPPRESSED_PERIOD_END : RMC_RESTART_PENDING);
    }
    return neutral_restart(shot);
}

static const RmcMatchEvent *batch_cause(
    const RmcMatchEventLog *log, const RmcMatchEvent *events, uint64_t id)
{
    if (id == 0) {
        return NULL;
    }
    if (id <= log->count) {
        return &log->events[(size_t)(id - UINT64_C(1))];
    }
    return &events[(size_t)(id - (uint64_t)log->count - UINT64_C(1))];
}

bool rmc_match_event_log_append_batch(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcMatchEvent *events,
    size_t event_count)
{
    if (world == NULL || state == NULL || log == NULL || events == NULL ||
        !rmc_match_state_valid(world, state) ||
        (state->period != RMC_MATCH_FIRST_HALF &&
         state->period != RMC_MATCH_SECOND_HALF) ||
        log->events == NULL || log->capacity == 0 || event_count == 0 ||
        log->capacity > SIZE_MAX / sizeof(log->events[0]) ||
        log->count > log->capacity ||
        event_count > log->capacity - log->count ||
        log->count > UINT64_MAX ||
        event_count > UINT64_MAX - log->count ||
        log->match_id.value != state->fixture.id.value) {
        return false;
    }

    uint32_t previous_second = log->count == 0 ? 0 :
        log->events[log->count - 1].at_second;
    for (size_t i = 0; i < event_count; ++i) {
        const RmcMatchEvent *event = &events[i];
        if (event->match_id.value != log->match_id.value ||
            event->id != (uint64_t)log->count + (uint64_t)i + UINT64_C(1) ||
            (unsigned)event->type >= RMC_EVENT_TYPE_COUNT ||
            event->at_second > state->elapsed_seconds ||
            event->at_second < previous_second ||
            event->cause_event_id >= event->id ||
            !actor_on_field(state, event->primary_actor) ||
            !actor_on_field(state, event->secondary_actor) ||
            (event->team_id.value != 0 &&
             event->team_id.value != state->fixture.home_team.value &&
             event->team_id.value != state->fixture.away_team.value)) {
            return false;
        }
        const RmcMatchEvent *cause =
            batch_cause(log, events, event->cause_event_id);
        if ((cause != NULL &&
             (cause->id != event->cause_event_id ||
              cause->match_id.value != event->match_id.value ||
              cause->at_second > event->at_second)) ||
            !shot_valid(world, state, event, cause)) {
            return false;
        }
        previous_second = event->at_second;
    }

    /* All validation precedes this one non-fallible storage mutation. */
    memmove(&log->events[log->count], events,
            event_count * sizeof(events[0]));
    log->count += event_count;
    return true;
}

bool rmc_match_event_log_append(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcMatchEvent *event)
{
    return rmc_match_event_log_append_batch(world, state, log, event, 1);
}
