#include "match_cross_action.h"
#include "match_transitions.h"

_Static_assert(RMC_CROSS_ACTION_VERSION == 1, "Cross version");
_Static_assert(RMC_CROSS_OUTCOME_COMPLETED == 401, "Completed code");
_Static_assert(RMC_CROSS_OUTCOME_INTERCEPTED == 402, "Intercepted code");
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

static RmcMatchState active_state(void)
{
    RmcMatchState state = initial;

    assert(rmc_match_transition_apply(
        &world, &state, RMC_TRANSITION_START, 0
    ));

    state.mode = RMC_MATCH_BALL_IN_PLAY;
    state.possession.team_id = state.fixture.home_team;
    state.possession.initiator = state.home_lineup.players[1];
    state.possession.controller = state.home_lineup.players[1];
    state.possession.zone.sector = 2;
    state.possession.zone.lane = RMC_LANE_LEFT;
    state.possession.phase = RMC_PHASE_BUILDUP;

    assert(rmc_match_state_valid(&world, &state));
    return state;
}

static RmcPcg32 match_rng(void)
{
    RmcPcg32 rng = {0};

    assert(rmc_stream_init(
        &rng, &world.identity, RMC_STREAM_MATCH,
        initial.fixture.id.value
    ));
    return rng;
}

static RmcCrossAction cross_action(const RmcMatchState *state)
{
    const RmcCrossAction action = {
        .team_id = state->possession.team_id,
        .crosser = state->possession.controller,
        .recipient = state->home_lineup.players[2],
        .defender = state->away_lineup.players[1],
        .context = 0,
        .duration_seconds = 5
    };
    return action;
}

static void reject_unchanged(
    RmcMatchState *state,
    RmcPcg32 *rng,
    RmcMatchEventLog *log,
    const RmcCrossAction *action)
{
    const RmcMatchState old_state = *state;
    const RmcPcg32 old_rng = *rng;
    const RmcMatchEventLog old_log = *log;
    RmcMatchEvent old_events[3] = {{0}};

    assert(log->capacity <= 3);
    if (log->events != NULL) {
        memcpy(old_events, log->events,
               log->capacity * sizeof(log->events[0]));
    }

    RmcCrossResult result = {
        .probability = 777,
        .random_draw = 888,
        .outcome = RMC_CROSS_OUTCOME_INTERCEPTED
    };
    const RmcCrossResult old_result = result;

    assert(!rmc_match_cross_apply(
        &world, state, rng, log, action, &result
    ));

    assert(memcmp(state, &old_state, sizeof(*state)) == 0);
    assert(memcmp(rng, &old_rng, sizeof(*rng)) == 0);
    assert(memcmp(log, &old_log, sizeof(*log)) == 0);
    if (log->events != NULL) {
        assert(memcmp(log->events, old_events,
                      log->capacity * sizeof(log->events[0])) == 0);
    }
    assert(memcmp(&result, &old_result, sizeof(result)) == 0);
}

static RmcPlayer *test_player(RmcPlayerId id)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
            if (world.rosters[i].players[j].id.value == id.value) {
                return &world.rosters[i].players[j];
            }
        }
    }
    assert(false);
    return NULL;
}

static uint32_t expected_probability(const RmcCrossAction *action)
{
    const RmcPlayer *attacker = test_player(action->crosser);
    const RmcPlayer *defender = test_player(action->defender);
    const int32_t attack =
        (attacker->attributes[RMC_ATTR_CROSS] +
         attacker->attributes[RMC_ATTR_VISIONE]) / 2;
    const int32_t opposition =
        (defender->attributes[RMC_ATTR_MARCATURA] +
         defender->attributes[RMC_ATTR_POSIZIONAMENTO]) / 2;
    uint32_t probability = 0;
    assert(rmc_match_probability(
        attack, opposition, action->context, &probability
    ));
    return probability;
}

_Static_assert(RMC_CROSS_OUTCOME_LOOSE_BALL == 403, "Loose ball code");

