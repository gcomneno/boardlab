#include "match_cross_action.h"
#include "match_clock.h"

#include <stddef.h>
#include <stdint.h>

static bool lineup_contains(
    const RmcLineup *lineup,
    RmcPlayerId player)
{
    if (player.value == 0) {
        return false;
    }

    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (lineup->players[i].value == player.value) {
            return true;
        }
    }

    return false;
}

static const RmcPlayer *find_player(
    const RmcWorldCore *world,
    RmcTeamId team,
    RmcPlayerId player)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (world->rosters[i].team_id.value != team.value) {
            continue;
        }

        for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
            const RmcPlayer *candidate =
                &world->rosters[i].players[j];

            if (candidate->id.value == player.value) {
                return candidate;
            }
        }
    }

    return NULL;
}

/* Validate the stored envelopes using the event-log policy before drawing. */
static bool log_valid(
    const RmcMatchState *state,
    const RmcMatchEventLog *log)
{
    uint32_t previous_second = 0;

    for (size_t i = 0; i < log->count; ++i) {
        const RmcMatchEvent *event = &log->events[i];
        const RmcPlayerId actors[] = {
            event->primary_actor, event->secondary_actor
        };

        if (event->id != (uint64_t)i + UINT64_C(1) ||
            event->match_id.value != state->fixture.id.value ||
            (unsigned)event->type >= RMC_EVENT_TYPE_COUNT ||
            event->at_second < previous_second ||
            event->at_second > state->elapsed_seconds ||
            event->cause_event_id >= event->id ||
            (event->team_id.value != 0 &&
             event->team_id.value != state->fixture.home_team.value &&
             event->team_id.value != state->fixture.away_team.value)) {
            return false;
        }

        for (size_t j = 0; j < sizeof(actors) / sizeof(actors[0]); ++j) {
            if (actors[j].value != 0 &&
                !lineup_contains(&state->home_lineup, actors[j]) &&
                !lineup_contains(&state->away_lineup, actors[j])) {
                return false;
            }
        }
        previous_second = event->at_second;
    }

    return true;
}

