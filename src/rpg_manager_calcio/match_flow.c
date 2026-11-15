#include "match_flow.h"
#include "match_period_commit.h"
#include "match_stoppage.h"
#include "match_transitions.h"

static bool instructions_valid(RmcFlowInstructions x)
{
    return (unsigned)x.approach <= RMC_FLOW_DIRECT &&
        (unsigned)x.progression <= RMC_FLOW_DRIBBLE_FIRST &&
        (unsigned)x.pressure <= RMC_FLOW_PRESS_HIGH;
}

static bool cleared(const RmcMatchPossession *p)
{
    return p->team_id.value == 0 && p->initiator.value == 0 &&
        p->controller.value == 0 && p->action_count == 0 &&
        p->elapsed_seconds == 0 && p->zone.sector == 0 &&
        p->zone.lane == RMC_LANE_LEFT && p->phase == RMC_PHASE_BUILDUP;
}

static bool fixture_team(const RmcMatchState *s, RmcTeamId t)
{
    return t.value == s->fixture.home_team.value ||
        t.value == s->fixture.away_team.value;
}

static RmcFlowError metadata(const RmcMatchFlow *f)
{
    if (f == NULL || f->world == NULL || f->log.events == NULL ||
        f->journal.commands == NULL || f->log.capacity == 0 ||
        f->journal.capacity == 0 ||
        f->log.capacity > SIZE_MAX / sizeof(*f->log.events) ||
        f->journal.capacity > SIZE_MAX / sizeof(*f->journal.commands) ||
        f->log.count > f->log.capacity ||
        f->journal.count > f->journal.capacity ||
        f->log.count > UINT64_MAX || f->journal.count > UINT64_MAX ||
        !instructions_valid(f->home_instructions) ||
        !instructions_valid(f->away_instructions) ||
        f->config.operation_limit == 0 ||
        f->config.operation_limit > RMC_FLOW_DEFAULT_OPERATION_LIMIT) {
        return RMC_FLOW_INVALID_ARGUMENT;
    }
    if (f->config.flow_version != RMC_MATCH_FLOW_VERSION ||
        (f->config.selector_version != RMC_MATCH_SELECTOR_V1_VERSION &&
         f->config.selector_version != RMC_MATCH_SELECTOR_V2_VERSION)) {
        return RMC_FLOW_UNSUPPORTED_VERSION;
    }
    if (!rmc_match_state_valid(f->world, &f->state) ||
        !fixture_team(&f->state, f->config.first_opening_team) ||
        f->log.match_id.value != f->state.fixture.id.value ||
        (f->match_rng.increment & UINT64_C(1)) == 0 ||
        f->sporting_count > f->journal.count ||
        f->reset_count > f->journal.count ||
        f->journal.count > f->config.operation_limit ||
        (f->away_window_submitted && !f->home_window_submitted)) {
        return RMC_FLOW_INVALID_STATE;
    }
    if (f->log.count != 0) {
        const RmcMatchEvent *e = &f->log.events[f->log.count - 1];
        if (e->id != (uint64_t)f->log.count ||
            e->match_id.value != f->state.fixture.id.value ||
            e->at_second != f->state.elapsed_seconds ||
            f->log.events[0].type != RMC_EVENT_PERIOD_START ||
            f->log.events[0].team_id.value != f->config.first_opening_team.value) {
            return RMC_FLOW_INVALID_STATE;
        }
    } else if (f->state.period != RMC_MATCH_NOT_STARTED) {
        return RMC_FLOW_INVALID_STATE;
    }
    return RMC_FLOW_OK;
}

