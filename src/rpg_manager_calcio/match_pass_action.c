#include "match_pass_action.h"
#include "match_clock.h"

#include <stddef.h>
#include <stdint.h>

static bool lineup_contains(
    const RmcLineup *lineup,
    RmcPlayerId player_id)
{
    if (player_id.value == 0) {
        return false;
    }

    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (lineup->players[i].value == player_id.value) {
            return true;
        }
    }

    return false;
}

static const RmcRoster *team_roster(
    const RmcWorldCore *world,
    RmcTeamId team_id)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (world->rosters[i].team_id.value ==
            team_id.value) {
            return &world->rosters[i];
        }
    }

    return NULL;
}

static const RmcPlayer *roster_player(
    const RmcRoster *roster,
    RmcPlayerId player_id)
{
    if (roster == NULL || player_id.value == 0) {
        return NULL;
    }

    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        if (roster->players[i].id.value ==
            player_id.value) {
            return &roster->players[i];
        }
    }

    return NULL;
}

bool rmc_match_pass_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcPassAction *action,
    RmcPassResult *result)
{
    if (world == NULL || state == NULL ||
        match_rng == NULL || log == NULL ||
        action == NULL || result == NULL ||
        action->duration_seconds == 0 ||
        action->context < -20 ||
        action->context > 20 ||
        !rmc_match_state_valid(world, state) ||
        state->mode != RMC_MATCH_BALL_IN_PLAY ||
        action->team_id.value == 0 ||
        action->team_id.value !=
            state->possession.team_id.value ||
        action->passer.value == 0 ||
        action->passer.value !=
            state->possession.controller.value ||
        action->recipient.value == 0 ||
        action->recipient.value == action->passer.value ||
        action->opponent.value == 0) {
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

    if (!lineup_contains(own_lineup, action->passer) ||
        !lineup_contains(own_lineup, action->recipient) ||
        !lineup_contains(opponent_lineup, action->opponent)) {
        return false;
    }

    const RmcPlayer *passer = roster_player(
        team_roster(world, action->team_id),
        action->passer
    );

    const RmcPlayer *opponent = roster_player(
        team_roster(world, opponent_team),
        action->opponent
    );

    if (passer == NULL || opponent == NULL) {
        return false;
    }

    const int32_t attack =
        ((int32_t)passer->attributes[RMC_ATTR_PASSAGGIO] +
         (int32_t)passer->attributes[RMC_ATTR_VISIONE]) / 2;

    const int32_t opposition =
        ((int32_t)opponent->attributes[RMC_ATTR_MARCATURA] +
         (int32_t)opponent->attributes[RMC_ATTR_POSIZIONAMENTO]) / 2;

    uint32_t probability = 0;

    if (!rmc_match_probability(
            attack, opposition,
            action->context, &probability)) {
        return false;
    }

    /*
     * Advance the candidate state before consuming RNG.
     * Invalid time boundaries reject the action here.
     */
    RmcMatchState next = *state;

    if (!rmc_match_clock_advance(
            world, &next, action->duration_seconds)) {
        return false;
    }

    if (log->events == NULL ||
        log->count >= log->capacity ||
        log->count >= UINT64_MAX ||
        log->match_id.value != state->fixture.id.value) {
        return false;
    }

    /*
     * Work on a copy: even a rejected draw or event
     * append cannot advance the caller's PRNG.
     */
    RmcPcg32 next_rng = *match_rng;
    uint32_t draw = 0;

    if (!rmc_pcg32_bounded(
            &next_rng, RMC_PROBABILITY_SCALE, &draw)) {
        return false;
    }

    const bool completed = draw < probability;

    next.possession.controller = completed
        ? action->recipient
        : (RmcPlayerId){0};

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
    event.primary_actor = action->passer;
    event.secondary_actor = action->recipient;
    event.outcome_code = completed
        ? RMC_PASS_OUTCOME_COMPLETED
        : RMC_PASS_OUTCOME_LOOSE_BALL;

    /*
     * Append is the only potentially failing commit
     * operation. No state/RNG mutation precedes it.
     */
    if (!rmc_match_event_log_append(
            world, &next, log, &event)) {
        return false;
    }

    *state = next;
    *match_rng = next_rng;

    const RmcPassResult next_result = {
        .probability = probability,
        .random_draw = draw,
        .outcome = completed
            ? RMC_PASS_OUTCOME_COMPLETED
            : RMC_PASS_OUTCOME_LOOSE_BALL
    };

    *result = next_result;
    return true;
}
