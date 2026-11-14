#include "match_opening_kickoff.h"
#include "match_transitions.h"
#include "match_period_commit.h"
#include "match_clock.h"
#include "match_pass_action.h"
#include "match_carry_action.h"
#include "match_dribble_action.h"
#include "match_tackle_action.h"
#include "match_recovery.h"
#include "match_restart_action.h"
#include "match_shot_action.h"
#include "match_interruption.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(RMC_EVENT_SCHEMA_VERSION == 2 && RMC_SHOT_MODEL_VERSION == 1 &&
               RMC_RULES_VERSION == 1 && RMC_GENERATOR_VERSION == 1,
               "Preserve frozen versions");

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

enum { STORAGE = 256 };
static RmcWorldCore world;
static RmcMatchState initial;

typedef struct {
    RmcMatchState state;
    RmcMatchEvent storage[STORAGE];
    RmcMatchEventLog log;
    RmcOpeningKickoffAction action;
    RmcOpeningKickoffResult result;
    RmcPcg32 rng;
} Run;

static const RmcRoster *roster(RmcTeamId team)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (world.rosters[i].team_id.value == team.value) {
            return &world.rosters[i];
        }
    }
    assert(false);
    return NULL;
}

static void setup(void)
{
    const RmcWorldIdentity identity = {
        .world_seed = UINT64_C(20261114),
        .generator_version = RMC_GENERATOR_VERSION,
        .rules_version = RMC_RULES_VERSION
    };
    assert(rmc_world_core_generate(&identity, &world));
    RmcIdentityState ids = world.ids;
    RmcCalendar calendar;
    assert(rmc_calendar_generate(&world, &ids, &calendar));
    assert(rmc_calendar_valid(&world, &ids, &calendar));
    const RmcFixture *f = &calendar.fixtures[0];
    RmcLineup home, away;
    assert(rmc_roster_lineup(roster(f->home_team), RMC_FORMATION_442, &home));
    assert(rmc_roster_lineup(roster(f->away_team), RMC_FORMATION_433, &away));
    assert(rmc_match_state_init(&world, f, &home, &away, &initial));
}

static const RmcLineup *lineup(const RmcMatchState *state, RmcTeamId team)
{
    return team.value == state->fixture.home_team.value ?
        &state->home_lineup : &state->away_lineup;
}

static RmcTeamId opposite(const RmcMatchState *state, RmcTeamId team)
{
    return team.value == state->fixture.home_team.value ?
        state->fixture.away_team : state->fixture.home_team;
}

static void init_run(Run *run, bool home)
{
    memset(run, 0xa5, sizeof(*run));
    run->state = initial;
    assert(rmc_match_event_log_init(&run->state, run->storage, STORAGE, &run->log));
    assert(rmc_stream_init(&run->rng, &world.identity, RMC_STREAM_MATCH,
                           initial.fixture.id.value));
    run->action = (RmcOpeningKickoffAction){
        .opening_period = RMC_MATCH_FIRST_HALF,
        .opening_team = home ? initial.fixture.home_team : initial.fixture.away_team,
        .taker = home ? initial.home_lineup.players[1] : initial.away_lineup.players[1]
    };
}

static void clone(Run *to, const Run *from)
{
    memcpy(to, from, sizeof(*to));
    to->log.events = to->storage;
}

static void reject(Run *run)
{
    unsigned char before[sizeof(*run)];
    memcpy(before, run, sizeof(*run));
    assert(!rmc_match_opening_kickoff_apply(
        &world, &run->state, &run->log, &run->action, &run->result));
    /* Includes RNG, result, metadata and every unused storage byte. */
    assert(memcmp(before, run, sizeof(*run)) == 0);
}

static void neutral_shot(const RmcShotEventPayload *s)
{
    assert(s->model_version == 0 && s->chance_quality == 0 && s->context == 0);
    assert(s->duration_seconds == 0 && s->applied_talent_mask == 0);
    assert(s->source_zone.sector == 0 && s->source_zone.lane == RMC_LANE_LEFT);
    assert(s->defender_id.value == 0 && s->goalkeeper_id.value == 0);
    assert(s->trajectory == RMC_SHOT_TRAJECTORY_NONE);
    assert(s->restart_kind == RMC_RESTART_NONE && s->restart_awarded_team.value == 0);
    assert(s->restart_zone.sector == 0 && s->restart_zone.lane == RMC_LANE_LEFT);
    assert(s->restart_disposition == RMC_RESTART_DISPOSITION_NONE);
}

