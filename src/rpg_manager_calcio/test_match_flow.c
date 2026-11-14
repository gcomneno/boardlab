/* Shared local harness is also included by the replay executable; no mock
 * engine, artificial successful state or alternate production path. */
#include "match_flow_selector.h"
#include "career.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(RMC_RULES_VERSION == 1 && RMC_GENERATOR_VERSION == 1 &&
    RMC_EVENT_SCHEMA_VERSION == 2 && RMC_MATCH_FLOW_VERSION == 1 &&
    RMC_MATCH_SELECTOR_VERSION == 1, "frozen versions");

#ifdef RMC_WRAP_MALLOC
static bool fail_allocation;
static bool fail_on_apply;
void *__real_malloc(size_t size);
void *__wrap_malloc(size_t size);
void *__wrap_malloc(size_t size)
{
    if (fail_allocation) { fail_allocation = false; return NULL; }
    return __real_malloc(size);
}
#endif

static RmcCareerSetup career;
static RmcLineup home, away;
static const RmcFixture *fixture;

typedef struct {
    RmcMatchFlow flow;
    RmcMatchEvent *events;
    RmcMatchFlowCommand *commands;
} FlowRun;

static const RmcRoster *roster(RmcTeamId team)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (career.foundation.world.rosters[i].team_id.value == team.value) {
            return &career.foundation.world.rosters[i];
        }
    }
    assert(false);
    return NULL;
}

static void setup(uint64_t seed)
{
    const RmcWorldIdentity identity = {seed, RMC_GENERATOR_VERSION, RMC_RULES_VERSION};
    assert(rmc_career_setup_generate(&identity, &career));
    assert(rmc_career_setup_valid(&career));
    fixture = &career.foundation.calendar.fixtures[0];
    assert(rmc_roster_lineup(roster(fixture->home_team), RMC_FORMATION_442, &home));
    assert(rmc_roster_lineup(roster(fixture->away_team), RMC_FORMATION_433, &away));
}

static void init_run(FlowRun *run, bool home_opens, uint64_t guard)
{
    assert(RMC_FLOW_DEFAULT_EVENT_CAPACITY <= SIZE_MAX / sizeof(*run->events));
    assert(RMC_FLOW_DEFAULT_COMMAND_CAPACITY <= SIZE_MAX / sizeof(*run->commands));
    run->events = malloc(RMC_FLOW_DEFAULT_EVENT_CAPACITY * sizeof(*run->events));
    run->commands = malloc(RMC_FLOW_DEFAULT_COMMAND_CAPACITY * sizeof(*run->commands));
    assert(run->events != NULL && run->commands != NULL);
    memset(run->events, 0xa5, RMC_FLOW_DEFAULT_EVENT_CAPACITY * sizeof(*run->events));
    memset(run->commands, 0x5a, RMC_FLOW_DEFAULT_COMMAND_CAPACITY * sizeof(*run->commands));
    const RmcMatchFlowConfig config = {1, 1, home_opens ? fixture->home_team : fixture->away_team, guard};
    assert(rmc_match_flow_init(&career.foundation.world, fixture, &home, &away, &config,
        run->events, RMC_FLOW_DEFAULT_EVENT_CAPACITY,
        run->commands, RMC_FLOW_DEFAULT_COMMAND_CAPACITY, &run->flow) == RMC_FLOW_OK);
    assert(run->flow.state.period == RMC_MATCH_NOT_STARTED && run->flow.log.count == 0);
    RmcPcg32 canonical;
    assert(rmc_stream_init(&canonical, &career.foundation.world.identity, RMC_STREAM_MATCH, fixture->id.value));
    assert(run->flow.match_rng.state == canonical.state && run->flow.match_rng.increment == canonical.increment);
}

static void destroy(FlowRun *run)
{
    free(run->events);
    free(run->commands);
}

static RmcFlowInstructions triple(unsigned bits)
{
    return (RmcFlowInstructions){(RmcFlowApproach)(bits & 1),
        (RmcFlowProgression)((bits >> 1) & 1), (RmcFlowPressure)((bits >> 2) & 1)};
}

static RmcMatchFlowCommand instruction(const RmcMatchFlow *f, const RmcMatchFlowNext *n, unsigned bits)
{
    return (RmcMatchFlowCommand){.command_id = n->observed_command_count + 1,
        .observed_command_count = n->observed_command_count, .match_id = f->state.fixture.id,
        .expected_period = f->state.period, .kind = RMC_FLOW_INSTRUCTION,
        .data.instruction = {n->team_id, f->state.period, bits == 8, bits == 8 ? (RmcFlowInstructions){0} : triple(bits)}};
}

static bool same_instructions(RmcFlowInstructions a, RmcFlowInstructions b)
{
    return a.approach == b.approach && a.progression == b.progression && a.pressure == b.pressure;
}

