#include "match_restart_action.h"
#include "match_shot_action.h"
#include "match_pass_action.h"
#include "match_clock.h"
#include "match_period_commit.h"
#include "match_transitions.h"
#include "match_stoppage.h"
#include "match_sequence_apply.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(RMC_RESTART_GOAL_KICK == 1 && RMC_RESTART_KICKOFF == 2,
               "Restart outcomes");
_Static_assert(RMC_EVENT_SCHEMA_VERSION == 2 && RMC_SHOT_MODEL_VERSION == 1 &&
               RMC_RULES_VERSION == 1 && RMC_GENERATOR_VERSION == 1,
               "Frozen versions");

#ifdef RMC_WRAP_MALLOC
static bool fail_allocation;
void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size);
void *__wrap_malloc(size_t size)
{
    if (fail_allocation) {
        fail_allocation = false;
        return NULL;
    }
    return __real_malloc(size);
}
#endif

enum { STORAGE = 64 };
static RmcWorldCore world, original_world;
static RmcMatchState initial;

typedef struct {
    RmcMatchState state;
    RmcMatchEvent storage[STORAGE];
    RmcMatchEventLog log;
    RmcRestartAction action;
    RmcRestartResult result;
    RmcPcg32 rng;
    RmcShotResult shot_result;
} Run;

static void setup(void)
{
    const RmcWorldIdentity identity = {
        .world_seed = UINT64_C(20261114),
        .generator_version = RMC_GENERATOR_VERSION,
        .rules_version = RMC_RULES_VERSION
    };
    assert(rmc_world_core_generate(&identity, &world));
    const RmcFixture fixture = {
        .id = {1}, .home_team = world.teams[0].id,
        .away_team = world.teams[1].id, .matchday = 1
    };
    RmcLineup home, away;
    assert(rmc_roster_lineup(&world.rosters[0], RMC_FORMATION_442, &home));
    assert(rmc_roster_lineup(&world.rosters[1], RMC_FORMATION_433, &away));
    assert(rmc_match_state_init(&world, &fixture, &home, &away, &initial));
    original_world = world;
}

static RmcPlayer *player(RmcPlayerId id)
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

static void possession(RmcMatchState *state, bool home)
{
    const RmcLineup *lineup = home ? &state->home_lineup : &state->away_lineup;
    state->mode = RMC_MATCH_BALL_IN_PLAY;
    state->possession = (RmcMatchPossession){0};
    state->possession.team_id = home ? state->fixture.home_team : state->fixture.away_team;
    state->possession.initiator = lineup->players[2];
    state->possession.controller = lineup->players[1];
    state->possession.zone = (RmcTacticalZone){3, RMC_LANE_CENTRE};
    state->possession.phase = RMC_PHASE_CHANCE;
    state->possession.action_count = 31; /* Shot is the last action in the window. */
    state->possession.elapsed_seconds = 20;
}

static RmcMatchState active(bool second, bool home)
{
    RmcMatchState state = initial;
    assert(rmc_match_transition_apply(&world, &state, RMC_TRANSITION_START, 0));
    state.mode = RMC_MATCH_BALL_IN_PLAY;
    state.first_half_stoppage_seconds = 60;
    if (second) {
        assert(rmc_match_clock_advance(&world, &state, 2760));
        assert(rmc_match_transition_apply(&world, &state, RMC_TRANSITION_HALF_TIME, 2760));
        assert(rmc_match_transition_apply(&world, &state, RMC_TRANSITION_SECOND_HALF, 2760));
        state.mode = RMC_MATCH_BALL_IN_PLAY;
        state.second_half_stoppage_seconds = 90;
    }
    assert(rmc_match_clock_advance(&world, &state, 100));
    possession(&state, home);
    state.home_goals = 2;
    state.away_goals = 3;
    assert(rmc_match_state_valid(&world, &state));
    return state;
}

static void init_run(Run *run, const RmcMatchState *state)
{
    memset(run, 0xa5, sizeof(*run));
    run->state = *state;
    assert(rmc_stream_init(&run->rng, &world.identity, RMC_STREAM_MATCH, state->fixture.id.value));
    assert(rmc_match_event_log_init(state, run->storage, STORAGE, &run->log));
}

static void clone(Run *to, const Run *from)
{
    memcpy(to, from, sizeof(*to));
    to->log.events = to->storage;
}

