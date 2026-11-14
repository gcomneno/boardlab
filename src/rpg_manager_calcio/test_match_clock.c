#include "match_clock.h"
#include "match_transitions.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static RmcWorldCore world;
static RmcMatchState initial;

static void setup(void)
{
    const RmcWorldIdentity identity = {
        .world_seed = UINT64_C(20261114),
        .generator_version = RMC_GENERATOR_VERSION,
        .rules_version = RMC_RULES_VERSION
    };

    assert(rmc_world_core_generate(&identity, &world));

    const RmcFixture fixture = {
        .id = {UINT64_C(1)},
        .home_team = world.teams[0].id,
        .away_team = world.teams[1].id,
        .matchday = 1
    };

    RmcLineup home;
    RmcLineup away;

    assert(rmc_roster_lineup(
        &world.rosters[0], RMC_FORMATION_442, &home
    ));

    assert(rmc_roster_lineup(
        &world.rosters[1], RMC_FORMATION_433, &away
    ));

    assert(rmc_match_state_init(
        &world, &fixture, &home, &away, &initial
    ));
}

static RmcMatchState first_half(void)
{
    RmcMatchState state = initial;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_START, 0
    ));

    state.mode = RMC_MATCH_BALL_IN_PLAY;
    assert(rmc_match_state_valid(&world, &state));

    return state;
}

static RmcMatchState second_half(void)
{
    RmcMatchState state = first_half();

    assert(rmc_match_clock_advance(&world, &state, 2700));

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_HALF_TIME, 2700
    ));

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_SECOND_HALF, 2700
    ));

    state.mode = RMC_MATCH_BALL_IN_PLAY;
    assert(rmc_match_state_valid(&world, &state));

    return state;
}

static void reject_unchanged(
    RmcMatchState *state,
    uint32_t delta)
{
    const RmcMatchState before = *state;

    assert(!rmc_match_clock_advance(&world, state, delta));

    assert(memcmp(state, &before, sizeof(before)) == 0);
}

static void test_first_half_boundaries(void)
{
    RmcMatchState state = first_half();

    reject_unchanged(&state, 0);
    reject_unchanged(&state, 2701);

    assert(rmc_match_clock_advance(&world, &state, 1));
    assert(state.elapsed_seconds == 1);

    assert(rmc_match_clock_advance(&world, &state, 2699));
    assert(state.elapsed_seconds == 2700);

    reject_unchanged(&state, 1);
    reject_unchanged(&state, UINT32_MAX);

    assert(rmc_match_state_valid(&world, &state));
}

static void test_second_half_boundaries(void)
{
    RmcMatchState state = second_half();

    assert(state.elapsed_seconds == 2700);

    reject_unchanged(&state, 2701);

    assert(rmc_match_clock_advance(&world, &state, 2699));
    assert(state.elapsed_seconds == 5399);

    assert(rmc_match_clock_advance(&world, &state, 1));
    assert(state.elapsed_seconds == 5400);

    reject_unchanged(&state, 1);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_FINISH, 5400
    ));

    reject_unchanged(&state, 1);
}

static void test_possession_clock(void)
{
    RmcMatchState state = first_half();

    state.possession.team_id = state.fixture.home_team;
    state.possession.initiator = state.home_lineup.players[0];
    state.possession.controller = state.home_lineup.players[0];

    assert(rmc_match_state_valid(&world, &state));

    assert(rmc_match_clock_advance(&world, &state, 35));
    assert(state.elapsed_seconds == 35);
    assert(state.possession.elapsed_seconds == 35);

    assert(rmc_match_clock_advance(&world, &state, 10));
    assert(state.elapsed_seconds == 45);
    assert(state.possession.elapsed_seconds == 45);

    reject_unchanged(&state, 2656);

    state.possession.elapsed_seconds = 46;
    assert(!rmc_match_state_valid(&world, &state));
    reject_unchanged(&state, 1);
}

static void test_modes_and_arguments(void)
{
    RmcMatchState state = initial;

    reject_unchanged(&state, 1);

    state = first_half();
    state.mode = RMC_MATCH_RESTART_PENDING;
    reject_unchanged(&state, 1);

    state.mode = RMC_MATCH_MANAGEMENT_WINDOW;
    reject_unchanged(&state, 1);

    state.mode = RMC_MATCH_PERIOD_ENDED;
    reject_unchanged(&state, 1);

    state = first_half();

    const RmcMatchState before = state;

    assert(!rmc_match_clock_advance(NULL, &state, 1));
    assert(memcmp(&state, &before, sizeof(state)) == 0);

    assert(!rmc_match_clock_advance(&world, NULL, 1));
}

static void test_dynamic_stoppage_deadlines(void)
{
    RmcMatchState state = first_half();

    state.first_half_stoppage_seconds = 120;
    assert(rmc_match_state_valid(&world, &state));

    assert(rmc_match_clock_advance(&world, &state, 2700));
    assert(state.elapsed_seconds == 2700);

    assert(rmc_match_clock_advance(&world, &state, 120));
    assert(state.elapsed_seconds == 2820);

    reject_unchanged(&state, 1);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_HALF_TIME, 2820
    ));
    assert(state.second_half_start_second == 2820);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_SECOND_HALF, 2820
    ));

    state.mode = RMC_MATCH_BALL_IN_PLAY;
    state.second_half_stoppage_seconds = 180;

    assert(rmc_match_state_valid(&world, &state));

    assert(rmc_match_clock_advance(&world, &state, 2700));
    assert(state.elapsed_seconds == 5520);

    assert(rmc_match_clock_advance(&world, &state, 180));
    assert(state.elapsed_seconds == 5700);

    reject_unchanged(&state, 1);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_FINISH, 5700
    ));

    assert(state.terminal_reason ==
           RMC_MATCH_TERMINAL_COMPLETED);
    assert(rmc_match_state_valid(&world, &state));
}

static void test_replay(void)
{
    RmcMatchState first = first_half();
    RmcMatchState second = first_half();

    const uint32_t increments[] = {
        1, 17, 39, 100, 300, 1000, 1243
    };

    for (size_t i = 0;
         i < sizeof(increments) / sizeof(increments[0]);
         ++i) {
        assert(rmc_match_clock_advance(
            &world, &first, increments[i]
        ));

        assert(rmc_match_clock_advance(
            &world, &second, increments[i]
        ));

        assert(memcmp(&first, &second, sizeof(first)) == 0);
    }

    assert(first.elapsed_seconds == 2700);
}

int main(void)
{
    setup();

    test_first_half_boundaries();
    test_second_half_boundaries();
    test_possession_clock();
    test_modes_and_arguments();
    test_dynamic_stoppage_deadlines();
    test_replay();

    puts("MATCH_CLOCK_FIRST_HALF=PASS");
    puts("MATCH_CLOCK_SECOND_HALF=PASS");
    puts("MATCH_CLOCK_POSSESSION=PASS");
    puts("MATCH_CLOCK_ATOMIC_REJECTION=PASS");
    puts("MATCH_CLOCK_REPLAY=PASS");
    puts("MATCH_CLOCK_DYNAMIC_STOPPAGE=PASS");

    return 0;
}