RmcFlowError rmc_match_flow_init(
    const RmcWorldCore *world, const RmcFixture *fixture,
    const RmcLineup *home, const RmcLineup *away,
    const RmcMatchFlowConfig *config,
    RmcMatchEvent *event_storage, size_t event_capacity,
    RmcMatchFlowCommand *command_storage, size_t command_capacity,
    RmcMatchFlow *destination)
{
    if (world == NULL || fixture == NULL || home == NULL || away == NULL ||
        config == NULL || destination == NULL || event_storage == NULL ||
        command_storage == NULL || event_capacity == 0 || command_capacity == 0 ||
        event_capacity > SIZE_MAX / sizeof(*event_storage) ||
        command_capacity > SIZE_MAX / sizeof(*command_storage) ||
        config->operation_limit > RMC_FLOW_DEFAULT_OPERATION_LIMIT) {
        return RMC_FLOW_INVALID_ARGUMENT;
    }
    if (config->flow_version != RMC_MATCH_FLOW_VERSION ||
        (config->selector_version != RMC_MATCH_SELECTOR_V1_VERSION &&
         config->selector_version != RMC_MATCH_SELECTOR_V2_VERSION)) {
        return RMC_FLOW_UNSUPPORTED_VERSION;
    }
    RmcMatchFlow f = {0};
    f.world = world;
    f.config = *config;
    if (f.config.operation_limit == 0) {
        f.config.operation_limit = RMC_FLOW_DEFAULT_OPERATION_LIMIT;
    }
    if (!rmc_match_state_init(world, fixture, home, away, &f.state) ||
        !fixture_team(&f.state, config->first_opening_team) ||
        !rmc_match_event_log_init(&f.state, event_storage, event_capacity, &f.log) ||
        !rmc_stream_init(&f.match_rng, &world->identity, RMC_STREAM_MATCH,
            fixture->id.value)) {
        return RMC_FLOW_INVALID_ARGUMENT;
    }
    f.journal.commands = command_storage;
    f.journal.capacity = command_capacity;
    *destination = f;
    return RMC_FLOW_OK;
}

/* Prefixes are exclusively API-produced and immutable. These admission checks
 * distinguish dispatch categories; qualified engines authenticate full causes
 * and history before committing. No speculative calls or state repair. */
static bool loose_cause(const RmcMatchFlow *f)
{
    const RmcMatchState *s = &f->state;
    if (f->log.count == 0) { return false; }
    const RmcMatchEvent *e = &f->log.events[f->log.count - 1];
    if (e->team_id.value != s->possession.team_id.value) { return false; }
    if (e->type == RMC_EVENT_ACTION) {
        return e->outcome_code == RMC_PASS_OUTCOME_LOOSE_BALL ||
            e->outcome_code == RMC_TACKLE_OUTCOME_LOOSE_BALL ||
            e->outcome_code == RMC_CROSS_OUTCOME_LOOSE_BALL;
    }
    if (e->type != RMC_EVENT_SHOT || f->log.count < 2 ||
        e->shot.model_version != RMC_SHOT_MODEL_VERSION ||
        e->cause_event_id != e->id - 1) { return false; }
    const RmcMatchEvent *chance = e - 1;
    if (chance->type != RMC_EVENT_ACTION || chance->outcome_code != RMC_SHOT_CHANCE) {
        return false;
    }
    return e->outcome_code == RMC_SHOT_BLOCKED ||
        e->outcome_code == RMC_SHOT_SAVED_REBOUND || e->outcome_code == RMC_SHOT_WOODWORK;
}

static const RmcMatchEvent *pending_shot(const RmcMatchFlow *f)
{
    if (f->log.count < 2) { return NULL; }
    const RmcMatchEvent *e = &f->log.events[f->log.count - 1];
    const bool goal = e->type == RMC_EVENT_GOAL;
    if (goal) {
        if (f->log.count < 3 || e->cause_event_id != e->id - 1) { return NULL; }
        --e;
    }
    if (e->type != RMC_EVENT_SHOT ||
        e->outcome_code != (goal ? RMC_SHOT_GOAL : RMC_SHOT_WIDE) ||
        e->shot.model_version != RMC_SHOT_MODEL_VERSION ||
        e->shot.restart_disposition != RMC_RESTART_PENDING ||
        e->shot.restart_kind != (goal ? RMC_RESTART_KICKOFF : RMC_RESTART_GOAL_KICK) ||
        !fixture_team(&f->state, e->shot.restart_awarded_team) ||
        e->shot.restart_awarded_team.value == e->team_id.value ||
        e->cause_event_id != e->id - 1 ||
        (e - 1)->type != RMC_EVENT_ACTION ||
        (e - 1)->outcome_code != RMC_SHOT_CHANCE) { return NULL; }
    return e;
}

