#include "match_recovery.h"
#include "match_transitions.h"
#include "match_tackle_action.h"
#include "match_cross_action.h"
#include "match_sequence_apply.h"

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

/* Generate the cause through production tackle resolution. */
static void prepare_tackle_loose(
    RmcMatchState *state,
    RmcPcg32 *rng,
    RmcMatchEvent storage[3],
    RmcMatchEventLog *log,
    uint32_t action_count,
    uint32_t at_second,
    bool second_half)
{
    RmcPcg32 candidate = match_rng();

    for (unsigned attempt = 0; attempt < 1024; ++attempt) {
        *state = active_state();
        state->possession.action_count = action_count;
        state->elapsed_seconds = at_second;
        if (second_half) {
            state->period = RMC_MATCH_SECOND_HALF;
            state->second_half_start_second = 2700;
        }
        *rng = candidate;
        memset(storage, 0, 3 * sizeof(storage[0]));
        assert(rmc_match_event_log_init(state, storage, 3, log));
        const RmcTackleAction action = {
            .team_id = state->fixture.away_team,
            .carrier = state->possession.controller,
            .defender = state->away_lineup.players[1],
            .duration_seconds = 5
        };
        RmcTackleResult result = {0};
        assert(rmc_match_tackle_apply(
            &world, state, rng, log, &action, &result
        ));
        if (result.outcome == RMC_TACKLE_OUTCOME_LOOSE_BALL) {
            assert(state->possession.controller.value == 0);
            assert(state->possession.team_id.value ==
                   state->fixture.home_team.value);
            assert(state->possession.action_count == action_count + 1);
            assert(log->count == 1);
            assert(storage[0].type == RMC_EVENT_ACTION);
            assert(storage[0].outcome_code == RMC_TACKLE_OUTCOME_LOOSE_BALL);
            assert(storage[0].team_id.value == state->possession.team_id.value);
            assert(storage[0].primary_actor.value == action.defender.value);
            return;
        }
        (void)rmc_pcg32_next(&candidate);
    }
    assert(!"No tackle loose ball found in deterministic search");
}

/* Search actual production crosses; never alter the resolved outcome. */
static void prepare_cross_loose(
    RmcMatchState *state,
    RmcPcg32 *rng,
    RmcMatchEvent storage[3],
    RmcMatchEventLog *log,
    uint32_t action_count,
    uint32_t at_second,
    bool second_half,
    uint8_t source_sector)
{
    RmcPcg32 candidate = match_rng();
    for (unsigned attempt = 0; attempt < 1024; ++attempt) {
        *state = active_state();
        state->possession.zone.sector = source_sector;
        state->possession.zone.lane = second_half ? RMC_LANE_RIGHT : RMC_LANE_LEFT;
        state->possession.action_count = action_count;
        state->elapsed_seconds = at_second;
        if (second_half) {
            state->period = RMC_MATCH_SECOND_HALF;
            state->second_half_start_second = 2700;
        }
        *rng = candidate;
        memset(storage, 0, 3 * sizeof(storage[0]));
        assert(rmc_match_event_log_init(state, storage, 3, log));
        const RmcCrossAction action = {
            .team_id = state->possession.team_id,
            .crosser = state->possession.controller,
            .recipient = state->home_lineup.players[2],
            .defender = state->away_lineup.players[1],
            .duration_seconds = 5
        };
        RmcCrossResult result = {0};
        assert(rmc_match_cross_apply(
            &world, state, rng, log, &action, &result
        ));
        if (result.outcome == RMC_CROSS_OUTCOME_LOOSE_BALL) {
            assert(state->possession.controller.value == 0);
            assert(state->possession.team_id.value == action.team_id.value);
            assert(state->possession.zone.sector == source_sector + 1);
            assert(state->possession.zone.lane == RMC_LANE_CENTRE);
            assert(state->possession.action_count == action_count + 1);
            assert(state->possession.phase == (source_sector == 3
                ? RMC_PHASE_CHANCE : RMC_PHASE_ATTACKING_DEVELOPMENT));
            assert(log->count == 1);
            assert(storage[0].type == RMC_EVENT_ACTION);
            assert(storage[0].outcome_code == RMC_CROSS_OUTCOME_LOOSE_BALL);
            assert(storage[0].team_id.value == action.team_id.value);
            assert(storage[0].primary_actor.value == action.crosser.value);
            assert(storage[0].secondary_actor.value == action.recipient.value);
            return;
        }
        (void)rmc_pcg32_next(&candidate);
    }
    assert(!"No cross loose ball found in deterministic search");
}