static void same_command(const RmcMatchFlowCommand *a, const RmcMatchFlowCommand *b)
{
    assert(a->command_id == b->command_id && a->observed_command_count == b->observed_command_count);
    assert(a->match_id.value == b->match_id.value && a->expected_period == b->expected_period && a->kind == b->kind);
    switch (a->kind) {
    case RMC_FLOW_INSTRUCTION:
        assert(a->data.instruction.team_id.value == b->data.instruction.team_id.value);
        assert(a->data.instruction.window == b->data.instruction.window && a->data.instruction.keep == b->data.instruction.keep);
        if (!a->data.instruction.keep) { assert(same_instructions(a->data.instruction.instructions, b->data.instruction.instructions)); }
        break;
    case RMC_FLOW_CLOSE_PERIOD: case RMC_FLOW_RESET_WINDOW: break;
    case RMC_FLOW_OPENING:
        assert(a->data.opening.opening_period == b->data.opening.opening_period);
        assert(a->data.opening.opening_team.value == b->data.opening.opening_team.value);
        assert(a->data.opening.taker.value == b->data.opening.taker.value);
        break;
    case RMC_FLOW_RESTART:
        assert(a->data.restart.restart_kind == b->data.restart.restart_kind);
        assert(a->data.restart.awarded_team.value == b->data.restart.awarded_team.value);
        assert(a->data.restart.taker.value == b->data.restart.taker.value);
        assert(a->data.restart.source_shot_event_id == b->data.restart.source_shot_event_id);
        break;
    case RMC_FLOW_RECOVERY:
        assert(a->data.recovery.incumbent.value == b->data.recovery.incumbent.value);
        assert(a->data.recovery.challenger.value == b->data.recovery.challenger.value);
        assert(a->data.recovery.context == b->data.recovery.context);
        assert(a->data.recovery.duration_seconds == b->data.recovery.duration_seconds);
        break;
    case RMC_FLOW_PASS:
        assert(a->data.pass.team_id.value == b->data.pass.team_id.value);
        assert(a->data.pass.passer.value == b->data.pass.passer.value);
        assert(a->data.pass.recipient.value == b->data.pass.recipient.value);
        assert(a->data.pass.opponent.value == b->data.pass.opponent.value);
        assert(a->data.pass.context == b->data.pass.context);
        assert(a->data.pass.duration_seconds == b->data.pass.duration_seconds);
        break;
    case RMC_FLOW_CARRY:
        assert(a->data.carry.team_id.value == b->data.carry.team_id.value);
        assert(a->data.carry.carrier.value == b->data.carry.carrier.value);
        assert(a->data.carry.opponent.value == b->data.carry.opponent.value);
        assert(a->data.carry.context == b->data.carry.context);
        assert(a->data.carry.duration_seconds == b->data.carry.duration_seconds);
        break;
    case RMC_FLOW_DRIBBLE:
        assert(a->data.dribble.team_id.value == b->data.dribble.team_id.value);
        assert(a->data.dribble.dribbler.value == b->data.dribble.dribbler.value);
        assert(a->data.dribble.opponent.value == b->data.dribble.opponent.value);
        assert(a->data.dribble.context == b->data.dribble.context);
        assert(a->data.dribble.duration_seconds == b->data.dribble.duration_seconds);
        break;
    case RMC_FLOW_TACKLE:
        assert(a->data.tackle.team_id.value == b->data.tackle.team_id.value);
        assert(a->data.tackle.carrier.value == b->data.tackle.carrier.value);
        assert(a->data.tackle.defender.value == b->data.tackle.defender.value);
        assert(a->data.tackle.context == b->data.tackle.context);
        assert(a->data.tackle.duration_seconds == b->data.tackle.duration_seconds);
        break;
    case RMC_FLOW_CROSS:
        assert(a->data.cross.team_id.value == b->data.cross.team_id.value);
        assert(a->data.cross.crosser.value == b->data.cross.crosser.value);
        assert(a->data.cross.recipient.value == b->data.cross.recipient.value);
        assert(a->data.cross.defender.value == b->data.cross.defender.value);
        assert(a->data.cross.context == b->data.cross.context);
        assert(a->data.cross.duration_seconds == b->data.cross.duration_seconds);
        break;
    case RMC_FLOW_SHOT:
        assert(a->data.shot.team_id.value == b->data.shot.team_id.value);
        assert(a->data.shot.shooter.value == b->data.shot.shooter.value);
        assert(a->data.shot.defender.value == b->data.shot.defender.value);
        assert(a->data.shot.goalkeeper.value == b->data.shot.goalkeeper.value);
        assert(a->data.shot.context == b->data.shot.context);
        assert(a->data.shot.duration_seconds == b->data.shot.duration_seconds);
        break;
    }
}

static RmcPlayerId outfield(RmcTeamId team, RmcPlayerId exclude)
{
    const RmcLineup *l = team.value == fixture->home_team.value ? &home : &away;
    RmcPlayerId best = {0};
    const RmcRoster *r = roster(team);
    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (l->slots[i] == RMC_SLOT_POR || l->players[i].value == exclude.value) { continue; }
        for (size_t j = 0; j < RMC_TEAM_PLAYERS; ++j) {
            if (r->players[j].id.value == l->players[i].value &&
                r->players[j].department != RMC_DEPARTMENT_GOALKEEPER &&
                (best.value == 0 || l->players[i].value < best.value)) { best = l->players[i]; }
        }
    }
    assert(best.value != 0);
    return best;
}

