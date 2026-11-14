#include "match_period_commit.h"
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

static RmcMatchState first_half(uint32_t stoppage)
{
    RmcMatchState state = initial;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_START, 0
    ));

    state.first_half_stoppage_seconds = stoppage;
    state.mode = RMC_MATCH_BALL_IN_PLAY;

    assert(rmc_match_clock_advance(
        &world, &state, 2700 + stoppage
    ));
    assert(rmc_match_state_valid(&world, &state));

    return state;
}

static RmcMatchState second_half(
    uint32_t first_stoppage,
    uint32_t second_stoppage)
{
    RmcMatchState state = first_half(first_stoppage);

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_HALF_TIME,
        state.elapsed_seconds
    ));
    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_SECOND_HALF,
        state.elapsed_seconds
    ));

    state.second_half_stoppage_seconds = second_stoppage;
    state.mode = RMC_MATCH_BALL_IN_PLAY;

    assert(rmc_match_clock_advance(
        &world, &state, 2700 + second_stoppage
    ));
    assert(rmc_match_state_valid(&world, &state));

    return state;
}

static void reject_unchanged(
    RmcMatchState *state,
    RmcMatchEventLog *log)
{
    const RmcMatchState state_before = *state;
    const RmcMatchEventLog log_before = *log;
    RmcMatchEvent storage_before[4] = {{0}};

    assert(log->events != NULL);
    assert(log->capacity <= 4);

    memcpy(
        storage_before,
        log->events,
        log->capacity * sizeof(log->events[0])
    );

    assert(!rmc_match_period_commit(&world, state, log));

    assert(memcmp(
        state, &state_before, sizeof(state_before)
    ) == 0);
    assert(memcmp(
        log, &log_before, sizeof(log_before)
    ) == 0);
    assert(memcmp(
        log->events, storage_before,
        log->capacity * sizeof(log->events[0])
    ) == 0);
}

static void test_first_half_commit(void)
{
    RmcMatchState state = first_half(120);
    RmcMatchEvent storage[4] = {{0}};
    RmcMatchEventLog log = {0};

    assert(rmc_match_event_log_init(
        &state, storage, 4, &log
    ));

    assert(rmc_match_period_commit(&world, &state, &log));

    assert(state.period == RMC_MATCH_HALF_TIME);
    assert(state.mode == RMC_MATCH_PERIOD_ENDED);
    assert(state.elapsed_seconds == 2820);
    assert(state.second_half_start_second == 2820);

    assert(log.count == 1);
    assert(storage[0].id == 1);
    assert(storage[0].type == RMC_EVENT_PERIOD_END);
    assert(storage[0].at_second == 2820);
    assert(storage[0].cause_event_id == 0);
    assert(storage[0].match_id.value ==
           state.fixture.id.value);

    reject_unchanged(&state, &log);
}

static void test_second_half_commit(void)
{
    RmcMatchState state = second_half(120, 180);
    RmcMatchEvent storage[4] = {{0}};
    RmcMatchEventLog log = {0};

    assert(rmc_match_event_log_init(
        &state, storage, 4, &log
    ));

    assert(rmc_match_period_commit(&world, &state, &log));

    assert(state.period == RMC_MATCH_FINISHED);
    assert(state.mode == RMC_MATCH_PERIOD_ENDED);
    assert(state.terminal_reason ==
           RMC_MATCH_TERMINAL_COMPLETED);
    assert(state.elapsed_seconds == 5700);

    assert(log.count == 2);
    assert(storage[0].id == 1);
    assert(storage[0].type == RMC_EVENT_PERIOD_END);
    assert(storage[0].at_second == 5700);

    assert(storage[1].id == 2);
    assert(storage[1].type == RMC_EVENT_MATCH_END);
    assert(storage[1].at_second == 5700);
    assert(storage[1].cause_event_id == storage[0].id);

    reject_unchanged(&state, &log);
}

