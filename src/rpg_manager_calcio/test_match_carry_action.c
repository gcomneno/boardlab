#include "match_carry_action.h"
#include "match_pass_action.h"
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
    state.possession.zone.sector = 1;
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

static RmcCarryAction carry_action(const RmcMatchState *state)
{
    const RmcCarryAction action = {
        .team_id = state->fixture.home_team,
        .carrier = state->home_lineup.players[1],
        .opponent = state->away_lineup.players[1],
        .context = 0,
        .duration_seconds = 5
    };
    return action;
}

static void reject_unchanged(
    RmcMatchState *state,
    RmcPcg32 *rng,
    RmcMatchEventLog *log,
    const RmcCarryAction *action)
{
    const RmcMatchState old_state = *state;
    const RmcPcg32 old_rng = *rng;
    const RmcMatchEventLog old_log = *log;
    RmcMatchEvent old_events[3] = {{0}};

    assert(log->events != NULL && log->capacity <= 3);
    memcpy(old_events, log->events,
           log->capacity * sizeof(log->events[0]));

    RmcCarryResult result = {
        .probability = 777,
        .random_draw = 888,
        .outcome = RMC_CARRY_OUTCOME_CONTAINED
    };
    const RmcCarryResult old_result = result;

    assert(!rmc_match_carry_apply(
        &world, state, rng, log, action, &result
    ));

    assert(memcmp(state, &old_state, sizeof(*state)) == 0);
    assert(memcmp(rng, &old_rng, sizeof(*rng)) == 0);
    assert(memcmp(log, &old_log, sizeof(*log)) == 0);
    assert(memcmp(log->events, old_events,
                  log->capacity * sizeof(log->events[0])) == 0);
    assert(memcmp(&result, &old_result, sizeof(result)) == 0);
}

static void test_both_outcomes(void)
{
    RmcPcg32 candidate = match_rng();
    bool advanced_seen = false;
    bool contained_seen = false;

    for (unsigned trial = 0; trial < 1024; ++trial) {
        RmcMatchState state = active_state();
        RmcPcg32 rng = candidate;
        RmcPcg32 expected_rng = candidate;
        RmcMatchEvent events[3] = {{0}};
        RmcMatchEventLog log = {0};
        RmcCarryResult result = {0};

        assert(rmc_match_event_log_init(
            &state, events, 3, &log
        ));

        const RmcCarryAction action = carry_action(&state);
        const RmcMatchPossession before = state.possession;

        uint32_t expected_draw = UINT32_MAX;
        assert(rmc_pcg32_bounded(
            &expected_rng, RMC_PROBABILITY_SCALE,
            &expected_draw
        ));

        assert(rmc_match_carry_apply(
            &world, &state, &rng, &log, &action, &result
        ));

        const bool advanced = expected_draw < result.probability;

        assert(result.random_draw == expected_draw);
        assert(memcmp(&rng, &expected_rng, sizeof(rng)) == 0);
        assert(result.outcome ==
               (advanced ? RMC_CARRY_OUTCOME_ADVANCED :
                           RMC_CARRY_OUTCOME_CONTAINED));

        assert(state.possession.team_id.value ==
               before.team_id.value);
        assert(state.possession.initiator.value ==
               before.initiator.value);
        assert(state.possession.controller.value ==
               before.controller.value);
        assert(state.possession.zone.lane == before.zone.lane);
        assert(state.possession.action_count ==
               before.action_count + 1);
        assert(state.possession.elapsed_seconds ==
               before.elapsed_seconds + action.duration_seconds);
        assert(state.elapsed_seconds == action.duration_seconds);
        assert(state.mode == RMC_MATCH_BALL_IN_PLAY);

        assert(state.possession.zone.sector ==
               before.zone.sector + (advanced ? 1 : 0));

        assert(log.count == 1);
        assert(events[0].id == 1);
        assert(events[0].type == RMC_EVENT_ACTION);
        assert(events[0].match_id.value ==
               state.fixture.id.value);
        assert(events[0].team_id.value ==
               action.team_id.value);
        assert(events[0].primary_actor.value ==
               action.carrier.value);
        assert(events[0].secondary_actor.value ==
               action.opponent.value);
        assert(events[0].outcome_code == (uint32_t)result.outcome);
        assert(events[0].outcome_code !=
               RMC_PASS_OUTCOME_LOOSE_BALL);
        assert(events[0].at_second == state.elapsed_seconds);
        assert(rmc_match_state_valid(&world, &state));

        if (advanced) {
            advanced_seen = true;
        } else {
            contained_seen = true;
        }

        if (advanced_seen && contained_seen) {
            break;
        }

        (void)rmc_pcg32_next(&candidate);
    }

    assert(advanced_seen && contained_seen);
}

