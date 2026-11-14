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

    assert(RMC_EVENT_SCHEMA_VERSION == 2);
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

static void reject_batch_unchanged(
    const RmcMatchState *state, RmcMatchEventLog *log,
    const RmcMatchEvent *events, size_t count)
{
    RmcMatchEvent before_storage[8];
    const RmcMatchEventLog before = *log;
    assert(log->capacity <= 8);
    memcpy(before_storage, log->events,
           log->capacity * sizeof(log->events[0]));
    assert(!rmc_match_event_log_append_batch(
        &world, state, log, events, count));
    assert(memcmp(&before, log, sizeof(before)) == 0);
    assert(memcmp(before_storage, log->events,
                  log->capacity * sizeof(log->events[0])) == 0);
}

static void test_schema_v2(void)
{
    assert(RMC_EVENT_PERIOD_START == 0 && RMC_EVENT_ACTION == 2);
    assert(RMC_EVENT_SHOT == 9 && RMC_EVENT_GOAL == 10);
    assert(RMC_EVENT_PERIOD_END == 13 && RMC_EVENT_MATCH_END == 14);
    assert(RMC_SHOT_MODEL_VERSION == 1);
    assert(RMC_SHOT_CHANCE == 500 && RMC_SHOT_GOAL == 501);
    assert(RMC_SHOT_SAVED_HELD == 502 && RMC_SHOT_SAVED_REBOUND == 503);
    assert(RMC_SHOT_BLOCKED == 504 && RMC_SHOT_WIDE == 505);
    assert(RMC_SHOT_WOODWORK == 506);
    assert(RMC_RESTART_NONE == 0 && RMC_RESTART_GOAL_KICK == 1);
    assert(RMC_RESTART_KICKOFF == 2);
    assert(RMC_RESTART_DISPOSITION_NONE == 0 && RMC_RESTART_PENDING == 1);
    assert(RMC_RESTART_SUPPRESSED_PERIOD_END == 2);
    const RmcMatchEvent event = {0};
    assert(event.shot.model_version == 0);
    assert(event.shot.chance_quality == 0);
    assert(event.shot.source_zone.sector == 0);
    assert(event.shot.source_zone.lane == RMC_LANE_LEFT);
    assert(event.shot.context == 0 && event.shot.duration_seconds == 0);
    assert(event.shot.defender_id.value == 0);
    assert(event.shot.goalkeeper_id.value == 0);
    assert(event.shot.applied_talent_mask == 0);
    assert(event.shot.trajectory == RMC_SHOT_TRAJECTORY_NONE);
    assert(event.shot.restart_kind == RMC_RESTART_NONE);
    assert(event.shot.restart_awarded_team.value == 0);
    assert(event.shot.restart_zone.sector == 0);
    assert(event.shot.restart_zone.lane == RMC_LANE_LEFT);
    assert(event.shot.restart_disposition == RMC_RESTART_DISPOSITION_NONE);
}