static void command(Run *run)
{
    const size_t index = run->log.count -
        (run->storage[run->log.count - 1].type == RMC_EVENT_GOAL ? 2u : 1u);
    const RmcMatchEvent *shot = &run->storage[index];
    run->action = (RmcRestartAction){
        .restart_kind = shot->shot.restart_kind,
        .awarded_team = shot->shot.restart_awarded_team,
        .source_shot_event_id = shot->id
    };
    const RmcLineup *lineup =
        run->action.awarded_team.value == run->state.fixture.home_team.value ?
        &run->state.home_lineup : &run->state.away_lineup;
    run->action.taker = lineup->players[
        run->action.restart_kind == RMC_RESTART_KICKOFF ? 1 : 0];
}

/* Search controlled, reproducible PCG32 inputs using real production shots.
 * Never manufacture a successful cause or alter its sporting result.
 */
static void shoot(Run *run, RmcShotEventOutcome outcome, RmcShotTrajectory trajectory)
{
    const bool home = run->state.possession.team_id.value == run->state.fixture.home_team.value;
    const RmcLineup *opponent = home ? &run->state.away_lineup : &run->state.home_lineup;
    const RmcShotAction action = {
        .team_id = run->state.possession.team_id,
        .shooter = run->state.possession.controller,
        .defender = opponent->players[1], .goalkeeper = opponent->players[0],
        .context = 0, .duration_seconds = 5
    };
    RmcPcg32 seed = run->rng;
    for (unsigned attempt = 0; attempt < 10000; ++attempt) {
        Run trial;
        clone(&trial, run);
        trial.rng = seed;
        assert(rmc_match_shot_apply(&world, &trial.state, &trial.rng, &trial.log,
                                   &action, &trial.shot_result));
        if (trial.shot_result.outcome == outcome &&
            (trajectory == RMC_SHOT_TRAJECTORY_NONE || trial.shot_result.trajectory == trajectory)) {
            clone(run, &trial);
            command(run);
            return;
        }
        (void)rmc_pcg32_next(&seed);
    }
    assert(false);
}

static void pending(Run *run, bool second, bool home, bool goal)
{
    const RmcMatchState state = active(second, home);
    init_run(run, &state);
    shoot(run, goal ? RMC_SHOT_GOAL : RMC_SHOT_WIDE, RMC_SHOT_TRAJECTORY_NONE);
}

static bool apply(Run *run)
{
    return rmc_match_restart_apply(&world, &run->state, &run->log, &run->action, &run->result);
}

static void reject(Run *run)
{
    unsigned char before[sizeof(*run)];
    memcpy(before, run, sizeof(*run));
    assert(!apply(run));
    /* Includes all log metadata, result, RNG, padding and unused storage. */
    assert(memcmp(before, run, sizeof(*run)) == 0);
}

static void same_payload(const RmcShotEventPayload *a, const RmcShotEventPayload *b)
{
    assert(a->model_version == b->model_version);
    assert(a->chance_quality == b->chance_quality);
    assert(a->source_zone.sector == b->source_zone.sector);
    assert(a->source_zone.lane == b->source_zone.lane);
    assert(a->context == b->context && a->duration_seconds == b->duration_seconds);
    assert(a->defender_id.value == b->defender_id.value);
    assert(a->goalkeeper_id.value == b->goalkeeper_id.value);
    assert(a->applied_talent_mask == b->applied_talent_mask);
}

static void same_event(const RmcMatchEvent *a, const RmcMatchEvent *b)
{
    assert(a->id == b->id && a->match_id.value == b->match_id.value);
    assert(a->at_second == b->at_second && a->type == b->type);
    assert(a->team_id.value == b->team_id.value);
    assert(a->primary_actor.value == b->primary_actor.value);
    assert(a->secondary_actor.value == b->secondary_actor.value);
    assert(a->outcome_code == b->outcome_code && a->cause_event_id == b->cause_event_id);
    same_payload(&a->shot, &b->shot);
    assert(a->shot.trajectory == b->shot.trajectory);
    assert(a->shot.restart_kind == b->shot.restart_kind);
    assert(a->shot.restart_awarded_team.value == b->shot.restart_awarded_team.value);
    assert(a->shot.restart_zone.sector == b->shot.restart_zone.sector);
    assert(a->shot.restart_zone.lane == b->shot.restart_zone.lane);
    assert(a->shot.restart_disposition == b->shot.restart_disposition);
}