static void verify_opening(Run *run)
{
    const RmcMatchState before = run->state;
    const RmcPcg32 rng = run->rng;
    const size_t count = run->log.count;
    RmcMatchEvent storage[STORAGE];
    memcpy(storage, run->storage, sizeof(storage));
    assert(rmc_match_opening_kickoff_apply(
        &world, &run->state, &run->log, &run->action, &run->result));
    assert(run->log.count == count + 2);
    assert(run->log.events == run->storage && run->log.capacity <= STORAGE);
    assert(memcmp(storage, run->storage, count * sizeof(storage[0])) == 0);
    assert(memcmp(&storage[count + 2], &run->storage[count + 2],
                  (STORAGE - count - 2) * sizeof(storage[0])) == 0);
    assert(run->state.period == before.period && run->state.mode == RMC_MATCH_BALL_IN_PLAY);
    assert(run->state.elapsed_seconds == before.elapsed_seconds);
    assert(run->state.second_half_start_second == before.second_half_start_second);
    assert(run->state.first_half_stoppage_seconds == before.first_half_stoppage_seconds);
    assert(run->state.second_half_stoppage_seconds == before.second_half_stoppage_seconds);
    assert(run->state.home_goals == before.home_goals && run->state.away_goals == before.away_goals);
    assert(run->state.terminal_reason == RMC_MATCH_TERMINAL_NONE);
    assert(memcmp(&before.fixture, &run->state.fixture, sizeof(before.fixture)) == 0);
    assert(memcmp(&before.home_lineup, &run->state.home_lineup, sizeof(before.home_lineup)) == 0);
    assert(memcmp(&before.away_lineup, &run->state.away_lineup, sizeof(before.away_lineup)) == 0);
    const RmcMatchPossession *p = &run->state.possession;
    assert(p->team_id.value == run->action.opening_team.value);
    assert(p->initiator.value == run->action.taker.value && p->controller.value == run->action.taker.value);
    assert(p->zone.sector == 2 && p->zone.lane == RMC_LANE_CENTRE);
    assert(p->phase == RMC_PHASE_BUILDUP && p->action_count == 0 && p->elapsed_seconds == 0);
    assert(run->rng.state == rng.state && run->rng.increment == rng.increment);
    const RmcMatchEvent *start = &run->storage[count];
    const RmcMatchEvent *restart = &run->storage[count + 1];
    assert(start->id == count + 1 && restart->id == count + 2);
    assert(start->type == RMC_EVENT_PERIOD_START && restart->type == RMC_EVENT_RESTART);
    assert(start->match_id.value == before.fixture.id.value && restart->match_id.value == before.fixture.id.value);
    assert(start->at_second == before.elapsed_seconds && restart->at_second == before.elapsed_seconds);
    assert(start->team_id.value == run->action.opening_team.value && restart->team_id.value == start->team_id.value);
    assert(start->primary_actor.value == 0 && start->secondary_actor.value == 0 && start->outcome_code == 0);
    assert(start->cause_event_id == (count == 0 ? 0 : run->storage[count - 1].id));
    assert(restart->primary_actor.value == run->action.taker.value && restart->secondary_actor.value == 0);
    assert(restart->outcome_code == RMC_RESTART_KICKOFF && restart->cause_event_id == start->id);
    neutral_shot(&start->shot);
    neutral_shot(&restart->shot);
    assert(run->result.period_start_event_id == start->id && run->result.restart_event_id == restart->id);
    assert(run->result.awarded_team.value == run->action.opening_team.value && run->result.taker.value == run->action.taker.value);
    assert(rmc_match_state_valid(&world, &run->state));
    reject(run); /* Single use even without another event or clock tick. */
}

/* The inactive -> pending transition stays private until opening commits.
 * Opening append is the only fallible caller-storage commit; publication of
 * the staged state is a plain assignment. Test failure at this exact boundary.
 */
static void staged_opening(Run *run)
{
    RmcMatchState candidate = run->state;
    assert(rmc_match_transition_apply(&world, &candidate,
        candidate.period == RMC_MATCH_NOT_STARTED ? RMC_TRANSITION_START : RMC_TRANSITION_SECOND_HALF,
        candidate.elapsed_seconds));
    assert(rmc_match_opening_kickoff_apply(
        &world, &candidate, &run->log, &run->action, &run->result));
    run->state = candidate;
}

static void pending_first(Run *run, bool home)
{
    init_run(run, home);
    assert(rmc_match_transition_apply(&world, &run->state, RMC_TRANSITION_START, 0));
}

