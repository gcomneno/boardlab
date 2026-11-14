#include "match_state.h"

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

    assert(rmc_match_state_valid(&world, &initial));
}

static void test_initial_state(void)
{
    assert(initial.fixture.id.value == 1);
    assert(initial.period == RMC_MATCH_NOT_STARTED);
    assert(initial.mode == RMC_MATCH_PERIOD_ENDED);
    assert(initial.terminal_reason == RMC_MATCH_TERMINAL_NONE);
    assert(initial.elapsed_seconds == 0);
    assert(initial.home_goals == 0);
    assert(initial.away_goals == 0);
    assert(initial.possession.team_id.value == 0);
    assert(initial.possession.controller.value == 0);
    assert(initial.possession.action_count == 0);
}

static void test_fixture_rejection(void)
{
    RmcMatchState state = initial;

    state.fixture.id.value = 0;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.fixture.home_team = state.fixture.away_team;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.fixture.matchday = 0;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.fixture.matchday = RMC_CALENDAR_MATCHDAYS + 1;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.fixture.away_team.value = UINT64_MAX;
    assert(!rmc_match_state_valid(&world, &state));
}

static void test_lineup_rejection(void)
{
    RmcMatchState state = initial;

    state.home_lineup.players[1] =
        state.home_lineup.players[0];
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.home_lineup.players[0] =
        state.away_lineup.players[0];
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.home_lineup.slots[0] = RMC_SLOT_ATT;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.home_lineup.formation = RMC_FORMATION_352;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    ++state.home_lineup.secondary_assignments;
    assert(!rmc_match_state_valid(&world, &state));
}

static void test_possession_rejection(void)
{
    RmcMatchState state = initial;

    state.possession.zone.sector = RMC_FIELD_SECTORS;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.possession.zone.lane = RMC_LANE_COUNT;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.possession.phase = RMC_PHASE_COUNT;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.possession.controller = state.home_lineup.players[0];
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.possession.team_id.value = UINT64_MAX;
    assert(!rmc_match_state_valid(&world, &state));
}

static void test_possession_mode_invariants(void)
{
    RmcMatchState state = initial;

    state.period = RMC_MATCH_FIRST_HALF;
    state.mode = RMC_MATCH_BALL_IN_PLAY;
    state.elapsed_seconds = 120;
    state.possession.team_id = state.fixture.home_team;
    state.possession.initiator = state.home_lineup.players[0];
    state.possession.controller = state.home_lineup.players[0];
    state.possession.elapsed_seconds = 30;

    assert(rmc_match_state_valid(&world, &state));

    state.mode = RMC_MATCH_RESTART_PENDING;
    assert(!rmc_match_state_valid(&world, &state));

    state.mode = RMC_MATCH_MANAGEMENT_WINDOW;
    assert(!rmc_match_state_valid(&world, &state));

    state.mode = RMC_MATCH_PERIOD_ENDED;
    assert(!rmc_match_state_valid(&world, &state));

    state.mode = RMC_MATCH_BALL_IN_PLAY;
    state.possession.initiator.value = 0;
    assert(!rmc_match_state_valid(&world, &state));

    state.possession.initiator = state.home_lineup.players[0];
    state.possession.elapsed_seconds = 121;
    assert(!rmc_match_state_valid(&world, &state));

    state.possession.elapsed_seconds = 30;
    state.possession.controller = state.away_lineup.players[0];
    assert(!rmc_match_state_valid(&world, &state));

    state.possession.team_id.value = 0;
    state.possession.initiator.value = 0;
    state.possession.controller.value = 0;
    state.possession.elapsed_seconds = 0;
    state.mode = RMC_MATCH_BALL_IN_PLAY;

    assert(rmc_match_state_valid(&world, &state));
}

static void test_persistent_stoppage(void)
{
    RmcMatchState state = initial;

    assert(state.first_half_stoppage_seconds == 0);
    assert(state.second_half_stoppage_seconds == 0);

    state.first_half_stoppage_seconds = 1;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.period = RMC_MATCH_FIRST_HALF;
    state.mode = RMC_MATCH_RESTART_PENDING;

    state.first_half_stoppage_seconds = 120;
    assert(rmc_match_state_valid(&world, &state));

    state.first_half_stoppage_seconds = 901;
    assert(!rmc_match_state_valid(&world, &state));

    state.first_half_stoppage_seconds = 120;
    state.second_half_stoppage_seconds = 1;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.period = RMC_MATCH_SECOND_HALF;
    state.mode = RMC_MATCH_RESTART_PENDING;
    state.elapsed_seconds = 2820;
    state.second_half_start_second = 2820;
    state.first_half_stoppage_seconds = 120;
    state.second_half_stoppage_seconds = 180;
    assert(rmc_match_state_valid(&world, &state));

    state.second_half_stoppage_seconds = 901;
    assert(!rmc_match_state_valid(&world, &state));

    state.second_half_stoppage_seconds = 180;
    state.second_half_start_second = 2819;
    assert(!rmc_match_state_valid(&world, &state));

    state.second_half_start_second = 2821;
    assert(!rmc_match_state_valid(&world, &state));
}

static void test_period_rejection(void)
{
    RmcMatchState state = initial;

    state.elapsed_seconds = 1;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.home_goals = 1;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.period = RMC_MATCH_HALF_TIME;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.period = RMC_MATCH_SECOND_HALF;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.period = RMC_MATCH_FINISHED;
    assert(!rmc_match_state_valid(&world, &state));

    state = initial;
    state.terminal_reason = RMC_MATCH_TERMINAL_COMPLETED;
    assert(!rmc_match_state_valid(&world, &state));
}

static void test_atomic_initialization(void)
{
    RmcMatchState destination;
    memset(&destination, 0xA5, sizeof(destination));

    const RmcMatchState before = destination;

    RmcFixture invalid = initial.fixture;
    invalid.id.value = 0;

    assert(!rmc_match_state_init(
        &world,
        &invalid,
        &initial.home_lineup,
        &initial.away_lineup,
        &destination
    ));

    assert(memcmp(
        &destination, &before, sizeof(destination)
    ) == 0);

    assert(!rmc_match_state_init(
        &world,
        &initial.fixture,
        &initial.home_lineup,
        &initial.away_lineup,
        NULL
    ));
}

int main(void)
{
    setup();

    test_initial_state();
    test_fixture_rejection();
    test_lineup_rejection();
    test_possession_rejection();
    test_possession_mode_invariants();
    test_persistent_stoppage();
    test_period_rejection();
    test_atomic_initialization();

    puts("MATCH_STATE_INITIAL_VALID=PASS");
    puts("MATCH_STATE_FIXTURE_REJECTION=PASS");
    puts("MATCH_STATE_LINEUP_REJECTION=PASS");
    puts("MATCH_STATE_POSSESSION_REJECTION=PASS");
    puts("MATCH_STATE_POSSESSION_MODE_INVARIANTS=PASS");
    puts("MATCH_STATE_PERSISTENT_STOPPAGE=PASS");
    puts("MATCH_STATE_PERIOD_REJECTION=PASS");
    puts("MATCH_STATE_ATOMIC_INIT=PASS");

    return 0;
}
