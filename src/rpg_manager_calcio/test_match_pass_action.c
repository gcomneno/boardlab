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
    state.possession.zone.lane = RMC_LANE_CENTRE;
    state.possession.phase = RMC_PHASE_BUILDUP;

    assert(rmc_match_state_valid(&world, &state));
    return state;
}

static RmcPassAction action_for(
    const RmcMatchState *state)
{
    RmcPassAction action = {0};

    action.team_id = state->fixture.home_team;
    action.passer = state->home_lineup.players[1];
    action.recipient = state->home_lineup.players[2];
    action.opponent = state->away_lineup.players[1];
    action.context = 0;
    action.duration_seconds = 5;

    return action;
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

static const RmcPlayer *find_player(
    const RmcRoster *roster,
    RmcPlayerId id)
{
    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        if (roster->players[i].id.value == id.value) {
            return &roster->players[i];
        }
    }

    return NULL;
}

static uint32_t expected_probability(
    const RmcPassAction *action)
{
    const RmcPlayer *passer =
        find_player(&world.rosters[0], action->passer);
    const RmcPlayer *opponent =
        find_player(&world.rosters[1], action->opponent);

    assert(passer != NULL);
    assert(opponent != NULL);

    const int32_t attack =
        ((int32_t)passer->attributes[RMC_ATTR_PASSAGGIO] +
         (int32_t)passer->attributes[RMC_ATTR_VISIONE]) / 2;

    const int32_t opposition =
        ((int32_t)opponent->attributes[RMC_ATTR_MARCATURA] +
         (int32_t)opponent->attributes[RMC_ATTR_POSIZIONAMENTO]) / 2;

    uint32_t probability = 0;

    assert(rmc_match_probability(
        attack, opposition, action->context, &probability
    ));

    return probability;
}

static void reject_unchanged(
    RmcMatchState *state,
    RmcPcg32 *rng,
    RmcMatchEventLog *log,
    const RmcPassAction *action)
{
    const RmcMatchState before_state = *state;
    const RmcPcg32 before_rng = *rng;
    const RmcMatchEventLog before_log = *log;
    RmcMatchEvent before_storage[2] = {{0}};
    RmcPassResult result = {
        .probability = 777,
        .random_draw = 888,
        .outcome = RMC_PASS_OUTCOME_COMPLETED
    };
    const RmcPassResult before_result = result;

    assert(log->events != NULL);
    assert(log->capacity <= 2);

    memcpy(
        before_storage, log->events,
        log->capacity * sizeof(log->events[0])
    );

    assert(!rmc_match_pass_apply(
        &world, state, rng, log, action, &result
    ));

    assert(memcmp(
        state, &before_state, sizeof(before_state)
    ) == 0);
    assert(memcmp(
        rng, &before_rng, sizeof(before_rng)
    ) == 0);
    assert(memcmp(
        log, &before_log, sizeof(before_log)
    ) == 0);
    assert(memcmp(
        log->events, before_storage,
        log->capacity * sizeof(log->events[0])
    ) == 0);
    assert(memcmp(
        &result, &before_result, sizeof(result)
    ) == 0);
}

static void test_success_failure_and_rng(void)
{
    RmcMatchState base_state = active_state();
    const RmcPassAction action = action_for(&base_state);
    RmcPcg32 candidate_rng = match_rng();

    const uint32_t probability =
        expected_probability(&action);

    unsigned successes = 0;
    unsigned failures = 0;

    for (unsigned trial = 0; trial < 256; ++trial) {
        RmcMatchState state = base_state;
        RmcPcg32 rng = candidate_rng;
        RmcPcg32 expected_rng = candidate_rng;
        RmcMatchEvent storage[2] = {{0}};
        RmcMatchEventLog log = {0};
        RmcPassResult result = {0};
        uint32_t expected_draw = UINT32_MAX;

        assert(rmc_match_event_log_init(
            &state, storage, 2, &log
        ));

        assert(rmc_pcg32_bounded(
            &expected_rng, RMC_PROBABILITY_SCALE,
            &expected_draw
        ));

        assert(rmc_match_pass_apply(
            &world, &state, &rng, &log,
            &action, &result
        ));

        const bool completed = expected_draw < probability;

        assert(result.probability == probability);
        assert(result.random_draw == expected_draw);
        assert(result.outcome ==
               (completed
                    ? RMC_PASS_OUTCOME_COMPLETED
                    : RMC_PASS_OUTCOME_LOOSE_BALL));

        assert(memcmp(
            &rng, &expected_rng, sizeof(rng)
        ) == 0);

        assert(state.elapsed_seconds ==
               action.duration_seconds);
        assert(state.possession.elapsed_seconds ==
               action.duration_seconds);
        assert(state.possession.action_count == 1);
        assert(state.possession.team_id.value ==
               action.team_id.value);
        assert(state.possession.controller.value ==
               (completed ? action.recipient.value : 0));

        assert(log.count == 1);
        assert(storage[0].id == 1);
        assert(storage[0].type == RMC_EVENT_ACTION);
        assert(storage[0].at_second ==
               action.duration_seconds);
        assert(storage[0].team_id.value ==
               action.team_id.value);
        assert(storage[0].primary_actor.value ==
               action.passer.value);
        assert(storage[0].secondary_actor.value ==
               action.recipient.value);
        assert(storage[0].outcome_code ==
               (uint32_t)result.outcome);
        assert(rmc_match_state_valid(&world, &state));

        successes += completed ? 1u : 0u;
        failures += completed ? 0u : 1u;

        (void)rmc_pcg32_next(&candidate_rng);
    }

    assert(successes > 0);
    assert(failures > 0);
}