static void pass(Run *run, uint32_t duration)
{
    const RmcTeamId team = run->state.possession.team_id;
    const RmcLineup *own = lineup(&run->state, team);
    const RmcLineup *other = lineup(&run->state, opposite(&run->state, team));
    const RmcPassAction action = {
        .team_id = team, .passer = run->state.possession.controller,
        .recipient = own->players[own->players[1].value == run->state.possession.controller.value ? 2 : 1],
        .opponent = other->players[1], .duration_seconds = duration
    };
    RmcPassResult result;
    assert(rmc_match_pass_apply(&world, &run->state, &run->rng, &run->log, &action, &result));
}

static void prepare_second(Run *run)
{
    assert(run->state.period == RMC_MATCH_HALF_TIME);
    const RmcTeamId award = opposite(&run->state, run->storage[0].team_id);
    run->action = (RmcOpeningKickoffAction){
        .opening_period = RMC_MATCH_SECOND_HALF, .opening_team = award,
        .taker = lineup(&run->state, award)->players[2]
    };
    assert(rmc_match_transition_apply(&world, &run->state,
        RMC_TRANSITION_SECOND_HALF, run->state.elapsed_seconds));
}

static void second_boundary(Run *run, bool home, bool shot_closure)
{
    init_run(run, home);
    staged_opening(run);
    if (shot_closure) {
        const RmcLineup *other = lineup(&run->state,
            opposite(&run->state, run->state.possession.team_id));
        const RmcShotAction action = {
            .team_id = run->state.possession.team_id,
            .shooter = run->state.possession.controller,
            .defender = other->players[1], .goalkeeper = other->players[0],
            .duration_seconds = 2700
        };
        RmcShotResult result;
        assert(rmc_match_shot_apply(&world, &run->state, &run->rng, &run->log, &action, &result));
    } else {
        pass(run, 2700);
        assert(rmc_match_period_commit(&world, &run->state, &run->log));
    }
    assert(run->storage[run->log.count - 1].type == RMC_EVENT_PERIOD_END);
    prepare_second(run);
}

static void test_openings(void)
{
    for (unsigned home = 0; home < 2; ++home) {
        for (size_t slot = 1; slot < RMC_LINEUP_PLAYERS; ++slot) {
            Run run;
            pending_first(&run, home != 0);
            run.action.taker = lineup(&run.state, run.action.opening_team)->players[slot];
            run.log.capacity = 2; /* Exactly two slots. */
            verify_opening(&run);
        }
        for (unsigned shot = 0; shot < 2; ++shot) {
            Run run;
            second_boundary(&run, home != 0, shot != 0);
            const RmcTeamId first = run.storage[0].team_id;
            assert(first.value != run.action.opening_team.value);
            run.log.capacity = run.log.count + 2;
            verify_opening(&run);
            run.log.capacity = STORAGE;
            pass(&run, 1); /* Immediate production PASS after second opening. */
        }
        Run run;
        init_run(&run, home != 0);
        staged_opening(&run);
        Run duplicate;
        clone(&duplicate, &run);
        duplicate.state.mode = RMC_MATCH_RESTART_PENDING;
        duplicate.state.possession = (RmcMatchPossession){0};
        reject(&duplicate); /* Empty possession cannot reopen an existing half. */
        pass(&run, 1); /* Immediate production PASS after first opening. */
    }
}

