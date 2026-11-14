#include "match_state.h"
#include "match_stoppage.h"

#include <stddef.h>

static const RmcRoster *find_roster(
    const RmcWorldCore *world,
    RmcTeamId team)
{
    if (team.value == 0) {
        return NULL;
    }

    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (world->teams[i].id.value == team.value &&
            world->rosters[i].team_id.value == team.value) {
            return &world->rosters[i];
        }
    }

    return NULL;
}

static const RmcPlayer *find_player(
    const RmcRoster *roster,
    RmcPlayerId id)
{
    if (id.value == 0) {
        return NULL;
    }

    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        if (roster->players[i].id.value == id.value) {
            return &roster->players[i];
        }
    }

    return NULL;
}

static bool lineup_valid(
    const RmcRoster *roster,
    const RmcLineup *lineup)
{
    const RmcSlot *expected =
        rmc_formation_slots(lineup->formation);

    if (expected == NULL) {
        return false;
    }

    unsigned secondary = 0;

    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (lineup->slots[i] != expected[i]) {
            return false;
        }

        const RmcPlayer *player =
            find_player(roster, lineup->players[i]);

        if (player == NULL) {
            return false;
        }

        const uint16_t slot_bit =
            (uint16_t)(UINT16_C(1) << (unsigned)expected[i]);

        if (((player->primary_slots |
              player->secondary_slots) & slot_bit) == 0) {
            return false;
        }

        if ((player->primary_slots & slot_bit) == 0) {
            ++secondary;
        }

        for (size_t j = 0; j < i; ++j) {
            if (lineup->players[i].value ==
                lineup->players[j].value) {
                return false;
            }
        }
    }

    return secondary == lineup->secondary_assignments;
}

static bool active_player(
    const RmcMatchState *state,
    RmcTeamId team,
    RmcPlayerId player)
{
    const RmcLineup *lineup = NULL;

    if (team.value == state->fixture.home_team.value) {
        lineup = &state->home_lineup;
    } else if (team.value == state->fixture.away_team.value) {
        lineup = &state->away_lineup;
    } else {
        return false;
    }

    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (lineup->players[i].value == player.value) {
            return true;
        }
    }

    return false;
}

