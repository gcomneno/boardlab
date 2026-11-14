#include "match_recovery.h"
#include "match_transitions.h"
#include "match_tackle_action.h"
#include "match_cross_action.h"
#include "match_sequence_apply.h"
#include "match_shot_action.h"
#include "match_clock.h"

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

enum { SHOT_STORAGE = 8 };

typedef struct {
    RmcMatchState state;
    RmcPcg32 rng;
    RmcMatchEvent storage[SHOT_STORAGE];
    RmcMatchEventLog log;
    RmcRecoveryResult result;
} ShotRun;

static const RmcShotEventOutcome loose_shots[] = {
    RMC_SHOT_SAVED_REBOUND, RMC_SHOT_BLOCKED, RMC_SHOT_WOODWORK
};

static RmcRecoveryAction shot_recovery_action(const ShotRun *run)
{
    const bool home = run->state.possession.team_id.value ==
                      run->state.fixture.home_team.value;
    const RmcRecoveryAction action = {
        .incumbent = home ? run->state.home_lineup.players[2] :
                            run->state.away_lineup.players[2],
        .challenger = home ? run->state.away_lineup.players[1] :
                             run->state.home_lineup.players[1],
        .context = -3, .duration_seconds = 3
    };
    return action;
}

static void copy_shot_run(ShotRun *to, const ShotRun *from)
{
    memcpy(to, from, sizeof(*to));
    to->log.events = to->storage;
}

static bool recover_shot(ShotRun *run, const RmcRecoveryAction *action)
{
    return rmc_match_recovery_apply(&world, &run->state, &run->rng,
                                    &run->log, action, &run->result);
}

static void reject_shot(ShotRun *run, const RmcRecoveryAction *action)
{
    unsigned char before[sizeof(*run)];
    memcpy(before, run, sizeof(*run));
    assert(!recover_shot(run, action));
    /* Includes all metadata, result padding and unused backing slots, even
     * when deliberately corrupting the advertised count/capacity/pointer.
     */
    assert(memcmp(before, run, sizeof(*run)) == 0);
}

/* Deterministic search through the real match stream. All event records and
 * loose-ball effects come from production calls; no final outcome is edited.
 */
static void prepare_shot(
    ShotRun *run, RmcShotEventOutcome wanted, bool home,
    RmcTacticalZone source, uint32_t action_count, uint32_t at_second,
    bool second_half, bool via_cross)
{
    RmcPcg32 candidate = match_rng();
    for (unsigned trial = 0; trial < 4096; ++trial) {
        memset(run, 0xa5, sizeof(*run));
        run->state = active_state();
        RmcMatchState *state = &run->state;
        const RmcLineup *attack = home ? &state->home_lineup : &state->away_lineup;
        const RmcLineup *defence = home ? &state->away_lineup : &state->home_lineup;
        state->possession.team_id = home ? state->fixture.home_team :
                                          state->fixture.away_team;
        state->possession.initiator = attack->players[3];
        state->possession.controller = attack->players[1];
        state->possession.zone = source;
        state->possession.phase = RMC_PHASE_ATTACKING_DEVELOPMENT;
        state->possession.action_count = action_count;
        state->elapsed_seconds = at_second;
        if (second_half) {
            state->period = RMC_MATCH_SECOND_HALF;
            state->second_half_start_second = 2700;
        }
        run->rng = candidate;
        assert(rmc_match_event_log_init(state, run->storage, SHOT_STORAGE,
                                        &run->log));
        bool completed = true;
        if (via_cross) {
            const RmcCrossAction cross = {
                .team_id = state->possession.team_id,
                .crosser = state->possession.controller,
                .recipient = attack->players[2], .defender = defence->players[1],
                .duration_seconds = 5
            };
            RmcCrossResult crossed = {0};
            assert(rmc_match_cross_apply(&world, state, &run->rng, &run->log,
                                         &cross, &crossed));
            completed = crossed.outcome == RMC_CROSS_OUTCOME_COMPLETED;
            if (completed) {
                assert(state->possession.controller.value == cross.recipient.value);
            }
        }
        if (completed) {
            const RmcTacticalZone shot_source = state->possession.zone;
            const RmcPossessionPhase shot_phase = state->possession.phase;
            const RmcShotAction shot = {
                .team_id = state->possession.team_id,
                .shooter = state->possession.controller,
                .defender = defence->players[1], .goalkeeper = defence->players[0],
                .duration_seconds = 5
            };
            RmcShotResult resolved = {0};
            assert(rmc_match_shot_apply(&world, state, &run->rng, &run->log,
                                        &shot, &resolved));
            if (resolved.outcome == wanted) {
                const size_t final = via_cross ? 2u : 1u;
                assert(run->storage[final].type == RMC_EVENT_SHOT);
                assert(run->storage[final].outcome_code == (uint32_t)wanted);
                assert(run->storage[final].cause_event_id == final);
                if (state->mode == RMC_MATCH_BALL_IN_PLAY &&
                    wanted != RMC_SHOT_SAVED_HELD) {
                    assert(state->possession.controller.value == 0);
                    assert(state->possession.team_id.value == shot.team_id.value);
                    assert(state->possession.initiator.value == attack->players[3].value);
                    assert(state->possession.action_count ==
                           action_count + (via_cross ? 2u : 1u));
                    assert(state->possession.zone.sector ==
                           (wanted == RMC_SHOT_BLOCKED ? shot_source.sector : 4));
                    assert(state->possession.zone.lane ==
                           (wanted == RMC_SHOT_BLOCKED ? shot_source.lane : RMC_LANE_CENTRE));
                    assert(state->possession.phase ==
                           (wanted == RMC_SHOT_BLOCKED ? shot_phase :
                                                       RMC_PHASE_CHANCE));
                }
                return;
            }
        }
        (void)rmc_pcg32_next(&candidate);
    }
    assert(!"No requested production shot found in deterministic search");
}