static void test_rejections(void)
{
    for (unsigned second = 0; second < 2; ++second) {
        Run good;
        if (second) { second_boundary(&good, true, false); }
        else { pending_first(&good, true); }
        for (unsigned which = 0; which < 43; ++which) {
            Run run;
            clone(&run, &good);
            switch (which) {
            case 0: run.action.taker.value = 0; break;
            case 1: run.action.taker.value = UINT64_MAX; break;
            case 2: run.action.taker = lineup(&run.state, run.action.opening_team)->players[0]; break;
            case 3: run.action.taker = lineup(&run.state, opposite(&run.state, run.action.opening_team))->players[1]; break;
            case 4: run.action.opening_team.value = 0; break;
            case 5: run.action.opening_team.value = UINT64_MAX; break;
            case 6: run.action.opening_period = RMC_MATCH_NOT_STARTED; break;
            case 7: run.action.opening_period = RMC_MATCH_FINISHED; break;
            case 8: run.action.opening_period = second ? RMC_MATCH_FIRST_HALF : RMC_MATCH_SECOND_HALF; break;
            case 9: run.state.elapsed_seconds += 1; break;
            case 10: run.state.elapsed_seconds = UINT32_MAX; break;
            case 11: run.state.period = RMC_MATCH_NOT_STARTED; break;
            case 12: run.state.period = RMC_MATCH_HALF_TIME; break;
            case 13: run.state.period = RMC_MATCH_FINISHED; break;
            case 14: run.state.mode = RMC_MATCH_BALL_IN_PLAY; break;
            case 15: run.state.mode = RMC_MATCH_MANAGEMENT_WINDOW; break;
            case 16: run.state.mode = RMC_MATCH_PERIOD_ENDED; break;
            case 17: run.state.possession.zone.sector = 1; break;
            case 18: run.state.possession.zone.lane = RMC_LANE_CENTRE; break;
            case 19: run.state.possession.phase = RMC_PHASE_TRANSITION; break;
            case 20: run.state.possession.initiator = run.action.taker; break;
            case 21: run.state.possession.controller = run.action.taker; break;
            case 22: run.state.possession.action_count = 1; break;
            case 23: run.state.possession.elapsed_seconds = 1; break;
            case 24: run.state.possession.team_id = run.action.opening_team; break;
            case 25: run.state.fixture.id.value = 0; break;
            case 26: run.state.fixture.away_team = run.state.fixture.home_team; break;
            case 27: run.state.fixture.matchday = 0; break;
            case 28: run.state.home_lineup.players[1] = run.state.home_lineup.players[2]; break;
            case 29: run.state.away_lineup.slots[0] = RMC_SLOT_ATT; break;
            case 30: run.log.events = NULL; break;
            case 31: run.log.match_id.value = 0; break;
            case 32: run.log.capacity = run.log.count + 1; break;
            case 33: run.log.capacity = run.log.count; break;
            case 34: run.log.count = run.log.capacity + 1; break;
            case 35: run.log.capacity = SIZE_MAX; break;
            case 36: run.log.count = SIZE_MAX; break;
            case 37: run.log.count = (size_t)UINT64_MAX - 1; run.log.capacity = SIZE_MAX; break;
            case 38: run.log.capacity = SIZE_MAX / sizeof(RmcMatchEvent) + 1; break;
            case 39: run.state.terminal_reason = RMC_MATCH_TERMINAL_COMPLETED; break;
            case 40: run.state.second_half_stoppage_seconds = 1; break;
            case 41: run.state.second_half_start_second += 1; break;
            case 42: if (second) { --run.log.count; } else { run.state.home_goals = 1; } break;
            default: assert(false);
            }
            reject(&run);
        }
        const RmcRoster *r = roster(good.action.opening_team);
        for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
            bool active = false;
            const RmcLineup *l = lineup(&good.state, good.action.opening_team);
            for (size_t j = 0; j < RMC_LINEUP_PLAYERS; ++j) {
                active = active || l->players[j].value == r->players[i].id.value;
            }
            if (!active) {
                Run run;
                clone(&run, &good);
                run.action.taker = r->players[i].id;
                reject(&run);
            }
        }
        unsigned char before[sizeof(good)];
        memcpy(before, &good, sizeof(good));
        for (unsigned absent = 0; absent < 5; ++absent) {
            assert(!rmc_match_opening_kickoff_apply(absent == 0 ? NULL : &world,
                absent == 1 ? NULL : &good.state, absent == 2 ? NULL : &good.log,
                absent == 3 ? NULL : &good.action, absent == 4 ? NULL : &good.result));
            assert(memcmp(before, &good, sizeof(good)) == 0);
        }
        if (second) {
            Run run;
            clone(&run, &good);
            run.action.opening_team = opposite(&run.state, run.action.opening_team);
            run.action.taker = lineup(&run.state, run.action.opening_team)->players[1];
            reject(&run);
#ifdef RMC_WRAP_MALLOC
            fail_allocation = true;
            reject(&good);
            assert(!fail_allocation);
#endif
        } else {
            Run run;
            clone(&run, &good);
            run.state.first_half_stoppage_seconds = 1;
            reject(&run);
            clone(&run, &good);
            run.state.away_goals = 1;
            reject(&run);
        }
    }
}