static RmcMatchFlowCommand choose(RmcMatchFlow *f, unsigned home_bits, unsigned away_bits, unsigned policy)
{
    RmcMatchFlowNext n, twice;
    const RmcMatchFlow before = *f;
    assert(rmc_match_flow_next(f, &n) == RMC_FLOW_OK);
    assert(rmc_match_flow_next(f, &twice) == RMC_FLOW_OK);
    assert(n.kind == twice.kind && n.observed_command_count == twice.observed_command_count);
    RmcMatchFlowCommand c, again;
    if (n.kind == RMC_FLOW_NEED_INSTRUCTION) {
        c = instruction(f, &n, n.team_id.value == fixture->home_team.value ? home_bits : away_bits);
    } else {
        assert(rmc_match_flow_select(f, &n, &c) == RMC_FLOW_OK);
        assert(rmc_match_flow_select(f, &n, &again) == RMC_FLOW_OK);
        same_command(&c, &again);
    }
    assert(memcmp(&before, f, sizeof(before)) == 0); /* purity, not replay equality */
    const RmcMatchPossession *p = &f->state.possession;
    if (n.kind == RMC_FLOW_NEED_SPORTING_ACTION) {
        const RmcTeamId other = p->team_id.value == fixture->home_team.value ? fixture->away_team : fixture->home_team;
        const bool keeper = p->controller.value == (p->team_id.value == fixture->home_team.value ? home.players[0].value : away.players[0].value);
        const RmcFlowInstructions defending = other.value == fixture->home_team.value ? f->home_instructions : f->away_instructions;
        const RmcFlowInstructions own_instruction = p->team_id.value == fixture->home_team.value ? f->home_instructions : f->away_instructions;
        const uint64_t q = f->sporting_count % 5;
        RmcFlowCommandKind expected;
        if (keeper) { expected = RMC_FLOW_PASS; }
        else if (q == 2 || (q == 1 && defending.pressure == RMC_FLOW_PRESS_HIGH)) { expected = RMC_FLOW_TACKLE; }
        else if (p->zone.sector == 4 || (q == 4 && p->zone.sector >= 3 && own_instruction.approach == RMC_FLOW_DIRECT)) { expected = RMC_FLOW_SHOT; }
        else if (q == 0) { expected = RMC_FLOW_PASS; }
        else { expected = own_instruction.progression == RMC_FLOW_CARRY_FIRST ? RMC_FLOW_CARRY : RMC_FLOW_DRIBBLE; }
        assert(c.kind == expected && c.kind != RMC_FLOW_CROSS);
        uint32_t duration = 0, nominal = 0;
        int32_t context = 99;
        switch (c.kind) {
        case RMC_FLOW_PASS:
            duration = c.data.pass.duration_seconds; nominal = 8; context = c.data.pass.context;
            assert(c.data.pass.passer.value == p->controller.value && c.data.pass.team_id.value == p->team_id.value);
            assert(c.data.pass.recipient.value == outfield(p->team_id, p->controller).value);
            assert(c.data.pass.opponent.value == outfield(other, (RmcPlayerId){0}).value); break;
        case RMC_FLOW_TACKLE:
            duration = c.data.tackle.duration_seconds; nominal = 5; context = c.data.tackle.context;
            assert(c.data.tackle.team_id.value == other.value && c.data.tackle.carrier.value == p->controller.value);
            assert(c.data.tackle.defender.value == outfield(other, (RmcPlayerId){0}).value); break;
        case RMC_FLOW_SHOT:
            duration = c.data.shot.duration_seconds; nominal = 6; context = c.data.shot.context;
            assert(c.data.shot.team_id.value == p->team_id.value && c.data.shot.shooter.value == p->controller.value);
            assert(c.data.shot.defender.value == outfield(other, (RmcPlayerId){0}).value);
            assert(c.data.shot.goalkeeper.value == (other.value == fixture->home_team.value ? home.players[0].value : away.players[0].value)); break;
        case RMC_FLOW_CARRY:
            duration = c.data.carry.duration_seconds; nominal = 10; context = c.data.carry.context;
            assert(c.data.carry.team_id.value == p->team_id.value && c.data.carry.carrier.value == p->controller.value);
            assert(c.data.carry.opponent.value == outfield(other, (RmcPlayerId){0}).value); break;
        case RMC_FLOW_DRIBBLE:
            duration = c.data.dribble.duration_seconds; nominal = 7; context = c.data.dribble.context;
            assert(c.data.dribble.team_id.value == p->team_id.value && c.data.dribble.dribbler.value == p->controller.value);
            assert(c.data.dribble.opponent.value == outfield(other, (RmcPlayerId){0}).value); break;
        default: assert(false);
        }
        assert(context == 0 && duration == (nominal < n.remaining_seconds ? nominal : n.remaining_seconds) && duration > 0);
        /* Declared engineering policy 1: legal high-context passes only.
         * Outcomes remain genuine; keeps long possession windows observable.
         * Policy 2: shoot from eligible control using all remaining time,
         * otherwise use canonical progression. This tests integrated whistles.
         * Neither policy is the canonical managerial selector. */
        if (policy == 1 || policy == 3) {
            c.kind = RMC_FLOW_PASS;
            c.data.pass = (RmcPassAction){p->team_id, p->controller, outfield(p->team_id, p->controller),
                outfield(other, (RmcPlayerId){0}), 20, n.remaining_seconds < 8 ? n.remaining_seconds : 8};
            /* Engineering policy 3: when 31 actions have actually accumulated,
             * finish with a real pass. Deadline must beat reset/recovery. */
            if (policy == 3 && p->action_count == 31) {
                c.data.pass.context = -20;
                c.data.pass.duration_seconds = n.remaining_seconds;
            }
        } else if (policy == 2 && !keeper && p->zone.sector >= 2) {
            c.kind = RMC_FLOW_SHOT;
            c.data.shot = (RmcShotAction){p->team_id, p->controller, outfield(other, (RmcPlayerId){0}),
                other.value == fixture->home_team.value ? home.players[0] : away.players[0], 0, n.remaining_seconds};
        }
    }
    return c;
}