static void test_rejections(void)
{
    RmcMatchState state = active_state();
    RmcPcg32 rng = match_rng();
    RmcMatchEvent events[3] = {{0}};
    RmcMatchEventLog log = {0};

    assert(rmc_match_event_log_init(
        &state, events, 3, &log
    ));

    const RmcCarryAction valid = carry_action(&state);
    RmcCarryAction bad = valid;

    bad.team_id = state.fixture.away_team;
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.carrier = state.home_lineup.players[2];
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.opponent = state.home_lineup.players[3];
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.opponent.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.duration_seconds = 0;
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.context = 21;
    reject_unchanged(&state, &rng, &log, &bad);

    bad.context = -21;
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.duration_seconds = 2701;
    reject_unchanged(&state, &rng, &log, &bad);

    state.possession.zone.sector = 4;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.zone.sector = 1;

    state.possession.controller.value = 0;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.controller = valid.carrier;

    state.possession.action_count = 32;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.action_count = 0;

    RmcMatchEventLog invalid_log = log;
    invalid_log.count = invalid_log.capacity;
    reject_unchanged(&state, &rng, &invalid_log, &valid);

    invalid_log = log;
    invalid_log.match_id.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &invalid_log, &valid);

    state.mode = RMC_MATCH_RESTART_PENDING;
    reject_unchanged(&state, &rng, &log, &valid);
}

static void test_deterministic_replay(void)
{
    RmcMatchState first = active_state();
    RmcMatchState second = first;
    RmcPcg32 first_rng = match_rng();
    RmcPcg32 second_rng = first_rng;

    RmcMatchEvent first_events[3] = {{0}};
    RmcMatchEvent second_events[3] = {{0}};
    RmcMatchEventLog first_log = {0};
    RmcMatchEventLog second_log = {0};

    assert(rmc_match_event_log_init(
        &first, first_events, 3, &first_log
    ));
    assert(rmc_match_event_log_init(
        &second, second_events, 3, &second_log
    ));

    const RmcCarryAction action = carry_action(&first);
    RmcCarryResult first_result = {0};
    RmcCarryResult second_result = {0};

    assert(rmc_match_carry_apply(
        &world, &first, &first_rng,
        &first_log, &action, &first_result
    ));
    assert(rmc_match_carry_apply(
        &world, &second, &second_rng,
        &second_log, &action, &second_result
    ));

    assert(memcmp(&first, &second, sizeof(first)) == 0);
    assert(memcmp(&first_rng, &second_rng,
                  sizeof(first_rng)) == 0);
    assert(first_log.count == second_log.count);
    assert(memcmp(first_events, second_events,
                  sizeof(first_events)) == 0);
    assert(first_result.probability == second_result.probability);
    assert(first_result.random_draw == second_result.random_draw);
    assert(first_result.outcome == second_result.outcome);
}

int main(void)
{
    setup();
    test_both_outcomes();
    test_rejections();
    test_deterministic_replay();

    puts("CARRY_ADVANCED_AND_CONTAINED=PASS");
    puts("CARRY_ZONE_AND_POSSESSION=PASS");
    puts("CARRY_EVENT_SEMANTICS=PASS");
    puts("CARRY_INVALID_REJECTION_ATOMICITY=PASS");
    puts("CARRY_DETERMINISTIC_REPLAY=PASS");

    return 0;
}
