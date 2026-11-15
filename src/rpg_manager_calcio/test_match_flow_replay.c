#define RMC_FLOW_REPLAY_TEST
#include "test_match_flow.c"

/* Semantic comparisons deliberately enumerate fields. Rejection-only byte
 * comparisons in the common harness are never used as replay equality. */
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
        assert(x[side]->formation == y[side]->formation && x[side]->secondary_assignments == y[side]->secondary_assignments);
        for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
            assert(x[side]->players[i].value == y[side]->players[i].value && x[side]->slots[i] == y[side]->slots[i]);
        }
    }
    assert(a->period == b->period && a->mode == b->mode && a->terminal_reason == b->terminal_reason);
    assert(a->elapsed_seconds == b->elapsed_seconds && a->second_half_start_second == b->second_half_start_second);
    assert(a->first_half_stoppage_seconds == b->first_half_stoppage_seconds && a->second_half_stoppage_seconds == b->second_half_stoppage_seconds);
    assert(a->home_goals == b->home_goals && a->away_goals == b->away_goals);
    const RmcMatchPossession *p = &a->possession, *q = &b->possession;
    assert(p->team_id.value == q->team_id.value && p->initiator.value == q->initiator.value && p->controller.value == q->controller.value);
    assert(p->zone.sector == q->zone.sector && p->zone.lane == q->zone.lane && p->phase == q->phase);
    assert(p->action_count == q->action_count && p->elapsed_seconds == q->elapsed_seconds);
}

static void same_flow(const FlowRun *a, const FlowRun *b)
{
    const RmcMatchFlow *f = &a->flow, *g = &b->flow;
    same_state(&f->state, &g->state);
    assert(f->match_rng.state == g->match_rng.state && f->match_rng.increment == g->match_rng.increment);
    assert(f->config.flow_version == g->config.flow_version && f->config.selector_version == g->config.selector_version);
    assert(f->config.first_opening_team.value == g->config.first_opening_team.value && f->config.operation_limit == g->config.operation_limit);
    assert(f->sporting_count == g->sporting_count && f->reset_count == g->reset_count);
    assert(f->home_window_submitted == g->home_window_submitted && f->away_window_submitted == g->away_window_submitted);
    assert(same_instructions(f->home_instructions, g->home_instructions) && same_instructions(f->away_instructions, g->away_instructions));
    assert(f->journal.count == g->journal.count && f->journal.capacity == g->journal.capacity);
    assert(f->log.count == g->log.count && f->log.capacity == g->log.capacity && f->log.match_id.value == g->log.match_id.value);
    for (size_t i = 0; i < f->log.count; ++i) { same_event(&a->events[i], &b->events[i]); }
    for (size_t i = 0; i < f->journal.count; ++i) { same_command(&a->commands[i], &b->commands[i]); }
    RmcMatchFlowStatus x, y;
    assert(rmc_match_flow_status(f, &x) == RMC_FLOW_OK && rmc_match_flow_status(g, &y) == RMC_FLOW_OK);
    assert(x.period == y.period && x.mode == y.mode && x.terminal_reason == y.terminal_reason);
    assert(x.at_second == y.at_second && x.home_goals == y.home_goals && x.away_goals == y.away_goals);
    assert(x.accepted_commands == y.accepted_commands && x.sporting_operations == y.sporting_operations && x.event_count == y.event_count);
    assert(x.complete == y.complete && x.needs_instruction == y.needs_instruction);
}