static RmcMatchFlowStepResult apply(FlowRun *run, const RmcMatchFlowCommand *c)
{
    const RmcMatchFlow *f = &run->flow;
    const uint32_t second = f->state.elapsed_seconds;
    const RmcPcg32 rng = f->match_rng;
    const uint64_t sporting = f->sporting_count;
    const size_t count = f->journal.count;
    RmcMatchFlowStepResult r;
    const RmcFlowError err = rmc_match_flow_apply(&run->flow, c, &r);
    if (err != RMC_FLOW_OK) {
        fprintf(stderr, "apply error=%u kind=%u period=%u second=%u command=%zu\n",
            (unsigned)err, (unsigned)c->kind, (unsigned)f->state.period, second, count);
    }
    assert(err == RMC_FLOW_OK);
    assert(f->journal.count == count + 1 && r.accepted_command_id == count + 1);
    same_command(c, &run->commands[count]);
    if (c->kind >= RMC_FLOW_RECOVERY) {
        assert(f->state.elapsed_seconds > second && f->sporting_count == sporting + 1);
    } else {
        assert(f->state.elapsed_seconds == second && f->sporting_count == sporting);
        assert(rng.state == f->match_rng.state && rng.increment == f->match_rng.increment);
    }
    if (r.entered_half_time) {
        assert(f->state.period == RMC_MATCH_HALF_TIME && f->state.elapsed_seconds == 2700);
        assert(!f->home_window_submitted && !f->away_window_submitted);
    }
    return r;
}

static void open_first(FlowRun *run, unsigned h, unsigned a)
{
    for (unsigned i = 0; i < 3; ++i) {
        RmcMatchFlowCommand c = choose(&run->flow, h, a, 0);
        assert(c.kind == (i < 2 ? RMC_FLOW_INSTRUCTION : RMC_FLOW_OPENING));
        apply(run, &c);
    }
    assert(run->flow.state.period == RMC_MATCH_FIRST_HALF && run->flow.state.mode == RMC_MATCH_BALL_IN_PLAY);
    assert(run->flow.log.count == 2 && run->events[0].type == RMC_EVENT_PERIOD_START && run->events[1].type == RMC_EVENT_RESTART);
    assert(run->flow.state.possession.action_count == 0 && run->flow.state.elapsed_seconds == 0);
}

static void finish(FlowRun *run, unsigned h, unsigned a, unsigned policy)
{
    for (uint64_t guard = 0; guard < RMC_FLOW_DEFAULT_OPERATION_LIMIT; ++guard) {
        RmcMatchFlowStatus s;
        assert(rmc_match_flow_status(&run->flow, &s) == RMC_FLOW_OK);
        if (s.complete) { return; }
        const RmcMatchFlowCommand c = choose(&run->flow, h, a, policy);
        apply(run, &c);
    }
    assert(false); /* finite failure, never retry a rejected command */
}

typedef struct { unsigned goals, wides, recoveries, pass_loose, tackle_loose, shot_loose, starts, ends, match_ends;
    unsigned goal_followed_play, wide_followed_play; } Evidence;