static void test_batch_envelopes(void)
{
    RmcMatchState state = started();
    state.elapsed_seconds = 20;
    RmcMatchEvent storage[8];
    memset(storage, 0xa5, sizeof(storage));
    RmcMatchEventLog log;
    assert(rmc_match_event_log_init(&state, storage, 8, &log));
    RmcMatchEvent prefix = make_event(1, 1, RMC_EVENT_PERIOD_START);
    assert(rmc_match_event_log_append(&world, &state, &log, &prefix));
    RmcMatchEvent batch[2] = {
        make_event(2, 10, RMC_EVENT_ACTION),
        make_event(3, 20, RMC_EVENT_RECOVERY)
    };
    batch[0].cause_event_id = 1;
    batch[1].cause_event_id = 2;
    const RmcMatchEvent good = batch[1];

    /* The second element fails after a valid first: no partial writes. */
    batch[1].id = 4;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1] = good;
    batch[1].at_second = 9;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1].at_second = 21;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1] = good;
    batch[1].match_id.value++;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1] = good;
    batch[1].type = (RmcMatchEventType)-1;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1] = good;
    batch[1].primary_actor.value = UINT64_MAX;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1] = good;
    batch[1].secondary_actor.value = UINT64_MAX;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1] = good;
    batch[1].team_id.value = UINT64_MAX;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1] = good;
    batch[1].cause_event_id = 3;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1].cause_event_id = 4;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1] = good;
    batch[1].shot.context = 1; /* Absent payload must be fully neutral. */
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[1] = good;
    batch[0].at_second = 0;
    reject_batch_unchanged(&state, &log, batch, 2);
    batch[0].at_second = 10;
    log.match_id.value++;
    reject_batch_unchanged(&state, &log, batch, 2);
    log.match_id = state.fixture.id;
    reject_batch_unchanged(&state, &log, batch, 0);
    reject_batch_unchanged(&state, &log, NULL, 2);
    const size_t capacity = log.capacity;
    log.capacity = 2;
    reject_batch_unchanged(&state, &log, batch, 2);
    log.capacity = capacity;

    assert(rmc_match_event_log_append_batch(&world, &state, &log, batch, 2));
    assert(log.count == 3 && storage[1].cause_event_id == 1);
    assert(storage[2].cause_event_id == 2);
    assert(memcmp(storage + 1, batch, sizeof(batch)) == 0);
    unsigned char untouched[5 * sizeof(RmcMatchEvent)];
    memset(untouched, 0xa5, sizeof(untouched));
    assert(memcmp(storage + 3, untouched, sizeof(untouched)) == 0);

    /* Overlapping input is copied safely after complete validation. */
    storage[4] = make_event(4, 20, RMC_EVENT_ACTION);
    storage[5] = make_event(5, 20, RMC_EVENT_ACTION);
    storage[5].cause_event_id = 4;
    assert(rmc_match_event_log_append_batch(
        &world, &state, &log, storage + 4, 2));
    assert(log.count == 5 && storage[3].id == 4 && storage[4].id == 5);
    assert(storage[4].cause_event_id == 4);

    const RmcMatchEventLog before = log;
    assert(!rmc_match_event_log_append_batch(NULL, &state, &log, batch, 2));
    assert(!rmc_match_event_log_append_batch(&world, NULL, &log, batch, 2));
    assert(!rmc_match_event_log_append_batch(&world, &state, NULL, batch, 2));
    assert(memcmp(&before, &log, sizeof(log)) == 0);

    /* Synthetic impossible extents must fail before any buffer access. */
    log.capacity = SIZE_MAX;
    log.count = SIZE_MAX;
    const RmcMatchEventLog huge = log;
    RmcMatchEvent bytes_before[8];
    memcpy(bytes_before, storage, sizeof(storage));
    assert(!rmc_match_event_log_append_batch(&world, &state, &log, batch, 2));
    assert(memcmp(&huge, &log, sizeof(log)) == 0);
    assert(memcmp(bytes_before, storage, sizeof(storage)) == 0);
}

static RmcMatchEvent shot_chance(const RmcMatchState *state, uint64_t id)
{
    RmcMatchEvent event = make_event(id, 20, RMC_EVENT_ACTION);
    event.team_id = state->fixture.home_team;
    event.primary_actor = state->home_lineup.players[1];
    event.secondary_actor = state->away_lineup.players[0];
    event.outcome_code = RMC_SHOT_CHANCE;
    event.shot.model_version = RMC_SHOT_MODEL_VERSION;
    event.shot.chance_quality = 75;
    event.shot.source_zone.sector = 4;
    event.shot.source_zone.lane = RMC_LANE_CENTRE;
    event.shot.duration_seconds = 5;
    event.shot.defender_id = state->away_lineup.players[1];
    event.shot.goalkeeper_id = event.secondary_actor;
    return event;
}

