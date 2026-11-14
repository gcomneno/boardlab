#include "match_opening_kickoff.h"
#include "match_carry_action.h"
#include "match_cross_action.h"
#include "match_dribble_action.h"
#include "match_recovery.h"
#include "match_stoppage.h"
#include "match_tackle_action.h"

#include <stdlib.h>

static RmcTeamId opposite(const RmcMatchState *state, RmcTeamId team)
{
    return team.value == state->fixture.home_team.value ?
        state->fixture.away_team : state->fixture.home_team;
}

static const RmcPlayer *participant(
    const RmcWorldCore *world, const RmcMatchState *state,
    RmcTeamId team, RmcPlayerId actor, int role)
{
    const RmcLineup *lineup;
    if (team.value == state->fixture.home_team.value) {
        lineup = &state->home_lineup;
    } else if (team.value == state->fixture.away_team.value) {
        lineup = &state->away_lineup;
    } else {
        return NULL;
    }
    bool assigned = false;
    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (actor.value != 0 && lineup->players[i].value == actor.value &&
            (role < 0 || (lineup->slots[i] == RMC_SLOT_POR) == (role == 1))) {
            assigned = true;
        }
    }
    if (!assigned) {
        return NULL;
    }
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (world->teams[i].id.value != team.value ||
            world->rosters[i].team_id.value != team.value) {
            continue;
        }
        for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
            const RmcPlayer *p = &world->rosters[i].players[j];
            if (p->id.value == actor.value && p->team_id.value == team.value &&
                (role < 0 || (p->department == RMC_DEPARTMENT_GOALKEEPER) ==
                              (role == 1))) {
                return p;
            }
        }
    }
    return NULL;
}

static bool empty_possession(const RmcMatchPossession *p)
{
    return p->team_id.value == 0 && p->initiator.value == 0 &&
        p->controller.value == 0 && p->zone.sector == 0 &&
        p->zone.lane == RMC_LANE_LEFT && p->phase == RMC_PHASE_BUILDUP &&
        p->action_count == 0 && p->elapsed_seconds == 0;
}

/* Neutral Shot payloads, IDs, chronology and all Shot-specific causal fields
 * are checked by the existing logger on private storage before this grammar.
 * This grammar adds the lifecycle and mandatory sporting-chain constraints
 * which the generic logger deliberately does not supply. It never resamples.
 */