static Evidence evidence(const FlowRun *run)
{
    Evidence x = {0};
    const RmcMatchFlow *f = &run->flow;
    unsigned hg = 0, ag = 0;
    for (size_t i = 0; i < f->log.count; ++i) {
        const RmcMatchEvent *e = &run->events[i];
        assert(e->id == i + 1 && e->match_id.value == fixture->id.value);
        assert(i == 0 || e->at_second >= run->events[i - 1].at_second);
        assert(e->cause_event_id <= i);
        if (e->type == RMC_EVENT_PERIOD_START) {
            ++x.starts;
            assert(e->team_id.value == (x.starts == 1 ? f->config.first_opening_team.value :
                f->config.first_opening_team.value == fixture->home_team.value ? fixture->away_team.value : fixture->home_team.value));
            assert(e->at_second == (x.starts == 1 ? 0u : 2700u));
        }
        if (e->type == RMC_EVENT_PERIOD_END) { ++x.ends; assert(e->at_second == (x.ends == 1 ? 2700u : 5400u)); }
        if (e->type == RMC_EVENT_MATCH_END) { ++x.match_ends; assert(e->cause_event_id == i); }
        if (e->type == RMC_EVENT_GOAL) {
            ++x.goals;
            assert(i > 0 && run->events[i - 1].type == RMC_EVENT_SHOT && e->cause_event_id == i);
            if (e->team_id.value == fixture->home_team.value) { ++hg; }
            else { assert(e->team_id.value == fixture->away_team.value); ++ag; }
        }
        if (e->type == RMC_EVENT_SHOT && e->outcome_code == RMC_SHOT_WIDE) { ++x.wides; }
        if (e->type == RMC_EVENT_RECOVERY) {
            ++x.recoveries;
            assert(i > 0 && e->cause_event_id == i);
            const RmcMatchEvent *cause = &run->events[i - 1];
            if (cause->type == RMC_EVENT_SHOT) { ++x.shot_loose; }
            else if (cause->outcome_code == RMC_PASS_OUTCOME_LOOSE_BALL) { ++x.pass_loose; }
            else { assert(cause->outcome_code == RMC_TACKLE_OUTCOME_LOOSE_BALL); ++x.tackle_loose; }
        }
        if (e->type == RMC_EVENT_RESTART && e->cause_event_id != 0) {
            const RmcMatchEvent *cause = &run->events[e->cause_event_id - 1];
            if (cause->type == RMC_EVENT_SHOT) {
                assert(e->team_id.value == cause->shot.restart_awarded_team.value);
                assert(e->outcome_code == (uint32_t)cause->shot.restart_kind);
                const RmcLineup *l = e->team_id.value == fixture->home_team.value ? &home : &away;
                if (e->outcome_code == RMC_RESTART_GOAL_KICK) { assert(e->primary_actor.value == l->players[0].value); }
                else { assert(e->primary_actor.value == outfield(e->team_id, (RmcPlayerId){0}).value); }
                if (i + 1 < f->log.count && run->events[i + 1].at_second > e->at_second) {
                    if (cause->outcome_code == RMC_SHOT_GOAL) { ++x.goal_followed_play; }
                    if (cause->outcome_code == RMC_SHOT_WIDE) { ++x.wide_followed_play; }
                }
            }
        }
    }
    assert(x.starts == 2 && x.ends == 2 && x.match_ends == 1);
    assert(hg == f->state.home_goals && ag == f->state.away_goals);
    assert(f->state.period == RMC_MATCH_FINISHED && f->state.mode == RMC_MATCH_PERIOD_ENDED &&
        f->state.terminal_reason == RMC_MATCH_TERMINAL_COMPLETED && f->state.elapsed_seconds == 5400);
    return x;
}

/* Byte comparisons are rejection checks ONLY: every backing byte and output
 * must survive, including unused sentinel slots and inactive union bytes. */
static void reject(FlowRun *run, const RmcMatchFlowCommand *c, RmcFlowError expected)
{
    const RmcMatchFlow before = run->flow;
    const size_t eb = RMC_FLOW_DEFAULT_EVENT_CAPACITY * sizeof(*run->events);
    const size_t cb = RMC_FLOW_DEFAULT_COMMAND_CAPACITY * sizeof(*run->commands);
    void *events = malloc(eb), *commands = malloc(cb);
    assert(events != NULL && commands != NULL);
    memcpy(events, run->events, eb); memcpy(commands, run->commands, cb);
    RmcMatchFlowStepResult result;
    memset(&result, 0x3c, sizeof(result));
    unsigned char output[sizeof(result)]; memcpy(output, &result, sizeof(result));
#ifdef RMC_WRAP_MALLOC
    if (fail_on_apply) { fail_allocation = true; }
#endif
    assert(rmc_match_flow_apply(&run->flow, c, &result) == expected);
#ifdef RMC_WRAP_MALLOC
    if (fail_on_apply) { assert(!fail_allocation); fail_on_apply = false; }
#endif
    assert(memcmp(&before, &run->flow, sizeof(before)) == 0);
    assert(memcmp(events, run->events, eb) == 0 && memcmp(commands, run->commands, cb) == 0);
    assert(memcmp(output, &result, sizeof(result)) == 0);
    free(events); free(commands);
}