/* Verify production resolution, including byte-identical replay, for one draw. */
static RmcCrossOutcome check_resolution(
    const RmcMatchState *before,
    const RmcCrossAction *action,
    RmcPcg32 candidate)
{
    RmcMatchState state = *before;
    RmcMatchState replay = *before;
    RmcPcg32 rng = candidate;
    RmcPcg32 replay_rng = candidate;
    RmcPcg32 expected_rng = candidate;
    RmcMatchEvent events[3] = {{0}};
    RmcMatchEvent replay_events[3] = {{0}};
    RmcMatchEventLog log = {0};
    RmcMatchEventLog replay_log = {0};
    RmcCrossResult result = {0};
    RmcCrossResult replay_result = {0};
    const RmcMatchEvent preceding = {
        .id = 1,
        .match_id = before->fixture.id,
        .at_second = before->elapsed_seconds,
        .type = RMC_EVENT_POSSESSION_START,
        .team_id = before->possession.team_id,
        .primary_actor = before->possession.initiator
    };

    assert(rmc_match_event_log_init(&state, events, 3, &log));
    assert(rmc_match_event_log_init(&replay, replay_events, 3, &replay_log));
    assert(rmc_match_event_log_append(&world, &state, &log, &preceding));
    assert(rmc_match_event_log_append(
        &world, &replay, &replay_log, &preceding
    ));

    uint32_t draw = UINT32_MAX;
    assert(rmc_pcg32_bounded(&expected_rng, RMC_PROBABILITY_SCALE, &draw));
    assert(rmc_match_cross_apply(
        &world, &state, &rng, &log, action, &result
    ));
    assert(rmc_match_cross_apply(
        &world, &replay, &replay_rng, &replay_log, action, &replay_result
    ));

    const uint32_t probability = expected_probability(action);
    const uint32_t limit = probability + 1000 > RMC_PROBABILITY_SCALE
        ? RMC_PROBABILITY_SCALE : probability + 1000;
    const RmcCrossOutcome expected = draw < probability
        ? RMC_CROSS_OUTCOME_COMPLETED
        : (draw < limit ? RMC_CROSS_OUTCOME_LOOSE_BALL
                        : RMC_CROSS_OUTCOME_INTERCEPTED);
    assert(result.probability == probability);
    assert(result.random_draw == draw);
    assert(result.outcome == expected);
    assert(memcmp(&rng, &expected_rng, sizeof(rng)) == 0);
    assert(state.elapsed_seconds ==
           before->elapsed_seconds + action->duration_seconds);
    assert(state.mode == RMC_MATCH_BALL_IN_PLAY);
    assert(state.period == before->period);
    assert(state.possession.zone.lane == RMC_LANE_CENTRE);

    if (expected == RMC_CROSS_OUTCOME_INTERCEPTED) {
        assert(state.possession.team_id.value ==
               (action->team_id.value == before->fixture.home_team.value
                    ? before->fixture.away_team.value
                    : before->fixture.home_team.value));
        assert(state.possession.controller.value == action->defender.value);
        assert(state.possession.initiator.value == action->defender.value);
        assert(state.possession.action_count == 0);
        assert(state.possession.elapsed_seconds == 0);
        assert(state.possession.zone.sector ==
               4 - (before->possession.zone.sector + 1));
        assert(state.possession.phase == RMC_PHASE_TRANSITION);
    } else {
        assert(state.possession.team_id.value ==
               before->possession.team_id.value);
        assert(state.possession.initiator.value ==
               before->possession.initiator.value);
        assert(state.possession.controller.value ==
               (expected == RMC_CROSS_OUTCOME_LOOSE_BALL
                    ? 0 : action->recipient.value));
        assert(state.possession.action_count ==
               before->possession.action_count + 1);
        assert(state.possession.elapsed_seconds ==
               before->possession.elapsed_seconds + action->duration_seconds);
        assert(state.possession.zone.sector == before->possession.zone.sector + 1);
        assert(state.possession.phase == (before->possession.zone.sector == 3
            ? RMC_PHASE_CHANCE : RMC_PHASE_ATTACKING_DEVELOPMENT));
    }

    assert(log.count == 2);
    assert(memcmp(&events[0], &preceding, sizeof(preceding)) == 0);
    assert(events[1].id == 2);
    assert(events[1].type == RMC_EVENT_ACTION);
    assert(events[1].match_id.value == state.fixture.id.value);
    assert(events[1].at_second == state.elapsed_seconds);
    assert(events[1].team_id.value == state.possession.team_id.value);
    assert(events[1].primary_actor.value == action->crosser.value);
    assert(events[1].secondary_actor.value == action->recipient.value);
    assert(events[1].outcome_code == (uint32_t)expected);
    assert(events[1].cause_event_id == 0);
    assert(events[2].id == 0);
    assert(rmc_match_state_valid(&world, &state));

    assert(memcmp(&state, &replay, sizeof(state)) == 0);
    assert(memcmp(&rng, &replay_rng, sizeof(rng)) == 0);
    assert(log.count == replay_log.count);
    assert(memcmp(events, replay_events, sizeof(events)) == 0);
    assert(result.probability == replay_result.probability);
    assert(result.random_draw == replay_result.random_draw);
    assert(result.outcome == replay_result.outcome);
    return expected;
}