bool rmc_match_state_valid(
    const RmcWorldCore *world,
    const RmcMatchState *state)
{
    if (world == NULL || state == NULL ||
        !rmc_world_core_valid(world)) {
        return false;
    }

    const RmcFixture *fixture = &state->fixture;
    const RmcRoster *home =
        find_roster(world, fixture->home_team);
    const RmcRoster *away =
        find_roster(world, fixture->away_team);

    if (fixture->id.value == 0 ||
        fixture->matchday < 1 ||
        fixture->matchday > RMC_CALENDAR_MATCHDAYS ||
        fixture->home_team.value == fixture->away_team.value ||
        home == NULL || away == NULL ||
        !lineup_valid(home, &state->home_lineup) ||
        !lineup_valid(away, &state->away_lineup)) {
        return false;
    }

    if ((unsigned)state->period >= RMC_MATCH_PERIOD_COUNT ||
        (unsigned)state->mode >= RMC_MATCH_MODE_COUNT ||
        (unsigned)state->terminal_reason >=
            RMC_MATCH_TERMINAL_COUNT) {
        return false;
    }

    const RmcMatchPossession *pos = &state->possession;

    if (pos->zone.sector >= RMC_FIELD_SECTORS ||
        (unsigned)pos->zone.lane >= RMC_LANE_COUNT ||
        (unsigned)pos->phase >= RMC_PHASE_COUNT) {
        return false;
    }

    if (pos->team_id.value == 0) {
        if (pos->initiator.value != 0 ||
            pos->controller.value != 0 ||
            pos->action_count != 0 ||
            pos->elapsed_seconds != 0) {
            return false;
        }
    } else {
        if (pos->team_id.value != fixture->home_team.value &&
            pos->team_id.value != fixture->away_team.value) {
            return false;
        }

        if ((pos->initiator.value != 0 &&
             !active_player(state, pos->team_id, pos->initiator)) ||
            (pos->controller.value != 0 &&
             !active_player(state, pos->team_id, pos->controller))) {
            return false;
        }
    }

    /*
     * Active possession requires a valid initiator and
     * a live ball. Its local clock cannot exceed the
     * absolute match clock.
     *
     * BALL_IN_PLAY without possession remains legal:
     * the ball may be temporarily contested.
     */
    if (pos->team_id.value != 0 &&
        (pos->initiator.value == 0 ||
         state->mode != RMC_MATCH_BALL_IN_PLAY ||
         pos->elapsed_seconds > state->elapsed_seconds)) {
        return false;
    }

    if (state->first_half_stoppage_seconds >
            RMC_STOPPAGE_MAX_SECONDS ||
        state->second_half_stoppage_seconds >
            RMC_STOPPAGE_MAX_SECONDS) {
        return false;
    }

    if ((state->period == RMC_MATCH_NOT_STARTED ||
         state->period == RMC_MATCH_FIRST_HALF ||
         state->period == RMC_MATCH_HALF_TIME) &&
        state->second_half_stoppage_seconds != 0) {
        return false;
    }

    if (state->period == RMC_MATCH_NOT_STARTED &&
        state->first_half_stoppage_seconds != 0) {
        return false;
    }

    if (state->period == RMC_MATCH_NOT_STARTED ||
        state->period == RMC_MATCH_FIRST_HALF) {
        if (state->second_half_start_second != 0) {
            return false;
        }
    } else {
        const uint32_t minimum_start =
            UINT32_C(2700) + state->first_half_stoppage_seconds;

        if (state->second_half_start_second < minimum_start ||
            state->second_half_start_second >
                state->elapsed_seconds) {
            return false;
        }
    }

    if (state->period == RMC_MATCH_NOT_STARTED) {
        return state->mode == RMC_MATCH_PERIOD_ENDED &&
               state->terminal_reason == RMC_MATCH_TERMINAL_NONE &&
               state->elapsed_seconds == 0 &&
               state->home_goals == 0 &&
               state->away_goals == 0 &&
               pos->team_id.value == 0;
    }

    if (state->period == RMC_MATCH_FINISHED) {
        return state->mode == RMC_MATCH_PERIOD_ENDED &&
               state->terminal_reason != RMC_MATCH_TERMINAL_NONE &&
               pos->team_id.value == 0 &&
               (state->terminal_reason !=
                    RMC_MATCH_TERMINAL_COMPLETED ||
                state->elapsed_seconds >= 5400);
    }

    if (state->terminal_reason != RMC_MATCH_TERMINAL_NONE) {
        return false;
    }

    if (state->period == RMC_MATCH_HALF_TIME) {
        return state->elapsed_seconds >= 2700 &&
               state->mode == RMC_MATCH_PERIOD_ENDED &&
               pos->team_id.value == 0;
    }

    if (state->period == RMC_MATCH_SECOND_HALF &&
        state->elapsed_seconds < state->second_half_start_second) {
        return false;
    }

    if (state->mode == RMC_MATCH_PERIOD_ENDED &&
        pos->team_id.value != 0) {
        return false;
    }

    return true;
}

bool rmc_match_state_init(
    const RmcWorldCore *world,
    const RmcFixture *fixture,
    const RmcLineup *home_lineup,
    const RmcLineup *away_lineup,
    RmcMatchState *destination)
{
    if (world == NULL || fixture == NULL ||
        home_lineup == NULL || away_lineup == NULL ||
        destination == NULL) {
        return false;
    }

    RmcMatchState next = {0};

    next.fixture = *fixture;
    next.home_lineup = *home_lineup;
    next.away_lineup = *away_lineup;

    next.period = RMC_MATCH_NOT_STARTED;
    next.mode = RMC_MATCH_PERIOD_ENDED;
    next.terminal_reason = RMC_MATCH_TERMINAL_NONE;
    next.possession.phase = RMC_PHASE_BUILDUP;
    next.possession.zone.lane = RMC_LANE_LEFT;

    if (!rmc_match_state_valid(world, &next)) {
        return false;
    }

    *destination = next;
    return true;
}
