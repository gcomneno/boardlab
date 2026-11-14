#include "match_sequence_apply.h"
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

static void reject_unchanged(
    RmcMatchState *state,
    RmcSequenceDecision requested)
{
    const RmcMatchState before = *state;

    assert(!rmc_match_sequence_apply(
        &world, state, requested
    ));
    assert(memcmp(state, &before, sizeof(before)) == 0);
}

static void test_possession_termination(void)
{
    RmcMatchState state = first_half();

    activate_possession(&state);
    state.possession.action_count = 31;

    reject_unchanged(&state, RMC_SEQUENCE_RESET_ACTION_WINDOW);

    state.possession.action_count = 32;
    const RmcMatchPossession possession_before = state.possession;
    const uint32_t clock_before = state.elapsed_seconds;
    const uint16_t goals_before = state.home_goals;

    assert(rmc_match_sequence_apply(
        &world, &state, RMC_SEQUENCE_RESET_ACTION_WINDOW
    ));

    assert(state.mode == RMC_MATCH_BALL_IN_PLAY);
    assert(state.possession.team_id.value ==
           possession_before.team_id.value);
    assert(state.possession.initiator.value ==
           possession_before.initiator.value);
    assert(state.possession.controller.value ==
           possession_before.controller.value);
    assert(state.possession.zone.sector ==
           possession_before.zone.sector);
    assert(state.possession.zone.lane ==
           possession_before.zone.lane);
    assert(state.possession.phase == possession_before.phase);
    assert(state.possession.action_count == 0);
    assert(state.possession.elapsed_seconds ==
           possession_before.elapsed_seconds);
    assert(state.elapsed_seconds == clock_before);
    assert(state.home_goals == goals_before);
    assert(state.period == RMC_MATCH_FIRST_HALF);
    assert(rmc_match_state_valid(&world, &state));

    reject_unchanged(&state, RMC_SEQUENCE_RESET_ACTION_WINDOW);
}

static void test_first_half_boundary(void)
{
    RmcMatchState state = first_half();

    state.first_half_stoppage_seconds = 120;
    state.elapsed_seconds = 2819;

    reject_unchanged(&state, RMC_SEQUENCE_END_PERIOD);

    state.elapsed_seconds = 2820;
    activate_possession(&state);
    state.possession.action_count = 32;

    reject_unchanged(&state, RMC_SEQUENCE_RESET_ACTION_WINDOW);

    assert(rmc_match_sequence_apply(
        &world, &state, RMC_SEQUENCE_END_PERIOD
    ));

    assert(state.period == RMC_MATCH_HALF_TIME);
    assert(state.mode == RMC_MATCH_PERIOD_ENDED);
    assert(state.second_half_start_second == 2820);
    assert(state.elapsed_seconds == 2820);
    assert(state.first_half_stoppage_seconds == 120);
    assert(state.possession.team_id.value == 0);
    assert(rmc_match_state_valid(&world, &state));
}

static void test_second_half_boundary(void)
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
    reject_unchanged(&state, RMC_SEQUENCE_END_PERIOD);

    state.elapsed_seconds = 5700;
    activate_possession(&state);

    assert(rmc_match_sequence_apply(
        &world, &state, RMC_SEQUENCE_END_PERIOD
    ));

    assert(state.period == RMC_MATCH_FINISHED);
    assert(state.mode == RMC_MATCH_PERIOD_ENDED);
    assert(state.terminal_reason ==
           RMC_MATCH_TERMINAL_COMPLETED);
    assert(state.elapsed_seconds == 5700);
    assert(state.second_half_start_second == 2820);
    assert(state.first_half_stoppage_seconds == 120);
    assert(state.second_half_stoppage_seconds == 180);
    assert(state.possession.team_id.value == 0);
    assert(rmc_match_state_valid(&world, &state));
}

static void test_invalid_requests(void)
{
    RmcMatchState state = initial;

    reject_unchanged(&state, RMC_SEQUENCE_END_PERIOD);
    reject_unchanged(&state, RMC_SEQUENCE_RESET_ACTION_WINDOW);

    state = first_half();

    reject_unchanged(&state, RMC_SEQUENCE_ALLOW_ACTION);
    reject_unchanged(
        &state, (RmcSequenceDecision)RMC_SEQUENCE_DECISION_COUNT
    );

    state.mode = RMC_MATCH_RESTART_PENDING;
    reject_unchanged(&state, RMC_SEQUENCE_END_PERIOD);

    state = first_half();
    state.possession.team_id = state.fixture.home_team;
    reject_unchanged(&state, RMC_SEQUENCE_RESET_ACTION_WINDOW);

    state = first_half();
    const RmcMatchState before = state;

    assert(!rmc_match_sequence_apply(
        NULL, &state, RMC_SEQUENCE_END_PERIOD
    ));
    assert(memcmp(&state, &before, sizeof(state)) == 0);

    assert(!rmc_match_sequence_apply(
        &world, NULL, RMC_SEQUENCE_END_PERIOD
    ));
}

static void test_deterministic_replay(void)
{
    RmcMatchState first = first_half();
    RmcMatchState second = first_half();

    activate_possession(&first);
    activate_possession(&second);

    first.possession.action_count = 32;
    second.possession.action_count = 32;

    assert(rmc_match_sequence_apply(
        &world, &first, RMC_SEQUENCE_RESET_ACTION_WINDOW
    ));
    assert(rmc_match_sequence_apply(
        &world, &second, RMC_SEQUENCE_RESET_ACTION_WINDOW
    ));

    assert(memcmp(&first, &second, sizeof(first)) == 0);
}

int main(void)
{
    setup();

    test_possession_termination();
    test_first_half_boundary();
    test_second_half_boundary();
    test_invalid_requests();
    test_deterministic_replay();

    puts("SEQUENCE_APPLY_POSSESSION=PASS");
    puts("SEQUENCE_APPLY_FIRST_HALF=PASS");
    puts("SEQUENCE_APPLY_SECOND_HALF=PASS");
    puts("SEQUENCE_APPLY_ATOMIC_REJECTION=PASS");
    puts("SEQUENCE_APPLY_REPLAY=PASS");

    return 0;
}
