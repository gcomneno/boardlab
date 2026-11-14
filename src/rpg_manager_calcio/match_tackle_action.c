#include "match_tackle_action.h"
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

bool rmc_match_tackle_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcTackleAction *action,
    RmcTackleResult *result)
{
    if (world == NULL || state == NULL ||
        match_rng == NULL || log == NULL ||
        action == NULL || result == NULL ||
        !rmc_match_state_valid(world, state) ||
        state->mode != RMC_MATCH_BALL_IN_PLAY ||
        state->possession.team_id.value == 0 ||
        action->team_id.value == 0 ||
        action->team_id.value ==
            state->possession.team_id.value ||
        (action->team_id.value != state->fixture.home_team.value &&
         action->team_id.value != state->fixture.away_team.value) ||
        action->carrier.value == 0 ||
        action->carrier.value !=
            state->possession.controller.value ||
        action->defender.value == 0 ||
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

    if (!lineup_contains(own_lineup, action->carrier) ||
        !lineup_contains(opponent_lineup, action->defender)) {
        return false;
    }

    const RmcPlayer *carrier = find_player(
        world, state->possession.team_id, action->carrier
    );

    const RmcPlayer *defender = find_player(
        world, opponent_team, action->defender
    );

    if (carrier == NULL || defender == NULL) {
        return false;
    }

    const int32_t attack =
        ((int32_t)carrier->attributes[RMC_ATTR_CONTROLLO] +
         (int32_t)carrier->attributes[RMC_ATTR_FORZA]) / 2;

    const int32_t opposition =
        ((int32_t)defender->attributes[RMC_ATTR_CONTRASTO] +
         (int32_t)defender->attributes[RMC_ATTR_POSIZIONAMENTO]) / 2;

    uint32_t probability = 0;

    if (!rmc_match_probability(
            opposition, attack,
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
    const RmcTackleOutcome outcome = draw < probability
        ? RMC_TACKLE_OUTCOME_WON
        : (draw < loose_limit ? RMC_TACKLE_OUTCOME_LOOSE_BALL
                              : RMC_TACKLE_OUTCOME_RETAINED);

    if (outcome == RMC_TACKLE_OUTCOME_WON) {
        const RmcTacticalZone old_zone = next.possession.zone;

        next.possession = (RmcMatchPossession){0};
        next.possession.team_id = action->team_id;
        next.possession.initiator = action->defender;
        next.possession.controller = action->defender;
        next.possession.zone.sector =
            (uint8_t)(RMC_FIELD_SECTORS - 1u - old_zone.sector);
        next.possession.zone.lane = old_zone.lane;
        next.possession.phase = RMC_PHASE_TRANSITION;
    } else {
        if (outcome == RMC_TACKLE_OUTCOME_LOOSE_BALL) {
            next.possession.controller = (RmcPlayerId){0};
        }
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
    event.primary_actor = action->defender;
    event.secondary_actor = action->carrier;
    event.outcome_code = outcome;

    /* Append validates before writing; no fallible work follows it. */
    if (!rmc_match_event_log_append(
            world, &next, log, &event)) {
        return false;
    }

    *state = next;
    *match_rng = next_rng;

    const RmcTackleResult next_result = {
        .probability = probability,
        .random_draw = draw,
        .outcome = outcome
    };

    *result = next_result;
    return true;
}