static void same_state(const RmcMatchState *a, const RmcMatchState *b)
{
    assert(a->fixture.id.value == b->fixture.id.value);
    assert(a->period == b->period && a->mode == b->mode);
    assert(a->terminal_reason == b->terminal_reason);
    assert(a->elapsed_seconds == b->elapsed_seconds);
    assert(a->second_half_start_second == b->second_half_start_second);
    assert(a->first_half_stoppage_seconds == b->first_half_stoppage_seconds);
    assert(a->second_half_stoppage_seconds == b->second_half_stoppage_seconds);
    assert(a->home_goals == b->home_goals && a->away_goals == b->away_goals);
    assert(a->possession.team_id.value == b->possession.team_id.value);
    assert(a->possession.initiator.value == b->possession.initiator.value);
    assert(a->possession.controller.value == b->possession.controller.value);
    assert(a->possession.zone.sector == b->possession.zone.sector);
    assert(a->possession.zone.lane == b->possession.zone.lane);
    assert(a->possession.phase == b->possession.phase);
    assert(a->possession.action_count == b->possession.action_count);
    assert(a->possession.elapsed_seconds == b->possession.elapsed_seconds);
    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        assert(a->home_lineup.players[i].value == b->home_lineup.players[i].value);
        assert(a->away_lineup.players[i].value == b->away_lineup.players[i].value);
        assert(a->home_lineup.slots[i] == b->home_lineup.slots[i]);
        assert(a->away_lineup.slots[i] == b->away_lineup.slots[i]);
    }
}


static void verify_restart(Run *run)
{
    Run before;
    clone(&before, run);
    assert(apply(run));
    assert(run->log.count == before.log.count + 1u);
    assert(run->log.capacity == before.log.capacity && run->log.events == run->storage);
    assert(run->log.match_id.value == before.log.match_id.value);
    assert(memcmp(before.storage, run->storage, before.log.count * sizeof(run->storage[0])) == 0);
    const size_t used = run->log.count * sizeof(run->storage[0]);
    assert(memcmp((unsigned char *)before.storage + used,
                  (unsigned char *)run->storage + used, sizeof(run->storage) - used) == 0);
    assert(run->rng.state == before.rng.state && run->rng.increment == before.rng.increment);
    const RmcMatchEvent expected = {
        .id = before.log.count + 1u, .match_id = before.state.fixture.id,
        .at_second = before.state.elapsed_seconds, .type = RMC_EVENT_RESTART,
        .team_id = before.action.awarded_team, .primary_actor = before.action.taker,
        .outcome_code = (uint32_t)before.action.restart_kind,
        .cause_event_id = before.action.source_shot_event_id
    };
    same_event(&run->storage[before.log.count], &expected);
    assert(run->result.restart_event_id == expected.id);
    assert(run->result.source_shot_event_id == before.action.source_shot_event_id);
    assert(run->result.source_goal_event_id ==
        (before.action.restart_kind == RMC_RESTART_KICKOFF ? before.log.count : 0));
    RmcMatchState expected_state = before.state;
    expected_state.mode = RMC_MATCH_BALL_IN_PLAY;
    expected_state.possession = (RmcMatchPossession){0};
    expected_state.possession.team_id = before.action.awarded_team;
    expected_state.possession.initiator = before.action.taker;
    expected_state.possession.controller = before.action.taker;
    expected_state.possession.zone = (RmcTacticalZone){
        before.action.restart_kind == RMC_RESTART_KICKOFF ? 2 : 0, RMC_LANE_CENTRE
    };
    same_state(&run->state, &expected_state);
    assert(rmc_match_state_valid(&world, &run->state));
    RmcSequenceDecision decision;
    assert(rmc_match_sequence_decide(&world, &run->state, &decision));
    assert(decision == RMC_SEQUENCE_ALLOW_ACTION);
}

static void resume_pass(Run *run)
{
    const bool home = run->state.possession.team_id.value == run->state.fixture.home_team.value;
    const RmcLineup *lineup = home ? &run->state.home_lineup : &run->state.away_lineup;
    const RmcLineup *opponent = home ? &run->state.away_lineup : &run->state.home_lineup;
    const RmcPassAction action = {
        .team_id = run->state.possession.team_id,
        .passer = run->state.possession.controller, .recipient = lineup->players[2],
        .opponent = opponent->players[1], .context = 0, .duration_seconds = 1
    };
    RmcPcg32 expected_rng = run->rng;
    uint32_t draw;
    assert(rmc_pcg32_bounded(&expected_rng, RMC_PROBABILITY_SCALE, &draw));
    RmcPassResult result;
    const size_t count = run->log.count;
    const uint32_t clock = run->state.elapsed_seconds;
    assert(rmc_match_pass_apply(&world, &run->state, &run->rng, &run->log, &action, &result));
    assert(result.random_draw == draw);
    assert(run->rng.state == expected_rng.state && run->rng.increment == expected_rng.increment);
    assert(run->state.possession.action_count == 1 && run->state.possession.elapsed_seconds == 1);
    assert(run->state.elapsed_seconds == clock + 1u && run->log.count == count + 1u);
    assert(run->storage[count].type == RMC_EVENT_ACTION);
}