static void test_atomicity(void)
{
    FlowRun run;
    init_run(&run, true, 0);
    RmcMatchFlowCommand c = choose(&run.flow, 8, 8, 0);
    RmcMatchFlowCommand bad = c;
    bad.data.instruction.keep = false;
    bad.data.instruction.instructions.pressure = (RmcFlowPressure)2;
    reject(&run, &bad, RMC_FLOW_INVALID_ARGUMENT);
    bad = c; bad.data.instruction.team_id = fixture->away_team;
    reject(&run, &bad, RMC_FLOW_INVALID_ARGUMENT);
    apply(&run, &c);
    reject(&run, &c, RMC_FLOW_STALE_COMMAND);
    c = choose(&run.flow, 8, 8, 0); apply(&run, &c);
    c = choose(&run.flow, 8, 8, 0);
    run.flow.log.capacity = 1;
    reject(&run, &c, RMC_FLOW_RESOURCE_EXHAUSTED);
    assert(run.flow.state.period == RMC_MATCH_NOT_STARTED); /* staging stayed private */
    run.flow.log.capacity = RMC_FLOW_DEFAULT_EVENT_CAPACITY;
    bad = c; bad.data.opening.taker = home.players[0];
    reject(&run, &bad, RMC_FLOW_OPERATION_REJECTED);
    apply(&run, &c);
    c = choose(&run.flow, 8, 8, 0);
    bad = c; bad.match_id.value = 0; reject(&run, &bad, RMC_FLOW_WRONG_OPERATION);
    bad = c; bad.expected_period = RMC_MATCH_PERIOD_COUNT; reject(&run, &bad, RMC_FLOW_INVALID_ARGUMENT);
    bad = c; bad.command_id = 0; reject(&run, &bad, RMC_FLOW_STALE_COMMAND);
    bad = c; bad.command_id = UINT64_MAX; reject(&run, &bad, RMC_FLOW_STALE_COMMAND);
    bad = c; bad.observed_command_count = UINT64_MAX; reject(&run, &bad, RMC_FLOW_STALE_COMMAND);
    bad = c; bad.kind = RMC_FLOW_RESTART; reject(&run, &bad, RMC_FLOW_WRONG_OPERATION);
    bad = c; bad.data.pass.recipient = c.data.pass.passer; reject(&run, &bad, RMC_FLOW_OPERATION_REJECTED);
    bad = c; bad.data.pass.duration_seconds = 0; reject(&run, &bad, RMC_FLOW_INVALID_ARGUMENT);
    bad = c; bad.data.pass.duration_seconds = UINT32_MAX; reject(&run, &bad, RMC_FLOW_INVALID_ARGUMENT);
    run.flow.journal.capacity = run.flow.journal.count;
    reject(&run, &c, RMC_FLOW_RESOURCE_EXHAUSTED);
    run.flow.journal.capacity = RMC_FLOW_DEFAULT_COMMAND_CAPACITY;
    run.flow.log.capacity = run.flow.log.count;
    reject(&run, &c, RMC_FLOW_RESOURCE_EXHAUSTED);
    run.flow.log.capacity = SIZE_MAX / sizeof(*run.events) + 1;
    reject(&run, &c, RMC_FLOW_INVALID_ARGUMENT);
    run.flow.log.capacity = RMC_FLOW_DEFAULT_EVENT_CAPACITY;
    run.flow.journal.capacity = SIZE_MAX / sizeof(*run.commands) + 1;
    reject(&run, &c, RMC_FLOW_INVALID_ARGUMENT);
    run.flow.journal.capacity = RMC_FLOW_DEFAULT_COMMAND_CAPACITY;
    /* Malformed metadata tests only. No mutated state is used for success. */
    const RmcMatchFlow valid = run.flow;
    run.flow.log.count = SIZE_MAX;
    reject(&run, &c, RMC_FLOW_INVALID_ARGUMENT);
    run.flow = valid;
    run.flow.journal.count = SIZE_MAX;
    reject(&run, &c, RMC_FLOW_INVALID_ARGUMENT);
    run.flow = valid;
    run.flow.sporting_count = UINT64_MAX;
    reject(&run, &c, RMC_FLOW_INVALID_STATE);
    run.flow = valid;
    run.flow.state.mode = RMC_MATCH_RESTART_PENDING;
    reject(&run, &c, RMC_FLOW_INVALID_STATE);
    RmcMatchFlowNext n, ns;
    RmcMatchFlowStatus status, ss;
    RmcMatchFlowCommand selection, cs;
    memset(&n, 0x27, sizeof(n)); memcpy(&ns, &n, sizeof(n));
    memset(&status, 0x27, sizeof(status)); memcpy(&ss, &status, sizeof(status));
    memset(&selection, 0x27, sizeof(selection)); memcpy(&cs, &selection, sizeof(selection));
    assert(rmc_match_flow_next(&run.flow, &n) == RMC_FLOW_INVALID_STATE);
    assert(rmc_match_flow_status(&run.flow, &status) == RMC_FLOW_INVALID_STATE);
    assert(rmc_match_flow_select(&run.flow, &n, &selection) == RMC_FLOW_INVALID_STATE);
    assert(memcmp(&n, &ns, sizeof(n)) == 0 && memcmp(&status, &ss, sizeof(status)) == 0);
    assert(memcmp(&selection, &cs, sizeof(selection)) == 0);
    run.flow = valid;
    assert(rmc_match_flow_next(&run.flow, &n) == RMC_FLOW_OK);
    --n.observed_command_count;
    assert(rmc_match_flow_select(&run.flow, &n, &selection) == RMC_FLOW_STALE_COMMAND);
    assert(memcmp(&selection, &cs, sizeof(selection)) == 0);
    destroy(&run);

    init_run(&run, false, 3);
    open_first(&run, 8, 8);
    c = choose(&run.flow, 8, 8, 0);
    reject(&run, &c, RMC_FLOW_RESOURCE_EXHAUSTED);
    reject(&run, &c, RMC_FLOW_RESOURCE_EXHAUSTED); /* deterministic, no automatic retry */
    destroy(&run);
}