static void test_three_outcomes_and_replay(void)
{
    /* Exercise both teams, legal source sectors and both wings. */
    for (unsigned away = 0; away < 2; ++away) {
        for (unsigned sector = 2; sector <= 3; ++sector) {
            for (unsigned wing = 0; wing < 2; ++wing) {
                RmcMatchState state = active_state();
                if (away) {
                    state.possession.team_id = state.fixture.away_team;
                    state.possession.initiator = state.away_lineup.players[1];
                    state.possession.controller = state.away_lineup.players[1];
                }
                state.elapsed_seconds = 12;
                state.possession.elapsed_seconds = 7;
                state.possession.action_count = 4;
                state.possession.zone.sector = (uint8_t)sector;
                state.possession.zone.lane = wing ? RMC_LANE_RIGHT : RMC_LANE_LEFT;
                RmcCrossAction action = cross_action(&state);
                if (away) {
                    action.team_id = state.fixture.away_team;
                    action.crosser = state.away_lineup.players[1];
                    action.recipient = state.away_lineup.players[2];
                    action.defender = state.home_lineup.players[1];
                }
                bool seen[3] = {false};
                RmcPcg32 candidate = match_rng();
                for (unsigned trial = 0; trial < 1024; ++trial) {
                    const RmcCrossOutcome outcome =
                        check_resolution(&state, &action, candidate);
                    seen[outcome - RMC_CROSS_OUTCOME_COMPLETED] = true;
                    if (seen[0] && seen[1] && seen[2]) {
                        break;
                    }
                    (void)rmc_pcg32_next(&candidate);
                }
                assert(seen[0] && seen[1] && seen[2]);
            }
        }
    }
}

static void test_probability_and_band_edges(void)
{
    RmcMatchState state = active_state();
    RmcCrossAction action = cross_action(&state);
    RmcPlayer *crosser = test_player(action.crosser);
    RmcPlayer *defender = test_player(action.defender);
    const RmcPlayer old_crosser = *crosser;
    const RmcPlayer old_defender = *defender;
    crosser->attributes[RMC_ATTR_CROSS] = 81;
    crosser->attributes[RMC_ATTR_VISIONE] = 40;
    crosser->attributes[RMC_ATTR_VELOCITA] = 1;
    defender->attributes[RMC_ATTR_MARCATURA] = 32;
    defender->attributes[RMC_ATTR_POSIZIONAMENTO] = 49;
    uint32_t probability = 0;
    assert(rmc_match_probability(60, 40, -20, &probability));
    action.context = -20;
    assert(expected_probability(&action) == probability);
    (void)check_resolution(&state, &action, match_rng());
    action.context = 20;
    assert(rmc_match_probability(60, 40, 20, &probability));
    assert(expected_probability(&action) == probability);
    (void)check_resolution(&state, &action, match_rng());

    /* P=5000, test both exact threshold draws and their predecessors. */
    crosser->attributes[RMC_ATTR_CROSS] = 50;
    crosser->attributes[RMC_ATTR_VISIONE] = 51;
    defender->attributes[RMC_ATTR_MARCATURA] = 50;
    defender->attributes[RMC_ATTR_POSIZIONAMENTO] = 51;
    action.context = 0;
    assert(expected_probability(&action) == 5000);
    const uint32_t targets[] = {4999, 5000, 5999, 6000};
    bool seen[4] = {false};
    RmcPcg32 candidate = match_rng();
    for (unsigned trial = 0; trial < 200000; ++trial) {
        RmcPcg32 probe = candidate;
        uint32_t draw = 0;
        assert(rmc_pcg32_bounded(&probe, RMC_PROBABILITY_SCALE, &draw));
        for (size_t i = 0; i < 4; ++i) {
            if (!seen[i] && draw == targets[i]) {
                (void)check_resolution(&state, &action, candidate);
                seen[i] = true;
            }
        }
        if (seen[0] && seen[1] && seen[2] && seen[3]) {
            break;
        }
        (void)rmc_pcg32_next(&candidate);
    }
    assert(seen[0] && seen[1] && seen[2] && seen[3]);

    /* A saturated upper band still makes 9999 a loose ball. */
    crosser->attributes[RMC_ATTR_CROSS] = 100;
    crosser->attributes[RMC_ATTR_VISIONE] = 100;
    defender->attributes[RMC_ATTR_MARCATURA] = 1;
    defender->attributes[RMC_ATTR_POSIZIONAMENTO] = 1;
    action.context = 20;
    assert(expected_probability(&action) == 9981);
    bool saturated_seen = false;
    for (unsigned trial = 0; trial < 200000; ++trial) {
        RmcPcg32 probe = candidate;
        uint32_t draw = 0;
        assert(rmc_pcg32_bounded(&probe, RMC_PROBABILITY_SCALE, &draw));
        if (draw == 9999) {
            assert(check_resolution(&state, &action, candidate) ==
                   RMC_CROSS_OUTCOME_LOOSE_BALL);
            saturated_seen = true;
            break;
        }
        (void)rmc_pcg32_next(&candidate);
    }
    assert(saturated_seen);
    *crosser = old_crosser;
    *defender = old_defender;
}

