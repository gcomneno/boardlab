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

static void reject_unchanged(
    RmcMatchState *state,
    RmcMatchTransition transition,
    uint32_t at_second)
{
    const RmcMatchState before = *state;

    assert(!rmc_match_transition_apply(
        &world, state, transition, at_second
    ));

    assert(memcmp(state, &before, sizeof(before)) == 0);
}

static void test_complete_lifecycle(void)
{
    RmcMatchState state = initial;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_START, 0
    ));
    assert(state.period == RMC_MATCH_FIRST_HALF);
    assert(state.mode == RMC_MATCH_RESTART_PENDING);
    assert(rmc_match_state_valid(&world, &state));

    state.mode = RMC_MATCH_BALL_IN_PLAY;
    state.possession.team_id = state.fixture.home_team;
    state.possession.initiator = state.home_lineup.players[0];
    state.possession.controller = state.home_lineup.players[0];
    state.possession.action_count = 3;
    state.possession.elapsed_seconds = 40;
    state.elapsed_seconds = 2700;

    assert(rmc_match_state_valid(&world, &state));

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_HALF_TIME, 2700
    ));
    assert(state.period == RMC_MATCH_HALF_TIME);
    assert(state.mode == RMC_MATCH_PERIOD_ENDED);
    assert(state.possession.team_id.value == 0);
    assert(state.possession.controller.value == 0);
    assert(state.possession.action_count == 0);
    assert(rmc_match_state_valid(&world, &state));

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_SECOND_HALF, 2700
    ));
    assert(state.period == RMC_MATCH_SECOND_HALF);
    assert(state.mode == RMC_MATCH_RESTART_PENDING);

    state.home_goals = 2;
    state.away_goals = 1;
    state.elapsed_seconds = 5400;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_FINISH, 5400
    ));
    assert(state.period == RMC_MATCH_FINISHED);
    assert(state.mode == RMC_MATCH_PERIOD_ENDED);
    assert(state.terminal_reason == RMC_MATCH_TERMINAL_COMPLETED);
    assert(state.elapsed_seconds == 5400);
    assert(state.home_goals == 2);
    assert(state.away_goals == 1);
    assert(rmc_match_state_valid(&world, &state));

    for (unsigned t = 0; t < RMC_TRANSITION_COUNT; ++t) {
        reject_unchanged(
            &state, (RmcMatchTransition)t, 5400
        );
    }
}

static void test_illegal_order(void)
{
    RmcMatchState state = initial;

    reject_unchanged(&state, RMC_TRANSITION_HALF_TIME, 2700);
    reject_unchanged(&state, RMC_TRANSITION_SECOND_HALF, 2700);
    reject_unchanged(&state, RMC_TRANSITION_FINISH, 5400);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_START, 0
    ));

    reject_unchanged(&state, RMC_TRANSITION_START, 0);
    reject_unchanged(&state, RMC_TRANSITION_SECOND_HALF, 2700);
    reject_unchanged(&state, RMC_TRANSITION_FINISH, 5400);

    state.elapsed_seconds = 2700;
    assert(rmc_match_state_valid(&world, &state));

    reject_unchanged(&state, RMC_TRANSITION_HALF_TIME, 2800);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_HALF_TIME, 2700
    ));

    reject_unchanged(&state, RMC_TRANSITION_START, 2700);
    reject_unchanged(&state, RMC_TRANSITION_HALF_TIME, 2700);
    reject_unchanged(&state, RMC_TRANSITION_FINISH, 5400);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_SECOND_HALF, 2700
    ));

    reject_unchanged(&state, RMC_TRANSITION_START, 2700);
    reject_unchanged(&state, RMC_TRANSITION_HALF_TIME, 3000);
    reject_unchanged(&state, RMC_TRANSITION_SECOND_HALF, 3000);
}