static void test_insufficient_capacity(void)
{
    RmcMatchState state = second_half(120, 180);
    RmcMatchEvent storage[2] = {{0}};
    RmcMatchEventLog log = {0};

    assert(rmc_match_event_log_init(
        &state, storage, 2, &log
    ));

    RmcMatchEvent previous = {0};
    previous.id = 1;
    previous.match_id = state.fixture.id;
    previous.at_second = 5700;
    previous.type = RMC_EVENT_ACTION;

    assert(rmc_match_event_log_append(
        &world, &state, &log, &previous
    ));
    assert(log.count == 1);

    /* Two events required, only one slot remains. */
    reject_unchanged(&state, &log);

    RmcMatchState first = first_half(0);
    RmcMatchEvent one_slot[1] = {{0}};
    RmcMatchEventLog first_log = {0};

    assert(rmc_match_event_log_init(
        &first, one_slot, 1, &first_log
    ));

    assert(rmc_match_period_commit(
        &world, &first, &first_log
    ));
    assert(first_log.count == 1);

    reject_unchanged(&first, &first_log);
}

static void test_invalid_transition_and_log(void)
{
    RmcMatchState state = first_half(120);
    RmcMatchEvent storage[4] = {{0}};
    RmcMatchEventLog log = {0};

    assert(rmc_match_event_log_init(
        &state, storage, 4, &log
    ));

    state.elapsed_seconds = 2819;
    reject_unchanged(&state, &log);

    state.elapsed_seconds = 2820;
    state.mode = RMC_MATCH_RESTART_PENDING;
    reject_unchanged(&state, &log);

    state.mode = RMC_MATCH_BALL_IN_PLAY;
    log.match_id.value = UINT64_MAX;
    reject_unchanged(&state, &log);

    log.match_id = state.fixture.id;

    RmcMatchEvent old_event = {0};
    old_event.id = 1;
    old_event.match_id = state.fixture.id;
    old_event.at_second = 2820;
    old_event.type = RMC_EVENT_ACTION;

    assert(rmc_match_event_log_append(
        &world, &state, &log, &old_event
    ));

    /* A pre-existing future timestamp prevents append. */
    storage[0].at_second = 2821;
    reject_unchanged(&state, &log);

    const RmcMatchState state_before = state;
    const RmcMatchEventLog log_before = log;

    assert(!rmc_match_period_commit(NULL, &state, &log));
    assert(!rmc_match_period_commit(&world, NULL, &log));
    assert(!rmc_match_period_commit(&world, &state, NULL));

    assert(memcmp(
        &state, &state_before, sizeof(state)
    ) == 0);
    assert(memcmp(
        &log, &log_before, sizeof(log)
    ) == 0);
}

static void test_deterministic_replay(void)
{
    RmcMatchState first = second_half(120, 180);
    RmcMatchState second = second_half(120, 180);

    RmcMatchEvent first_storage[4] = {{0}};
    RmcMatchEvent second_storage[4] = {{0}};
    RmcMatchEventLog first_log = {0};
    RmcMatchEventLog second_log = {0};

    assert(rmc_match_event_log_init(
        &first, first_storage, 4, &first_log
    ));
    assert(rmc_match_event_log_init(
        &second, second_storage, 4, &second_log
    ));

    assert(rmc_match_period_commit(
        &world, &first, &first_log
    ));
    assert(rmc_match_period_commit(
        &world, &second, &second_log
    ));

    assert(memcmp(&first, &second, sizeof(first)) == 0);
    assert(first_log.count == second_log.count);
    assert(memcmp(
        first_storage, second_storage,
        first_log.count * sizeof(first_storage[0])
    ) == 0);
}

int main(void)
{
    setup();

    test_first_half_commit();
    test_second_half_commit();
    test_insufficient_capacity();
    test_invalid_transition_and_log();
    test_deterministic_replay();

    puts("PERIOD_COMMIT_FIRST_HALF=PASS");
    puts("PERIOD_COMMIT_SECOND_HALF=PASS");
    puts("PERIOD_COMMIT_CAPACITY_ATOMICITY=PASS");
    puts("PERIOD_COMMIT_INVALID_REJECTION=PASS");
    puts("PERIOD_COMMIT_DETERMINISTIC_REPLAY=PASS");

    return 0;
}
