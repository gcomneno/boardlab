#include "match_dribble_action.h"
#include "match_pass_action.h"

_Static_assert(RMC_DRIBBLE_ACTION_VERSION == 1, "Dribble version");
_Static_assert(RMC_DRIBBLE_OUTCOME_SUCCESS == 201, "Success code");
_Static_assert(RMC_DRIBBLE_OUTCOME_CONTAINED == 202, "Contained code");
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

static RmcDribbleAction dribble_action(const RmcMatchState *state)
{
    const RmcDribbleAction action = {
        .team_id = state->fixture.home_team,
        .dribbler = state->home_lineup.players[1],
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
    const RmcDribbleAction *action)
{
    const RmcMatchState old_state = *state;
    const RmcPcg32 old_rng = *rng;
    const RmcMatchEventLog old_log = *log;
    RmcMatchEvent old_events[3] = {{0}};

    assert(log->events != NULL && log->capacity <= 3);
    memcpy(old_events, log->events,
           log->capacity * sizeof(log->events[0]));

    RmcDribbleResult result = {
        .probability = 777,
        .random_draw = 888,
        .outcome = RMC_DRIBBLE_OUTCOME_CONTAINED
    };
    const RmcDribbleResult old_result = result;

    assert(!rmc_match_dribble_apply(
        &world, state, rng, log, action, &result
    ));

    assert(memcmp(state, &old_state, sizeof(*state)) == 0);
    assert(memcmp(rng, &old_rng, sizeof(*rng)) == 0);
    assert(memcmp(log, &old_log, sizeof(*log)) == 0);
    assert(memcmp(log->events, old_events,
                  log->capacity * sizeof(log->events[0])) == 0);
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

static uint32_t expected_probability(const RmcDribbleAction *action)
{
    const RmcPlayer *attacker = test_player(action->dribbler);
    const RmcPlayer *defender = test_player(action->opponent);
    const int32_t attack =
        (attacker->attributes[RMC_ATTR_DRIBBLING] +
         attacker->attributes[RMC_ATTR_VELOCITA]) / 2;
    const int32_t opposition =
        (defender->attributes[RMC_ATTR_CONTRASTO] +
         defender->attributes[RMC_ATTR_POSIZIONAMENTO]) / 2;
    uint32_t probability = 0;
    assert(rmc_match_probability(
        attack, opposition, action->context, &probability
    ));
    return probability;
}

static void test_both_outcomes(void)
{
    RmcPcg32 candidate = match_rng();
    bool succeeded_seen = false;
    bool contained_seen = false;

    for (unsigned trial = 0; trial < 1024; ++trial) {
        RmcMatchState state = active_state();
        RmcPcg32 rng = candidate;
        RmcPcg32 expected_rng = candidate;
        RmcMatchEvent events[3] = {{0}};
        RmcMatchEventLog log = {0};
        RmcDribbleResult result = {0};

        assert(rmc_match_event_log_init(
            &state, events, 3, &log
        ));

        const RmcDribbleAction action = dribble_action(&state);
        const RmcMatchPossession before = state.possession;

        uint32_t expected_draw = UINT32_MAX;
        assert(rmc_pcg32_bounded(
            &expected_rng, RMC_PROBABILITY_SCALE,
            &expected_draw
        ));

        assert(rmc_match_dribble_apply(
            &world, &state, &rng, &log, &action, &result
        ));

        const bool succeeded = expected_draw < result.probability;

        assert(result.probability == expected_probability(&action));
        assert(result.random_draw == expected_draw);
        assert(memcmp(&rng, &expected_rng, sizeof(rng)) == 0);
        assert(result.outcome ==
               (succeeded ? RMC_DRIBBLE_OUTCOME_SUCCESS :
                           RMC_DRIBBLE_OUTCOME_CONTAINED));

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
               before.zone.sector + (succeeded ? 1 : 0));

        assert(log.count == 1);
        assert(events[0].id == 1);
        assert(events[0].cause_event_id == 0);
        assert(events[1].id == 0 && events[2].id == 0);
        assert(events[0].type == RMC_EVENT_ACTION);
        assert(events[0].match_id.value ==
               state.fixture.id.value);
        assert(events[0].team_id.value ==
               action.team_id.value);
        assert(events[0].primary_actor.value ==
               action.dribbler.value);
        assert(events[0].secondary_actor.value ==
               action.opponent.value);
        assert(events[0].outcome_code == (uint32_t)result.outcome);
        assert(events[0].outcome_code !=
               RMC_PASS_OUTCOME_LOOSE_BALL);
        assert(events[0].at_second == state.elapsed_seconds);
        assert(rmc_match_state_valid(&world, &state));

        if (succeeded) {
            succeeded_seen = true;
        } else {
            contained_seen = true;
        }

        if (succeeded_seen && contained_seen) {
            break;
        }

        (void)rmc_pcg32_next(&candidate);
    }

    assert(succeeded_seen && contained_seen);
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

    const RmcDribbleAction valid = dribble_action(&state);
    RmcDribbleAction bad = valid;

    bad.team_id = state.fixture.away_team;
    reject_unchanged(&state, &rng, &log, &bad);

    bad = valid;
    bad.dribbler = state.home_lineup.players[2];
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

    state.possession.zone.sector = RMC_FIELD_SECTORS - 1;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.zone.sector = 1;

    state.possession.controller.value = 0;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.controller = valid.dribbler;

    state.possession.action_count = RMC_POSSESSION_MAX_ACTIONS;
    reject_unchanged(&state, &rng, &log, &valid);
    state.possession.action_count = 0;

    RmcMatchEventLog invalid_log = log;
    invalid_log.count = invalid_log.capacity;
    reject_unchanged(&state, &rng, &invalid_log, &valid);

    invalid_log = log;
    invalid_log.match_id.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &invalid_log, &valid);

    bad = valid;
    bad.dribbler.value = 0;
    reject_unchanged(&state, &rng, &log, &bad);
    bad.dribbler.value = UINT64_MAX;
    reject_unchanged(&state, &rng, &log, &bad);
    bad = valid;
    bad.opponent.value = 0;
    reject_unchanged(&state, &rng, &log, &bad);
    bad = valid;
    bad.duration_seconds = UINT32_MAX;
    reject_unchanged(&state, &rng, &log, &bad);

    state.elapsed_seconds = 2700;
    reject_unchanged(&state, &rng, &log, &valid);
    state.elapsed_seconds = 0;

    /* Reject an actor removed from the world, even if still in a lineup. */
    RmcPlayer *defender = test_player(valid.opponent);
    const RmcPlayerId saved_id = defender->id;
    defender->id.value = 0;
    reject_unchanged(&state, &rng, &log, &valid);
    defender->id = saved_id;

    /* Exercise append rejection after resolving only temporary state/PRNG. */
    events[0].at_second = valid.duration_seconds + 1;
    log.count = 1;
    reject_unchanged(&state, &rng, &log, &valid);
    log.count = 0;

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

    const RmcDribbleAction action = dribble_action(&first);
    RmcDribbleResult first_result = {0};
    RmcDribbleResult second_result = {0};

    for (unsigned i = 0; i < 3; ++i) {
        assert(rmc_match_dribble_apply(
            &world, &first, &first_rng,
            &first_log, &action, &first_result
        ));
        assert(rmc_match_dribble_apply(
            &world, &second, &second_rng,
            &second_log, &action, &second_result
        ));
    }

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

static void test_away_and_boundaries(void)
{
    RmcMatchState state = active_state();
    state.possession.team_id = state.fixture.away_team;
    state.possession.controller = state.away_lineup.players[1];
    state.possession.initiator = state.away_lineup.players[1];
    state.possession.zone.lane = RMC_LANE_RIGHT;
    state.possession.action_count = RMC_POSSESSION_MAX_ACTIONS - 2;
    state.elapsed_seconds = 2690;
    RmcPcg32 rng = match_rng();
    RmcMatchEvent events[3] = {{0}};
    RmcMatchEventLog log = {0};
    assert(rmc_match_event_log_init(&state, events, 3, &log));
    RmcDribbleAction action = {
        .team_id = state.fixture.away_team,
        .dribbler = state.away_lineup.players[1],
        .opponent = state.home_lineup.players[1],
        .context = -20,
        .duration_seconds = 5
    };

    /* Odd sums require floor division; CONTROLLO must not affect dribbling. */
    RmcPlayer *attacker = test_player(action.dribbler);
    RmcPlayer *defender = test_player(action.opponent);
    const RmcPlayer saved_attacker = *attacker;
    const RmcPlayer saved_defender = *defender;
    attacker->attributes[RMC_ATTR_DRIBBLING] = 81;
    attacker->attributes[RMC_ATTR_VELOCITA] = 40;
    attacker->attributes[RMC_ATTR_CONTROLLO] = 1;
    defender->attributes[RMC_ATTR_CONTRASTO] = 32;
    defender->attributes[RMC_ATTR_POSIZIONAMENTO] = 49;

    assert(rmc_match_state_valid(&world, &state));

    for (unsigned i = 0; i < 2; ++i) {
        RmcDribbleResult result = {0};
        const uint32_t probability = expected_probability(&action);
        const RmcMatchPossession before = state.possession;
        assert(rmc_match_dribble_apply(
            &world, &state, &rng, &log, &action, &result
        ));
        assert(result.probability == probability);
        assert(state.possession.zone.sector == before.zone.sector +
               (result.outcome == RMC_DRIBBLE_OUTCOME_SUCCESS ? 1 : 0));
        assert(state.possession.zone.lane == RMC_LANE_RIGHT);
        assert(state.possession.team_id.value == action.team_id.value);
        assert(state.possession.controller.value == action.dribbler.value);
        assert(log.count == i + 1);
        assert(events[i].id == i + 1);
        assert(events[i].primary_actor.value == action.dribbler.value);
        assert(events[i].secondary_actor.value == action.opponent.value);
        assert(events[i].outcome_code == (uint32_t)result.outcome);
        action.context = 20;
    }
    assert(state.elapsed_seconds == 2700);
    assert(state.possession.action_count == RMC_POSSESSION_MAX_ACTIONS);
    reject_unchanged(&state, &rng, &log, &action);
    *attacker = saved_attacker;
    *defender = saved_defender;
}

int main(void)
{
    setup();
    test_both_outcomes();
    test_rejections();
    test_deterministic_replay();
    test_away_and_boundaries();

    puts("DRIBBLE_SUCCESS_AND_CONTAINED=PASS");
    puts("DRIBBLE_ZONE_AND_POSSESSION=PASS");
    puts("DRIBBLE_EVENT_SEMANTICS=PASS");
    puts("DRIBBLE_INVALID_REJECTION_ATOMICITY=PASS");
    puts("DRIBBLE_DETERMINISTIC_REPLAY=PASS");

    return 0;
}