static void test_init_rejections(void)
{
    FlowRun run; init_run(&run, true, 0);
    const RmcMatchFlow before = run.flow;
    RmcMatchFlowConfig config = {1, 1, fixture->home_team, 0};
    for (unsigned i = 0; i < 6; ++i) {
        config.flow_version = i == 0 ? 2 : 1;
        config.selector_version = i == 1 ? 2 : 1;
        config.first_opening_team.value = i == 2 ? 0 : fixture->home_team.value;
        const size_t ec = i == 3 ? SIZE_MAX / sizeof(*run.events) + 1 : RMC_FLOW_DEFAULT_EVENT_CAPACITY;
        const size_t cc = i == 4 ? SIZE_MAX / sizeof(*run.commands) + 1 : RMC_FLOW_DEFAULT_COMMAND_CAPACITY;
        assert(rmc_match_flow_init(&career.foundation.world, fixture, &home, &away, &config,
            run.events, ec, run.commands, i == 5 ? 0 : cc, &run.flow) != RMC_FLOW_OK);
        assert(memcmp(&before, &run.flow, sizeof(before)) == 0);
    }
    destroy(&run);
}

static void test_resource_branches(void)
{
    for (unsigned policy = 0; policy < 4; ++policy) {
        FlowRun run; init_run(&run, true, 0);
        unsigned reservations[6] = {0};
        bool reset_tested = false;
        bool allocated[12] = {false};
        bool branch_checked[12][2] = {{false}};
        unsigned loose_reset = 0, loose_closure = 0, standalone = 0, suppressed = 0, deadline_cap = 0;
        for (uint64_t guard = 0; guard < RMC_FLOW_DEFAULT_OPERATION_LIMIT; ++guard) {
            RmcMatchFlowNext n;
            assert(rmc_match_flow_next(&run.flow, &n) == RMC_FLOW_OK);
            if (n.kind == RMC_FLOW_COMPLETE) { break; }
            RmcMatchFlowCommand c = choose(&run.flow, 8, 8, policy);
            if (c.kind == RMC_FLOW_RESET_WINDOW && run.flow.state.possession.controller.value == 0) { ++loose_reset; }
            if (c.kind == RMC_FLOW_CLOSE_PERIOD) {
                ++standalone;
                if (run.flow.state.possession.action_count == 32) { ++deadline_cap; }
                if (run.flow.state.possession.controller.value == 0) { ++loose_closure; }
            }
#ifdef RMC_WRAP_MALLOC
            if (!allocated[c.kind] && (c.kind == RMC_FLOW_SHOT || c.kind == RMC_FLOW_RECOVERY ||
                c.kind == RMC_FLOW_RESTART || (c.kind == RMC_FLOW_OPENING && run.flow.state.period == RMC_MATCH_HALF_TIME))) {
                fail_on_apply = true;
                reject(&run, &c, RMC_FLOW_OPERATION_REJECTED);
                allocated[c.kind] = true;
            }
#else
            (void)allocated;
#endif
            if (c.kind == RMC_FLOW_SHOT) {
                size_t maximum = 3;
                if (c.data.shot.duration_seconds == n.remaining_seconds) {
                    maximum += run.flow.state.period == RMC_MATCH_FIRST_HALF ? 1 : 2;
                }
                if (!reservations[maximum]++) {
                    const size_t capacity = run.flow.log.capacity;
                    run.flow.log.capacity = run.flow.log.count + maximum - 1;
                    reject(&run, &c, RMC_FLOW_RESOURCE_EXHAUSTED);
                    run.flow.log.capacity = capacity;
                }
            }
            if (c.kind == RMC_FLOW_RESET_WINDOW && !reset_tested) {
                const size_t capacity = run.flow.journal.capacity;
                run.flow.journal.capacity = run.flow.journal.count;
                reject(&run, &c, RMC_FLOW_RESOURCE_EXHAUSTED);
                run.flow.journal.capacity = capacity;
                reset_tested = true;
            }
            if (c.kind == RMC_FLOW_SHOT && c.data.shot.duration_seconds == n.remaining_seconds) { ++suppressed; }
            const unsigned half = run.flow.state.period >= RMC_MATCH_HALF_TIME ? 1 : 0;
            if (!branch_checked[c.kind][half]) {
                branch_checked[c.kind][half] = true;
                size_t maximum = 0;
                if (c.kind == RMC_FLOW_OPENING) { maximum = 2; }
                else if (c.kind == RMC_FLOW_CLOSE_PERIOD) { maximum = half ? 2 : 1; }
                else if (c.kind == RMC_FLOW_RESTART || c.kind >= RMC_FLOW_RECOVERY) { maximum = 1; }
                if (c.kind == RMC_FLOW_SHOT) {
                    maximum = 3 + (c.data.shot.duration_seconds == n.remaining_seconds ? (half ? 2 : 1) : 0);
                }
                const size_t event_capacity = run.flow.log.capacity;
                const size_t journal_capacity = run.flow.journal.capacity;
                run.flow.journal.capacity = run.flow.journal.count;
                /* First instruction has count zero, an invalid zero capacity;
                 * all later branches exercise valid full journal metadata. */
                if (run.flow.journal.count != 0) { reject(&run, &c, RMC_FLOW_RESOURCE_EXHAUSTED); }
                run.flow.journal.capacity = journal_capacity;
                if (maximum > 0) {
                    run.flow.log.capacity = run.flow.log.count + maximum - 1;
                    reject(&run, &c, RMC_FLOW_RESOURCE_EXHAUSTED);
                }
                /* Exact maximum fit, including event-free operations with a
                 * full log, must still accept. Restore only resource capacity. */
                if (run.flow.log.count + maximum > 0) {
                    run.flow.log.capacity = run.flow.log.count + maximum;
                } else { run.flow.log.capacity = event_capacity; }
                const RmcMatchFlowStepResult step = apply(&run, &c);
                assert(step.appended_event_count <= maximum);
                run.flow.log.capacity = event_capacity;
            } else { apply(&run, &c); }
        }
        evidence(&run);
        if (policy == 0) { assert(reservations[3] > 0); }
        if (policy == 1) { assert(reset_tested && run.flow.reset_count > 0 && standalone == 2 && loose_reset > 0); }
        if (policy == 3) { assert(deadline_cap == 2 && standalone == 2 && loose_closure > 0); }
        if (policy == 2) { assert(reservations[4] == 1 && reservations[5] == 1 && standalone == 0 && suppressed == 2); }
        printf("boundary evidence policy=%u loose-reset=%u loose-closure=%u standalone=%u integrated=%u deadline-cap=%u\n",
            policy, loose_reset, loose_closure, standalone, suppressed, deadline_cap);
        printf("policy=%u seed=20261114 fixture=%llu commands=%zu events=%zu sporting=%llu resets=%llu score=%u:%u\n",
            policy, (unsigned long long)fixture->id.value, run.flow.journal.count, run.flow.log.count,
            (unsigned long long)run.flow.sporting_count, (unsigned long long)run.flow.reset_count,
            run.flow.state.home_goals, run.flow.state.away_goals);
        destroy(&run);
    }
}