RmcFlowError rmc_match_flow_next(const RmcMatchFlow *f, RmcMatchFlowNext *destination)
{
    if (destination == NULL) { return RMC_FLOW_INVALID_ARGUMENT; }
    RmcFlowError err = metadata(f);
    if (err != RMC_FLOW_OK) { return err; }
    const RmcMatchState *s = &f->state;
    RmcMatchFlowNext n = {.observed_command_count = (uint64_t)f->journal.count};
    if (s->period == RMC_MATCH_FINISHED) {
        if (s->terminal_reason != RMC_MATCH_TERMINAL_COMPLETED || !cleared(&s->possession) ||
            f->log.count < 2 || f->log.events[f->log.count - 1].type != RMC_EVENT_MATCH_END ||
            f->log.events[f->log.count - 2].type != RMC_EVENT_PERIOD_END) {
            return RMC_FLOW_INVALID_STATE;
        }
        n.kind = RMC_FLOW_COMPLETE;
    } else if (s->period == RMC_MATCH_NOT_STARTED || s->period == RMC_MATCH_HALF_TIME) {
        if (s->mode != RMC_MATCH_PERIOD_ENDED || !cleared(&s->possession) ||
            (s->period == RMC_MATCH_NOT_STARTED && f->log.count != 0)) {
            return RMC_FLOW_INVALID_STATE;
        }
        if (s->period == RMC_MATCH_HALF_TIME &&
            (f->log.count == 0 || f->log.events[f->log.count - 1].type != RMC_EVENT_PERIOD_END ||
             !rmc_stoppage_deadline(0, s->first_half_stoppage_seconds, &n.deadline_second) ||
             s->elapsed_seconds != n.deadline_second ||
             s->second_half_start_second != n.deadline_second)) { return RMC_FLOW_INVALID_STATE; }
        if (!f->home_window_submitted || !f->away_window_submitted) {
            n.kind = RMC_FLOW_NEED_INSTRUCTION;
            n.team_id = !f->home_window_submitted ? s->fixture.home_team : s->fixture.away_team;
        } else {
            n.kind = RMC_FLOW_NEED_OPENING;
            n.team_id = f->config.first_opening_team;
            if (s->period == RMC_MATCH_HALF_TIME) {
                n.team_id = n.team_id.value == s->fixture.home_team.value ?
                    s->fixture.away_team : s->fixture.home_team;
            }
        }
    } else {
        if ((s->period != RMC_MATCH_FIRST_HALF && s->period != RMC_MATCH_SECOND_HALF) ||
            !f->home_window_submitted || !f->away_window_submitted ||
            !rmc_stoppage_deadline(s->period == RMC_MATCH_FIRST_HALF ? 0 : s->second_half_start_second,
                s->period == RMC_MATCH_FIRST_HALF ? s->first_half_stoppage_seconds :
                s->second_half_stoppage_seconds, &n.deadline_second) ||
            s->elapsed_seconds > n.deadline_second) { return RMC_FLOW_INVALID_STATE; }
        n.remaining_seconds = n.deadline_second - s->elapsed_seconds;
        if (s->mode == RMC_MATCH_BALL_IN_PLAY) {
            if (n.remaining_seconds == 0) { n.kind = RMC_FLOW_NEED_CLOSURE; }
            else if (s->possession.team_id.value == 0) { return RMC_FLOW_INVALID_STATE; }
            else if (s->possession.action_count >= RMC_POSSESSION_MAX_ACTIONS) {
                n.kind = RMC_FLOW_NEED_RESET;
            } else if (s->possession.controller.value == 0) {
                if (!loose_cause(f)) { return RMC_FLOW_INVALID_STATE; }
                n.kind = RMC_FLOW_NEED_RECOVERY;
            } else { n.kind = RMC_FLOW_NEED_SPORTING_ACTION; }
        } else if (s->mode == RMC_MATCH_RESTART_PENDING && n.remaining_seconds > 0 &&
                   cleared(&s->possession)) {
            const RmcMatchEvent *shot = pending_shot(f);
            if (shot == NULL) { return RMC_FLOW_INVALID_STATE; }
            n.kind = RMC_FLOW_NEED_RESTART;
            n.team_id = shot->shot.restart_awarded_team;
        } else { return RMC_FLOW_INVALID_STATE; }
    }
    *destination = n;
    return RMC_FLOW_OK;
}