/* 0: pass, 1: tackle, 2/3: cross from the corresponding sector. */
static void prepare_cause(
    unsigned source,
    RmcMatchState *state,
    RmcPcg32 *rng,
    RmcMatchEvent storage[3],
    RmcMatchEventLog *log)
{
    if (source >= 2) {
        prepare_cross_loose(state, rng, storage, log, 0, 0, false,
                            (uint8_t)source);
    } else if (source == 1) {
        prepare_tackle_loose(state, rng, storage, log, 0, 0, false);
    } else {
        prepare_loose(state, rng, storage, log);
    }
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

static void test_both_outcomes_and_causality(unsigned source)
{
    RmcMatchState loose;
    RmcPcg32 loose_rng;
    RmcMatchEvent cause_storage[3] = {{0}};
    RmcMatchEventLog cause_log = {0};

    prepare_cause(source, &loose, &loose_rng, cause_storage, &cause_log);

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
        assert(storage[1].match_id.value == state.fixture.id.value);
        assert(storage[1].team_id.value == state.possession.team_id.value);
        assert(storage[1].secondary_actor.value ==
               (retained ? action.challenger.value : action.incumbent.value));
        assert(storage[1].outcome_code == (uint32_t)result.outcome);
        assert(memcmp(&storage[0], &cause, sizeof(cause)) == 0);

        if (retained) {
            retained_seen = true;
            assert(state.possession.team_id.value ==
                   loose.possession.team_id.value);
            assert(state.possession.controller.value ==
                   action.incumbent.value);
            assert(state.possession.action_count ==
                   loose.possession.action_count + 1);
            assert(state.possession.phase == loose.possession.phase);
            assert(state.possession.elapsed_seconds ==
                   loose.possession.elapsed_seconds + action.duration_seconds);
            assert(state.possession.zone.sector == loose.possession.zone.sector);
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
            assert(state.possession.zone.sector == 4 - loose.possession.zone.sector);
            assert(state.possession.phase ==
                   RMC_PHASE_TRANSITION);
            assert(result.winner.value ==
                   action.challenger.value);
        }

        assert(state.possession.zone.lane == RMC_LANE_CENTRE);
        assert(rmc_match_state_valid(&world, &state));

        /* Once the ball is controlled, it cannot be recovered again. */
        reject_unchanged(&state, &rng, &log, &action);
        /* Even clearing the controller cannot make the resolved cause latest. */
        state.possession.controller.value = 0;
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

static void test_deterministic_replay(unsigned source)
{
    RmcMatchState first;
    RmcPcg32 first_rng;
    RmcMatchEvent first_storage[3] = {{0}};
    RmcMatchEventLog first_log = {0};

    prepare_cause(source, &first, &first_rng, first_storage, &first_log);

    RmcMatchState second = first;
    RmcPcg32 second_rng = first_rng;
    RmcMatchEvent second_storage[3] = {{0}};
    RmcMatchEventLog second_log = {0};

    if (source != 0) {
        prepare_cause(source, &second, &second_rng, second_storage, &second_log);
        assert(memcmp(&first, &second, sizeof(first)) == 0);
        assert(memcmp(&first_rng, &second_rng, sizeof(first_rng)) == 0);
        assert(memcmp(first_storage, second_storage, sizeof(first_storage)) == 0);
    } else {
        assert(rmc_match_event_log_init(
            &second, second_storage, 3, &second_log
        ));
        assert(rmc_match_event_log_append(
            &world, &second, &second_log, &first_storage[0]
        ));
    }

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

static void test_cause_rejections(unsigned source)
{
    RmcMatchState state;
    RmcPcg32 rng;
    RmcMatchEvent storage[3] = {{0}};
    RmcMatchEventLog log = {0};
    prepare_cause(source, &state, &rng, storage, &log);
    const RmcRecoveryAction action = recovery_action(&state);
    const RmcMatchEvent cause = storage[0];

    const uint32_t invalid_outcomes[] = {
        RMC_PASS_OUTCOME_COMPLETED, RMC_TACKLE_OUTCOME_RETAINED,
        RMC_TACKLE_OUTCOME_WON, RMC_CROSS_OUTCOME_COMPLETED,
        RMC_CROSS_OUTCOME_INTERCEPTED, 0, 101, 201, 404, UINT32_MAX
    };
    for (size_t i = 0;
         i < sizeof(invalid_outcomes) / sizeof(invalid_outcomes[0]); ++i) {
        storage[0].outcome_code = invalid_outcomes[i];
        reject_unchanged(&state, &rng, &log, &action);
    }
    storage[0] = cause;
    storage[0].type = RMC_EVENT_RECOVERY;
    reject_unchanged(&state, &rng, &log, &action);
    storage[0] = cause;
    storage[0].match_id.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &log, &action);
    storage[0] = cause;
    storage[0].team_id = state.fixture.away_team;
    reject_unchanged(&state, &rng, &log, &action);
    storage[0].team_id.value = 0;
    reject_unchanged(&state, &rng, &log, &action);
    storage[0] = cause;
    storage[0].id = 0;
    reject_unchanged(&state, &rng, &log, &action);
    storage[0].id = 2;
    reject_unchanged(&state, &rng, &log, &action);
    storage[0] = cause;
    storage[0].at_second = state.elapsed_seconds + 1;
    reject_unchanged(&state, &rng, &log, &action);
    storage[0] = cause;
    storage[0].cause_event_id = cause.id;
    reject_unchanged(&state, &rng, &log, &action);
    storage[0] = cause;

    state.possession.controller = action.incumbent;
    reject_unchanged(&state, &rng, &log, &action);
    state.possession.controller.value = 0;
    RmcRecoveryAction bad = action;
    bad.incumbent = state.away_lineup.players[2];
    reject_unchanged(&state, &rng, &log, &bad);
    bad = action;
    bad.challenger = state.home_lineup.players[1];
    reject_unchanged(&state, &rng, &log, &bad);

    /* An older qualifying event does not authorize recovery. */
    RmcMatchEvent later = {
        .id = 2, .match_id = state.fixture.id,
        .at_second = state.elapsed_seconds,
        .type = RMC_EVENT_TACTICAL_DECISION
    };
    assert(rmc_match_event_log_append(&world, &state, &log, &later));
    reject_unchanged(&state, &rng, &log, &action);

    /* A latest qualifying cause still cannot predate its predecessor. */
    storage[1] = cause;
    storage[1].id = 2;
    storage[1].at_second = cause.at_second - 1;
    reject_unchanged(&state, &rng, &log, &action);
}

static void test_action_window_and_period_precedence(bool from_cross)
{
    /* Repeat in each half, before and exactly at its period deadline. */
    for (unsigned second_half = 0; second_half < 2; ++second_half) {
        for (unsigned at_end = 0; at_end < 2; ++at_end) {
            RmcMatchState state;
            RmcPcg32 rng;
            RmcMatchEvent storage[3] = {{0}};
            RmcMatchEventLog log = {0};
            const uint32_t start = second_half ? 2700 : 0;
            if (from_cross) {
                prepare_cross_loose(
                    &state, &rng, storage, &log,
                    RMC_POSSESSION_MAX_ACTIONS - 1,
                    start + (at_end ? 2695 : 0), second_half != 0,
                    second_half ? 3 : 2
                );
            } else {
                prepare_tackle_loose(
                    &state, &rng, storage, &log,
                    RMC_POSSESSION_MAX_ACTIONS - 1,
                    start + (at_end ? 2695 : 0), second_half != 0
                );
            }
            assert(state.possession.action_count == RMC_POSSESSION_MAX_ACTIONS);
            const RmcRecoveryAction action = recovery_action(&state);
            reject_unchanged(&state, &rng, &log, &action);
            const RmcMatchState before = state;
            const RmcPcg32 before_rng = rng;
            const RmcMatchEventLog before_log = log;
            RmcMatchEvent before_storage[3];
            memcpy(before_storage, storage, sizeof(storage));
            RmcSequenceDecision decision = RMC_SEQUENCE_ALLOW_ACTION;
            assert(rmc_match_sequence_decide(&world, &state, &decision));

            if (at_end) {
                assert(decision == RMC_SEQUENCE_END_PERIOD);
                assert(!rmc_match_sequence_apply(
                    &world, &state, RMC_SEQUENCE_RESET_ACTION_WINDOW
                ));
                assert(memcmp(&state, &before, sizeof(state)) == 0);
                reject_unchanged(&state, &rng, &log, &action);
                assert(rmc_match_sequence_apply(
                    &world, &state, RMC_SEQUENCE_END_PERIOD
                ));
                assert(state.period == (second_half
                    ? RMC_MATCH_FINISHED : RMC_MATCH_HALF_TIME));
                assert(state.mode == RMC_MATCH_PERIOD_ENDED);
                reject_unchanged(&state, &rng, &log, &action);
            } else {
                assert(decision == RMC_SEQUENCE_RESET_ACTION_WINDOW);
                assert(rmc_match_sequence_apply(
                    &world, &state, RMC_SEQUENCE_RESET_ACTION_WINDOW
                ));
                RmcMatchState expected = before;
                expected.possession.action_count = 0;
                assert(memcmp(&state, &expected, sizeof(state)) == 0);
                assert(state.possession.controller.value == 0);
                assert(memcmp(&rng, &before_rng, sizeof(rng)) == 0);
                assert(memcmp(&log, &before_log, sizeof(log)) == 0);
                assert(memcmp(storage, before_storage, sizeof(storage)) == 0);
                assert(rmc_match_sequence_decide(&world, &state, &decision));
                assert(decision == RMC_SEQUENCE_ALLOW_ACTION);
                RmcRecoveryResult result = {0};
                assert(rmc_match_recovery_apply(
                    &world, &state, &rng, &log, &action, &result
                ));
                assert(log.count == 2);
                assert(storage[1].cause_event_id == before_storage[0].id);
                assert(memcmp(&storage[0], &before_storage[0],
                              sizeof(storage[0])) == 0);
                reject_unchanged(&state, &rng, &log, &action);
            }
            if (at_end) {
                assert(memcmp(&rng, &before_rng, sizeof(rng)) == 0);
                assert(memcmp(&log, &before_log, sizeof(log)) == 0);
                assert(memcmp(storage, before_storage, sizeof(storage)) == 0);
            }
        }
    }
}

int main(void)
{
    setup();

    test_both_outcomes_and_causality(false);
    test_both_outcomes_and_causality(true);
    test_invalid_actions_and_atomicity();
    test_deterministic_replay(false);
    test_deterministic_replay(true);
    test_both_outcomes_and_causality(2);
    test_both_outcomes_and_causality(3);
    test_deterministic_replay(2);
    test_deterministic_replay(3);
    test_cause_rejections(1);
    test_cause_rejections(2);
    test_cause_rejections(3);
    test_action_window_and_period_precedence(false);
    test_action_window_and_period_precedence(true);

    puts("RECOVERY_BOTH_OUTCOMES=PASS");
    puts("RECOVERY_TURNOVER_COORDINATES=PASS");
    puts("RECOVERY_CAUSAL_EVENT=PASS");
    puts("RECOVERY_NO_DUPLICATE_RESOLUTION=PASS");
    puts("RECOVERY_INVALID_REJECTION_ATOMICITY=PASS");
    puts("RECOVERY_PCG32_REPLAY=PASS");
    puts("RECOVERY_TACKLE_BOTH_OUTCOMES_AND_CAUSALITY=PASS");
    puts("RECOVERY_TACKLE_CAUSE_REJECTION=PASS");
    puts("RECOVERY_ACTION_32_RESET_PRESERVES_LOOSE_BALL=PASS");
    puts("RECOVERY_PERIOD_END_PRECEDENCE=PASS");
    puts("RECOVERY_CROSS_BOTH_OUTCOMES_GEOMETRY_AND_CAUSALITY=PASS");
    puts("RECOVERY_CROSS_CAUSE_REJECTION_AND_REPLAY=PASS");
    puts("RECOVERY_CROSS_ACTION_32_RESET_AND_PERIOD_END=PASS");

    return 0;
}