static void test_history_corruption(void)
{
    for (unsigned shot = 0; shot < 2; ++shot) {
        Run good;
        second_boundary(&good, true, shot != 0);
        for (unsigned which = 0; which < 45; ++which) {
            Run run;
            clone(&run, &good);
            RmcMatchEvent *s = &run.storage[0];
            RmcMatchEvent *r = &run.storage[1];
            RmcMatchEvent *a = &run.storage[2];
            RmcMatchEvent *e = &run.storage[run.log.count - 1];
            switch (which) {
            case 0: s->id = 2; break;
            case 1: r->id = UINT64_MAX; break;
            case 2: e->id = 0; break;
            case 3: s->match_id.value = 0; break;
            case 4: r->match_id.value += 1; break;
            case 5: e->match_id.value += 1; break;
            case 6: s->type = RMC_EVENT_POSSESSION_START; break;
            case 7: s->type = RMC_EVENT_SHOT; break;
            case 8: r->type = RMC_EVENT_GOAL; break;
            case 9: r->type = RMC_EVENT_PERIOD_START; break;
            case 10: e->type = RMC_EVENT_ACTION; break;
            case 11: e->type = RMC_EVENT_MATCH_END; break;
            case 12: s->team_id.value = 0; break;
            case 13: s->team_id.value = UINT64_MAX; break;
            case 14: s->team_id = run.state.fixture.away_team; break;
            case 15: r->team_id = run.state.fixture.away_team; break;
            case 16: e->team_id = run.state.fixture.home_team; break;
            case 17: s->primary_actor = r->primary_actor; break;
            case 18: s->secondary_actor = r->primary_actor; break;
            case 19: r->primary_actor = run.state.home_lineup.players[0]; break;
            case 20: r->primary_actor = run.state.away_lineup.players[1]; break;
            case 21: r->secondary_actor = r->primary_actor; break;
            case 22: e->primary_actor = r->primary_actor; break;
            case 23: e->secondary_actor = r->primary_actor; break;
            case 24: s->outcome_code = 1; break;
            case 25: r->outcome_code = RMC_RESTART_GOAL_KICK; break;
            case 26: e->outcome_code = 1; break;
            case 27: s->cause_event_id = 1; break;
            case 28: r->cause_event_id = 0; break;
            case 29: r->cause_event_id = 2; break;
            case 30: e->cause_event_id = 1; break;
            case 31: s->at_second = 1; break;
            case 32: r->at_second = 1; break;
            case 33: --e->at_second; break;
            case 34: ++e->at_second; break;
            case 35: s->shot.chance_quality = 1; break;
            case 36: r->shot.restart_kind = RMC_RESTART_KICKOFF; break;
            case 37: e->shot.source_zone.lane = RMC_LANE_CENTRE; break;
            case 38: a->cause_event_id = 1; break;
            case 39: a->type = RMC_EVENT_PERIOD_START; break;
            case 40: a->primary_actor = run.state.away_lineup.players[1]; break;
            case 41: run.log.count = 0; break;
            case 42: run.log.count = 1; break;
            case 43: run.log.count = 2; break;
            case 44: a->outcome_code = 999; break;
            default: assert(false);
            }
            reject(&run);
        }
        if (shot) {
            for (unsigned which = 0; which < 8; ++which) {
                Run run;
                clone(&run, &good);
                RmcMatchEvent *s = &run.storage[3];
                switch (which) {
                case 0: s->cause_event_id = 1; break;
                case 1: s->cause_event_id = 0; break;
                case 2: s->shot.chance_quality = 101; break;
                case 3: s->shot.model_version = 0; break;
                case 4: s->shot.duration_seconds = UINT32_MAX; break;
                case 5: s->shot.source_zone.sector = 0; break;
                case 6: s->shot.restart_disposition = RMC_RESTART_PENDING; s->shot.restart_kind = RMC_RESTART_KICKOFF; break;
                case 7: s->type = RMC_EVENT_RESTART; break;
                default: assert(false);
                }
                reject(&run);
            }
        }
    }
}

static void test_transaction_boundary(void)
{
    for (unsigned second = 0; second < 2; ++second) {
        Run run;
        if (second) {
            init_run(&run, false);
            staged_opening(&run);
            pass(&run, 2700);
            assert(rmc_match_period_commit(&world, &run.state, &run.log));
            const RmcTeamId award = opposite(&run.state, run.storage[0].team_id);
            run.action = (RmcOpeningKickoffAction){RMC_MATCH_SECOND_HALF, award,
                lineup(&run.state, award)->players[1]};
        } else {
            init_run(&run, true);
        }
        run.log.capacity = run.log.count + 1;
        unsigned char before[sizeof(run)];
        memcpy(before, &run, sizeof(run));
        RmcMatchState candidate = run.state;
        assert(rmc_match_transition_apply(&world, &candidate,
            second ? RMC_TRANSITION_SECOND_HALF : RMC_TRANSITION_START,
            candidate.elapsed_seconds));
        const RmcMatchState pending = candidate;
        assert(!rmc_match_opening_kickoff_apply(&world, &candidate, &run.log, &run.action, &run.result));
        assert(memcmp(&candidate, &pending, sizeof(candidate)) == 0);
        assert(memcmp(before, &run, sizeof(run)) == 0);
        run.log.capacity = STORAGE;
        staged_opening(&run);
        assert(run.state.mode == RMC_MATCH_BALL_IN_PLAY);
    }
}