/* Total reporting helper: used after sporting success, with no fallible work. */
static RmcMatchFlowStatus status_value(const RmcMatchFlow *f)
{
    const RmcMatchState *s = &f->state;
    return (RmcMatchFlowStatus){
        .period = s->period, .mode = s->mode, .terminal_reason = s->terminal_reason,
        .at_second = s->elapsed_seconds, .home_goals = s->home_goals, .away_goals = s->away_goals,
        .accepted_commands = (uint64_t)f->journal.count, .sporting_operations = f->sporting_count,
        .event_count = f->log.count,
        .complete = s->period == RMC_MATCH_FINISHED && s->terminal_reason == RMC_MATCH_TERMINAL_COMPLETED,
        .needs_instruction = (s->period == RMC_MATCH_NOT_STARTED || s->period == RMC_MATCH_HALF_TIME) &&
            (!f->home_window_submitted || !f->away_window_submitted)
    };
}

RmcFlowError rmc_match_flow_status(const RmcMatchFlow *f, RmcMatchFlowStatus *destination)
{
    if (destination == NULL) { return RMC_FLOW_INVALID_ARGUMENT; }
    RmcMatchFlowNext n;
    RmcFlowError err = rmc_match_flow_next(f, &n);
    if (err != RMC_FLOW_OK) { return err; }
    *destination = status_value(f);
    return RMC_FLOW_OK;
}

static bool compatible(RmcFlowNextKind n, RmcFlowCommandKind c)
{
    switch (n) {
    case RMC_FLOW_NEED_INSTRUCTION: return c == RMC_FLOW_INSTRUCTION;
    case RMC_FLOW_NEED_OPENING: return c == RMC_FLOW_OPENING;
    case RMC_FLOW_NEED_CLOSURE: return c == RMC_FLOW_CLOSE_PERIOD;
    case RMC_FLOW_NEED_RESET: return c == RMC_FLOW_RESET_WINDOW;
    case RMC_FLOW_NEED_RESTART: return c == RMC_FLOW_RESTART;
    case RMC_FLOW_NEED_RECOVERY: return c == RMC_FLOW_RECOVERY;
    case RMC_FLOW_NEED_SPORTING_ACTION: return c >= RMC_FLOW_PASS && c <= RMC_FLOW_SHOT;
    case RMC_FLOW_COMPLETE: return false;
    }
    return false;
}

