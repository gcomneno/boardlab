#include "match_interruption.h"
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

static RmcMatchState started(void)
{
    RmcMatchState state = initial;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_START, 0
    ));

    return state;
}

static void reject_unchanged(
    RmcMatchState *state,
    uint32_t duration)
{
    const RmcMatchState before = *state;

    assert(!rmc_match_interruption_record(
        &world, state, duration
    ));
    assert(memcmp(state, &before, sizeof(before)) == 0);
}

static void test_first_half_accumulation(void)
{
    RmcMatchState state = started();
    const uint32_t clock_before = state.elapsed_seconds;
    const uint16_t goals_before = state.home_goals;

    assert(rmc_match_interruption_record(&world, &state, 30));
    assert(rmc_match_interruption_record(&world, &state, 90));

    assert(state.first_half_stoppage_seconds == 120);
    assert(state.second_half_stoppage_seconds == 0);
    assert(state.elapsed_seconds == clock_before);
    assert(state.home_goals == goals_before);
    assert(state.possession.team_id.value == 0);
    assert(rmc_match_state_valid(&world, &state));

    assert(rmc_match_interruption_record(&world, &state, 780));
    assert(state.first_half_stoppage_seconds == 900);

    reject_unchanged(&state, 1);
    reject_unchanged(&state, 0);
    reject_unchanged(&state, UINT32_MAX);
}

static void test_second_half_independence(void)
{
    RmcMatchState state = started();

    assert(rmc_match_interruption_record(&world, &state, 120));

    state.mode = RMC_MATCH_BALL_IN_PLAY;

    assert(rmc_match_clock_advance(&world, &state, 2820));
    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_HALF_TIME, 2820
    ));
    assert(state.second_half_start_second == 2820);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_SECOND_HALF, 2820
    ));

    assert(rmc_match_interruption_record(&world, &state, 180));

    assert(state.first_half_stoppage_seconds == 120);
    assert(state.second_half_stoppage_seconds == 180);
    assert(state.elapsed_seconds == 2820);
    assert(rmc_match_state_valid(&world, &state));

    state.mode = RMC_MATCH_BALL_IN_PLAY;
    assert(rmc_match_clock_advance(&world, &state, 2880));
    assert(state.elapsed_seconds == 5700);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_FINISH, 5700
    ));

    reject_unchanged(&state, 1);
}

static void test_modes_and_periods(void)
{
    RmcMatchState state = initial;

    reject_unchanged(&state, 10);

    state = started();
    state.mode = RMC_MATCH_BALL_IN_PLAY;
    reject_unchanged(&state, 10);

    state.mode = RMC_MATCH_MANAGEMENT_WINDOW;
    assert(rmc_match_interruption_record(&world, &state, 10));
    assert(state.first_half_stoppage_seconds == 10);

    state.mode = RMC_MATCH_PERIOD_ENDED;
    reject_unchanged(&state, 10);

    state = started();
    state.mode = RMC_MATCH_MANAGEMENT_WINDOW;
    state.possession.team_id = state.fixture.home_team;
    state.possession.initiator = state.home_lineup.players[0];
    assert(!rmc_match_state_valid(&world, &state));
    reject_unchanged(&state, 10);
}

static void test_atomic_arguments(void)
{
    RmcMatchState state = started();
    const RmcMatchState before = state;

    assert(!rmc_match_interruption_record(NULL, &state, 1));
    assert(memcmp(&state, &before, sizeof(before)) == 0);

    assert(!rmc_match_interruption_record(&world, NULL, 1));

    reject_unchanged(&state, 0);
    reject_unchanged(&state, 901);
    reject_unchanged(&state, UINT32_MAX);
}

static void test_deterministic_replay(void)
{
    RmcMatchState first = started();
    RmcMatchState second = started();

    const uint32_t interruptions[] = {
        5, 30, 60, 25
    };

    for (size_t i = 0;
         i < sizeof(interruptions) / sizeof(interruptions[0]);
         ++i) {
        assert(rmc_match_interruption_record(
            &world, &first, interruptions[i]
        ));

        assert(rmc_match_interruption_record(
            &world, &second, interruptions[i]
        ));

        assert(memcmp(&first, &second, sizeof(first)) == 0);
    }

    assert(first.first_half_stoppage_seconds == 120);
}

int main(void)
{
    setup();

    test_first_half_accumulation();
    test_second_half_independence();
    test_modes_and_periods();
    test_atomic_arguments();
    test_deterministic_replay();

    puts("INTERRUPTION_FIRST_HALF=PASS");
    puts("INTERRUPTION_SECOND_HALF=PASS");
    puts("INTERRUPTION_MODES=PASS");
    puts("INTERRUPTION_ATOMIC_REJECTION=PASS");
    puts("INTERRUPTION_REPLAY=PASS");

    return 0;
}