static void test_world_validation(void)
{
    const RmcWorldCore saved = world;
    for (unsigned second = 0; second < 2; ++second) {
        Run good;
        if (second) { second_boundary(&good, true, false); }
        else { pending_first(&good, true); }
        for (unsigned which = 0; which < 5; ++which) {
            world = saved;
            Run run;
            clone(&run, &good);
            switch (which) {
            case 0: world.identity.rules_version = 2; break;
            case 1: world.identity.generator_version = 2; break;
            case 2: world.rosters[0].team_id = world.teams[1].id; break;
            case 3: world.rosters[0].players[1].team_id = world.teams[1].id; break;
            case 4: world.rosters[0].players[1].energy = 101; break;
            default: assert(false);
            }
            reject(&run);
        }
        world = saved;
    }
}

/* Noncanonical engineering policy: legal timed sporting operations selected
 * from current participants only, one canonical MATCH stream, no injected
 * possession/clock/mode and no reseeding. Used to qualify mixed histories. */
static void sporting_step(Run *run, unsigned step, bool ending_shot)
{
    RmcMatchState *s = &run->state;
    const RmcTeamId team = s->possession.team_id;
    const RmcLineup *own = lineup(s, team);
    const RmcLineup *other = lineup(s, opposite(s, team));
    const uint32_t remaining = 2700 - s->elapsed_seconds;
    assert(remaining > 0);
    if (s->mode == RMC_MATCH_RESTART_PENDING) {
        const RmcMatchEvent *shot = &run->storage[run->log.count - 1];
        if (shot->type == RMC_EVENT_GOAL) { --shot; }
        const RmcTeamId award = shot->shot.restart_awarded_team;
        const RmcRestartAction action = {
            .restart_kind = shot->shot.restart_kind, .awarded_team = award,
            .taker = lineup(s, award)->players[shot->shot.restart_kind == RMC_RESTART_GOAL_KICK ? 0 : 1],
            .source_shot_event_id = shot->id
        };
        RmcRestartResult result;
        assert(rmc_match_restart_apply(&world, s, &run->log, &action, &result));
    } else if (s->possession.controller.value == 0) {
        const RmcRecoveryAction action = {
            .incumbent = own->players[1], .challenger = other->players[1],
            .duration_seconds = 1
        };
        RmcRecoveryResult result;
        assert(rmc_match_recovery_apply(&world, s, &run->rng, &run->log, &action, &result));
    } else if (s->possession.controller.value == own->players[0].value) {
        pass(run, 1);
    } else if (s->possession.zone.sector >= 2 && (step % 4 == 0 || ending_shot)) {
        const RmcShotAction action = {
            .team_id = team, .shooter = s->possession.controller,
            .defender = other->players[1], .goalkeeper = other->players[0],
            .duration_seconds = ending_shot ? remaining : 1
        };
        RmcShotResult result;
        assert(rmc_match_shot_apply(&world, s, &run->rng, &run->log, &action, &result));
    } else if (step % 5 == 0) {
        const RmcTackleAction action = {
            .team_id = opposite(s, team), .carrier = s->possession.controller,
            .defender = other->players[1], .duration_seconds = 1
        };
        RmcTackleResult result;
        assert(rmc_match_tackle_apply(&world, s, &run->rng, &run->log, &action, &result));
    } else if (s->possession.zone.sector < 4) {
        if (step % 2 == 0) {
            const RmcCarryAction action = {team, s->possession.controller, other->players[1], 0, 1};
            RmcCarryResult result;
            assert(rmc_match_carry_apply(&world, s, &run->rng, &run->log, &action, &result));
        } else {
            const RmcDribbleAction action = {team, s->possession.controller, other->players[1], 0, 1};
            RmcDribbleResult result;
            assert(rmc_match_dribble_apply(&world, s, &run->rng, &run->log, &action, &result));
        }
    } else {
        pass(run, 1);
    }
}