static void test_temporal_boundaries(void)
{
    RmcMatchState state = initial;

    reject_unchanged(&state, RMC_TRANSITION_START, 1);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_START, 0
    ));

    reject_unchanged(&state, RMC_TRANSITION_HALF_TIME, 2699);
    reject_unchanged(&state, RMC_TRANSITION_HALF_TIME, 2700);

    state.elapsed_seconds = 2750;
    assert(rmc_match_state_valid(&world, &state));

    reject_unchanged(&state, RMC_TRANSITION_HALF_TIME, 2800);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_HALF_TIME, 2750
    ));

    reject_unchanged(&state, RMC_TRANSITION_SECOND_HALF, 2749);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_SECOND_HALF, 2750
    ));

    reject_unchanged(&state, RMC_TRANSITION_FINISH, 5399);

    state.elapsed_seconds = 5500;
    assert(rmc_match_state_valid(&world, &state));

    reject_unchanged(&state, RMC_TRANSITION_FINISH, 5499);
    reject_unchanged(&state, RMC_TRANSITION_FINISH, 5600);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_FINISH, 5500
    ));
}

static void test_invalid_arguments_and_states(void)
{
    RmcMatchState state = initial;

    reject_unchanged(&state, RMC_TRANSITION_COUNT, 0);

    const RmcMatchState before = state;

    assert(!rmc_match_transition_apply(
        NULL, &state, RMC_TRANSITION_START, 0
    ));
    assert(memcmp(&state, &before, sizeof(state)) == 0);

    assert(!rmc_match_transition_apply(
        &world, NULL, RMC_TRANSITION_START, 0
    ));

    state.fixture.id.value = 0;
    reject_unchanged(&state, RMC_TRANSITION_START, 0);
}

static void test_stoppage_shifted_second_half(void)
{
    RmcMatchState state = initial;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_START, 0
    ));

    state.first_half_stoppage_seconds = 120;
    state.elapsed_seconds = 2700;
    assert(rmc_match_state_valid(&world, &state));

    reject_unchanged(&state, RMC_TRANSITION_HALF_TIME, 2700);

    state.elapsed_seconds = 2820;
    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_HALF_TIME, 2820
    ));

    assert(state.second_half_start_second == 2820);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_SECOND_HALF, 2820
    ));

    state.second_half_stoppage_seconds = 180;
    state.elapsed_seconds = 5520;

    reject_unchanged(&state, RMC_TRANSITION_FINISH, 5520);

    state.elapsed_seconds = 5700;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_FINISH, 5700
    ));

    assert(state.terminal_reason ==
           RMC_MATCH_TERMINAL_COMPLETED);
    assert(state.second_half_start_second == 2820);
    assert(state.elapsed_seconds == 5700);
}

static void test_replay(void)
{
    RmcMatchState first = initial;
    RmcMatchState second = initial;

    const RmcMatchTransition transitions[] = {
        RMC_TRANSITION_START,
        RMC_TRANSITION_HALF_TIME,
        RMC_TRANSITION_SECOND_HALF,
        RMC_TRANSITION_FINISH
    };

    const uint32_t times[] = {0, 2700, 2700, 5400};

    for (size_t i = 0; i < 4; ++i) {
        if (transitions[i] == RMC_TRANSITION_HALF_TIME ||
            transitions[i] == RMC_TRANSITION_FINISH) {
            first.elapsed_seconds = times[i];
            second.elapsed_seconds = times[i];
        }

        assert(rmc_match_transition_apply(
            &world, &first, transitions[i], times[i]
        ));
        assert(rmc_match_transition_apply(
            &world, &second, transitions[i], times[i]
        ));
        assert(memcmp(&first, &second, sizeof(first)) == 0);
    }
}

int main(void)
{
    setup();

    test_complete_lifecycle();
    test_illegal_order();
    test_temporal_boundaries();
    test_invalid_arguments_and_states();
    test_stoppage_shifted_second_half();
    test_replay();

    puts("MATCH_TRANSITION_LIFECYCLE=PASS");
    puts("MATCH_TRANSITION_ILLEGAL_ORDER=PASS");
    puts("MATCH_TRANSITION_TEMPORAL_BOUNDARIES=PASS");
    puts("MATCH_TRANSITION_ATOMIC_REJECTION=PASS");
    puts("MATCH_TRANSITION_REPLAY=PASS");
    puts("MATCH_TRANSITION_SHIFTED_SECOND_HALF=PASS");

    return 0;
}