static bool history_order_valid(
    const RmcWorldCore *world, const RmcMatchState *state,
    const RmcMatchEventLog *log, uint32_t deadline)
{
    const RmcMatchEvent *start = &log->events[0];
    const RmcMatchEvent *opening = &log->events[1];
    const RmcMatchEvent *end = &log->events[log->count - 1];
    if (start->type != RMC_EVENT_PERIOD_START || start->at_second != 0 ||
        start->primary_actor.value != 0 || start->secondary_actor.value != 0 ||
        start->outcome_code != 0 || start->cause_event_id != 0 ||
        opening->type != RMC_EVENT_RESTART || opening->at_second != 0 ||
        opening->outcome_code != RMC_RESTART_KICKOFF ||
        opening->team_id.value != start->team_id.value ||
        opening->secondary_actor.value != 0 || opening->cause_event_id != start->id ||
        participant(world, state, start->team_id, opening->primary_actor, 0) == NULL ||
        end->type != RMC_EVENT_PERIOD_END || end->at_second != deadline ||
        end->team_id.value != 0 || end->primary_actor.value != 0 ||
        end->secondary_actor.value != 0 || end->outcome_code != 0 ||
        end->cause_event_id != 0) {
        return false;
    }

    RmcTeamId team = start->team_id;
    RmcPlayerId controller = opening->primary_actor;
    RmcTacticalZone zone = {2, RMC_LANE_CENTRE};
    bool loose = false;
    const RmcMatchEvent *pending = NULL;
    const RmcMatchEvent *previous = opening;
    for (size_t i = 2; i < log->count - 1; ++i) {
        const RmcMatchEvent *e = &log->events[i];
        const RmcTeamId opponent = opposite(state, team);
        if (pending != NULL) {
            if (e->type != RMC_EVENT_RESTART ||
                pending->shot.restart_disposition != RMC_RESTART_PENDING ||
                e->cause_event_id != pending->id ||
                e->at_second != pending->at_second ||
                e->outcome_code != (uint32_t)pending->shot.restart_kind ||
                e->team_id.value != pending->shot.restart_awarded_team.value ||
                e->secondary_actor.value != 0 ||
                participant(world, state, e->team_id, e->primary_actor,
                    e->outcome_code == RMC_RESTART_GOAL_KICK ? 1 : 0) == NULL) {
                return false;
            }
            team = e->team_id;
            controller = e->primary_actor;
            zone = pending->shot.restart_zone;
            pending = NULL;
        } else if (e->type == RMC_EVENT_RECOVERY) {
            const bool retained = e->outcome_code == RMC_RECOVERY_RETAINED;
            if (!loose || e->at_second <= previous->at_second ||
                e->cause_event_id != previous->id ||
                (!retained && e->outcome_code != RMC_RECOVERY_TURNOVER) ||
                e->team_id.value != (retained ? team.value : opponent.value) ||
                participant(world, state, e->team_id, e->primary_actor, -1) == NULL ||
                participant(world, state, opposite(state, e->team_id),
                    e->secondary_actor, -1) == NULL) {
                return false;
            }
            if (!retained) {
                zone.sector = (uint8_t)(4u - zone.sector);
            }
            team = e->team_id;
            controller = e->primary_actor;
            loose = false;
        } else if (e->type == RMC_EVENT_ACTION) {
            if (loose || e->at_second <= previous->at_second ||
                e->cause_event_id != 0) {
                return false;
            }
            const bool tackle = e->outcome_code >= RMC_TACKLE_OUTCOME_RETAINED &&
                                e->outcome_code <= RMC_TACKLE_OUTCOME_LOOSE_BALL;
            if (tackle) {
                const bool won = e->outcome_code == RMC_TACKLE_OUTCOME_WON;
                if ((controller.value != 0 && e->secondary_actor.value != controller.value) ||
                    participant(world, state, team, e->secondary_actor, -1) == NULL ||
                    participant(world, state, opponent, e->primary_actor, -1) == NULL ||
                    e->team_id.value != (won ? opponent.value : team.value)) {
                    return false;
                }
                if (won) {
                    team = opponent;
                    controller = e->primary_actor;
                    zone.sector = (uint8_t)(4u - zone.sector);
                }
                loose = e->outcome_code == RMC_TACKLE_OUTCOME_LOOSE_BALL;
            } else {
                if ((controller.value != 0 && e->primary_actor.value != controller.value) ||
                    participant(world, state, team, e->primary_actor, -1) == NULL) {
                    return false;
                }
                switch (e->outcome_code) {
                case RMC_PASS_OUTCOME_COMPLETED:
                case RMC_PASS_OUTCOME_LOOSE_BALL:
                    if (e->team_id.value != team.value ||
                        e->primary_actor.value == e->secondary_actor.value ||
                        participant(world, state, team, e->secondary_actor, -1) == NULL) {
                        return false;
                    }
                    controller = e->secondary_actor;
                    loose = e->outcome_code == RMC_PASS_OUTCOME_LOOSE_BALL;
                    break;
                case RMC_CARRY_OUTCOME_ADVANCED:
                case RMC_CARRY_OUTCOME_CONTAINED:
                case RMC_DRIBBLE_OUTCOME_SUCCESS:
                case RMC_DRIBBLE_OUTCOME_CONTAINED:
                    if (zone.sector >= 4 || e->team_id.value != team.value ||
                        participant(world, state, opponent, e->secondary_actor, -1) == NULL) {
                        return false;
                    }
                    controller = e->primary_actor;
                    if (e->outcome_code == RMC_CARRY_OUTCOME_ADVANCED ||
                        e->outcome_code == RMC_DRIBBLE_OUTCOME_SUCCESS) {
                        ++zone.sector;
                    }
                    break;
                case RMC_CROSS_OUTCOME_COMPLETED:
                case RMC_CROSS_OUTCOME_INTERCEPTED:
                case RMC_CROSS_OUTCOME_LOOSE_BALL:
                    /* No canonical centre-opening action creates a lateral
                     * lane, but retain the actual cross envelope semantics. */
                    if ((zone.sector != 2 && zone.sector != 3) ||
                        zone.lane == RMC_LANE_CENTRE ||
                        e->primary_actor.value == e->secondary_actor.value ||
                        participant(world, state, team, e->secondary_actor, -1) == NULL ||
                        e->team_id.value != (e->outcome_code == RMC_CROSS_OUTCOME_INTERCEPTED ?
                            opponent.value : team.value)) {
                        return false;
                    }
                    ++zone.sector;
                    zone.lane = RMC_LANE_CENTRE;
                    controller = e->secondary_actor;
                    if (e->outcome_code == RMC_CROSS_OUTCOME_INTERCEPTED) {
                        team = opponent;
                        zone.sector = (uint8_t)(4u - zone.sector);
                        /* The interceptor is not recorded in this envelope. */
                        controller = (RmcPlayerId){0};
                    }
                    loose = e->outcome_code == RMC_CROSS_OUTCOME_LOOSE_BALL;
                    break;
                case RMC_SHOT_CHANCE: {
                    if (e->team_id.value != team.value ||
                        e->shot.model_version != RMC_SHOT_MODEL_VERSION ||
                        e->shot.source_zone.sector != zone.sector ||
                        e->shot.source_zone.lane != zone.lane ||
                        e->shot.duration_seconds > e->at_second - previous->at_second ||
                        i + 1 >= log->count - 1) {
                        return false;
                    }
                    const RmcMatchEvent *shot = &log->events[++i];
                    if (shot->type != RMC_EVENT_SHOT ||
                        shot->shot.model_version != RMC_SHOT_MODEL_VERSION ||
                        shot->outcome_code < RMC_SHOT_GOAL ||
                        shot->outcome_code > RMC_SHOT_WOODWORK ||
                        shot->cause_event_id != e->id) {
                        return false;
                    }
                    e = shot;
                    if (shot->outcome_code == RMC_SHOT_GOAL) {
                        if (i + 1 >= log->count - 1 ||
                            log->events[i + 1].type != RMC_EVENT_GOAL) {
                            return false;
                        }
                        e = &log->events[++i];
                        if (e->shot.model_version != RMC_SHOT_MODEL_VERSION ||
                            e->outcome_code != RMC_SHOT_GOAL ||
                            e->cause_event_id != shot->id) {
                            return false;
                        }
                    }
                    if (shot->outcome_code == RMC_SHOT_GOAL ||
                        shot->outcome_code == RMC_SHOT_WIDE) {
                        pending = shot;
                    } else if (shot->outcome_code == RMC_SHOT_SAVED_HELD) {
                        team = opponent;
                        controller = shot->shot.goalkeeper_id;
                        zone = (RmcTacticalZone){0, RMC_LANE_CENTRE};
                    } else {
                        loose = true;
                        if (shot->outcome_code != RMC_SHOT_BLOCKED) {
                            zone = (RmcTacticalZone){4, RMC_LANE_CENTRE};
                        }
                    }
                    break;
                }
                default:
                    return false;
                }
            }
        } else {
            /* Includes stray/duplicated starts, endings, SHOTs and GOALs. */
            return false;
        }
        previous = e;
    }
    return pending == NULL ||
        (pending->at_second == deadline &&
         pending->shot.restart_disposition == RMC_RESTART_SUPPRESSED_PERIOD_END);
}