static void same_event(const RmcMatchEvent *a, const RmcMatchEvent *b)
{
    assert(a->id == b->id && a->match_id.value == b->match_id.value);
    assert(a->type == b->type && a->at_second == b->at_second);
    assert(a->team_id.value == b->team_id.value && a->primary_actor.value == b->primary_actor.value);
    assert(a->secondary_actor.value == b->secondary_actor.value);
    assert(a->outcome_code == b->outcome_code && a->cause_event_id == b->cause_event_id);
    const RmcShotEventPayload *x = &a->shot, *y = &b->shot;
    assert(x->model_version == y->model_version && x->chance_quality == y->chance_quality);
    assert(x->source_zone.sector == y->source_zone.sector && x->source_zone.lane == y->source_zone.lane);
    assert(x->context == y->context && x->duration_seconds == y->duration_seconds);
    assert(x->defender_id.value == y->defender_id.value && x->goalkeeper_id.value == y->goalkeeper_id.value);
    assert(x->applied_talent_mask == y->applied_talent_mask && x->trajectory == y->trajectory);
    assert(x->restart_kind == y->restart_kind && x->restart_awarded_team.value == y->restart_awarded_team.value);
    assert(x->restart_zone.sector == y->restart_zone.sector && x->restart_zone.lane == y->restart_zone.lane);
    assert(x->restart_disposition == y->restart_disposition);
}

static void same_state(const RmcMatchState *a, const RmcMatchState *b)
{
    assert(a->fixture.id.value == b->fixture.id.value && a->fixture.matchday == b->fixture.matchday);
    assert(a->fixture.home_team.value == b->fixture.home_team.value && a->fixture.away_team.value == b->fixture.away_team.value);
    const RmcLineup *x[] = {&a->home_lineup, &a->away_lineup};
    const RmcLineup *y[] = {&b->home_lineup, &b->away_lineup};
    for (size_t side = 0; side < 2; ++side) {
        assert(x[side]->formation == y[side]->formation &&
               x[side]->secondary_assignments == y[side]->secondary_assignments);
        for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
            assert(x[side]->players[i].value == y[side]->players[i].value);
            assert(x[side]->slots[i] == y[side]->slots[i]);
        }
    }
    assert(a->period == b->period && a->mode == b->mode && a->terminal_reason == b->terminal_reason);
    assert(a->elapsed_seconds == b->elapsed_seconds && a->second_half_start_second == b->second_half_start_second);
    assert(a->first_half_stoppage_seconds == b->first_half_stoppage_seconds &&
           a->second_half_stoppage_seconds == b->second_half_stoppage_seconds);
    assert(a->home_goals == b->home_goals && a->away_goals == b->away_goals);
    const RmcMatchPossession *p = &a->possession, *q = &b->possession;
    assert(p->team_id.value == q->team_id.value && p->initiator.value == q->initiator.value &&
           p->controller.value == q->controller.value);
    assert(p->zone.sector == q->zone.sector && p->zone.lane == q->zone.lane && p->phase == q->phase);
    assert(p->action_count == q->action_count && p->elapsed_seconds == q->elapsed_seconds);
}

static void test_mixed_history_and_replay(void)
{
    bool saw_recovery = false, saw_restart = false, saw_goal = false;
    for (unsigned home = 0; home < 2; ++home) {
        Run run, replay;
        init_run(&run, home != 0);
        init_run(&replay, home != 0);
        staged_opening(&run);
        staged_opening(&replay);
        for (unsigned step = 0; step < 100 && run.state.period == RMC_MATCH_FIRST_HALF; ++step) {
            sporting_step(&run, step, step >= 30);
            sporting_step(&replay, step, step >= 30);
        }
        assert(run.state.period == RMC_MATCH_HALF_TIME);
        assert(replay.state.period == RMC_MATCH_HALF_TIME);
        same_state(&run.state, &replay.state);
        prepare_second(&run);
        prepare_second(&replay);
        /* Corrupt each non-opening causal family in an otherwise real prefix.
         * All mutations are invalid-input tests, never sporting setup. */
        for (size_t i = 2; i < run.log.count - 1; ++i) {
            const RmcMatchEvent *e = &run.storage[i];
            saw_recovery = saw_recovery || e->type == RMC_EVENT_RECOVERY;
            saw_restart = saw_restart || e->type == RMC_EVENT_RESTART;
            saw_goal = saw_goal || e->type == RMC_EVENT_GOAL;
            for (unsigned which = 0; which < 6; ++which) {
                Run bad;
                clone(&bad, &run);
                switch (which) {
                case 0: bad.storage[i].cause_event_id = e->cause_event_id == 0 ? 1 : 0; break;
                case 1: bad.storage[i].primary_actor.value = 0; break;
                case 2: bad.storage[i].type = RMC_EVENT_PERIOD_START; break;
                case 3: bad.storage[i].at_second = 0; break;
                case 4: bad.storage[i].team_id.value = 0; break;
                case 5: bad.storage[i].shot = (RmcShotEventPayload){0};
                        bad.storage[i].outcome_code = 0; break;
                default: assert(false);
                }
                reject(&bad);
            }
        }
        verify_opening(&run);
        verify_opening(&replay);
        Run duplicate;
        clone(&duplicate, &run);
        duplicate.state.mode = RMC_MATCH_RESTART_PENDING;
        duplicate.state.possession = (RmcMatchPossession){0};
        reject(&duplicate);
        pass(&run, 1);
        pass(&replay, 1);
        assert(run.log.count == replay.log.count);
        for (size_t i = 0; i < run.log.count; ++i) {
            same_event(&run.storage[i], &replay.storage[i]);
        }
        assert(run.rng.state == replay.rng.state && run.rng.increment == replay.rng.increment);
        same_state(&run.state, &replay.state);
    }
    assert(saw_recovery && saw_restart && saw_goal);
}