static void test_integration(void)
{
    for (unsigned second = 0; second < 2; ++second) {
        for (unsigned home = 0; home < 2; ++home) {
            for (unsigned goal = 0; goal < 2; ++goal) {
                Run a, b;
                pending(&a, second != 0, home != 0, goal != 0);
                clone(&b, &a);
                a.log.capacity = a.log.count + 1u; /* Exactly one free slot. */
                verify_restart(&a);
                verify_restart(&b);
                same_state(&a.state, &b.state);
                assert(a.result.restart_event_id == b.result.restart_event_id);
                assert(a.result.source_shot_event_id == b.result.source_shot_event_id);
                assert(a.result.source_goal_event_id == b.result.source_goal_event_id);
                for (size_t i = 0; i < a.log.count; ++i) {
                    same_event(&a.storage[i], &b.storage[i]);
                }
                reject(&a); /* Single use. */
                a.state.mode = RMC_MATCH_RESTART_PENDING;
                a.state.possession = (RmcMatchPossession){0};
                reject(&a); /* Even restoring pending cannot replay an old cause. */
                a.log.capacity = STORAGE;
                clone(&a, &b);
                resume_pass(&a);
                resume_pass(&b);
                same_state(&a.state, &b.state);
                assert(a.rng.state == b.rng.state && a.rng.increment == b.rng.increment);
                same_event(&a.storage[a.log.count - 1], &b.storage[b.log.count - 1]);
            }
        }
    }
}

static void test_commands_and_state(void)
{
    for (unsigned goal = 0; goal < 2; ++goal) {
        Run good;
        pending(&good, false, true, goal != 0);
        for (unsigned which = 0; which < 47; ++which) {
            Run run;
            clone(&run, &good);
            switch (which) {
            case 0: run.action.restart_kind = RMC_RESTART_NONE; break;
            case 1: run.action.restart_kind = (RmcRestartKind)3; break;
            case 2: run.action.restart_kind = goal ? RMC_RESTART_GOAL_KICK : RMC_RESTART_KICKOFF; break;
            case 3: run.action.awarded_team = run.state.fixture.home_team; break;
            case 4: run.action.awarded_team.value = 0; break;
            case 5: run.action.awarded_team.value = UINT64_MAX; break;
            case 6: run.action.taker.value = 0; break;
            case 7: run.action.taker.value = UINT64_MAX; break;
            case 8: run.action.taker = run.state.home_lineup.players[1]; break;
            case 9: run.action.taker = run.state.away_lineup.players[goal ? 0 : 1]; break;
            case 10: run.action.source_shot_event_id = 0; break;
            case 11: --run.action.source_shot_event_id; break;
            case 12: ++run.action.source_shot_event_id; break;
            case 13: run.action.source_shot_event_id = UINT64_MAX; break;
            case 14: run.state.possession.zone.sector = 1; break;
            case 15: run.state.possession.zone.lane = RMC_LANE_CENTRE; break;
            case 16: run.state.possession.phase = RMC_PHASE_TRANSITION; break;
            case 17: run.state.possession.action_count = 1; break;
            case 18: run.state.possession.elapsed_seconds = 1; break;
            case 19: run.state.possession.initiator = run.action.taker; break;
            case 20: run.state.possession.controller = run.action.taker; break;
            case 21: possession(&run.state, false); run.state.mode = RMC_MATCH_RESTART_PENDING; break;
            case 22: run.state.mode = RMC_MATCH_BALL_IN_PLAY; break;
            case 23: run.state.mode = RMC_MATCH_MANAGEMENT_WINDOW; break;
            case 24: run.state.mode = RMC_MATCH_PERIOD_ENDED; break;
            case 25: run.state.period = RMC_MATCH_NOT_STARTED; break;
            case 26: run.state.period = RMC_MATCH_HALF_TIME; break;
            case 27: run.state.period = RMC_MATCH_FINISHED; break;
            case 28: run.state.fixture.id.value = 0; break;
            case 29: run.state.fixture.away_team = run.state.fixture.home_team; break;
            case 30: run.state.home_lineup.players[1] = run.state.home_lineup.players[2]; break;
            case 31: run.state.away_lineup.slots[0] = RMC_SLOT_ATT; break;
            case 32: ++run.state.elapsed_seconds; break;
            case 33: --run.state.elapsed_seconds; break;
            case 34: run.log.events = NULL; break;
            case 35: run.log.match_id.value = 0; break;
            case 36: run.log.capacity = run.log.count; break;
            case 37: run.log.capacity = 0; break;
            case 38: run.log.count = run.log.capacity + 1u; break;
            case 39: run.log.capacity = SIZE_MAX; break;
            case 40: run.log.count = SIZE_MAX; break;
            case 41: run.log.count = (size_t)UINT64_MAX; run.log.capacity = SIZE_MAX; break;
            case 42: run.log.capacity = SIZE_MAX / sizeof(RmcMatchEvent) + 1u; break;
            case 43: run.log.count = 0; break;
            case 44: run.log.count = 1; break;
            case 45: run.state.fixture.matchday = 0; break;
            case 46: run.state.terminal_reason = RMC_MATCH_TERMINAL_COMPLETED; break;
            default: assert(false);
            }
            reject(&run);
        }
        /* Every bench player, including the reserve goalkeeper, is rejected. */
        for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
            RmcPlayerId id = world.rosters[1].players[i].id;
            bool on_field = false;
            for (size_t j = 0; j < RMC_LINEUP_PLAYERS; ++j) {
                on_field = on_field || good.state.away_lineup.players[j].value == id.value;
            }
            if (!on_field) {
                Run run;
                clone(&run, &good);
                run.action.taker = id;
                reject(&run);
            }
        }
        for (size_t i = 1; i < RMC_LINEUP_PLAYERS; ++i) {
            Run run;
            clone(&run, &good);
            run.action.taker = run.state.away_lineup.players[i];
            if (goal) { verify_restart(&run); } else { reject(&run); }
        }
        unsigned char before[sizeof(good)];
        memcpy(before, &good, sizeof(good));
        for (unsigned absent = 0; absent < 5; ++absent) {
            assert(!rmc_match_restart_apply(absent == 0 ? NULL : &world,
                absent == 1 ? NULL : &good.state, absent == 2 ? NULL : &good.log,
                absent == 3 ? NULL : &good.action, absent == 4 ? NULL : &good.result));
            assert(memcmp(before, &good, sizeof(good)) == 0);
        }
#ifdef RMC_WRAP_MALLOC
        fail_allocation = true;
        reject(&good);
        assert(!fail_allocation);
#endif
    }
}