RmcFlowError rmc_match_flow_apply(
    RmcMatchFlow *f, const RmcMatchFlowCommand *command, RmcMatchFlowStepResult *destination)
{
    if (command == NULL || destination == NULL || (unsigned)command->kind > RMC_FLOW_SHOT ||
        (unsigned)command->expected_period >= RMC_MATCH_PERIOD_COUNT) {
        return RMC_FLOW_INVALID_ARGUMENT;
    }
    RmcMatchFlowNext n;
    RmcFlowError err = rmc_match_flow_next(f, &n);
    if (err != RMC_FLOW_OK) { return err; }
    if (f->journal.count >= UINT64_MAX) { return RMC_FLOW_RESOURCE_EXHAUSTED; }
    if (command->command_id != (uint64_t)f->journal.count + 1 ||
        command->observed_command_count != (uint64_t)f->journal.count) { return RMC_FLOW_STALE_COMMAND; }
    if (command->match_id.value != f->state.fixture.id.value ||
        command->expected_period != f->state.period || !compatible(n.kind, command->kind)) {
        return RMC_FLOW_WRONG_OPERATION;
    }
    /* Normalize active input only; no inactive union member is read. */
    RmcMatchFlowCommand c = {
        .command_id = command->command_id, .observed_command_count = command->observed_command_count,
        .match_id = command->match_id, .expected_period = command->expected_period, .kind = command->kind
    };
    uint32_t duration = 0;
    size_t events = 0;
    switch (c.kind) {
    case RMC_FLOW_INSTRUCTION:
        c.data.instruction.team_id = command->data.instruction.team_id;
        c.data.instruction.window = command->data.instruction.window;
        c.data.instruction.keep = command->data.instruction.keep;
        if (c.data.instruction.team_id.value != n.team_id.value ||
            c.data.instruction.window != f->state.period) { return RMC_FLOW_INVALID_ARGUMENT; }
        if (!c.data.instruction.keep) {
            c.data.instruction.instructions = command->data.instruction.instructions;
            if (!instructions_valid(c.data.instruction.instructions)) { return RMC_FLOW_INVALID_ARGUMENT; }
        }
        break;
    case RMC_FLOW_OPENING:
        c.data.opening = command->data.opening; events = 2;
        if ((unsigned)c.data.opening.opening_period >= RMC_MATCH_PERIOD_COUNT) { return RMC_FLOW_INVALID_ARGUMENT; }
        if (c.data.opening.opening_team.value != n.team_id.value ||
            c.data.opening.opening_period != (f->state.period == RMC_MATCH_NOT_STARTED ?
                RMC_MATCH_FIRST_HALF : RMC_MATCH_SECOND_HALF)) { return RMC_FLOW_WRONG_OPERATION; }
        break;
    case RMC_FLOW_CLOSE_PERIOD: events = f->state.period == RMC_MATCH_FIRST_HALF ? 1 : 2; break;
    case RMC_FLOW_RESET_WINDOW: break;
    case RMC_FLOW_RESTART:
        c.data.restart = command->data.restart;
        if ((unsigned)c.data.restart.restart_kind > RMC_RESTART_KICKOFF) { return RMC_FLOW_INVALID_ARGUMENT; }
        events = 1;
        break;
#define TIMED(kind, member) case kind: c.data.member = command->data.member; \
        duration = c.data.member.duration_seconds; events = 1; break
    TIMED(RMC_FLOW_RECOVERY, recovery);
    TIMED(RMC_FLOW_PASS, pass);
    TIMED(RMC_FLOW_CARRY, carry);
    TIMED(RMC_FLOW_DRIBBLE, dribble);
    TIMED(RMC_FLOW_TACKLE, tackle);
    TIMED(RMC_FLOW_CROSS, cross);
    TIMED(RMC_FLOW_SHOT, shot);
#undef TIMED
    }
    const bool timed = c.kind >= RMC_FLOW_RECOVERY;
    if (timed && (duration == 0 || duration > n.remaining_seconds)) { return RMC_FLOW_INVALID_ARGUMENT; }
    if (c.kind == RMC_FLOW_SHOT) {
        events = 3;
        if (duration == n.remaining_seconds) { events += f->state.period == RMC_MATCH_FIRST_HALF ? 1 : 2; }
    }
    if (f->journal.count >= f->journal.capacity ||
        (uint64_t)f->journal.count >= f->config.operation_limit ||
        events > f->log.capacity - f->log.count ||
        events > UINT64_MAX - (uint64_t)f->log.count ||
        (timed && f->sporting_count == UINT64_MAX) ||
        (c.kind == RMC_FLOW_RESET_WINDOW && f->reset_count == UINT64_MAX)) {
        return RMC_FLOW_RESOURCE_EXHAUSTED;
    }
    RmcMatchFlow candidate = *f;
    RmcMatchFlowStepResult result = {.accepted_command_id = c.command_id, .first_event_index = f->log.count};
    bool ok = false;
    switch (c.kind) {
    case RMC_FLOW_INSTRUCTION:
        if (n.team_id.value == f->state.fixture.home_team.value) {
            if (!c.data.instruction.keep) { candidate.home_instructions = c.data.instruction.instructions; }
            candidate.home_window_submitted = true;
        } else {
            if (!c.data.instruction.keep) { candidate.away_instructions = c.data.instruction.instructions; }
            candidate.away_window_submitted = true;
        }
        ok = true;
        break;
    case RMC_FLOW_OPENING: {
        RmcOpeningKickoffResult r;
        ok = rmc_match_transition_apply(f->world, &candidate.state,
            f->state.period == RMC_MATCH_NOT_STARTED ? RMC_TRANSITION_START : RMC_TRANSITION_SECOND_HALF,
            f->state.elapsed_seconds) &&
            rmc_match_opening_kickoff_apply(f->world, &candidate.state, &candidate.log, &c.data.opening, &r);
        break;
    }
    case RMC_FLOW_CLOSE_PERIOD:
        ok = rmc_match_period_commit(f->world, &candidate.state, &candidate.log); break;
    case RMC_FLOW_RESET_WINDOW:
        ok = rmc_match_sequence_apply(f->world, &candidate.state, RMC_SEQUENCE_RESET_ACTION_WINDOW); break;
    case RMC_FLOW_RESTART: {
        RmcRestartResult r;
        ok = rmc_match_restart_apply(f->world, &candidate.state, &candidate.log, &c.data.restart, &r); break;
    }
#define APPLY(kind, member, type, fn) case kind: { type r; \
        ok = fn(f->world, &candidate.state, &candidate.match_rng, &candidate.log, &c.data.member, &r); break; }
    APPLY(RMC_FLOW_RECOVERY, recovery, RmcRecoveryResult, rmc_match_recovery_apply)
    APPLY(RMC_FLOW_PASS, pass, RmcPassResult, rmc_match_pass_apply)
    APPLY(RMC_FLOW_CARRY, carry, RmcCarryResult, rmc_match_carry_apply)
    APPLY(RMC_FLOW_DRIBBLE, dribble, RmcDribbleResult, rmc_match_dribble_apply)
    APPLY(RMC_FLOW_TACKLE, tackle, RmcTackleResult, rmc_match_tackle_apply)
    APPLY(RMC_FLOW_CROSS, cross, RmcCrossResult, rmc_match_cross_apply)
    APPLY(RMC_FLOW_SHOT, shot, RmcShotResult, rmc_match_shot_apply)
#undef APPLY
    }
    if (!ok) { return RMC_FLOW_OPERATION_REJECTED; }
    /* No fallible operation after the underlying event append. */
    if (timed) { ++candidate.sporting_count; }
    if (c.kind == RMC_FLOW_RESET_WINDOW) { ++candidate.reset_count; }
    if (f->state.period == RMC_MATCH_FIRST_HALF && candidate.state.period == RMC_MATCH_HALF_TIME) {
        candidate.home_window_submitted = false;
        candidate.away_window_submitted = false;
        result.entered_half_time = true;
    }
    candidate.journal.commands[candidate.journal.count] = c;
    ++candidate.journal.count;
    result.appended_event_count = candidate.log.count - result.first_event_index;
    result.status = status_value(&candidate);
    *f = candidate;
    *destination = result;
    return RMC_FLOW_OK;
}

RmcFlowError rmc_match_flow_replay(RmcMatchFlow *f, const RmcMatchFlowCommand *records, size_t count)
{
    if (f == NULL || (count != 0 && records == NULL) || count > SIZE_MAX / sizeof(*records)) {
        return RMC_FLOW_INVALID_ARGUMENT;
    }
    RmcMatchFlowNext n;
    RmcFlowError err = rmc_match_flow_next(f, &n);
    if (err != RMC_FLOW_OK) { return err; }
    for (size_t i = 0; i < count; ++i) {
        RmcMatchFlowStepResult result;
        err = rmc_match_flow_apply(f, &records[i], &result);
        if (err != RMC_FLOW_OK) { return err; }
    }
    return RMC_FLOW_OK;
}