static int32_t contender_effectiveness(RmcPlayerId id)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
            const RmcPlayer *p = &world.rosters[i].players[j];
            if (p->id.value == id.value) {
                return ((int32_t)p->attributes[RMC_ATTR_CONTRASTO] +
                        (int32_t)p->attributes[RMC_ATTR_POSIZIONAMENTO]) / 2;
            }
        }
    }
    assert(false);
    return 0;
}

/* Replay compares semantic fields; byte snapshots are only for immutability. */
static void same_shot_event(const RmcMatchEvent *a, const RmcMatchEvent *b)
{
    assert(a->id == b->id && a->match_id.value == b->match_id.value);
    assert(a->type == b->type && a->at_second == b->at_second);
    assert(a->team_id.value == b->team_id.value);
    assert(a->primary_actor.value == b->primary_actor.value);
    assert(a->secondary_actor.value == b->secondary_actor.value);
    assert(a->outcome_code == b->outcome_code && a->cause_event_id == b->cause_event_id);
    assert(a->shot.model_version == b->shot.model_version);
    assert(a->shot.chance_quality == b->shot.chance_quality);
    assert(a->shot.source_zone.sector == b->shot.source_zone.sector);
    assert(a->shot.source_zone.lane == b->shot.source_zone.lane);
    assert(a->shot.context == b->shot.context);
    assert(a->shot.duration_seconds == b->shot.duration_seconds);
    assert(a->shot.defender_id.value == b->shot.defender_id.value);
    assert(a->shot.goalkeeper_id.value == b->shot.goalkeeper_id.value);
    assert(a->shot.applied_talent_mask == b->shot.applied_talent_mask);
    assert(a->shot.trajectory == b->shot.trajectory);
    assert(a->shot.restart_kind == b->shot.restart_kind);
    assert(a->shot.restart_awarded_team.value == b->shot.restart_awarded_team.value);
    assert(a->shot.restart_zone.sector == b->shot.restart_zone.sector);
    assert(a->shot.restart_zone.lane == b->shot.restart_zone.lane);
    assert(a->shot.restart_disposition == b->shot.restart_disposition);
}

