#include "match_restart_action.h"
#include "match_sequence.h"
#include "match_stoppage.h"

#include <stdlib.h>

/* Reuse the qualified schema validator on private storage. Historical shots
 * retain their own half's deadline, even when the active half is the second.
 * Current-lineup requirements are intentionally retained for the full prefix.
 */
static bool prefix_valid(
    const RmcWorldCore *world, const RmcMatchState *state,
    const RmcMatchEventLog *log)
{
    RmcMatchEvent *storage = malloc(log->count * sizeof(*storage));
    if (storage == NULL) {
        return false;
    }
    RmcMatchEventLog check = {
        .events = storage, .capacity = log->count, .match_id = log->match_id
    };
    bool valid = true;
    if (state->period == RMC_MATCH_SECOND_HALF) {
        uint32_t first_deadline;
        valid = rmc_stoppage_deadline(
            0, state->first_half_stoppage_seconds, &first_deadline);
        size_t first_count = 0;
        if (valid) {
            while (first_count < log->count &&
                   log->events[first_count].at_second <= first_deadline) {
                ++first_count;
            }
        }
        if (valid && first_count != 0) {
            RmcMatchState first = *state;
            first.period = RMC_MATCH_FIRST_HALF;
            first.elapsed_seconds = first_deadline;
            first.second_half_start_second = 0;
            first.second_half_stoppage_seconds = 0;
            first.possession = (RmcMatchPossession){0};
            valid = rmc_match_event_log_append_batch(
                world, &first, &check, log->events, first_count);
        }
    }
    if (valid && check.count < log->count) {
        valid = rmc_match_event_log_append_batch(
            world, state, &check, &log->events[check.count],
            log->count - check.count);
    }
    free(storage);
    return valid;
}

static bool taker_valid(
    const RmcWorldCore *world, const RmcMatchState *state,
    const RmcRestartAction *action)
{
    const RmcLineup *lineup;
    if (action->awarded_team.value == state->fixture.home_team.value) {
        lineup = &state->home_lineup;
    } else if (action->awarded_team.value == state->fixture.away_team.value) {
        lineup = &state->away_lineup;
    } else {
        return false;
    }
    const bool goalkeeper = action->restart_kind == RMC_RESTART_GOAL_KICK;
    bool assigned = false;
    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (lineup->players[i].value == action->taker.value &&
            (lineup->slots[i] == RMC_SLOT_POR) == goalkeeper) {
            assigned = true;
        }
    }
    if (!assigned || action->taker.value == 0) {
        return false;
    }
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (world->teams[i].id.value != action->awarded_team.value ||
            world->rosters[i].team_id.value != action->awarded_team.value) {
            continue;
        }
        for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
            const RmcPlayer *p = &world->rosters[i].players[j];
            if (p->id.value == action->taker.value &&
                p->team_id.value == action->awarded_team.value) {
                return (p->department == RMC_DEPARTMENT_GOALKEEPER) ==
                       goalkeeper;
            }
        }
    }
    return false;
}