static void test_shot_batches(void)
{
    RmcMatchState state = started();
    state.elapsed_seconds = 20;
    RmcMatchEvent storage[8];
    memset(storage, 0x5a, sizeof(storage));
    RmcMatchEventLog log;
    assert(rmc_match_event_log_init(&state, storage, 8, &log));
    RmcMatchEvent batch[3];
    batch[0] = shot_chance(&state, 1);
    batch[1] = batch[0];
    batch[1].id = 2;
    batch[1].cause_event_id = 1;
    batch[1].type = RMC_EVENT_SHOT;
    batch[1].outcome_code = RMC_SHOT_GOAL;
    batch[1].shot.trajectory = RMC_SHOT_TRAJECTORY_ON_TARGET;
    batch[1].shot.restart_kind = RMC_RESTART_KICKOFF;
    batch[1].shot.restart_awarded_team = state.fixture.away_team;
    batch[1].shot.restart_zone.sector = 2;
    batch[1].shot.restart_zone.lane = RMC_LANE_CENTRE;
    batch[1].shot.restart_disposition = RMC_RESTART_PENDING;
    batch[2] = batch[1];
    batch[2].id = 3;
    batch[2].cause_event_id = 2;
    batch[2].type = RMC_EVENT_GOAL;
    const RmcMatchEvent valid[3] = {batch[0], batch[1], batch[2]};

#define REJECT_SHOT_CHANGE(statement) do { \
    memcpy(batch, valid, sizeof(batch)); \
    statement; \
    reject_batch_unchanged(&state, &log, batch, 3); \
} while (0)
    REJECT_SHOT_CHANGE(batch[0].shot.model_version = 2);
    REJECT_SHOT_CHANGE(batch[0].shot.model_version = 0);
    REJECT_SHOT_CHANGE(batch[0].shot.chance_quality = 101);
    REJECT_SHOT_CHANGE(batch[0].shot.source_zone.sector = 1);
    REJECT_SHOT_CHANGE(batch[0].shot.source_zone.sector = 5);
    REJECT_SHOT_CHANGE(batch[0].shot.source_zone.lane = (RmcTacticalLane)-1);
    REJECT_SHOT_CHANGE(batch[0].shot.context = 21);
    REJECT_SHOT_CHANGE(batch[0].shot.context = -21);
    REJECT_SHOT_CHANGE(batch[0].shot.duration_seconds = 0);
    REJECT_SHOT_CHANGE(batch[0].shot.duration_seconds = 21);
    REJECT_SHOT_CHANGE(batch[0].shot.defender_id = state.home_lineup.players[1]);
    REJECT_SHOT_CHANGE(batch[0].shot.defender_id = state.away_lineup.players[0]);
    REJECT_SHOT_CHANGE(batch[0].shot.goalkeeper_id = state.away_lineup.players[1]);
    REJECT_SHOT_CHANGE(batch[0].shot.goalkeeper_id = state.home_lineup.players[0]);
    REJECT_SHOT_CHANGE(batch[0].primary_actor = state.home_lineup.players[0]);
    REJECT_SHOT_CHANGE(batch[0].primary_actor = state.away_lineup.players[1]);
    REJECT_SHOT_CHANGE(batch[0].secondary_actor.value = 0);
    REJECT_SHOT_CHANGE(batch[0].team_id.value = 0);
    REJECT_SHOT_CHANGE(batch[0].shot.applied_talent_mask = UINT32_MAX);
    REJECT_SHOT_CHANGE(batch[0].shot.trajectory = RMC_SHOT_TRAJECTORY_WIDE);
    REJECT_SHOT_CHANGE(batch[0].shot.restart_kind = RMC_RESTART_GOAL_KICK);
    REJECT_SHOT_CHANGE(batch[1].shot.chance_quality--);
    REJECT_SHOT_CHANGE(batch[1].cause_event_id = 0);
    REJECT_SHOT_CHANGE(batch[1].type = RMC_EVENT_RECOVERY);
    REJECT_SHOT_CHANGE(batch[1].shot.trajectory = RMC_SHOT_TRAJECTORY_WIDE);
    REJECT_SHOT_CHANGE(batch[1].shot.restart_kind = RMC_RESTART_GOAL_KICK);
    REJECT_SHOT_CHANGE(batch[1].shot.restart_awarded_team = state.fixture.home_team);
    REJECT_SHOT_CHANGE(batch[1].shot.restart_zone.sector = 0);
    REJECT_SHOT_CHANGE(batch[1].shot.restart_disposition = RMC_RESTART_DISPOSITION_NONE);
    REJECT_SHOT_CHANGE(batch[1].shot.restart_disposition = (RmcRestartDisposition)3);
    REJECT_SHOT_CHANGE(batch[2].cause_event_id = 1);
    REJECT_SHOT_CHANGE(batch[2].outcome_code = RMC_SHOT_SAVED_HELD);
    REJECT_SHOT_CHANGE(batch[2].shot.restart_disposition = RMC_RESTART_SUPPRESSED_PERIOD_END);