static void same_recovered_state(const RmcMatchState *a, const RmcMatchState *b)
{
    assert(a->fixture.id.value == b->fixture.id.value);
    assert(a->fixture.home_team.value == b->fixture.home_team.value);
    assert(a->fixture.away_team.value == b->fixture.away_team.value);
    assert(a->fixture.matchday == b->fixture.matchday);
    assert(a->period == b->period && a->mode == b->mode);
    assert(a->terminal_reason == b->terminal_reason);
    assert(a->elapsed_seconds == b->elapsed_seconds);
    assert(a->first_half_stoppage_seconds == b->first_half_stoppage_seconds);
    assert(a->second_half_stoppage_seconds == b->second_half_stoppage_seconds);
    assert(a->second_half_start_second == b->second_half_start_second);
    assert(a->home_goals == b->home_goals && a->away_goals == b->away_goals);
    assert(a->possession.team_id.value == b->possession.team_id.value);
    assert(a->possession.initiator.value == b->possession.initiator.value);
    assert(a->possession.controller.value == b->possession.controller.value);
    assert(a->possession.zone.sector == b->possession.zone.sector);
    assert(a->possession.zone.lane == b->possession.zone.lane);
    assert(a->possession.phase == b->possession.phase);
    assert(a->possession.action_count == b->possession.action_count);
    assert(a->possession.elapsed_seconds == b->possession.elapsed_seconds);
    const RmcLineup *al[] = {&a->home_lineup, &a->away_lineup};
    const RmcLineup *bl[] = {&b->home_lineup, &b->away_lineup};
    for (size_t side = 0; side < 2; ++side) {
        assert(al[side]->formation == bl[side]->formation);
        assert(al[side]->secondary_assignments == bl[side]->secondary_assignments);
        for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
            assert(al[side]->players[i].value == bl[side]->players[i].value);
            assert(al[side]->slots[i] == bl[side]->slots[i]);
        }
    }
}

static void test_shot_outcomes_and_replay(
    RmcShotEventOutcome outcome, bool home, RmcTacticalZone source, bool via_cross)
{
    ShotRun loose, replay;
    prepare_shot(&loose, outcome, home, source, 7, 100, false, via_cross);
    prepare_shot(&replay, outcome, home, source, 7, 100, false, via_cross);
    const RmcRecoveryAction action = shot_recovery_action(&loose);
    const RmcMatchState before = loose.state;
    const size_t prefix = loose.log.count;
    assert(prefix == (via_cross ? 3u : 2u));
    if (via_cross) {
        assert(loose.storage[0].type == RMC_EVENT_ACTION);
        assert(loose.storage[0].outcome_code == RMC_CROSS_OUTCOME_COMPLETED);
        assert(loose.storage[1].primary_actor.value ==
               loose.storage[0].secondary_actor.value);
    }
    uint32_t probability = 0;
    assert(rmc_match_probability(contender_effectiveness(action.incumbent),
                                  contender_effectiveness(action.challenger),
                                  action.context, &probability));
    RmcPcg32 candidate = loose.rng;
    bool retained_seen = false, turnover_seen = false;
    for (unsigned trial = 0; trial < 1024; ++trial) {
        ShotRun run, second;
        copy_shot_run(&run, &loose);
        copy_shot_run(&second, &replay);
        run.rng = candidate;
        second.rng = candidate;
        RmcPcg32 expected_rng = candidate;
        uint32_t expected_draw = UINT32_MAX;
        assert(rmc_pcg32_bounded(&expected_rng, RMC_PROBABILITY_SCALE, &expected_draw));
        assert(recover_shot(&run, &action));
        assert(recover_shot(&second, &action));
        const bool retained = expected_draw < probability;
        assert(run.result.probability == probability);
        assert(run.result.random_draw == expected_draw);
        assert(run.result.outcome == (retained ? RMC_RECOVERY_RETAINED : RMC_RECOVERY_TURNOVER));
        assert(run.result.winner.value ==
               (retained ? action.incumbent.value : action.challenger.value));
        assert(run.rng.state == expected_rng.state);
        assert(run.rng.increment == expected_rng.increment);
        assert(run.log.count == prefix + 1);
        const RmcMatchEvent *event = &run.storage[prefix];
        assert(event->id == prefix + 1 && event->type == RMC_EVENT_RECOVERY);
        assert(event->cause_event_id == loose.storage[prefix - 1].id);
        assert(event->cause_event_id != loose.storage[prefix - 2].id);
        assert(event->match_id.value == before.fixture.id.value);
        assert(event->at_second == before.elapsed_seconds + action.duration_seconds);
        assert(event->team_id.value == run.state.possession.team_id.value);
        assert(event->primary_actor.value == run.result.winner.value);
        assert(event->secondary_actor.value ==
               (retained ? action.challenger.value : action.incumbent.value));
        assert(event->outcome_code == (uint32_t)run.result.outcome);
        assert(event->shot.model_version == 0);
        assert(memcmp(run.storage, loose.storage, prefix * sizeof(run.storage[0])) == 0);
        assert(memcmp(&run.storage[prefix + 1], &loose.storage[prefix + 1],
                      (SHOT_STORAGE - prefix - 1) * sizeof(run.storage[0])) == 0);
        assert(run.state.elapsed_seconds == before.elapsed_seconds + action.duration_seconds);
        assert(run.state.mode == RMC_MATCH_BALL_IN_PLAY);
        assert(run.state.period == before.period);
        assert(run.state.home_goals == before.home_goals);
        assert(run.state.away_goals == before.away_goals);
        assert(run.state.possession.zone.lane == before.possession.zone.lane);
        assert(run.state.possession.controller.value == run.result.winner.value);
        if (retained) {
            retained_seen = true;
            assert(run.state.possession.team_id.value == before.possession.team_id.value);
            assert(run.state.possession.initiator.value == before.possession.initiator.value);
            assert(run.state.possession.action_count == before.possession.action_count + 1);
            assert(run.state.possession.elapsed_seconds ==
                   before.possession.elapsed_seconds + action.duration_seconds);
            assert(run.state.possession.phase == before.possession.phase);
            assert(run.state.possession.zone.sector == before.possession.zone.sector);
        } else {
            turnover_seen = true;
            assert(run.state.possession.team_id.value ==
                   (home ? before.fixture.away_team.value : before.fixture.home_team.value));
            assert(run.state.possession.initiator.value == action.challenger.value);
            assert(run.state.possession.action_count == 0);
            assert(run.state.possession.elapsed_seconds == 0);
            assert(run.state.possession.phase == RMC_PHASE_TRANSITION);
            assert(run.state.possession.zone.sector == 4 - before.possession.zone.sector);
        }
        assert(rmc_match_state_valid(&world, &run.state));
        same_recovered_state(&run.state, &second.state);
        assert(run.rng.state == second.rng.state && run.rng.increment == second.rng.increment);
        assert(run.log.count == second.log.count);
        assert(run.log.capacity == second.log.capacity);
        assert(run.log.match_id.value == second.log.match_id.value);
        for (size_t i = 0; i < run.log.count; ++i) {
            same_shot_event(&run.storage[i], &second.storage[i]);
        }
        assert(run.result.probability == second.result.probability);
        assert(run.result.random_draw == second.result.random_draw);
        assert(run.result.outcome == second.result.outcome);
        assert(run.result.winner.value == second.result.winner.value);
        reject_shot(&run, &action);
        run.state.possession.controller.value = 0;
        reject_shot(&run, &action);
        if (retained_seen && turnover_seen) {
            break;
        }
        (void)rmc_pcg32_next(&candidate);
    }
    assert(retained_seen && turnover_seen);
}