int rmc_flow_acceptance(void)
{
    setup(UINT64_C(20261114));
    test_init_rejections();
    test_atomicity();
    test_resource_branches();
    Evidence total = {0};
    /* All 8x8 pairs of validated instruction triples, both windows; opener
     * alternates fixture side. KEEP is independently covered above. */
    for (unsigned h = 0; h < 8; ++h) {
        for (unsigned a = 0; a < 8; ++a) {
            FlowRun run; init_run(&run, (h + a) % 2 == 0, 0);
            finish(&run, h, a, 0);
            Evidence x = evidence(&run);
            total.goal_followed_play += x.goal_followed_play;
            total.wide_followed_play += x.wide_followed_play;
            total.pass_loose += x.pass_loose; total.tackle_loose += x.tackle_loose; total.shot_loose += x.shot_loose;
            RmcMatchFlowNext n;
            assert(rmc_match_flow_next(&run.flow, &n) == RMC_FLOW_OK && n.kind == RMC_FLOW_COMPLETE);
            RmcMatchFlowCommand invalid = {.command_id = run.flow.journal.count + 1,
                .observed_command_count = run.flow.journal.count, .match_id = fixture->id,
                .expected_period = RMC_MATCH_FINISHED, .kind = RMC_FLOW_CLOSE_PERIOD};
            if (h == 0 && a == 0) { reject(&run, &invalid, RMC_FLOW_WRONG_OPERATION); }
            destroy(&run);
        }
    }
    assert(total.goal_followed_play > 0 && total.wide_followed_play > 0);
    assert(total.pass_loose > 0 && total.tackle_loose > 0 && total.shot_loose > 0);
    printf("canonical 64 instruction pairs: GOAL->KICKOFF->play=%u WIDE->GOAL_KICK->play=%u recoveries pass/tackle/shot=%u/%u/%u\n",
        total.goal_followed_play, total.wide_followed_play, total.pass_loose, total.tackle_loose, total.shot_loose);
    return 0;
}

#ifndef RMC_FLOW_REPLAY_TEST
int main(void) { return rmc_flow_acceptance(); }
#endif