static void test_causal_corruption(void)
{
    for (unsigned goal = 0; goal < 2; ++goal) {
        Run good;
        pending(&good, false, true, goal != 0);
        for (unsigned which = 0; which < 51; ++which) {
            Run run;
            clone(&run, &good);
            RmcMatchEvent *chance = &run.storage[0];
            RmcMatchEvent *shot = &run.storage[1];
            RmcMatchEvent *last = &run.storage[run.log.count - 1];
            switch (which) {
            case 0: chance->id = 4; break;
            case 1: shot->id = UINT64_MAX; break;
            case 2: last->id = 0; break;
            case 3: chance->match_id.value = 0; break;
            case 4: shot->match_id.value = 2; break;
            case 5: last->match_id.value = 2; break;
            case 6: chance->type = RMC_EVENT_RESTART; break;
            case 7: shot->type = RMC_EVENT_ACTION; break;
            case 8: last->type = RMC_EVENT_ACTION; break;
            case 9: chance->outcome_code = RMC_SHOT_GOAL; break;
            case 10: shot->outcome_code = RMC_SHOT_BLOCKED; break;
            case 11: last->outcome_code = RMC_SHOT_WOODWORK; break;
            case 12: shot->cause_event_id = 0; break;
            case 13: shot->cause_event_id = 2; break;
            case 14: chance->cause_event_id = 1; break;
            case 15: last->cause_event_id = UINT64_MAX; break;
            case 16: --chance->at_second; break;
            case 17: --shot->at_second; break;
            case 18: ++last->at_second; break;
            case 19: chance->team_id = run.state.fixture.away_team; break;
            case 20: shot->team_id.value = 0; break;
            case 21: last->team_id.value = UINT64_MAX; break;
            case 22: chance->primary_actor = run.state.home_lineup.players[2]; break;
            case 23: shot->primary_actor = run.state.home_lineup.players[0]; break;
            case 24: last->primary_actor.value = 0; break;
            case 25: chance->secondary_actor.value = 0; break;
            case 26: shot->secondary_actor = run.state.away_lineup.players[1]; break;
            case 27: chance->shot.model_version = 2; break;
            case 28: shot->shot.model_version = 0; break;
            case 29: shot->shot.model_version = 2; break;
            case 30: chance->shot.chance_quality = 101; break;
            case 31: ++shot->shot.chance_quality; break;
            case 32: shot->shot.source_zone.sector = 1; break;
            case 33: shot->shot.source_zone.lane = RMC_LANE_COUNT; break;
            case 34: shot->shot.context = 21; break;
            case 35: shot->shot.duration_seconds = 0; break;
            case 36: shot->shot.duration_seconds = UINT32_MAX; break;
            case 37: shot->shot.defender_id = run.state.home_lineup.players[1]; break;
            case 38: shot->shot.goalkeeper_id = run.state.away_lineup.players[1]; break;
            case 39: shot->shot.applied_talent_mask = UINT32_MAX; break;
            case 40: shot->shot.trajectory = RMC_SHOT_TRAJECTORY_NONE; break;
            case 41: shot->shot.restart_kind = RMC_RESTART_NONE; break;
            case 42: shot->shot.restart_awarded_team = run.state.fixture.home_team; break;
            case 43: shot->shot.restart_zone.sector = goal ? 0 : 2; break;
            case 44: shot->shot.restart_zone.lane = RMC_LANE_LEFT; break;
            case 45: shot->shot.restart_disposition = RMC_RESTART_DISPOSITION_NONE; break;
            case 46: shot->shot.restart_disposition = RMC_RESTART_SUPPRESSED_PERIOD_END; break;
            case 47: chance->shot.trajectory = RMC_SHOT_TRAJECTORY_WIDE; break;
            case 48: chance->shot.restart_kind = RMC_RESTART_KICKOFF; break;
            case 49: last->shot.context = -21; break;
            case 50: last->shot.restart_zone.lane = RMC_LANE_RIGHT; break;
            default: assert(false);
            }
            reject(&run);
        }
        if (goal) {
            Run run;
            clone(&run, &good);
            --run.log.count; /* Orphan SHOT/GOAL without its mandatory GOAL. */
            reject(&run);
            clone(&run, &good);
            run.storage[2].cause_event_id = 1; /* GOAL must point to SHOT. */
            reject(&run);
            clone(&run, &good);
            run.storage[2].shot.chance_quality += 1u;
            reject(&run);
        }
        /* A schema-valid unrelated tail cannot complete or extend the cause. */
        RmcMatchEvent unrelated = {
            .id = good.log.count + 1u, .match_id = good.state.fixture.id,
            .at_second = good.state.elapsed_seconds, .type = RMC_EVENT_ACTION
        };
        assert(rmc_match_event_log_append(&world, &good.state, &good.log, &unrelated));
        reject(&good);
    }
    /* All non-restart outcomes are real shots, including a blocked latent wide. */
    const RmcShotEventOutcome outcomes[] = {
        RMC_SHOT_BLOCKED, RMC_SHOT_SAVED_HELD, RMC_SHOT_SAVED_REBOUND, RMC_SHOT_WOODWORK
    };
    for (size_t i = 0; i < sizeof(outcomes) / sizeof(outcomes[0]); ++i) {
        Run run;
        const RmcMatchState state = active(false, true);
        init_run(&run, &state);
        shoot(&run, outcomes[i], i == 0 ? RMC_SHOT_TRAJECTORY_WIDE : RMC_SHOT_TRAJECTORY_NONE);
        run.state.mode = RMC_MATCH_RESTART_PENDING;
        run.state.possession = (RmcMatchPossession){0};
        run.action.restart_kind = RMC_RESTART_GOAL_KICK;
        run.action.awarded_team = run.state.fixture.away_team;
        run.action.taker = run.state.away_lineup.players[0];
        reject(&run);
    }
}