static void test_shot_rejections(RmcShotEventOutcome outcome)
{
    ShotRun original, run;
    prepare_shot(&original, outcome, true, (RmcTacticalZone){3, RMC_LANE_LEFT},
                 7, 100, false, true);
    const RmcRecoveryAction action = shot_recovery_action(&original);
    /* A cross prefix also proves validation reaches beyond the adjacent pair. */
    assert(original.log.count == 3);
#define REJECT_SHOT(...) do { \
    copy_shot_run(&run, &original); \
    __VA_ARGS__; \
    reject_shot(&run, &action); \
} while (0)
    const uint32_t invalid_outcomes[] = {
        RMC_SHOT_CHANCE, RMC_SHOT_GOAL, RMC_SHOT_SAVED_HELD, RMC_SHOT_WIDE,
        RMC_PASS_OUTCOME_LOOSE_BALL, RMC_TACKLE_OUTCOME_LOOSE_BALL,
        RMC_CROSS_OUTCOME_LOOSE_BALL, 0, 507, UINT32_MAX
    };
    for (size_t i = 0; i < sizeof(invalid_outcomes) / sizeof(invalid_outcomes[0]); ++i) {
        REJECT_SHOT(run.storage[2].outcome_code = invalid_outcomes[i]);
    }
    REJECT_SHOT(run.storage[2].type = RMC_EVENT_ACTION);
    REJECT_SHOT(run.storage[2].id = 0);
    REJECT_SHOT(run.storage[2].id = 4);
    REJECT_SHOT(run.storage[2].match_id.value++);
    REJECT_SHOT(run.storage[2].team_id = run.state.fixture.away_team);
    REJECT_SHOT(run.storage[2].primary_actor = run.state.home_lineup.players[3]);
    REJECT_SHOT(run.storage[2].secondary_actor = run.state.away_lineup.players[1]);
    REJECT_SHOT(run.storage[2].at_second = run.state.elapsed_seconds + 1);
    REJECT_SHOT(run.storage[2].at_second--);
    REJECT_SHOT(run.storage[2].cause_event_id = 0);
    REJECT_SHOT(run.storage[2].cause_event_id = 1);
    REJECT_SHOT(run.storage[2].cause_event_id = 3);
    REJECT_SHOT(run.storage[2].shot.model_version = 0);
    REJECT_SHOT(run.storage[2].shot.model_version = 2);
    REJECT_SHOT(run.storage[2].shot.chance_quality++);
    REJECT_SHOT(run.storage[2].shot.trajectory = RMC_SHOT_TRAJECTORY_NONE);
    REJECT_SHOT(run.storage[2].shot.restart_kind = RMC_RESTART_KICKOFF);
    REJECT_SHOT(run.storage[2].shot.restart_awarded_team = run.state.fixture.away_team);
    REJECT_SHOT(run.storage[2].shot.restart_zone.lane = RMC_LANE_CENTRE);
    REJECT_SHOT(run.storage[2].shot.restart_disposition = RMC_RESTART_PENDING);
    REJECT_SHOT(run.storage[1].type = RMC_EVENT_TACTICAL_DECISION);
    REJECT_SHOT(run.storage[1].outcome_code = RMC_CROSS_OUTCOME_COMPLETED);
    REJECT_SHOT(run.storage[1].id = 0);
    REJECT_SHOT(run.storage[1].match_id.value++);
    REJECT_SHOT(run.storage[1].team_id = run.state.fixture.away_team);
    REJECT_SHOT(run.storage[1].primary_actor = run.state.home_lineup.players[3]);
    REJECT_SHOT(run.storage[1].secondary_actor = run.state.away_lineup.players[1]);
    REJECT_SHOT(run.storage[1].at_second++);
    REJECT_SHOT(run.storage[1].cause_event_id = 2);
    REJECT_SHOT(run.storage[1].shot.model_version = 0);
    REJECT_SHOT(run.storage[1].shot.model_version = 2);
    REJECT_SHOT(run.storage[1].shot.chance_quality = 101);
    REJECT_SHOT(run.storage[1].shot.source_zone.sector = 1);
    REJECT_SHOT(run.storage[1].shot.source_zone.lane = RMC_LANE_COUNT);
    REJECT_SHOT(run.storage[1].shot.context = 21);
    REJECT_SHOT(run.storage[1].shot.context = -21);
    REJECT_SHOT(run.storage[1].shot.duration_seconds = 0);
    REJECT_SHOT(run.storage[1].shot.duration_seconds = UINT32_MAX);
    REJECT_SHOT(run.storage[1].shot.defender_id = run.state.home_lineup.players[1]);
    REJECT_SHOT(run.storage[1].shot.goalkeeper_id = run.state.away_lineup.players[1]);
    REJECT_SHOT(run.storage[1].shot.applied_talent_mask = UINT32_MAX);
    REJECT_SHOT(run.storage[1].shot.trajectory = RMC_SHOT_TRAJECTORY_ON_TARGET);
    REJECT_SHOT(run.storage[1].shot.restart_kind = RMC_RESTART_GOAL_KICK);
    /* Matching corrupt payloads/actors must also fail schema/role validation. */
    REJECT_SHOT(run.storage[1].shot.chance_quality = 101;
                run.storage[2].shot.chance_quality = 101);
    REJECT_SHOT(run.storage[1].shot.duration_seconds = 0;
                run.storage[2].shot.duration_seconds = 0);
    REJECT_SHOT(run.storage[1].primary_actor = run.state.home_lineup.players[0];
                run.storage[2].primary_actor = run.state.home_lineup.players[0]);
    REJECT_SHOT(run.storage[1].primary_actor = run.state.away_lineup.players[2];
                run.storage[2].primary_actor = run.state.away_lineup.players[2]);
    REJECT_SHOT(run.storage[1].primary_actor.value = UINT64_MAX;
                run.storage[2].primary_actor.value = UINT64_MAX);
    REJECT_SHOT(run.storage[1].shot.defender_id = run.state.home_lineup.players[1];
                run.storage[2].shot.defender_id = run.state.home_lineup.players[1]);
    REJECT_SHOT(run.storage[0].id = 0);
    REJECT_SHOT(run.storage[0].match_id.value++);
    REJECT_SHOT(run.storage[0].team_id.value = UINT64_MAX);
    REJECT_SHOT(run.storage[0].primary_actor.value = UINT64_MAX);
    REJECT_SHOT(run.storage[0].at_second = run.state.elapsed_seconds + 1);
    REJECT_SHOT(run.storage[0].cause_event_id = 1);
    REJECT_SHOT(run.storage[0].shot.model_version = 2);
    REJECT_SHOT(run.state.possession.zone.sector = 3);
    REJECT_SHOT(run.state.possession.zone.lane = RMC_LANE_RIGHT);
    if (outcome != RMC_SHOT_BLOCKED) {
        REJECT_SHOT(run.state.possession.phase = RMC_PHASE_BUILDUP);
    }
    REJECT_SHOT(run.state.possession.controller = action.incumbent);
    REJECT_SHOT(run.state.possession.controller = action.challenger);
    REJECT_SHOT(run.state.fixture.id.value++);
    REJECT_SHOT(run.state.possession.team_id = run.state.fixture.away_team;
                run.state.possession.initiator = run.state.away_lineup.players[2]);
    REJECT_SHOT(run.state.possession.action_count = RMC_POSSESSION_MAX_ACTIONS);
    REJECT_SHOT(run.state.possession.action_count = UINT32_MAX);
    REJECT_SHOT(run.state.possession.elapsed_seconds = run.state.elapsed_seconds + 1);
    REJECT_SHOT(run.state.possession.elapsed_seconds = 4);
    REJECT_SHOT(run.state.period = RMC_MATCH_SECOND_HALF;
                run.state.second_half_start_second = 2700;
                run.state.elapsed_seconds = 2700);
    REJECT_SHOT(run.state.mode = RMC_MATCH_RESTART_PENDING);
    REJECT_SHOT(run.state.elapsed_seconds--);
    REJECT_SHOT(run.rng.increment &= ~UINT64_C(1));
    REJECT_SHOT(run.log.match_id.value++);
    REJECT_SHOT(run.log.count = 0);
    REJECT_SHOT(run.log.count = run.log.capacity);
    REJECT_SHOT(run.log.count = SIZE_MAX);
    REJECT_SHOT(run.log.capacity = 0);
    REJECT_SHOT(run.log.capacity = run.log.count);
    REJECT_SHOT(run.log.capacity = SIZE_MAX);
    REJECT_SHOT(run.log.events = NULL);