static bool first_history_valid(
    const RmcWorldCore *world, const RmcMatchState *state,
    const RmcMatchEventLog *log, uint32_t deadline)
{
    if (log->count < 3) {
        return false;
    }
    RmcMatchEvent *storage = malloc(log->count * sizeof(*storage));
    if (storage == NULL) {
        return false;
    }
    RmcMatchEventLog check = {
        .events = storage, .capacity = log->count, .match_id = log->match_id
    };
    RmcMatchState first = *state;
    first.period = RMC_MATCH_FIRST_HALF;
    first.elapsed_seconds = deadline;
    first.second_half_start_second = 0;
    first.second_half_stoppage_seconds = 0;
    const bool valid = rmc_match_event_log_append_batch(
        world, &first, &check, log->events, log->count) &&
        history_order_valid(world, &first, log, deadline);
    free(storage);
    return valid;
}

bool rmc_match_opening_kickoff_apply(
    const RmcWorldCore *world, RmcMatchState *state, RmcMatchEventLog *log,
    const RmcOpeningKickoffAction *action, RmcOpeningKickoffResult *result)
{
    if (world == NULL || state == NULL || log == NULL ||
        action == NULL || result == NULL || !rmc_match_state_valid(world, state) ||
        (state->period != RMC_MATCH_FIRST_HALF && state->period != RMC_MATCH_SECOND_HALF) ||
        action->opening_period != state->period ||
        state->mode != RMC_MATCH_RESTART_PENDING ||
        !empty_possession(&state->possession) ||
        log->events == NULL || log->capacity > SIZE_MAX / sizeof(log->events[0]) ||
        log->count > log->capacity || log->capacity - log->count < 2 ||
        log->count > UINT64_MAX - UINT64_C(2) ||
        log->match_id.value != state->fixture.id.value ||
        participant(world, state, action->opening_team, action->taker, 0) == NULL) {
        return false;
    }
    uint64_t cause = 0;
    if (state->period == RMC_MATCH_FIRST_HALF) {
        if (state->elapsed_seconds != 0 || log->count != 0 ||
            state->home_goals != 0 || state->away_goals != 0 ||
            state->first_half_stoppage_seconds != 0 ||
            state->second_half_stoppage_seconds != 0) {
            return false;
        }
    } else {
        uint32_t deadline;
        if (!rmc_stoppage_deadline(0, state->first_half_stoppage_seconds, &deadline) ||
            state->elapsed_seconds != state->second_half_start_second ||
            state->elapsed_seconds != deadline || state->second_half_stoppage_seconds != 0 ||
            !first_history_valid(world, state, log, deadline)) {
            return false;
        }
        /* Only authenticated history may select the opposite beneficiary. */
        if (action->opening_team.value != opposite(state, log->events[0].team_id).value) {
            return false;
        }
        cause = log->events[log->count - 1].id;
    }

    RmcMatchState next = *state;
    next.mode = RMC_MATCH_BALL_IN_PLAY;
    next.possession = (RmcMatchPossession){0};
    next.possession.team_id = action->opening_team;
    next.possession.initiator = action->taker;
    next.possession.controller = action->taker;
    next.possession.zone = (RmcTacticalZone){2, RMC_LANE_CENTRE};
    next.possession.phase = RMC_PHASE_BUILDUP;
    RmcSequenceDecision decision;
    if (!rmc_match_state_valid(world, &next) ||
        !rmc_match_sequence_decide(world, &next, &decision) ||
        decision != RMC_SEQUENCE_ALLOW_ACTION) {
        return false;
    }
    RmcMatchEvent events[2] = {{0}};
    events[0].id = (uint64_t)log->count + UINT64_C(1);
    events[0].match_id = state->fixture.id;
    events[0].at_second = state->elapsed_seconds;
    events[0].type = RMC_EVENT_PERIOD_START;
    events[0].team_id = action->opening_team;
    events[0].cause_event_id = cause;
    events[1] = events[0];
    ++events[1].id;
    events[1].type = RMC_EVENT_RESTART;
    events[1].primary_actor = action->taker;
    events[1].outcome_code = RMC_RESTART_KICKOFF;
    events[1].cause_event_id = events[0].id;
    const RmcOpeningKickoffResult next_result = {
        .period_start_event_id = events[0].id, .restart_event_id = events[1].id,
        .awarded_team = action->opening_team, .taker = action->taker
    };
    /* First caller-owned write and last fallible operation. */
    if (!rmc_match_event_log_append_batch(world, &next, log, events, 2)) {
        return false;
    }
    *state = next;
    *result = next_result;
    return true;
}