static void test_stoppage_and_pending_shot_causes(void)
{
    bool saw_goal = false, saw_wide = false;
    for (unsigned home = 0; home < 2; ++home) {
        Run run;
        init_run(&run, home != 0);
        staged_opening(&run);
        for (unsigned step = 0; step < 30; ++step) {
            sporting_step(&run, step, false);
            if (run.state.mode == RMC_MATCH_RESTART_PENDING) {
                const RmcMatchEvent *last = &run.storage[run.log.count - 1];
                const RmcMatchEvent *shot = last->type == RMC_EVENT_GOAL ? last - 1 : last;
                saw_goal = saw_goal || shot->outcome_code == RMC_SHOT_GOAL;
                saw_wide = saw_wide || shot->outcome_code == RMC_SHOT_WIDE;
                Run bad;
                clone(&bad, &run);
                reject(&bad); /* A real Shot cause never authorizes opening. */
            }
        }
        unsigned step = 30;
        while (run.state.mode != RMC_MATCH_RESTART_PENDING && step < 100) {
            sporting_step(&run, step++, false);
        }
        assert(run.state.mode == RMC_MATCH_RESTART_PENDING);
        assert(rmc_match_interruption_record(&world, &run.state, 60));
        sporting_step(&run, step, false); /* Consume the real pending restart. */
        assert(rmc_match_clock_advance(&world, &run.state, 2760 - run.state.elapsed_seconds));
        assert(rmc_match_period_commit(&world, &run.state, &run.log));
        prepare_second(&run);
        assert(run.state.first_half_stoppage_seconds == 60 && run.state.second_half_start_second == 2760);
        verify_opening(&run);
        pass(&run, 1);
    }
    assert(saw_goal && saw_wide);
}

static void test_frozen_restart_rejects_openings(void)
{
    for (unsigned second = 0; second < 2; ++second) {
        Run run;
        if (second) { second_boundary(&run, true, true); }
        else { pending_first(&run, true); }
        const RmcRestartAction action = {
            .restart_kind = RMC_RESTART_KICKOFF,
            .awarded_team = run.action.opening_team, .taker = run.action.taker,
            .source_shot_event_id = 1
        };
        RmcRestartResult result;
        memset(&result, 0x5a, sizeof(result));
        unsigned char before[sizeof(run)], output[sizeof(result)];
        memcpy(before, &run, sizeof(run));
        memcpy(output, &result, sizeof(result));
        assert(!rmc_match_restart_apply(&world, &run.state, &run.log, &action, &result));
        assert(memcmp(before, &run, sizeof(run)) == 0);
        assert(memcmp(output, &result, sizeof(result)) == 0);
    }
}

int main(void)
{
    setup();
    test_openings();
    test_rejections();
    test_history_corruption();
    test_transaction_boundary();
    test_world_validation();
    test_mixed_history_and_replay();
    test_stoppage_and_pending_shot_causes();
    test_frozen_restart_rejects_openings();
    puts("OPENING_BOTH_HALVES_TEAMS_TAKERS_AND_PRODUCTION_PASS=PASS");
    puts("OPENING_AUTHENTICATED_HISTORY_SINGLE_USE_AND_ATOMIC_REJECTIONS=PASS");
    puts("OPENING_ZERO_TIME_RNG_ACTIONS_AND_DETERMINISTIC_REPLAY=PASS");
    puts("OPENING_PRODUCTION_SHOT_CLOSURE_AND_PRIVATE_LIFECYCLE_TRANSACTION=PASS");
    return 0;
}