#undef REJECT_SHOT
    RmcRecoveryAction bad = action;
    copy_shot_run(&run, &original);
    bad.incumbent = action.challenger;
    reject_shot(&run, &bad);
    bad = action;
    bad.challenger = action.incumbent;
    reject_shot(&run, &bad);
    bad = action;
    bad.incumbent.value = UINT64_MAX;
    reject_shot(&run, &bad);
    bad = action;
    bad.duration_seconds = 0;
    reject_shot(&run, &bad);
    bad.duration_seconds = UINT32_MAX;
    reject_shot(&run, &bad);
    bad = action;
    bad.context = 21;
    reject_shot(&run, &bad);
    bad.context = -21;
    reject_shot(&run, &bad);
    /* A quality event alone and an older shot hidden by a later event fail. */
    run.log.count = 2;
    reject_shot(&run, &action);
    copy_shot_run(&run, &original);
    const RmcMatchEvent later = {
        .id = 4, .match_id = run.state.fixture.id,
        .at_second = run.state.elapsed_seconds, .type = RMC_EVENT_TACTICAL_DECISION
    };
    assert(rmc_match_event_log_append(&world, &run.state, &run.log, &later));
    reject_shot(&run, &action);
    copy_shot_run(&run, &original);
    /* Exactly one remaining slot is sufficient for recovery. */
    run.log.capacity = run.log.count + 1;
    assert(recover_shot(&run, &action));
}