#undef REJECT_SHOT_CHANGE

    memcpy(batch, valid, sizeof(batch));
    assert(rmc_match_event_log_append_batch(&world, &state, &log, batch, 3));
    assert(log.count == 3 && storage[2].cause_event_id == 2);

    /* Every non-goal final outcome; chance may already be in the prefix. */
    for (uint32_t code = RMC_SHOT_SAVED_HELD; code <= RMC_SHOT_WOODWORK; ++code) {
        assert(rmc_match_event_log_init(&state, storage, 8, &log));
        batch[0] = shot_chance(&state, 1);
        assert(rmc_match_event_log_append(&world, &state, &log, &batch[0]));
        batch[1] = batch[0];
        batch[1].id = 2;
        batch[1].cause_event_id = 1;
        batch[1].type = RMC_EVENT_SHOT;
        batch[1].outcome_code = code;
        batch[1].shot.trajectory = code == RMC_SHOT_WIDE ?
            RMC_SHOT_TRAJECTORY_WIDE : code == RMC_SHOT_WOODWORK ?
            RMC_SHOT_TRAJECTORY_WOODWORK : RMC_SHOT_TRAJECTORY_ON_TARGET;
        if (code == RMC_SHOT_WIDE) {
            batch[1].shot.restart_kind = RMC_RESTART_GOAL_KICK;
            batch[1].shot.restart_awarded_team = state.fixture.away_team;
            batch[1].shot.restart_zone.lane = RMC_LANE_CENTRE;
            batch[1].shot.restart_disposition = RMC_RESTART_PENDING;
        }
        assert(rmc_match_event_log_append_batch(&world, &state, &log, batch + 1, 1));
        RmcMatchEvent bad_goal = batch[1];
        bad_goal.id = 3;
        bad_goal.cause_event_id = 2;
        bad_goal.type = RMC_EVENT_GOAL;
        bad_goal.outcome_code = RMC_SHOT_GOAL;
        reject_batch_unchanged(&state, &log, &bad_goal, 1);
    }

    /* A sporting restart is explicitly suppressed only at the deadline. */
    state.elapsed_seconds = 2700;
    assert(rmc_match_event_log_init(&state, storage, 8, &log));
    memcpy(batch, valid, sizeof(batch));
    for (size_t i = 0; i < 3; ++i) {
        batch[i].at_second = 2700;
    }
    reject_batch_unchanged(&state, &log, batch, 3);
    batch[1].shot.restart_disposition = RMC_RESTART_SUPPRESSED_PERIOD_END;
    batch[2].shot.restart_disposition = RMC_RESTART_SUPPRESSED_PERIOD_END;
    assert(rmc_match_event_log_append_batch(&world, &state, &log, batch, 3));

    /* Both presence and absence of the applied talent must be truthful. */
    RmcPlayer *shooter = NULL;
    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        if (world.rosters[0].players[i].id.value == batch[0].primary_actor.value) {
            shooter = &world.rosters[0].players[i];
        }
    }
    assert(shooter != NULL);
    const uint8_t original_mask = shooter->talents.consolidated_mask;
    shooter->talents.consolidated_mask =
        (uint8_t)(UINT32_C(1) << RMC_TALENT_COMPOSURE);
    assert(rmc_match_event_log_init(&state, storage, 8, &log));
    reject_batch_unchanged(&state, &log, batch, 3);
    for (size_t i = 0; i < 3; ++i) {
        batch[i].shot.applied_talent_mask = UINT32_C(1) << RMC_TALENT_COMPOSURE;
    }
    assert(rmc_match_event_log_append_batch(&world, &state, &log, batch, 3));
    shooter->talents.consolidated_mask = original_mask;
}

int main(void)
{
    setup();

    test_schema_v2();
    test_batch_envelopes();
    test_shot_batches();
    test_initialization();
    test_order_and_causality();
    test_invalid_envelopes();
    test_capacity_and_terminal();
    test_arguments_and_replay();

    puts("EVENT_LOG_SCHEMA_V2_AND_SHOT_CAUSALITY=PASS");
    puts("EVENT_LOG_ATOMIC_BATCH=PASS");
    puts("EVENT_LOG_INITIALIZATION=PASS");
    puts("EVENT_LOG_ORDER_AND_CAUSALITY=PASS");
    puts("EVENT_LOG_INVALID_ENVELOPES=PASS");
    puts("EVENT_LOG_CAPACITY_AND_TERMINAL=PASS");
    puts("EVENT_LOG_ATOMIC_REJECTION=PASS");
    puts("EVENT_LOG_DETERMINISTIC_REPLAY=PASS");

    return 0;
}
