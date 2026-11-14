#include "match_recovery.h"
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
    state.possession.zone.lane = RMC_LANE_CENTRE;
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

static RmcPassAction pass_action(
    const RmcMatchState *state)
{
    RmcPassAction action = {0};

    action.team_id = state->fixture.home_team;
    action.passer = state->home_lineup.players[1];
    action.recipient = state->home_lineup.players[2];
    action.opponent = state->away_lineup.players[1];
    action.duration_seconds = 5;

    return action;
}

static RmcRecoveryAction recovery_action(
    const RmcMatchState *state)
{
    RmcRecoveryAction action = {0};

    action.incumbent = state->home_lineup.players[2];
    action.challenger = state->away_lineup.players[1];
    action.duration_seconds = 3;

    return action;
}

/*
 * Reproduce an actual failed pass rather than
 * manufacturing a loose-ball state or event.
 */
static void prepare_loose(
    RmcMatchState *state,
    RmcPcg32 *rng,
    RmcMatchEvent storage[3],
    RmcMatchEventLog *log)
{
    RmcPcg32 candidate = match_rng();

    for (unsigned attempt = 0; attempt < 1024; ++attempt) {
        *state = active_state();
        *rng = candidate;
        memset(storage, 0, 3 * sizeof(storage[0]));

        assert(rmc_match_event_log_init(
            state, storage, 3, log
        ));

        const RmcPassAction action = pass_action(state);
        RmcPassResult result = {0};

        assert(rmc_match_pass_apply(
            &world, state, rng, log, &action, &result
        ));

        if (result.outcome == RMC_PASS_OUTCOME_LOOSE_BALL) {
            assert(state->possession.controller.value == 0);
            assert(log->count == 1);
            assert(storage[0].type == RMC_EVENT_ACTION);
            assert(storage[0].outcome_code ==
                   RMC_PASS_OUTCOME_LOOSE_BALL);
            return;
        }

        (void)rmc_pcg32_next(&candidate);
    }

    assert(!"No failed pass found in deterministic search");
}

static void reject_unchanged(
    RmcMatchState *state,
    RmcPcg32 *rng,
    RmcMatchEventLog *log,
    const RmcRecoveryAction *action)
{
    const RmcMatchState before_state = *state;
    const RmcPcg32 before_rng = *rng;
    const RmcMatchEventLog before_log = *log;

    RmcMatchEvent before_storage[3] = {{0}};
    assert(log->events != NULL && log->capacity <= 3);

    memcpy(
        before_storage, log->events,
        log->capacity * sizeof(log->events[0])
    );

    RmcRecoveryResult result = {
        .probability = 777,
        .random_draw = 888,
        .outcome = RMC_RECOVERY_RETAINED,
        .winner = {999}
    };
    const RmcRecoveryResult before_result = result;

    assert(!rmc_match_recovery_apply(
        &world, state, rng, log, action, &result
    ));

    assert(memcmp(state, &before_state, sizeof(*state)) == 0);
    assert(memcmp(rng, &before_rng, sizeof(*rng)) == 0);
    assert(memcmp(log, &before_log, sizeof(*log)) == 0);
    assert(memcmp(
        log->events, before_storage,
        log->capacity * sizeof(log->events[0])
    ) == 0);
    assert(memcmp(
        &result, &before_result, sizeof(result)
    ) == 0);
}

static void test_both_outcomes_and_causality(void)
{
    RmcMatchState loose;
    RmcPcg32 loose_rng;
    RmcMatchEvent cause_storage[3] = {{0}};
    RmcMatchEventLog cause_log = {0};

    prepare_loose(
        &loose, &loose_rng, cause_storage, &cause_log
    );

    const RmcRecoveryAction action = recovery_action(&loose);
    const RmcMatchEvent cause = cause_storage[0];

    RmcPcg32 candidate = loose_rng;
    bool retained_seen = false;
    bool turnover_seen = false;

    for (unsigned trial = 0; trial < 1024; ++trial) {
        RmcMatchState state = loose;
        RmcPcg32 rng = candidate;
        RmcPcg32 expected_rng = candidate;
        RmcMatchEvent storage[3] = {{0}};
        RmcMatchEventLog log = {0};
        RmcRecoveryResult result = {0};

        assert(rmc_match_event_log_init(
            &state, storage, 3, &log
        ));
        assert(rmc_match_event_log_append(
            &world, &state, &log, &cause
        ));

        uint32_t expected_draw = UINT32_MAX;
        assert(rmc_pcg32_bounded(
            &expected_rng, RMC_PROBABILITY_SCALE,
            &expected_draw
        ));

        assert(rmc_match_recovery_apply(
            &world, &state, &rng, &log, &action, &result
        ));

        const bool retained =
            expected_draw < result.probability;

        assert(result.random_draw == expected_draw);
        assert(result.outcome ==
               (retained ? RMC_RECOVERY_RETAINED :
                           RMC_RECOVERY_TURNOVER));
        assert(memcmp(&rng, &expected_rng, sizeof(rng)) == 0);

        assert(state.elapsed_seconds ==
               loose.elapsed_seconds + action.duration_seconds);
        assert(log.count == 2);
        assert(storage[1].id == 2);
        assert(storage[1].type == RMC_EVENT_RECOVERY);
        assert(storage[1].cause_event_id == cause.id);
        assert(storage[1].at_second == state.elapsed_seconds);
        assert(storage[1].primary_actor.value ==
               result.winner.value);

        if (retained) {
            retained_seen = true;
            assert(state.possession.team_id.value ==
                   loose.possession.team_id.value);
            assert(state.possession.controller.value ==
                   action.incumbent.value);
            assert(state.possession.action_count ==
                   loose.possession.action_count + 1);
            assert(state.possession.zone.sector == 1);
            assert(result.winner.value ==
                   action.incumbent.value);
        } else {
            turnover_seen = true;
            assert(state.possession.team_id.value ==
                   loose.fixture.away_team.value);
            assert(state.possession.initiator.value ==
                   action.challenger.value);
            assert(state.possession.controller.value ==
                   action.challenger.value);
            assert(state.possession.action_count == 0);
            assert(state.possession.elapsed_seconds == 0);
            assert(state.possession.zone.sector == 3);
            assert(state.possession.phase ==
                   RMC_PHASE_TRANSITION);
            assert(result.winner.value ==
                   action.challenger.value);
        }

        assert(state.possession.zone.lane == RMC_LANE_CENTRE);
        assert(rmc_match_state_valid(&world, &state));

        /* Once the ball is controlled, it cannot be recovered again. */
        reject_unchanged(&state, &rng, &log, &action);

        if (retained_seen && turnover_seen) {
            break;
        }

        (void)rmc_pcg32_next(&candidate);
    }

    assert(retained_seen);
    assert(turnover_seen);
}