static void test_shot_window_and_deadline(RmcShotEventOutcome outcome)
{
    for (unsigned second_half = 0; second_half < 2; ++second_half) {
        const uint32_t start = second_half ? 2700 : 0;
        ShotRun run;
        prepare_shot(&run, outcome, true, (RmcTacticalZone){2, RMC_LANE_RIGHT},
                     RMC_POSSESSION_MAX_ACTIONS - 1, start + 100,
                     second_half != 0, false);
        const RmcRecoveryAction action = shot_recovery_action(&run);
        assert(run.state.possession.action_count == RMC_POSSESSION_MAX_ACTIONS);
        reject_shot(&run, &action);
        ShotRun before;
        copy_shot_run(&before, &run);
        RmcSequenceDecision decision = RMC_SEQUENCE_ALLOW_ACTION;
        assert(rmc_match_sequence_decide(&world, &run.state, &decision));
        assert(decision == RMC_SEQUENCE_RESET_ACTION_WINDOW);
        assert(rmc_match_sequence_apply(&world, &run.state, decision));
        RmcMatchState expected = before.state;
        expected.possession.action_count = 0;
        same_recovered_state(&run.state, &expected);
        assert(run.state.possession.controller.value == 0);
        assert(run.rng.state == before.rng.state && run.rng.increment == before.rng.increment);
        assert(run.log.count == before.log.count);
        assert(run.log.capacity == before.log.capacity);
        assert(run.log.match_id.value == before.log.match_id.value);
        assert(run.log.events == run.storage);
        assert(memcmp(&run.result, &before.result, sizeof(run.result)) == 0);
        assert(memcmp(run.storage, before.storage, sizeof(run.storage)) == 0);
        assert(recover_shot(&run, &action));
        assert(run.log.count == 3 && run.storage[2].cause_event_id == 2);

        /* Leave the production shot loose at count 32, then reach the deadline
         * with the existing clock API: END_PERIOD outranks window reset.
         */
        prepare_shot(&run, outcome, true, (RmcTacticalZone){2, RMC_LANE_RIGHT},
                     RMC_POSSESSION_MAX_ACTIONS - 1, start + 2694,
                     second_half != 0, false);
        assert(rmc_match_clock_advance(&world, &run.state, 1));
        assert(run.state.possession.action_count == RMC_POSSESSION_MAX_ACTIONS);
        assert(run.storage[run.log.count - 1].type == RMC_EVENT_SHOT);
        reject_shot(&run, &action);
        assert(rmc_match_sequence_decide(&world, &run.state, &decision));
        assert(decision == RMC_SEQUENCE_END_PERIOD);
        unsigned char state_before[sizeof(run.state)];
        memcpy(state_before, &run.state, sizeof(run.state));
        assert(!rmc_match_sequence_apply(&world, &run.state, RMC_SEQUENCE_RESET_ACTION_WINDOW));
        assert(memcmp(state_before, &run.state, sizeof(run.state)) == 0);
        assert(rmc_match_sequence_apply(&world, &run.state, RMC_SEQUENCE_END_PERIOD));
        assert(run.state.mode == RMC_MATCH_PERIOD_ENDED);
        assert(run.state.period == (second_half ? RMC_MATCH_FINISHED : RMC_MATCH_HALF_TIME));
        reject_shot(&run, &action);

        /* A shot ending exactly at the deadline closes in its own transaction. */
        prepare_shot(&run, outcome, true, (RmcTacticalZone){2, RMC_LANE_RIGHT},
                     RMC_POSSESSION_MAX_ACTIONS - 1, start + 2695,
                     second_half != 0, false);
        assert(run.state.mode == RMC_MATCH_PERIOD_ENDED);
        assert(run.state.period == (second_half ? RMC_MATCH_FINISHED : RMC_MATCH_HALF_TIME));
        assert(run.log.count == (second_half ? 4u : 3u));
        assert(run.storage[1].type == RMC_EVENT_SHOT);
        assert(run.storage[1].outcome_code == (uint32_t)outcome);
        assert(run.storage[2].type == RMC_EVENT_PERIOD_END);
        if (second_half) {
            assert(run.storage[3].type == RMC_EVENT_MATCH_END);
        }
        assert(run.state.possession.team_id.value == 0);
        assert(run.state.possession.controller.value == 0);
        reject_shot(&run, &action);
    }
}

