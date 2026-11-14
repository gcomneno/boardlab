#include "match_carry_action.h"
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

bool rmc_match_carry_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcCarryAction *action,
    RmcCarryResult *result)
{
    if (world == NULL || state == NULL ||
        match_rng == NULL || log == NULL ||
        action == NULL || result == NULL ||
        !rmc_match_state_valid(world, state) ||
        state->mode != RMC_MATCH_BALL_IN_PLAY ||
        state->possession.team_id.value == 0 ||
        action->team_id.value !=
            state->possession.team_id.value ||
        action->carrier.value == 0 ||
        action->carrier.value !=
            state->possession.controller.value ||
        action->opponent.value == 0 ||
        action->context < -20 || action->context > 20 ||
        action->duration_seconds == 0 ||
        state->possession.zone.sector >=
            RMC_FIELD_SECTORS - 1 ||
        log->events == NULL ||
        log->count >= log->capacity ||
        log->count >= UINT64_MAX ||
        log->match_id.value != state->fixture.id.value) {
        return false;
    }

    RmcSequenceDecision decision;

    if (!rmc_match_sequence_decide(
            world, state, &decision) ||
        decision != RMC_SEQUENCE_ALLOW_ACTION) {
        return false;
    }

    const bool home =
        action->team_id.value ==
        state->fixture.home_team.value;

    const RmcLineup *own_lineup =
        home ? &state->home_lineup : &state->away_lineup;

    const RmcLineup *opponent_lineup =
        home ? &state->away_lineup : &state->home_lineup;

    const RmcTeamId opponent_team =
        home ? state->fixture.away_team :
               state->fixture.home_team;

    if (!lineup_contains(own_lineup, action->carrier) ||
        !lineup_contains(opponent_lineup, action->opponent)) {
        return false;
    }

    const RmcPlayer *carrier = find_player(
        world, action->team_id, action->carrier
    );

    const RmcPlayer *opponent = find_player(
        world, opponent_team, action->opponent
    );

    if (carrier == NULL || opponent == NULL) {
        return false;
    }

    const int32_t attack =
        ((int32_t)carrier->attributes[RMC_ATTR_CONTROLLO] +
         (int32_t)carrier->attributes[RMC_ATTR_VELOCITA]) / 2;

    const int32_t opposition =
        ((int32_t)opponent->attributes[RMC_ATTR_CONTRASTO] +
         (int32_t)opponent->attributes[RMC_ATTR_POSIZIONAMENTO]) / 2;

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

    const bool advanced = draw < probability;

    if (advanced) {
        ++next.possession.zone.sector;
    }

    ++next.possession.action_count;

    if (!rmc_match_state_valid(world, &next)) {
        return false;
    }

    RmcMatchEvent event = {0};

    event.id = (uint64_t)log->count + UINT64_C(1);
    event.match_id = state->fixture.id;
    event.at_second = next.elapsed_seconds;
    event.type = RMC_EVENT_ACTION;
    event.team_id = action->team_id;
    event.primary_actor = action->carrier;
    event.secondary_actor = action->opponent;
    event.outcome_code = advanced
        ? RMC_CARRY_OUTCOME_ADVANCED
        : RMC_CARRY_OUTCOME_CONTAINED;

    if (!rmc_match_event_log_append(
            world, &next, log, &event)) {
        return false;
    }

    *state = next;
    *match_rng = next_rng;

    const RmcCarryResult next_result = {
        .probability = probability,
        .random_draw = draw,
        .outcome = advanced
            ? RMC_CARRY_OUTCOME_ADVANCED
            : RMC_CARRY_OUTCOME_CONTAINED
    };

    *result = next_result;
    return true;
}