static void test_world_and_roles(void)
{
    for (unsigned goal = 0; goal < 2; ++goal) {
        Run good;
        pending(&good, false, true, goal != 0);
        for (unsigned which = 0; which < 9; ++which) {
            Run run;
            clone(&run, &good);
            world = original_world;
            switch (which) {
            case 0: world.identity.rules_version = 2; break;
            case 1: world.identity.generator_version = 2; break;
            case 2: world.rosters[1].team_id = world.teams[0].id; break;
            case 3: player(run.action.taker)->team_id = world.teams[0].id; break;
            case 4: player(run.action.taker)->department = goal ? RMC_DEPARTMENT_GOALKEEPER : RMC_DEPARTMENT_DEFENDER; break;
            case 5: player(run.action.taker)->energy = 101; break;
            case 6: player(run.state.away_lineup.players[1])->primary_slots |= UINT16_C(1) << RMC_SLOT_POR;
                    run.state.away_lineup.players[0] = run.state.away_lineup.players[1]; break;
            case 7: run.state.away_lineup.players[1] = run.state.away_lineup.players[0]; break;
            case 8: world.teams[1].id.value = UINT64_MAX; break;
            default: assert(false);
            }
            reject(&run);
        }
        world = original_world;
        /* Explicit alternate POR occupant qualifies when used for the shot too. */
        RmcMatchState state = active(false, true);
        RmcPlayerId reserve = {0};
        for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
            const RmcPlayer *p = &world.rosters[1].players[i];
            if (p->department == RMC_DEPARTMENT_GOALKEEPER &&
                p->id.value != state.away_lineup.players[0].value) {
                reserve = p->id;
            }
        }
        assert(reserve.value != 0);
        state.away_lineup.players[0] = reserve;
        assert(rmc_match_state_valid(&world, &state));
        Run run;
        init_run(&run, &state);
        shoot(&run, goal ? RMC_SHOT_GOAL : RMC_SHOT_WIDE, RMC_SHOT_TRAJECTORY_NONE);
        verify_restart(&run);
        resume_pass(&run);
    }
}