/* First-half shot records remain valid in a second-half recovery prefix. */
static void test_shot_replay_across_halves(void)
{
    ShotRun run;
    prepare_shot(&run, RMC_SHOT_SAVED_REBOUND, true,
                 (RmcTacticalZone){3, RMC_LANE_LEFT}, 7, 2695, false, false);
    assert(run.state.period == RMC_MATCH_HALF_TIME);
    assert(rmc_match_transition_apply(&world, &run.state, RMC_TRANSITION_SECOND_HALF,
                                      run.state.elapsed_seconds));
    run.state.mode = RMC_MATCH_BALL_IN_PLAY;
    run.state.possession = (RmcMatchPossession){
        .team_id = run.state.fixture.away_team,
        .initiator = run.state.away_lineup.players[2],
        .controller = run.state.away_lineup.players[1],
        .zone = {3, RMC_LANE_RIGHT}, .phase = RMC_PHASE_ATTACKING_DEVELOPMENT
    };
    const RmcShotAction shot = {
        .team_id = run.state.fixture.away_team,
        .shooter = run.state.possession.controller,
        .defender = run.state.home_lineup.players[1],
        .goalkeeper = run.state.home_lineup.players[0], .duration_seconds = 5
    };
    ShotRun before;
    copy_shot_run(&before, &run);
    RmcPcg32 candidate = run.rng;
    bool found = false;
    for (unsigned trial = 0; trial < 4096; ++trial) {
        copy_shot_run(&run, &before);
        run.rng = candidate;
        RmcShotResult result = {0};
        assert(rmc_match_shot_apply(&world, &run.state, &run.rng, &run.log, &shot, &result));
        if (result.outcome == RMC_SHOT_SAVED_REBOUND) {
            found = true;
            break;
        }
        (void)rmc_pcg32_next(&candidate);
    }
    assert(found && run.log.count == 5);
    ShotRun replay;
    copy_shot_run(&replay, &run);
    const RmcRecoveryAction action = shot_recovery_action(&run);
    assert(recover_shot(&run, &action));
    assert(recover_shot(&replay, &action));
    assert(run.log.count == 6 && run.storage[5].cause_event_id == 5);
    same_recovered_state(&run.state, &replay.state);
    assert(run.rng.state == replay.rng.state && run.rng.increment == replay.rng.increment);
    for (size_t i = 0; i < run.log.count; ++i) {
        same_shot_event(&run.storage[i], &replay.storage[i]);
    }
}

