#include "match_event_log.h"
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
    assert(rmc_match_state_valid(&world, &state));

    return state;
}

static RmcMatchEvent make_event(
    uint64_t id,
    uint32_t at_second,
    RmcMatchEventType type)
{
    RmcMatchEvent event = {0};

    event.id = id;
    event.match_id = initial.fixture.id;
    event.at_second = at_second;
    event.type = type;

    return event;
}

static void reject_unchanged(
    const RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcMatchEvent *event)
{
    const RmcMatchEventLog before = *log;
    RmcMatchEvent storage_before[4] = {{0}};

    assert(log->capacity <= 4);
    memcpy(storage_before, log->events,
           log->capacity * sizeof(log->events[0]));

    assert(!rmc_match_event_log_append(
        &world, state, log, event
    ));

    assert(memcmp(log, &before, sizeof(before)) == 0);
    assert(memcmp(log->events, storage_before,
           log->capacity * sizeof(log->events[0])) == 0);
}

static void test_initialization(void)
{
    RmcMatchEvent storage[2] = {{0}};
    RmcMatchEventLog log = {0};

    assert(RMC_EVENT_SCHEMA_VERSION == 1);
    assert(RMC_EVENT_TYPE_COUNT == 15);

    assert(rmc_match_event_log_init(
        &initial, storage, 2, &log
    ));

    assert(log.events == storage);
    assert(log.capacity == 2);
    assert(log.count == 0);
    assert(log.match_id.value == initial.fixture.id.value);

    const RmcMatchEventLog before = log;

    assert(!rmc_match_event_log_init(
        NULL, storage, 2, &log
    ));
    assert(!rmc_match_event_log_init(
        &initial, NULL, 2, &log
    ));
    assert(!rmc_match_event_log_init(
        &initial, storage, 0, &log
    ));
    assert(!rmc_match_event_log_init(
        &initial, storage, 2, NULL
    ));

    assert(memcmp(&log, &before, sizeof(log)) == 0);
}

static void test_order_and_causality(void)
{
    RmcMatchState state = started();
    RmcMatchEvent storage[4] = {{0}};
    RmcMatchEventLog log = {0};

    assert(rmc_match_event_log_init(
        &state, storage, 4, &log
    ));

    RmcMatchEvent event =
        make_event(1, 0, RMC_EVENT_PERIOD_START);

    assert(rmc_match_event_log_append(
        &world, &state, &log, &event
    ));

    assert(log.count == 1);
    assert(log.events[0].id == 1);

    state.elapsed_seconds = 12;

    event = make_event(2, 12, RMC_EVENT_POSSESSION_START);
    event.team_id = state.fixture.home_team;
    event.primary_actor = state.home_lineup.players[0];
    event.cause_event_id = 1;

    assert(rmc_match_event_log_append(
        &world, &state, &log, &event
    ));

    event = make_event(3, 12, RMC_EVENT_ACTION);
    event.team_id = state.fixture.home_team;
    event.primary_actor = state.home_lineup.players[1];
    event.secondary_actor = state.home_lineup.players[2];
    event.outcome_code = 7;
    event.cause_event_id = 2;

    assert(rmc_match_event_log_append(
        &world, &state, &log, &event
    ));

    assert(log.count == 3);
    assert(log.events[2].cause_event_id == 2);
    assert(log.events[2].outcome_code == 7);
    assert(log.events[1].at_second ==
           log.events[2].at_second);
}

static void test_invalid_envelopes(void)
{
    RmcMatchState state = started();
    RmcMatchEvent storage[4] = {{0}};
    RmcMatchEventLog log = {0};

    assert(rmc_match_event_log_init(
        &state, storage, 4, &log
    ));

    state.elapsed_seconds = 20;

    RmcMatchEvent event =
        make_event(1, 20, RMC_EVENT_ACTION);

    event.id = 2;
    reject_unchanged(&state, &log, &event);

    event.id = 1;
    event.match_id.value = 999;
    reject_unchanged(&state, &log, &event);

    event.match_id = state.fixture.id;
    event.at_second = 21;
    reject_unchanged(&state, &log, &event);

    event.at_second = 20;
    event.type = (RmcMatchEventType)RMC_EVENT_TYPE_COUNT;
    reject_unchanged(&state, &log, &event);

    event.type = RMC_EVENT_ACTION;
    event.cause_event_id = 1;
    reject_unchanged(&state, &log, &event);

    event.cause_event_id = 0;
    event.team_id.value = UINT64_MAX;
    reject_unchanged(&state, &log, &event);

    event.team_id.value = 0;
    event.primary_actor.value = UINT64_MAX;
    reject_unchanged(&state, &log, &event);

    event.primary_actor.value = 0;

    assert(rmc_match_event_log_append(
        &world, &state, &log, &event
    ));

    event = make_event(2, 19, RMC_EVENT_ACTION);
    reject_unchanged(&state, &log, &event);

    event.at_second = 20;
    event.cause_event_id = 2;
    reject_unchanged(&state, &log, &event);

    event.cause_event_id = 1;
    assert(rmc_match_event_log_append(
        &world, &state, &log, &event
    ));
}