static void test_invalid_actions(void)
{
    RmcMatchState state = active_state();
    RmcPcg32 rng = match_rng();
    RmcMatchEvent storage[2] = {{0}};
    RmcMatchEventLog log = {0};

    assert(rmc_match_event_log_init(
        &state, storage, 2, &log
    ));

    const RmcPassAction valid = action_for(&state);
    RmcPassAction bad = valid;

    bad.team_id = state.fixture.away_team;
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.passer = valid.recipient;
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.recipient = valid.passer;
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.recipient = state.away_lineup.players[2];
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
    state.elapsed_seconds = 2698;
    bad.duration_seconds = 3;
    reject_unchanged(&state, &rng, &log, &bad);

    state = active_state();
    state.possession.action_count = RMC_POSSESSION_MAX_ACTIONS;
    reject_unchanged(&state, &rng, &log, &valid);

    state = active_state();
    state.possession.controller.value = 0;
    reject_unchanged(&state, &rng, &log, &valid);
}

static void test_event_capacity_and_order(void)
{
    RmcMatchState state = active_state();
    RmcPcg32 rng = match_rng();
    RmcMatchEvent storage[2] = {{0}};
    RmcMatchEventLog log = {0};
    const RmcPassAction action = action_for(&state);

    assert(rmc_match_event_log_init(
        &state, storage, 2, &log
    ));

    log.count = log.capacity;
    reject_unchanged(&state, &rng, &log, &action);

    log.count = 0;
    log.match_id.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &log, &action);

    log.match_id = state.fixture.id;

    RmcMatchEvent previous = {0};
    previous.id = 1;
    previous.match_id = state.fixture.id;
    previous.at_second = 7;
    previous.type = RMC_EVENT_ACTION;

    state.elapsed_seconds = 7;

    assert(rmc_match_event_log_append(
        &world, &state, &log, &previous
    ));

    /*
     * An event at the new match time must preserve
     * nondecreasing timestamps.
     */
    RmcPassResult result = {0};

    assert(rmc_match_pass_apply(
        &world, &state, &rng, &log, &action, &result
    ));

    assert(log.count == 2);
    assert(storage[1].id == 2);
    assert(storage[1].at_second == 12);
    assert(storage[1].at_second >= storage[0].at_second);
}

static void test_deterministic_replay(void)
{
    RmcMatchState first = active_state();
    RmcMatchState second = active_state();

    RmcPcg32 first_rng = match_rng();
    RmcPcg32 second_rng = match_rng();

    RmcMatchEvent first_storage[2] = {{0}};
    RmcMatchEvent second_storage[2] = {{0}};

    RmcMatchEventLog first_log = {0};
    RmcMatchEventLog second_log = {0};

    assert(rmc_match_event_log_init(
        &first, first_storage, 2, &first_log
    ));
    assert(rmc_match_event_log_init(
        &second, second_storage, 2, &second_log
    ));

    const RmcPassAction action = action_for(&first);
    RmcPassResult first_result = {0};
    RmcPassResult second_result = {0};

    assert(rmc_match_pass_apply(
        &world, &first, &first_rng,
        &first_log, &action, &first_result
    ));
    assert(rmc_match_pass_apply(
        &world, &second, &second_rng,
        &second_log, &action, &second_result
    ));

    assert(memcmp(&first, &second, sizeof(first)) == 0);
    assert(memcmp(
        &first_rng, &second_rng, sizeof(first_rng)
    ) == 0);
    assert(memcmp(
        &first_result, &second_result, sizeof(first_result)
    ) == 0);
    assert(first_log.count == second_log.count);
    assert(memcmp(
        first_storage, second_storage,
        first_log.count * sizeof(first_storage[0])
    ) == 0);
}

int main(void)
{
    setup();

    test_success_failure_and_rng();
    test_invalid_actions();
    test_event_capacity_and_order();
    test_deterministic_replay();

    puts("PASS_ACTION_SUCCESS_AND_FAILURE=PASS");
    puts("PASS_ACTION_PCG32_CONSUMPTION=PASS");
    puts("PASS_ACTION_INVALID_REJECTION_ATOMICITY=PASS");
    puts("PASS_ACTION_EVENT_LOG_CAPACITY_AND_ORDER=PASS");
    puts("PASS_ACTION_DETERMINISTIC_REPLAY=PASS");

    return 0;
}