static void test_deadlines_and_lifecycle(void)
{
    for (unsigned second = 0; second < 2; ++second) {
        for (unsigned goal = 0; goal < 2; ++goal) {
            RmcMatchState state = active(second != 0, true);
            uint32_t deadline;
            assert(rmc_stoppage_deadline(second ? state.second_half_start_second : 0,
                second ? state.second_half_stoppage_seconds : state.first_half_stoppage_seconds,
                &deadline));
            for (unsigned at_deadline = 0; at_deadline < 2; ++at_deadline) {
                state.elapsed_seconds = deadline - (at_deadline ? 5u : 6u);
                Run run;
                init_run(&run, &state);
                shoot(&run, goal ? RMC_SHOT_GOAL : RMC_SHOT_WIDE, RMC_SHOT_TRAJECTORY_NONE);
                if (at_deadline) {
                    /* Production shot closed the half and suppressed the restart. */
                    assert(run.state.period == (second ? RMC_MATCH_FINISHED : RMC_MATCH_HALF_TIME));
                    run.action = (RmcRestartAction){
                        .restart_kind = goal ? RMC_RESTART_KICKOFF : RMC_RESTART_GOAL_KICK,
                        .awarded_team = state.fixture.away_team,
                        .taker = state.away_lineup.players[goal ? 1 : 0],
                        .source_shot_event_id = 2
                    };
                    assert(run.storage[1].shot.restart_disposition == RMC_RESTART_SUPPRESSED_PERIOD_END);
                    reject(&run);
                    run.state.period = state.period;
                    run.state.terminal_reason = RMC_MATCH_TERMINAL_NONE;
                    run.state.mode = RMC_MATCH_RESTART_PENDING;
                    if (!second) { run.state.second_half_start_second = 0; }
                    reject(&run);
                } else {
                    /* A legal shot completed inside authorized stoppage. */
                    Run stale;
                    clone(&stale, &run);
                    stale.state.elapsed_seconds = deadline;
                    reject(&stale);
                    ++stale.state.elapsed_seconds;
                    reject(&stale);
                    verify_restart(&run);
                }
            }
            /* Lifecycle pending alone and arbitrary score never authorize play. */
            Run run;
            state = initial;
            assert(rmc_match_transition_apply(&world, &state, RMC_TRANSITION_START, 0));
            if (second) {
                state.mode = RMC_MATCH_BALL_IN_PLAY;
                assert(rmc_match_clock_advance(&world, &state, 2700));
                assert(rmc_match_transition_apply(&world, &state, RMC_TRANSITION_HALF_TIME, 2700));
                assert(rmc_match_transition_apply(&world, &state, RMC_TRANSITION_SECOND_HALF, 2700));
            }
            state.home_goals = 9;
            state.away_goals = 8;
            init_run(&run, &state);
            run.action = (RmcRestartAction){
                .restart_kind = goal ? RMC_RESTART_KICKOFF : RMC_RESTART_GOAL_KICK,
                .awarded_team = state.fixture.away_team,
                .taker = state.away_lineup.players[goal ? 1 : 0], .source_shot_event_id = 2
            };
            reject(&run);
            /* A first-half source cannot be made pending in the second half. */
            pending(&run, false, true, goal != 0);
            run.state.period = RMC_MATCH_SECOND_HALF;
            run.state.second_half_start_second = 2760;
            run.state.elapsed_seconds = 2860;
            reject(&run);
            /* Checked deadline overflow, even with otherwise valid state. */
            pending(&run, true, true, goal != 0);
            run.state.second_half_start_second = UINT32_MAX - 10u;
            run.state.elapsed_seconds = UINT32_MAX - 5u;
            assert(rmc_match_state_valid(&world, &run.state));
            reject(&run);
        }
    }
}