static void test_rejections(void)
{
    RmcMatchState state = active_state();
    RmcPcg32 rng = match_rng();
    RmcMatchEvent events[3] = {{0}};
    RmcMatchEventLog log = {0};
    assert(rmc_match_event_log_init(&state, events, 3, &log));
    const RmcCrossAction valid = cross_action(&state);
    RmcCrossAction bad = valid;

    bad.team_id = state.fixture.away_team;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.team_id = world.teams[2].id;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.team_id.value = 0;
    reject_unchanged(&state, &rng, &log, &bad);
    bad = valid;
    bad.crosser = state.home_lineup.players[2];
    reject_unchanged(&state, &rng, &log, &bad);
    bad.crosser.value = 0;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.crosser.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &log, &bad);
    bad = valid;
    bad.recipient = valid.crosser;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.recipient = state.away_lineup.players[2];
    reject_unchanged(&state, &rng, &log, &bad);
    bad.recipient = world.rosters[2].players[2].id;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.recipient.value = 0;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.recipient.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &log, &bad);
    bad = valid;
    bad.defender = state.home_lineup.players[3];
    reject_unchanged(&state, &rng, &log, &bad);
    bad.defender.value = 0;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.defender.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &log, &bad);

    /* A real roster player outside the respective lineup is ineligible. */
    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        RmcPlayerId id = world.rosters[1].players[i].id;
        bool on_field = false;
        for (size_t j = 0; j < RMC_LINEUP_PLAYERS; ++j) {
            on_field |= state.away_lineup.players[j].value == id.value;
        }
        if (!on_field) {
            bad = valid;
            bad.defender = id;
            reject_unchanged(&state, &rng, &log, &bad);
            break;
        }
    }
    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        const RmcPlayerId id = world.rosters[0].players[i].id;
        bool on_field = false;
        for (size_t j = 0; j < RMC_LINEUP_PLAYERS; ++j) {
            on_field |= state.home_lineup.players[j].value == id.value;
        }
        if (!on_field) {
            bad = valid;
            bad.recipient = id;
            reject_unchanged(&state, &rng, &log, &bad);
            bad = valid;
            bad.crosser = id;
            state.possession.controller = id;
            reject_unchanged(&state, &rng, &log, &bad);
            state.possession.controller = valid.crosser;
            break;
        }
    }
    bad = valid;
    bad.context = 21;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.context = -21;
    reject_unchanged(&state, &rng, &log, &bad);
    bad = valid;
    bad.duration_seconds = 0;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.duration_seconds = 2701;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.duration_seconds = UINT32_MAX;
    reject_unchanged(&state, &rng, &log, &bad);

    state.possession.controller.value = 0;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.controller = valid.crosser;
    state.possession.action_count = RMC_POSSESSION_MAX_ACTIONS;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.action_count = 0;
    state.elapsed_seconds = 2700;
    reject_unchanged(&state, &rng, &log, &valid);
    state.elapsed_seconds = 0;
    const uint8_t invalid_sectors[] = {0, 1, 4, RMC_FIELD_SECTORS, UINT8_MAX};
    for (size_t i = 0; i < sizeof(invalid_sectors); ++i) {
        state.possession.zone.sector = invalid_sectors[i];
        reject_unchanged(&state, &rng, &log, &valid);
    }
    state.possession.zone.sector = 2;
    state.possession.zone.lane = RMC_LANE_CENTRE;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.zone.lane = RMC_LANE_COUNT;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.zone.lane = (RmcTacticalLane)-1;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.zone.lane = RMC_LANE_LEFT;
    rng.increment &= ~UINT64_C(1);
    reject_unchanged(&state, &rng, &log, &valid);
    rng = match_rng();
    RmcMatchState saved_state = state;
    state.possession = (RmcMatchPossession){0};
    reject_unchanged(&state, &rng, &log, &valid);
    state = saved_state;
    state.home_lineup.slots[1] = RMC_SLOT_COUNT;
    reject_unchanged(&state, &rng, &log, &valid);
    state = saved_state;
    state.period = RMC_MATCH_PERIOD_COUNT;
    reject_unchanged(&state, &rng, &log, &valid);
    state = saved_state;

    RmcMatchEventLog invalid_log = log;
    invalid_log.count = invalid_log.capacity;
    reject_unchanged(&state, &rng, &invalid_log, &valid);
    invalid_log.count = invalid_log.capacity + 1;
    reject_unchanged(&state, &rng, &invalid_log, &valid);
    invalid_log = log;
    invalid_log.capacity = 0;
    reject_unchanged(&state, &rng, &invalid_log, &valid);
    invalid_log = log;
    invalid_log.events = NULL;
    reject_unchanged(&state, &rng, &invalid_log, &valid);
    invalid_log = log;
    invalid_log.match_id.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &invalid_log, &valid);

    /* Invalid world/lineup membership. */
    RmcPlayer *defender = test_player(valid.defender);
    const RmcPlayer saved = *defender;
    defender->id.value = 0;
    reject_unchanged(&state, &rng, &log, &valid);
    *defender = saved;

    const RmcMatchEvent preceding = {
        .id = 1, .match_id = state.fixture.id,
        .type = RMC_EVENT_POSSESSION_START
    };
    events[0] = preceding;
    log.count = 1;
    events[0].at_second = valid.duration_seconds + 1;
    reject_unchanged(&state, &rng, &log, &valid);
    events[0] = preceding;
    events[0].match_id.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &log, &valid);
    events[0] = preceding;
    events[0].id = 2;
    reject_unchanged(&state, &rng, &log, &valid);
    events[0] = preceding;
    events[0].cause_event_id = 1;
    reject_unchanged(&state, &rng, &log, &valid);
    events[0] = preceding;
    events[0].type = RMC_EVENT_TYPE_COUNT;
    reject_unchanged(&state, &rng, &log, &valid);
    events[0] = preceding;
    events[0].primary_actor.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &log, &valid);
    events[0] = preceding;
    events[0].team_id = world.teams[2].id;
    reject_unchanged(&state, &rng, &log, &valid);
    log.count = 0;

    state.mode = RMC_MATCH_RESTART_PENDING;
    reject_unchanged(&state, &rng, &log, &valid);
    state = initial;
    reject_unchanged(&state, &rng, &log, &valid);
}