static void test_capacity_and_terminal(void)
{
    RmcMatchState state = started();
    RmcMatchEvent storage[1] = {{0}};
    RmcMatchEventLog log = {0};

    assert(rmc_match_event_log_init(
        &state, storage, 1, &log
    ));

    RmcMatchEvent event =
        make_event(1, 0, RMC_EVENT_PERIOD_START);

    assert(rmc_match_event_log_append(
        &world, &state, &log, &event
    ));

    event = make_event(2, 0, RMC_EVENT_ACTION);

    const RmcMatchEventLog before = log;
    const RmcMatchEvent stored = storage[0];

    assert(!rmc_match_event_log_append(
        &world, &state, &log, &event
    ));
    assert(memcmp(&log, &before, sizeof(log)) == 0);
    assert(memcmp(&storage[0], &stored, sizeof(stored)) == 0);

    state.elapsed_seconds = 2700;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_HALF_TIME, 2700
    ));
    assert(!rmc_match_event_log_append(
        &world, &state, &log, &event
    ));

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_SECOND_HALF, 2700
    ));

    state.elapsed_seconds = 5400;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_FINISH, 5400
    ));

    assert(!rmc_match_event_log_append(
        &world, &state, &log, &event
    ));
}

static void test_arguments_and_replay(void)
{
    RmcMatchState state = started();
    RmcMatchEvent first_storage[4] = {{0}};
    RmcMatchEvent second_storage[4] = {{0}};
    RmcMatchEventLog first = {0};
    RmcMatchEventLog second = {0};

    assert(rmc_match_event_log_init(
        &state, first_storage, 4, &first
    ));
    assert(rmc_match_event_log_init(
        &state, second_storage, 4, &second
    ));

    RmcMatchEvent event =
        make_event(1, 0, RMC_EVENT_PERIOD_START);

    assert(!rmc_match_event_log_append(
        NULL, &state, &first, &event
    ));
    assert(!rmc_match_event_log_append(
        &world, NULL, &first, &event
    ));
    assert(!rmc_match_event_log_append(
        &world, &state, NULL, &event
    ));
    assert(!rmc_match_event_log_append(
        &world, &state, &first, NULL
    ));

    const RmcMatchEvent events[] = {
        make_event(1, 0, RMC_EVENT_PERIOD_START),
        make_event(2, 0, RMC_EVENT_POSSESSION_START),
        make_event(3, 0, RMC_EVENT_ACTION)
    };

    for (size_t i = 0; i < 3; ++i) {
        assert(rmc_match_event_log_append(
            &world, &state, &first, &events[i]
        ));
        assert(rmc_match_event_log_append(
            &world, &state, &second, &events[i]
        ));
    }

    assert(first.count == second.count);
    assert(memcmp(
        first_storage, second_storage,
        first.count * sizeof(first_storage[0])
    ) == 0);
}

int main(void)
{
    setup();

    test_initialization();
    test_order_and_causality();
    test_invalid_envelopes();
    test_capacity_and_terminal();
    test_arguments_and_replay();

    puts("EVENT_LOG_INITIALIZATION=PASS");
    puts("EVENT_LOG_ORDER_AND_CAUSALITY=PASS");
    puts("EVENT_LOG_INVALID_ENVELOPES=PASS");
    puts("EVENT_LOG_CAPACITY_AND_TERMINAL=PASS");
    puts("EVENT_LOG_ATOMIC_REJECTION=PASS");
    puts("EVENT_LOG_DETERMINISTIC_REPLAY=PASS");

    return 0;
}