static void replay_match_version(unsigned policy, bool home_opens, uint32_t version)
{
    FlowRun live, incremental, batch;
    init_run_version(&live, home_opens, 0, version);
    init_run_version(&incremental, home_opens, 0, version);
    init_run_version(&batch, home_opens, 0, version);
    bool half_seen = false;
    size_t half_count = 0;
    for (uint64_t guard = 0; guard < RMC_FLOW_DEFAULT_OPERATION_LIMIT; ++guard) {
        RmcMatchFlowNext n;
        assert(rmc_match_flow_next(&live.flow, &n) == RMC_FLOW_OK);
        if (n.kind == RMC_FLOW_COMPLETE) { break; }
        /* Asymmetric pre-match instructions, changed home triple and away KEEP
         * at half-time. Replay must retain decisions, not resample selector. */
        const bool second = live.flow.state.period >= RMC_MATCH_HALF_TIME;
        const RmcMatchFlowCommand c = choose(&live.flow, second ? 7 : 0, second ? 8 : 4, policy);
        const RmcMatchFlowStepResult step = apply(&live, &c);
        /* Source record is from a different executor's actual accepted journal. */
        assert(rmc_match_flow_replay(&incremental.flow, &live.commands[live.flow.journal.count - 1], 1) == RMC_FLOW_OK);
        same_flow(&live, &incremental);
        if (step.status.complete == false) {
            const bool after_half = live.flow.state.period >= RMC_MATCH_HALF_TIME;
            RmcMatchFlowCommand x = choose(&live.flow, after_half ? 7 : 0, after_half ? 8 : 4, policy);
            RmcMatchFlowCommand y = choose(&incremental.flow, after_half ? 7 : 0, after_half ? 8 : 4, policy);
            same_command(&x, &y); /* Destination journal reconstructs history. */
        }
        if (step.entered_half_time) {
            assert(!half_seen);
            half_seen = true;
            half_count = live.flow.journal.count;
            assert(rmc_match_flow_replay(&batch.flow, live.commands, half_count) == RMC_FLOW_OK);
            same_flow(&live, &batch); /* exact half-time score, RNG, windows */
            RmcMatchFlowCommand stale = live.commands[half_count - 1];
            reject(&batch, &stale, RMC_FLOW_STALE_COMMAND);
        }
    }
    assert(half_seen);
    assert(rmc_match_flow_replay(&batch.flow, &live.commands[half_count], live.flow.journal.count - half_count) == RMC_FLOW_OK);
    same_flow(&live, &incremental);
    same_flow(&live, &batch);
    Evidence x = evidence(&live);
    assert(live.flow.home_instructions.approach == RMC_FLOW_DIRECT && live.flow.away_instructions.pressure == RMC_FLOW_PRESS_HIGH);
    printf("semantic replay selector=%u policy=%u opener=%s commands=%zu events=%zu score=%u:%u goals=%u wides=%u RNG=%016llx/%016llx\n",
        version, policy, home_opens ? "home" : "away", live.flow.journal.count, live.flow.log.count,
        live.flow.state.home_goals, live.flow.state.away_goals, x.goals, x.wides,
        (unsigned long long)live.flow.match_rng.state, (unsigned long long)live.flow.match_rng.increment);
    destroy(&live); destroy(&incremental); destroy(&batch);
}

static void replay_match(unsigned policy, bool home_opens)
{
    replay_match_version(policy, home_opens, RMC_MATCH_SELECTOR_V1_VERSION);
}

