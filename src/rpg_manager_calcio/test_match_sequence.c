#include "match_sequence.h"
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

static void activate_possession(RmcMatchState *state)
{
    state->possession.team_id = state->fixture.home_team;
    state->possession.initiator = state->home_lineup.players[0];
    state->possession.controller = state->home_lineup.players[0];

    assert(rmc_match_state_valid(&world, state));
}

static RmcSequenceDecision decide(const RmcMatchState *state)
{
    RmcSequenceDecision result = RMC_SEQUENCE_END_PERIOD;

    assert(rmc_match_sequence_decide(
        &world, state, &result
    ));

    return result;
}

static void reject_unchanged(const RmcMatchState *state)
{
    const RmcMatchState before = *state;
    RmcSequenceDecision result = RMC_SEQUENCE_RESET_ACTION_WINDOW;

    assert(!rmc_match_sequence_decide(
        &world, state, &result
    ));

    assert(result == RMC_SEQUENCE_RESET_ACTION_WINDOW);
    assert(memcmp(state, &before, sizeof(before)) == 0);
}

static void test_action_limit(void)
{
    RmcMatchState state = first_half();

    assert(RMC_SEQUENCE_POLICY_VERSION == 1);
    assert(RMC_POSSESSION_MAX_ACTIONS == 32);

    assert(decide(&state) == RMC_SEQUENCE_ALLOW_ACTION);

    activate_possession(&state);

    state.possession.action_count = 31;
    assert(decide(&state) == RMC_SEQUENCE_ALLOW_ACTION);

    state.possession.action_count = 32;
    assert(decide(&state) == RMC_SEQUENCE_RESET_ACTION_WINDOW);

    state.possession.action_count = 33;
    assert(decide(&state) == RMC_SEQUENCE_RESET_ACTION_WINDOW);
}

static void test_first_half_deadline(void)
{
    RmcMatchState state = first_half();

    state.first_half_stoppage_seconds = 120;

    state.elapsed_seconds = 2819;
    assert(decide(&state) == RMC_SEQUENCE_ALLOW_ACTION);

    state.elapsed_seconds = 2820;
    assert(decide(&state) == RMC_SEQUENCE_END_PERIOD);

    activate_possession(&state);
    state.possession.action_count = 32;

    assert(decide(&state) == RMC_SEQUENCE_END_PERIOD);

    state.elapsed_seconds = 2821;
    reject_unchanged(&state);
}

static void test_second_half_deadline(void)
{
    RmcMatchState state = first_half();

    state.first_half_stoppage_seconds = 120;
    state.elapsed_seconds = 2820;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_HALF_TIME, 2820
    ));

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_SECOND_HALF, 2820
    ));

    state.mode = RMC_MATCH_BALL_IN_PLAY;
    state.second_half_stoppage_seconds = 180;

    state.elapsed_seconds = 5699;
    assert(decide(&state) == RMC_SEQUENCE_ALLOW_ACTION);

    state.elapsed_seconds = 5700;
    assert(decide(&state) == RMC_SEQUENCE_END_PERIOD);

    activate_possession(&state);
    state.possession.action_count = 32;

    assert(decide(&state) == RMC_SEQUENCE_END_PERIOD);
}

static void test_modes_and_invalid_states(void)
{
    RmcMatchState state = initial;

    reject_unchanged(&state);

    state = first_half();
    state.mode = RMC_MATCH_RESTART_PENDING;
    reject_unchanged(&state);

    state.mode = RMC_MATCH_MANAGEMENT_WINDOW;
    reject_unchanged(&state);

    state.mode = RMC_MATCH_PERIOD_ENDED;
    reject_unchanged(&state);

    state = first_half();
    state.possession.team_id = state.fixture.home_team;
    reject_unchanged(&state);
}

static void test_arguments_and_replay(void)
{
    RmcMatchState first = first_half();
    RmcMatchState second = first_half();
    RmcSequenceDecision result = RMC_SEQUENCE_RESET_ACTION_WINDOW;

    assert(!rmc_match_sequence_decide(
        NULL, &first, &result
    ));
    assert(result == RMC_SEQUENCE_RESET_ACTION_WINDOW);

    assert(!rmc_match_sequence_decide(
        &world, NULL, &result
    ));
    assert(result == RMC_SEQUENCE_RESET_ACTION_WINDOW);

    assert(!rmc_match_sequence_decide(
        &world, &first, NULL
    ));

    activate_possession(&first);
    activate_possession(&second);

    for (uint32_t action = 0; action <= 32; ++action) {
        first.possession.action_count = action;
        second.possession.action_count = action;

        assert(memcmp(&first, &second, sizeof(first)) == 0);
        assert(decide(&first) == decide(&second));
    }
}

int main(void)
{
    setup();

    test_action_limit();
    test_first_half_deadline();
    test_second_half_deadline();
    test_modes_and_invalid_states();
    test_arguments_and_replay();

    puts("SEQUENCE_ACTION_LIMIT=PASS");
    puts("SEQUENCE_FIRST_HALF_DEADLINE=PASS");
    puts("SEQUENCE_SECOND_HALF_DEADLINE=PASS");
    puts("SEQUENCE_ATOMIC_REJECTION=PASS");
    puts("SEQUENCE_DETERMINISTIC_REPLAY=PASS");

    return 0;
}