bool rmc_match_cross_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcCrossAction *action,
    RmcCrossResult *result)
{
    if (world == NULL || state == NULL ||
        match_rng == NULL || log == NULL ||
        action == NULL || result == NULL ||
        !rmc_match_state_valid(world, state) ||
        state->mode != RMC_MATCH_BALL_IN_PLAY ||
        state->possession.team_id.value == 0 ||
        action->team_id.value == 0 ||
        action->team_id.value !=
            state->possession.team_id.value ||
        (action->team_id.value != state->fixture.home_team.value &&
         action->team_id.value != state->fixture.away_team.value) ||
        action->crosser.value == 0 ||
        action->crosser.value !=
            state->possession.controller.value ||
        action->recipient.value == 0 ||
        action->recipient.value == action->crosser.value ||
        action->defender.value == 0 ||
        (match_rng->increment & UINT64_C(1)) == 0 ||
        (state->possession.zone.sector != 2 &&
         state->possession.zone.sector != 3) ||
        (state->possession.zone.lane != RMC_LANE_LEFT &&
         state->possession.zone.lane != RMC_LANE_RIGHT) ||
        action->context < -20 || action->context > 20 ||
        action->duration_seconds == 0 ||
        log->events == NULL ||
        log->count >= log->capacity ||
        log->count >= UINT64_MAX ||
        log->match_id.value != state->fixture.id.value) {
        return false;
    }

    if (!log_valid(state, log)) {
        return false;
    }

    RmcSequenceDecision decision;

    if (!rmc_match_sequence_decide(
            world, state, &decision) ||
        decision != RMC_SEQUENCE_ALLOW_ACTION) {
        return false;
    }

    const bool home =
        state->possession.team_id.value ==
        state->fixture.home_team.value;

    const RmcLineup *own_lineup =
        home ? &state->home_lineup : &state->away_lineup;

    const RmcLineup *opponent_lineup =
        home ? &state->away_lineup : &state->home_lineup;

    const RmcTeamId opponent_team =
        home ? state->fixture.away_team :
               state->fixture.home_team;

    if (!lineup_contains(own_lineup, action->crosser) ||
        !lineup_contains(own_lineup, action->recipient) ||
        !lineup_contains(opponent_lineup, action->defender)) {
        return false;
    }

    const RmcPlayer *crosser = find_player(
        world, state->possession.team_id, action->crosser
    );

    const RmcPlayer *defender = find_player(
        world, opponent_team, action->defender
    );

    if (crosser == NULL || defender == NULL ||
        find_player(world, action->team_id, action->recipient) == NULL) {
        return false;
    }

    const int32_t attack =
        ((int32_t)crosser->attributes[RMC_ATTR_CROSS] +
         (int32_t)crosser->attributes[RMC_ATTR_VISIONE]) / 2;

    const int32_t opposition =
        ((int32_t)defender->attributes[RMC_ATTR_MARCATURA] +
         (int32_t)defender->attributes[RMC_ATTR_POSIZIONAMENTO]) / 2;

    uint32_t probability = 0;

    if (!rmc_match_probability(
            attack, opposition,
            action->context, &probability)) {
        return false;
    }

    RmcMatchState next = *state;

    if (!rmc_match_clock_advance(
            world, &next, action->duration_seconds)) {
        return false;
    }

    RmcPcg32 next_rng = *match_rng;
    uint32_t draw = 0;

    if (!rmc_pcg32_bounded(
            &next_rng, RMC_PROBABILITY_SCALE, &draw)) {
        return false;
    }

    const uint32_t loose_limit =
        probability > RMC_PROBABILITY_SCALE - UINT32_C(1000)
            ? RMC_PROBABILITY_SCALE : probability + UINT32_C(1000);
    const RmcCrossOutcome outcome = draw < probability
        ? RMC_CROSS_OUTCOME_COMPLETED
        : (draw < loose_limit ? RMC_CROSS_OUTCOME_LOOSE_BALL
                              : RMC_CROSS_OUTCOME_INTERCEPTED);
    const RmcTacticalZone destination = {
        .sector = (uint8_t)(state->possession.zone.sector + 1u),
        .lane = RMC_LANE_CENTRE
    };

    if (outcome == RMC_CROSS_OUTCOME_INTERCEPTED) {
        next.possession = (RmcMatchPossession){0};
        next.possession.team_id = opponent_team;
        next.possession.initiator = action->defender;
        next.possession.controller = action->defender;
        next.possession.zone.sector =
            (uint8_t)(RMC_FIELD_SECTORS - 1u - destination.sector);
        next.possession.zone.lane = destination.lane;
        next.possession.phase = RMC_PHASE_TRANSITION;
    } else {
        next.possession.controller = outcome == RMC_CROSS_OUTCOME_COMPLETED
            ? action->recipient : (RmcPlayerId){0};
        next.possession.zone = destination;
        next.possession.phase = destination.sector == 4
            ? RMC_PHASE_CHANCE : RMC_PHASE_ATTACKING_DEVELOPMENT;
        ++next.possession.action_count;
    }

    if (!rmc_match_state_valid(world, &next)) {
        return false;
    }

    RmcMatchEvent event = {0};

    event.id = (uint64_t)log->count + UINT64_C(1);
    event.match_id = state->fixture.id;
    event.at_second = next.elapsed_seconds;
    event.type = RMC_EVENT_ACTION;
    event.team_id = next.possession.team_id;
    event.primary_actor = action->crosser;
    event.secondary_actor = action->recipient;
    event.outcome_code = outcome;

    /* Append validates before writing; no fallible work follows it. */
    if (!rmc_match_event_log_append(
            world, &next, log, &event)) {
        return false;
    }

    *state = next;
    *match_rng = next_rng;

    const RmcCrossResult next_result = {
        .probability = probability,
        .random_draw = draw,
        .outcome = outcome
    };

    *result = next_result;
    return true;
}