static void test_null_arguments(void)
{
    RmcMatchState state = active_state();
    RmcPcg32 rng = match_rng();
    RmcMatchEvent events[3] = {{0}};
    RmcMatchEventLog log = {0};
    const RmcCrossAction action = cross_action(&state);
    RmcCrossResult result = {777, 888, RMC_CROSS_OUTCOME_COMPLETED};
    assert(rmc_match_event_log_init(&state, events, 3, &log));
    const RmcMatchState old_state = state;
    const RmcPcg32 old_rng = rng;
    const RmcMatchEventLog old_log = log;
    const RmcCrossResult old_result = result;
    RmcMatchEvent old_events[3];
    memcpy(old_events, events, sizeof(events));
    for (unsigned absent = 0; absent < 6; ++absent) {
        assert(!rmc_match_cross_apply(
            absent == 0 ? NULL : &world,
            absent == 1 ? NULL : &state,
            absent == 2 ? NULL : &rng,
            absent == 3 ? NULL : &log,
            absent == 4 ? NULL : &action,
            absent == 5 ? NULL : &result
        ));
        assert(memcmp(&state, &old_state, sizeof(state)) == 0);
        assert(memcmp(&rng, &old_rng, sizeof(rng)) == 0);
        assert(memcmp(&log, &old_log, sizeof(log)) == 0);
        assert(memcmp(&result, &old_result, sizeof(result)) == 0);
        assert(memcmp(events, old_events, sizeof(events)) == 0);
    }
}

int main(void)
{
    setup();
    test_three_outcomes_and_replay();
    test_probability_and_band_edges();
    test_rejections();
    test_null_arguments();

    puts("CROSS_THREE_OUTCOMES_AND_POSSESSION=PASS");
    puts("CROSS_PROBABILITY_ONE_DRAW_AND_BAND_EDGES=PASS");
    puts("CROSS_EVENT_SEMANTICS=PASS");
    puts("CROSS_INVALID_REJECTION_ATOMICITY=PASS");
    puts("CROSS_DETERMINISTIC_REPLAY=PASS");
    return 0;
}