bool rmc_match_restart_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcRestartAction *action,
    RmcRestartResult *result)
{
    if (world == NULL || state == NULL || log == NULL ||
        action == NULL || result == NULL ||
        !rmc_match_state_valid(world, state) ||
        (state->period != RMC_MATCH_FIRST_HALF &&
         state->period != RMC_MATCH_SECOND_HALF) ||
        state->mode != RMC_MATCH_RESTART_PENDING ||
        (action->restart_kind != RMC_RESTART_KICKOFF &&
         action->restart_kind != RMC_RESTART_GOAL_KICK) ||
        action->source_shot_event_id == 0 ||
        log->events == NULL ||
        log->capacity > SIZE_MAX / sizeof(log->events[0]) ||
        log->count < 2 || log->count >= log->capacity ||
        log->count >= UINT64_MAX ||
        log->match_id.value != state->fixture.id.value) {
        return false;
    }
    const RmcMatchPossession *pos = &state->possession;
    if (pos->team_id.value != 0 || pos->initiator.value != 0 ||
        pos->controller.value != 0 || pos->action_count != 0 ||
        pos->elapsed_seconds != 0 || pos->zone.sector != 0 ||
        pos->zone.lane != RMC_LANE_LEFT || pos->phase != RMC_PHASE_BUILDUP) {
        return false;
    }

    const uint32_t start = state->period == RMC_MATCH_FIRST_HALF ?
        0 : state->second_half_start_second;
    uint32_t deadline;
    if (!rmc_stoppage_deadline(start,
            state->period == RMC_MATCH_FIRST_HALF ?
            state->first_half_stoppage_seconds : state->second_half_stoppage_seconds,
            &deadline) || state->elapsed_seconds >= deadline ||
        !taker_valid(world, state, action)) {
        return false;
    }

    const bool goal = action->restart_kind == RMC_RESTART_KICKOFF;
    if (goal && log->count < 3) {
        return false;
    }
    /* Index only after bounded storage and minimum chain sizes are known. */
    const size_t shot_index = log->count - (goal ? 2u : 1u);
    const RmcMatchEvent *shot = &log->events[shot_index];
    const RmcTeamId opponent =
        shot->team_id.value == state->fixture.home_team.value ?
        state->fixture.away_team : state->fixture.home_team;
    if (shot->type != RMC_EVENT_SHOT ||
        shot->id != (uint64_t)shot_index + UINT64_C(1) ||
        shot->id != action->source_shot_event_id ||
        shot->outcome_code != (goal ? RMC_SHOT_GOAL : RMC_SHOT_WIDE) ||
        shot->shot.model_version != RMC_SHOT_MODEL_VERSION ||
        shot->shot.restart_kind != action->restart_kind ||
        shot->shot.restart_awarded_team.value != action->awarded_team.value ||
        action->awarded_team.value != opponent.value ||
        shot->shot.restart_disposition != RMC_RESTART_PENDING ||
        shot->at_second != state->elapsed_seconds ||
        shot->at_second < start ||
        shot->shot.duration_seconds > shot->at_second - start ||
        (goal && log->events[log->count - 1].type != RMC_EVENT_GOAL) ||
        !prefix_valid(world, state, log)) {
        return false;
    }
    /* Prefix replay enforces adjacent ACTION/SHOT/GOAL, matching envelopes,
     * chance and final payload fields, actors, trajectory, zone and causes.
     */
    RmcMatchState next = *state;
    next.mode = RMC_MATCH_BALL_IN_PLAY;
    next.possession = (RmcMatchPossession){0};
    next.possession.team_id = action->awarded_team;
    next.possession.initiator = action->taker;
    next.possession.controller = action->taker;
    next.possession.zone = shot->shot.restart_zone;
    next.possession.phase = RMC_PHASE_BUILDUP;
    RmcSequenceDecision decision;
    if (!rmc_match_state_valid(world, &next) ||
        !rmc_match_sequence_decide(world, &next, &decision) ||
        decision != RMC_SEQUENCE_ALLOW_ACTION) {
        return false;
    }

    RmcMatchEvent event = {0};
    event.id = (uint64_t)log->count + UINT64_C(1);
    event.match_id = state->fixture.id;
    event.at_second = state->elapsed_seconds;
    event.type = RMC_EVENT_RESTART;
    event.team_id = action->awarded_team;
    event.primary_actor = action->taker;
    event.outcome_code = (uint32_t)action->restart_kind;
    event.cause_event_id = shot->id;
    const RmcRestartResult next_result = {
        .restart_event_id = event.id,
        .source_shot_event_id = shot->id,
        .source_goal_event_id = goal ? log->events[log->count - 1].id : 0
    };
    /* Last fallible operation: no caller-visible write has occurred yet. */
    if (!rmc_match_event_log_append(world, &next, log, &event)) {
        return false;
    }
    *state = next;
    *result = next_result;
    return true;
}