static void test_nonrecoverable_shots(void)
{
    const RmcShotEventOutcome outcomes[] = {
        RMC_SHOT_GOAL, RMC_SHOT_SAVED_HELD, RMC_SHOT_WIDE
    };
    for (size_t i = 0; i < sizeof(outcomes) / sizeof(outcomes[0]); ++i) {
        ShotRun run;
        prepare_shot(&run, outcomes[i], true, (RmcTacticalZone){3, RMC_LANE_LEFT},
                     7, 100, false, false);
        /* Keep the actual production SHOT pair but present a valid loose-ball
         * state, so rejection cannot rely on a controller or restart alone.
         * For a goal, expose the SHOT instead of the later GOAL record.
         */
        run.log.count = 2;
        run.state.mode = RMC_MATCH_BALL_IN_PLAY;
        run.state.possession = (RmcMatchPossession){
            .team_id = run.state.fixture.home_team,
            .initiator = run.state.home_lineup.players[3],
            .zone = {4, RMC_LANE_CENTRE}, .phase = RMC_PHASE_CHANCE,
            .action_count = 8, .elapsed_seconds = 5
        };
        assert(rmc_match_state_valid(&world, &run.state));
        assert(run.storage[1].type == RMC_EVENT_SHOT);
        assert(run.storage[1].outcome_code == (uint32_t)outcomes[i]);
        const RmcRecoveryAction action = shot_recovery_action(&run);
        reject_shot(&run, &action);
    }
}

static void test_shot_recovery(void)
{
    const RmcTacticalZone zones[] = {
        {2, RMC_LANE_LEFT}, {3, RMC_LANE_RIGHT}, {4, RMC_LANE_CENTRE}
    };
    for (size_t i = 0; i < sizeof(loose_shots) / sizeof(loose_shots[0]); ++i) {
        for (unsigned home = 0; home < 2; ++home) {
            for (size_t zone = 0; zone < sizeof(zones) / sizeof(zones[0]); ++zone) {
                test_shot_outcomes_and_replay(loose_shots[i], home != 0, zones[zone], false);
            }
        }
        test_shot_rejections(loose_shots[i]);
        test_shot_window_and_deadline(loose_shots[i]);
    }
    test_shot_outcomes_and_replay(RMC_SHOT_SAVED_REBOUND, true,
                                 (RmcTacticalZone){3, RMC_LANE_LEFT}, true);
    test_shot_outcomes_and_replay(RMC_SHOT_SAVED_REBOUND, false,
                                 (RmcTacticalZone){3, RMC_LANE_RIGHT}, true);
    test_shot_replay_across_halves();
    test_nonrecoverable_shots();
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
    test_shot_recovery();

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

    puts("RECOVERY_SHOT_THREE_ORIGINS_BOTH_OUTCOMES_AND_GEOMETRY=PASS");
    puts("RECOVERY_SHOT_CHAIN_REJECTION_ATOMICITY=PASS");
    puts("RECOVERY_SHOT_ACTION_32_AND_PERIOD_PRECEDENCE=PASS");
    puts("RECOVERY_CROSS_SHOT_REBOUND_INTEGRATION_AND_REPLAY=PASS");
    puts("RECOVERY_SHOT_REPLAY_ACROSS_HALVES=PASS");

    return 0;
}