static void test_v2_prefix_continuation(bool goalkeeper_history)
{
    FlowRun live, continued;
    init_run_version(&live, true, 0, RMC_MATCH_SELECTOR_V2_VERSION);
    init_run_version(&continued, true, 0, RMC_MATCH_SELECTOR_V2_VERSION);
    bool pass_seen[2] = {false}, failed_seen = false, prefix_found = false;
    size_t prefix = 0;
    for (uint64_t guard = 0; guard < RMC_FLOW_DEFAULT_OPERATION_LIMIT; ++guard) {
        RmcMatchFlowNext n; assert(rmc_match_flow_next(&live.flow, &n) == RMC_FLOW_OK);
        if (n.kind == RMC_FLOW_COMPLETE) { break; }
        RmcMatchFlowCommand c = choose(&live.flow, 0, 0, 0);
        if (goalkeeper_history && c.kind == RMC_FLOW_PASS) {
            const RmcLineup *l = c.data.pass.team_id.value == fixture->home_team.value ? &home : &away;
            if (c.data.pass.passer.value != l->players[0].value) { c.data.pass.recipient = l->players[0]; }
        }
        const RmcMatchFlowStepResult step = apply(&live, &c);
        if (c.kind == RMC_FLOW_PASS) {
            const size_t side = c.data.pass.team_id.value == fixture->home_team.value ? 0 : 1;
            pass_seen[side] = true;
            const bool failed = live.events[step.first_event_index].outcome_code == RMC_PASS_OUTCOME_LOOSE_BALL;
            failed_seen |= failed;
            const RmcLineup *l = side == 0 ? &home : &away;
            const bool keeper_control = !failed && c.data.pass.recipient.value == l->players[0].value;
            if (pass_seen[0] && pass_seen[1] && failed_seen && (goalkeeper_history ? keeper_control : failed)) {
                prefix_found = true; prefix = live.flow.journal.count; break;
            }
        }
    }
    assert(prefix_found);
    assert(rmc_match_flow_replay(&continued.flow, live.commands, prefix) == RMC_FLOW_OK);
    same_flow(&live, &continued);
    for (uint64_t guard = 0; guard < RMC_FLOW_DEFAULT_OPERATION_LIMIT; ++guard) {
        RmcMatchFlowNext n; assert(rmc_match_flow_next(&live.flow, &n) == RMC_FLOW_OK);
        if (n.kind == RMC_FLOW_COMPLETE) { break; }
        RmcMatchFlowCommand a = choose(&live.flow, 0, 0, 0);
        RmcMatchFlowCommand b = choose(&continued.flow, 0, 0, 0);
        same_command(&a, &b);
        apply(&live, &a); apply(&continued, &b);
        same_flow(&live, &continued);
    }
    evidence(&live); evidence(&continued);
    printf("v2 prefix continuation goalkeeper-history=%u prefix=%zu final-commands=%zu RNG=%016llx/%016llx\n",
        goalkeeper_history, prefix, live.flow.journal.count,
        (unsigned long long)live.flow.match_rng.state, (unsigned long long)live.flow.match_rng.increment);
    destroy(&live); destroy(&continued);
}

static void test_replay_prefix_failure(uint32_t version)
{
    FlowRun source, destination;
    init_run_version(&source, true, 0, version); init_run_version(&destination, true, 0, version);
    open_first(&source, 8, 8);
    RmcMatchFlowCommand records[3];
    for (size_t i = 0; i < 3; ++i) { records[i] = source.commands[i]; }
    records[2].data.opening.taker.value = 0;
    assert(rmc_match_flow_replay(&destination.flow, records, 3) == RMC_FLOW_OPERATION_REJECTED);
    assert(destination.flow.journal.count == 2 && destination.flow.log.count == 0 && destination.flow.state.period == RMC_MATCH_NOT_STARTED);
    reject(&destination, &records[2], RMC_FLOW_OPERATION_REJECTED);
    assert(rmc_match_flow_replay(&destination.flow, &source.commands[2], 1) == RMC_FLOW_OK);
    same_flow(&source, &destination);
    destroy(&source); destroy(&destination);
}

int main(void)
{
    setup(UINT64_C(20261114));
    test_replay_prefix_failure(RMC_MATCH_SELECTOR_V1_VERSION);
    test_replay_prefix_failure(RMC_MATCH_SELECTOR_V2_VERSION);
    replay_match(0, true);
    replay_match(0, false);
    replay_match(1, true);
    replay_match(2, false);
    replay_match(3, true);
    replay_match_version(0, true, RMC_MATCH_SELECTOR_V2_VERSION);
    replay_match_version(0, false, RMC_MATCH_SELECTOR_V2_VERSION);
    replay_match_version(1, true, RMC_MATCH_SELECTOR_V2_VERSION);
    replay_match_version(3, true, RMC_MATCH_SELECTOR_V2_VERSION);
    test_v2_prefix_continuation(false);
    test_v2_prefix_continuation(true);
    puts("Full-match semantic replay covers every event/command, half-time/final state and RNG; no portable encoding/hash claim.");
    return 0;
}
