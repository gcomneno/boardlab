#include "match_recovery.h"
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

static const RmcPlayer *active_roster_player(
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

static int32_t recovery_effectiveness(
    const RmcPlayer *player)
{
    return (
        (int32_t)player->attributes[RMC_ATTR_CONTRASTO] +
        (int32_t)player->attributes[RMC_ATTR_POSIZIONAMENTO]
    ) / 2;
}

bool rmc_match_recovery_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcRecoveryAction *action,
    RmcRecoveryResult *result)
{
    if (world == NULL || state == NULL ||
        match_rng == NULL || log == NULL ||
        action == NULL || result == NULL ||
        !rmc_match_state_valid(world, state) ||
        state->mode != RMC_MATCH_BALL_IN_PLAY ||
        state->possession.team_id.value == 0 ||
        state->possession.controller.value != 0 ||
        action->incumbent.value == 0 ||
        action->challenger.value == 0 ||
        action->incumbent.value == action->challenger.value ||
        action->context < -20 || action->context > 20 ||
        action->duration_seconds == 0 ||
        log->events == NULL ||
        log->count == 0 ||
        log->count >= log->capacity ||
        log->count >= UINT64_MAX ||
        log->match_id.value != state->fixture.id.value) {
        return false;
    }

    const RmcMatchEvent *cause =
        &log->events[log->count - 1];

    if (cause->id != (uint64_t)log->count ||
        cause->match_id.value != state->fixture.id.value ||
        cause->type != RMC_EVENT_ACTION ||
        cause->outcome_code != RMC_PASS_OUTCOME_LOOSE_BALL ||
        cause->team_id.value != state->possession.team_id.value ||
        cause->at_second > state->elapsed_seconds) {
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

    const RmcLineup *incumbent_lineup =
        home ? &state->home_lineup : &state->away_lineup;

    const RmcLineup *challenger_lineup =
        home ? &state->away_lineup : &state->home_lineup;

    const RmcTeamId challenger_team =
        home ? state->fixture.away_team :
               state->fixture.home_team;

    if (!lineup_contains(incumbent_lineup, action->incumbent) ||
        !lineup_contains(challenger_lineup, action->challenger)) {
        return false;
    }

    const RmcPlayer *incumbent = active_roster_player(
        world, state->possession.team_id, action->incumbent
    );

    const RmcPlayer *challenger = active_roster_player(
        world, challenger_team, action->challenger
    );

    if (incumbent == NULL || challenger == NULL) {
        return false;
    }

    uint32_t probability = 0;

    if (!rmc_match_probability(
            recovery_effectiveness(incumbent),
            recovery_effectiveness(challenger),
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

    const bool retained = draw < probability;
    const RmcPlayerId winner = retained
        ? action->incumbent : action->challenger;

    if (retained) {
        next.possession.controller = winner;
        ++next.possession.action_count;
    } else {
        const uint8_t old_sector =
            next.possession.zone.sector;
        const RmcTacticalLane old_lane =
            next.possession.zone.lane;

        next.possession = (RmcMatchPossession){0};
        next.possession.team_id = challenger_team;
        next.possession.initiator = winner;
        next.possession.controller = winner;
        next.possession.zone.sector =
            (uint8_t)(RMC_FIELD_SECTORS - 1u - old_sector);
        next.possession.zone.lane = old_lane;
        next.possession.phase = RMC_PHASE_TRANSITION;
    }

    if (!rmc_match_state_valid(world, &next)) {
        return false;
    }

    RmcMatchEvent event = {0};

    event.id = (uint64_t)log->count + UINT64_C(1);
    event.match_id = state->fixture.id;
    event.at_second = next.elapsed_seconds;
    event.type = RMC_EVENT_RECOVERY;
    event.team_id = retained
        ? state->possession.team_id : challenger_team;
    event.primary_actor = winner;
    event.secondary_actor = retained
        ? action->challenger : action->incumbent;
    event.outcome_code = retained
        ? RMC_RECOVERY_RETAINED : RMC_RECOVERY_TURNOVER;
    event.cause_event_id = cause->id;

    /*
     * Event append is the last fallible operation.
     * State, RNG and result are committed only after
     * the event is successfully recorded.
     */
    if (!rmc_match_event_log_append(
            world, &next, log, &event)) {
        return false;
    }

    *state = next;
    *match_rng = next_rng;

    const RmcRecoveryResult next_result = {
        .probability = probability,
        .random_draw = draw,
        .outcome = retained
            ? RMC_RECOVERY_RETAINED : RMC_RECOVERY_TURNOVER,
        .winner = winner
    };

    *result = next_result;
    return true;
}