static void test_invalid_actions_and_atomicity(void)
{
    RmcMatchState state;
    RmcPcg32 rng;
    RmcMatchEvent storage[3] = {{0}};
    RmcMatchEventLog log = {0};

    prepare_loose(&state, &rng, storage, &log);

    const RmcRecoveryAction valid = recovery_action(&state);
    RmcRecoveryAction bad = valid;

    bad.incumbent = state.away_lineup.players[2];
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.challenger = state.home_lineup.players[3];
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.challenger.value = UINT64_MAX;
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
    bad.duration_seconds = 2700;
    reject_unchanged(&state, &rng, &log, &bad);

    RmcMatchEventLog invalid_log = log;
    invalid_log.count = invalid_log.capacity;
    reject_unchanged(&state, &rng, &invalid_log, &valid);

    invalid_log = log;
    invalid_log.count = 0;
    reject_unchanged(&state, &rng, &invalid_log, &valid);

    invalid_log = log;
    invalid_log.match_id.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &invalid_log, &valid);

    storage[0].type = RMC_EVENT_RECOVERY;
    reject_unchanged(&state, &rng, &log, &valid);
    storage[0].type = RMC_EVENT_ACTION;

    storage[0].outcome_code = RMC_PASS_OUTCOME_COMPLETED;
    reject_unchanged(&state, &rng, &log, &valid);
    storage[0].outcome_code = RMC_PASS_OUTCOME_LOOSE_BALL;

    state.possession.action_count = RMC_POSSESSION_MAX_ACTIONS;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.action_count = 1;

    state.mode = RMC_MATCH_RESTART_PENDING;
    reject_unchanged(&state, &rng, &log, &valid);
}

static void test_deterministic_replay(void)
{
    RmcMatchState first;
    RmcPcg32 first_rng;
    RmcMatchEvent first_storage[3] = {{0}};
    RmcMatchEventLog first_log = {0};

    prepare_loose(
        &first, &first_rng, first_storage, &first_log
    );

    RmcMatchState second = first;
    RmcPcg32 second_rng = first_rng;
    RmcMatchEvent second_storage[3] = {{0}};
    RmcMatchEventLog second_log = {0};

    assert(rmc_match_event_log_init(
        &second, second_storage, 3, &second_log
    ));
    assert(rmc_match_event_log_append(
        &world, &second, &second_log, &first_storage[0]
    ));

    const RmcRecoveryAction action = recovery_action(&first);
    RmcRecoveryResult first_result = {0};
    RmcRecoveryResult second_result = {0};

    assert(rmc_match_recovery_apply(
        &world, &first, &first_rng,
        &first_log, &action, &first_result
    ));
    assert(rmc_match_recovery_apply(
        &world, &second, &second_rng,
        &second_log, &action, &second_result
    ));

    assert(memcmp(&first, &second, sizeof(first)) == 0);
    assert(memcmp(
        &first_rng, &second_rng, sizeof(first_rng)
    ) == 0);
    assert(first_log.count == second_log.count);
    assert(memcmp(
        first_storage, second_storage,
        first_log.count * sizeof(first_storage[0])
    ) == 0);
    assert(first_result.probability == second_result.probability);
    assert(first_result.random_draw == second_result.random_draw);
    assert(first_result.outcome == second_result.outcome);
    assert(first_result.winner.value ==
           second_result.winner.value);
}

int main(void)
{
    setup();

    test_both_outcomes_and_causality();
    test_invalid_actions_and_atomicity();
    test_deterministic_replay();

    puts("RECOVERY_BOTH_OUTCOMES=PASS");
    puts("RECOVERY_TURNOVER_COORDINATES=PASS");
    puts("RECOVERY_CAUSAL_EVENT=PASS");
    puts("RECOVERY_NO_DUPLICATE_RESOLUTION=PASS");
    puts("RECOVERY_INVALID_REJECTION_ATOMICITY=PASS");
    puts("RECOVERY_PCG32_REPLAY=PASS");

    return 0;
}