static void test_prefix_and_replay(void)
{
    Run good;
    pending(&good, false, true, true);
    const uint64_t old_source = good.action.source_shot_event_id;
    verify_restart(&good);
    /* Preserve real first-half history through qualified period transitions. */
    assert(rmc_match_clock_advance(&world, &good.state, 2760u - good.state.elapsed_seconds));
    assert(rmc_match_period_commit(&world, &good.state, &good.log));
    assert(rmc_match_transition_apply(&world, &good.state, RMC_TRANSITION_SECOND_HALF, 2760));
    good.state.mode = RMC_MATCH_BALL_IN_PLAY;
    good.state.second_half_stoppage_seconds = 90;
    assert(rmc_match_clock_advance(&world, &good.state, 100));
    possession(&good.state, false);
    shoot(&good, RMC_SHOT_WIDE, RMC_SHOT_TRAJECTORY_NONE);
    for (unsigned which = 0; which < 15; ++which) {
        Run run;
        clone(&run, &good);
        switch (which) {
        case 0: run.storage[0].id = 9; break;
        case 1: run.storage[0].match_id.value = 0; break;
        case 2: run.storage[0].type = RMC_EVENT_TYPE_COUNT; break;
        case 3: run.storage[0].at_second = run.state.elapsed_seconds + 1u; break;
        case 4: run.storage[0].cause_event_id = 1; break;
        case 5: run.storage[1].cause_event_id = 0; break;
        case 6: run.storage[1].shot.chance_quality = 101; break;
        case 7: run.storage[2].shot.trajectory = RMC_SHOT_TRAJECTORY_WIDE; break;
        case 8: run.storage[0].primary_actor.value = UINT64_MAX; break;
        case 9: run.storage[3].shot.chance_quality = 1; break;
        case 10: run.storage[3].secondary_actor.value = UINT64_MAX; break;
        case 11: run.storage[3].team_id.value = UINT64_MAX; break;
        case 12: run.storage[3].cause_event_id = 3; run.storage[2].id = 0; break;
        case 13: run.action.source_shot_event_id = old_source; break;
        case 14: run.storage[2].shot.restart_disposition = RMC_RESTART_SUPPRESSED_PERIOD_END; break;
        default: assert(false);
        }
        reject(&run);
    }
    Run replay;
    clone(&replay, &good);
    verify_restart(&good);
    verify_restart(&replay);
    resume_pass(&good);
    resume_pass(&replay);
    same_state(&good.state, &replay.state);
    assert(good.rng.state == replay.rng.state && good.rng.increment == replay.rng.increment);
    assert(good.log.count == replay.log.count);
    for (size_t i = 0; i < good.log.count; ++i) {
        same_event(&good.storage[i], &replay.storage[i]);
    }
    /* Keep the existing event-free continuing-possession reset semantics. */
    if (good.state.possession.controller.value == 0) {
        good.state.possession.controller = good.state.possession.initiator;
    }
    good.state.possession.action_count = 32;
    const uint32_t elapsed = good.state.possession.elapsed_seconds;
    const size_t count = good.log.count;
    RmcSequenceDecision decision;
    assert(rmc_match_sequence_decide(&world, &good.state, &decision));
    assert(decision == RMC_SEQUENCE_RESET_ACTION_WINDOW);
    assert(rmc_match_sequence_apply(&world, &good.state, decision));
    assert(good.state.possession.action_count == 0);
    assert(good.state.possession.elapsed_seconds == elapsed && good.log.count == count);
    good.state.elapsed_seconds = good.state.second_half_start_second + 2790u;
    good.state.possession.action_count = 32;
    assert(rmc_match_sequence_decide(&world, &good.state, &decision));
    assert(decision == RMC_SEQUENCE_END_PERIOD);
}

int main(void)
{
    setup();
    test_integration();
    test_commands_and_state();
    test_causal_corruption();
    test_world_and_roles();
    test_deadlines_and_lifecycle();
    test_prefix_and_replay();
    puts("RESTART_GOAL_KICKOFF_PASS_AND_WIDE_GOAL_KICK_PASS_BOTH_TEAMS_HALVES=PASS");
    puts("RESTART_CURRENT_OUTFIELD_AND_POR_TAKERS=PASS");
    puts("RESTART_CAUSAL_PREFIX_SINGLE_USE_AND_REJECTION_ATOMICITY=PASS");
    puts("RESTART_DEADLINES_SUPPRESSION_AND_ACTION_WINDOW=PASS");
    puts("RESTART_CAPACITY_HEADROOM_AND_ALLOCATION_FAILURE=PASS");
    puts("RESTART_ZERO_RNG_AND_DETERMINISTIC_MIXED_HALF_REPLAY=PASS");
    return 0;
}
