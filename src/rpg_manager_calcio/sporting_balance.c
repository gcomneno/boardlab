/* Standalone observer; sporting authority remains entirely in Match Flow. */
#include "career.h"
#include "match_flow_selector.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEED_LIMIT 4096u
#define PATH_LIMIT 900u
#define KEY "world_ordinal,world_seed,match_id,selector_version"
#define U PRIu64

_Static_assert(RMC_GENERATOR_VERSION == 1 && RMC_RULES_VERSION == 1 &&
    RMC_MATCH_FLOW_VERSION == 1 && RMC_EVENT_SCHEMA_VERSION == 2 &&
    RMC_SHOT_MODEL_VERSION == 1 && RMC_MATCH_SELECTOR_V1_VERSION == 1 &&
    RMC_MATCH_SELECTOR_V2_VERSION == 2, "measurement version review required");

typedef struct {
    const char *seeds, *prefix, *fixture_text, *selector_text;
    uint64_t fixture_id;
    unsigned selectors; /* bit 0 v1, bit 1 v2 */
    bool all, replay;
} Options;

typedef struct {
    unsigned k, leg, formation[2], tactics[2], opener;
} Scenario;

typedef struct {
    FILE *file[7];
    bool failed;
} Reports;

typedef struct {
    RmcMatchFlow flow;
    RmcMatchEvent *events;
    RmcMatchFlowCommand *commands;
} Run;

typedef struct {
    const RmcPlayer *player;
    uint64_t participations, controlled, receipts, goals, shots;
} PlayerStats;

typedef struct {
    uint64_t goals, shots, outcomes[6], passes, completed, failed;
    uint64_t tackles, tackle[3], recoveries;
    uint64_t recovery[4][2];
    /* Exact integer quality, sector 2..4, lane 0..2, trajectory 1..3. */
    uint64_t shot[6][3][3][101][3];
    PlayerStats players[11];
} TeamStats;

typedef struct {
    TeamStats team[2];
    uint64_t commands[12], events[RMC_EVENT_TYPE_COUNT], chances;
} Metrics;

typedef struct {
    uint64_t matches, goals, shots, scoring, draws, commands, events, replay;
} Totals;

typedef struct {
    uint64_t attempted, completed, incomplete, replay_failures, pairs, paired_goals[2], paired_shots[2];
    Totals selector[2];
} Campaign;

typedef struct {
    bool complete, half_seen;
    const char *stage, *replay;
    unsigned error;
    RmcMatchFlow half; /* Read-only snapshot; never executed. */
} Attempt;

static size_t bounded_length(const char *s, size_t maximum)
{
    size_t n = 0;
    while (n <= maximum && s[n] != '\0') { ++n; }
    return n;
}

static bool decimal(const char *s, uint64_t *value)
{
    const size_t n = bounded_length(s, 20);
    uint64_t v = 0;
    if (n == 0 || n > 20) { return false; }
    for (size_t i = 0; i < n; ++i) {
        if (s[i] < '0' || s[i] > '9') { return false; }
        const unsigned d = (unsigned)(s[i] - '0');
        if (v > (UINT64_MAX - d) / 10) { return false; }
        v = v * 10 + d;
    }
    *value = v;
    return true;
}

static void help(void)
{
    puts("rmc-sporting-balance --seeds FILE --fixtures all|MATCH_ID\n"
         "  --selectors 1|2|1,2 --matrix balanced-v1 --half-time keep\n"
         "  --replay none|all --output PREFIX\n"
         "All seven options are required exactly once. --help must appear alone.\n"
         "Seeds: 1..4096 unique decimal uint64 values, one per line.\n"
         "Reports are exploratory measurements; no calibration/acceptance claim.");
}

static bool options(int argc, char **argv, Options *o)
{
    static const char *const names[] = {"--seeds", "--fixtures", "--selectors",
        "--matrix", "--half-time", "--replay", "--output"};
    unsigned seen = 0;
    for (int i = 1; i < argc; ++i) {
        unsigned at = 7;
        for (unsigned j = 0; j < 7; ++j) {
            if (strcmp(argv[i], names[j]) == 0) { at = j; break; }
        }
        if (at == 7 || (seen & (1u << at)) != 0 || i + 1 >= argc) { return false; }
        seen |= 1u << at;
        const char *v = argv[++i];
        if (bounded_length(v, PATH_LIMIT) == 0 || bounded_length(v, PATH_LIMIT) > PATH_LIMIT) { return false; }
        switch (at) {
        case 0: o->seeds = v; break;
        case 1:
            o->fixture_text = v; o->all = strcmp(v, "all") == 0;
            if (!o->all && (!decimal(v, &o->fixture_id) || o->fixture_id == 0)) { return false; }
            break;
        case 2:
            if (strcmp(v, "1") == 0) { o->selectors = 1; o->selector_text = "1"; }
            else if (strcmp(v, "2") == 0) { o->selectors = 2; o->selector_text = "2"; }
            else if (strcmp(v, "1,2") == 0) { o->selectors = 3; o->selector_text = "1+2"; }
            else { return false; }
            break;
        case 3: if (strcmp(v, "balanced-v1") != 0) { return false; } break;
        case 4: if (strcmp(v, "keep") != 0) { return false; } break;
        case 5:
            if (strcmp(v, "all") != 0 && strcmp(v, "none") != 0) { return false; }
            o->replay = strcmp(v, "all") == 0; break;
        case 6: o->prefix = v; break;
        }
    }
    return seen == 127;
}

/* Bounded byte grammar also rejects embedded NUL, whitespace and long lines. */
static bool seed_manifest(const char *path, uint64_t *seeds, size_t *count)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) { return false; }
    char line[21];
    size_t used = 0, n = 0;
    bool cr = false, ok = true;
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '\r' && !cr) { cr = true; continue; }
        if (c == '\n') {
            line[used] = '\0';
            uint64_t seed;
            if (n == SEED_LIMIT || !decimal(line, &seed)) { ok = false; break; }
            for (size_t i = 0; i < n; ++i) { if (seeds[i] == seed) { ok = false; } }
            if (!ok) { break; }
            seeds[n++] = seed; used = 0; cr = false;
        } else {
            if (cr || c < '0' || c > '9' || used == 20) { ok = false; break; }
            line[used++] = (char)c;
        }
    }
    if (ok && (cr || ferror(f))) { ok = false; }
    if (ok && used != 0) {
        uint64_t seed;
        line[used] = '\0';
        if (n == SEED_LIMIT || !decimal(line, &seed)) { ok = false; }
        else {
            for (size_t i = 0; i < n; ++i) { if (seeds[i] == seed) { ok = false; } }
            if (ok) { seeds[n++] = seed; }
        }
    }
    if (fclose(f) != 0) { ok = false; }
    *count = n;
    return ok && n != 0;
}

static const RmcRoster *roster(const RmcWorldCore *world, RmcTeamId team)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (world->teams[i].id.value == team.value && world->rosters[i].team_id.value == team.value) {
            return &world->rosters[i];
        }
    }
    return NULL;
}

static bool scenario(const RmcCareerFoundation *f, size_t s, size_t index, Scenario *out)
{
    const size_t p = index % 28;
    const RmcFixture *a = &f->calendar.fixtures[p], *b = &f->calendar.fixtures[p + 28];
    if (a->home_team.value != b->away_team.value || a->away_team.value != b->home_team.value) { return false; }
    /* Reduce ordinal first: no multiplication can overflow. */
    Scenario x = {.k = (unsigned)((28 * (s % 1152) + p) % 1152), .leg = (unsigned)(index / 28)};
    const unsigned tactical = (x.k / 2) % 64, formation = x.k / 128;
    x.tactics[0] = tactical % 8; x.tactics[1] = tactical / 8;
    x.formation[0] = formation % 3; x.formation[1] = formation / 3;
    x.opener = x.k % 2;
    if (x.leg != 0) {
        unsigned t = x.tactics[0]; x.tactics[0] = x.tactics[1]; x.tactics[1] = t;
        t = x.formation[0]; x.formation[0] = x.formation[1]; x.formation[1] = t;
        x.opener = 1 - x.opener;
    }
    *out = x;
    return true;
}

static bool allocate_run(Run *r)
{
    if (RMC_FLOW_DEFAULT_EVENT_CAPACITY > SIZE_MAX / sizeof(*r->events) ||
        RMC_FLOW_DEFAULT_COMMAND_CAPACITY > SIZE_MAX / sizeof(*r->commands)) { return false; }
    r->events = malloc(RMC_FLOW_DEFAULT_EVENT_CAPACITY * sizeof(*r->events));
    r->commands = malloc(RMC_FLOW_DEFAULT_COMMAND_CAPACITY * sizeof(*r->commands));
    return r->events != NULL && r->commands != NULL;
}

static void free_run(Run *r) { free(r->events); free(r->commands); }

static bool same_instructions(RmcFlowInstructions a, RmcFlowInstructions b)
{
    return a.approach == b.approach && a.progression == b.progression && a.pressure == b.pressure;
}

/* Field enumeration follows the qualified replay tests; no representation equality. */
static bool same_command(const RmcMatchFlowCommand *a, const RmcMatchFlowCommand *b)
{
    if (!(a->command_id == b->command_id && a->observed_command_count == b->observed_command_count)) { return false; }
    if (!(a->match_id.value == b->match_id.value && a->expected_period == b->expected_period && a->kind == b->kind)) { return false; }
    switch (a->kind) {
    case RMC_FLOW_INSTRUCTION:
        if (!(a->data.instruction.team_id.value == b->data.instruction.team_id.value)) { return false; }
        if (!(a->data.instruction.window == b->data.instruction.window && a->data.instruction.keep == b->data.instruction.keep)) { return false; }
        if (!a->data.instruction.keep) { if (!(same_instructions(a->data.instruction.instructions, b->data.instruction.instructions))) { return false; } }
        break;
    case RMC_FLOW_CLOSE_PERIOD: case RMC_FLOW_RESET_WINDOW: break;
    case RMC_FLOW_OPENING:
        if (!(a->data.opening.opening_period == b->data.opening.opening_period)) { return false; }
        if (!(a->data.opening.opening_team.value == b->data.opening.opening_team.value)) { return false; }
        if (!(a->data.opening.taker.value == b->data.opening.taker.value)) { return false; }
        break;
    case RMC_FLOW_RESTART:
        if (!(a->data.restart.restart_kind == b->data.restart.restart_kind)) { return false; }
        if (!(a->data.restart.awarded_team.value == b->data.restart.awarded_team.value)) { return false; }
        if (!(a->data.restart.taker.value == b->data.restart.taker.value)) { return false; }
        if (!(a->data.restart.source_shot_event_id == b->data.restart.source_shot_event_id)) { return false; }
        break;
    case RMC_FLOW_RECOVERY:
        if (!(a->data.recovery.incumbent.value == b->data.recovery.incumbent.value)) { return false; }
        if (!(a->data.recovery.challenger.value == b->data.recovery.challenger.value)) { return false; }
        if (!(a->data.recovery.context == b->data.recovery.context)) { return false; }
        if (!(a->data.recovery.duration_seconds == b->data.recovery.duration_seconds)) { return false; }
        break;
    case RMC_FLOW_PASS:
        if (!(a->data.pass.team_id.value == b->data.pass.team_id.value)) { return false; }
        if (!(a->data.pass.passer.value == b->data.pass.passer.value)) { return false; }
        if (!(a->data.pass.recipient.value == b->data.pass.recipient.value)) { return false; }
        if (!(a->data.pass.opponent.value == b->data.pass.opponent.value)) { return false; }
        if (!(a->data.pass.context == b->data.pass.context)) { return false; }
        if (!(a->data.pass.duration_seconds == b->data.pass.duration_seconds)) { return false; }
        break;
    case RMC_FLOW_CARRY:
        if (!(a->data.carry.team_id.value == b->data.carry.team_id.value)) { return false; }
        if (!(a->data.carry.carrier.value == b->data.carry.carrier.value)) { return false; }
        if (!(a->data.carry.opponent.value == b->data.carry.opponent.value)) { return false; }
        if (!(a->data.carry.context == b->data.carry.context)) { return false; }
        if (!(a->data.carry.duration_seconds == b->data.carry.duration_seconds)) { return false; }
        break;
    case RMC_FLOW_DRIBBLE:
        if (!(a->data.dribble.team_id.value == b->data.dribble.team_id.value)) { return false; }
        if (!(a->data.dribble.dribbler.value == b->data.dribble.dribbler.value)) { return false; }
        if (!(a->data.dribble.opponent.value == b->data.dribble.opponent.value)) { return false; }
        if (!(a->data.dribble.context == b->data.dribble.context)) { return false; }
        if (!(a->data.dribble.duration_seconds == b->data.dribble.duration_seconds)) { return false; }
        break;
    case RMC_FLOW_TACKLE:
        if (!(a->data.tackle.team_id.value == b->data.tackle.team_id.value)) { return false; }
        if (!(a->data.tackle.carrier.value == b->data.tackle.carrier.value)) { return false; }
        if (!(a->data.tackle.defender.value == b->data.tackle.defender.value)) { return false; }
        if (!(a->data.tackle.context == b->data.tackle.context)) { return false; }
        if (!(a->data.tackle.duration_seconds == b->data.tackle.duration_seconds)) { return false; }
        break;
    case RMC_FLOW_CROSS:
        if (!(a->data.cross.team_id.value == b->data.cross.team_id.value)) { return false; }
        if (!(a->data.cross.crosser.value == b->data.cross.crosser.value)) { return false; }
        if (!(a->data.cross.recipient.value == b->data.cross.recipient.value)) { return false; }
        if (!(a->data.cross.defender.value == b->data.cross.defender.value)) { return false; }
        if (!(a->data.cross.context == b->data.cross.context)) { return false; }
        if (!(a->data.cross.duration_seconds == b->data.cross.duration_seconds)) { return false; }
        break;
    case RMC_FLOW_SHOT:
        if (!(a->data.shot.team_id.value == b->data.shot.team_id.value)) { return false; }
        if (!(a->data.shot.shooter.value == b->data.shot.shooter.value)) { return false; }
        if (!(a->data.shot.defender.value == b->data.shot.defender.value)) { return false; }
        if (!(a->data.shot.goalkeeper.value == b->data.shot.goalkeeper.value)) { return false; }
        if (!(a->data.shot.context == b->data.shot.context)) { return false; }
        if (!(a->data.shot.duration_seconds == b->data.shot.duration_seconds)) { return false; }
        break;
    }
    return true;
}

static bool same_event(const RmcMatchEvent *a, const RmcMatchEvent *b)
{
    if (!(a->id == b->id && a->match_id.value == b->match_id.value)) { return false; }
    if (!(a->type == b->type && a->at_second == b->at_second)) { return false; }
    if (!(a->team_id.value == b->team_id.value && a->primary_actor.value == b->primary_actor.value)) { return false; }
    if (!(a->secondary_actor.value == b->secondary_actor.value)) { return false; }
    if (!(a->outcome_code == b->outcome_code && a->cause_event_id == b->cause_event_id)) { return false; }
    const RmcShotEventPayload *x = &a->shot, *y = &b->shot;
    if (!(x->model_version == y->model_version && x->chance_quality == y->chance_quality)) { return false; }
    if (!(x->source_zone.sector == y->source_zone.sector && x->source_zone.lane == y->source_zone.lane)) { return false; }
    if (!(x->context == y->context && x->duration_seconds == y->duration_seconds)) { return false; }
    if (!(x->defender_id.value == y->defender_id.value && x->goalkeeper_id.value == y->goalkeeper_id.value)) { return false; }
    if (!(x->applied_talent_mask == y->applied_talent_mask && x->trajectory == y->trajectory)) { return false; }
    if (!(x->restart_kind == y->restart_kind && x->restart_awarded_team.value == y->restart_awarded_team.value)) { return false; }
    if (!(x->restart_zone.sector == y->restart_zone.sector && x->restart_zone.lane == y->restart_zone.lane)) { return false; }
    if (!(x->restart_disposition == y->restart_disposition)) { return false; }
    return true;
}

static bool same_state(const RmcMatchState *a, const RmcMatchState *b)
{
    if (!(a->fixture.id.value == b->fixture.id.value && a->fixture.matchday == b->fixture.matchday)) { return false; }
    if (!(a->fixture.home_team.value == b->fixture.home_team.value && a->fixture.away_team.value == b->fixture.away_team.value)) { return false; }
    const RmcLineup *x[] = {&a->home_lineup, &a->away_lineup};
    const RmcLineup *y[] = {&b->home_lineup, &b->away_lineup};
    for (size_t side = 0; side < 2; ++side) {
        if (!(x[side]->formation == y[side]->formation && x[side]->secondary_assignments == y[side]->secondary_assignments)) { return false; }
        for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
            if (!(x[side]->players[i].value == y[side]->players[i].value && x[side]->slots[i] == y[side]->slots[i])) { return false; }
        }
    }
    if (!(a->period == b->period && a->mode == b->mode && a->terminal_reason == b->terminal_reason)) { return false; }
    if (!(a->elapsed_seconds == b->elapsed_seconds && a->second_half_start_second == b->second_half_start_second)) { return false; }
    if (!(a->first_half_stoppage_seconds == b->first_half_stoppage_seconds && a->second_half_stoppage_seconds == b->second_half_stoppage_seconds)) { return false; }
    if (!(a->home_goals == b->home_goals && a->away_goals == b->away_goals)) { return false; }
    const RmcMatchPossession *p = &a->possession, *q = &b->possession;
    if (!(p->team_id.value == q->team_id.value && p->initiator.value == q->initiator.value && p->controller.value == q->controller.value)) { return false; }
    if (!(p->zone.sector == q->zone.sector && p->zone.lane == q->zone.lane && p->phase == q->phase)) { return false; }
    if (!(p->action_count == q->action_count && p->elapsed_seconds == q->elapsed_seconds)) { return false; }
    return true;
}

static bool same_flow(const RmcMatchFlow *f, const RmcMatchFlow *g)
{
    if (!same_state(&f->state, &g->state) ||
        f->match_rng.state != g->match_rng.state || f->match_rng.increment != g->match_rng.increment ||
        f->config.flow_version != g->config.flow_version || f->config.selector_version != g->config.selector_version ||
        f->config.first_opening_team.value != g->config.first_opening_team.value ||
        f->config.operation_limit != g->config.operation_limit ||
        f->sporting_count != g->sporting_count || f->reset_count != g->reset_count ||
        f->home_window_submitted != g->home_window_submitted || f->away_window_submitted != g->away_window_submitted ||
        !same_instructions(f->home_instructions, g->home_instructions) ||
        !same_instructions(f->away_instructions, g->away_instructions) ||
        f->journal.count != g->journal.count || f->journal.capacity != g->journal.capacity ||
        f->log.count != g->log.count || f->log.capacity != g->log.capacity ||
        f->log.match_id.value != g->log.match_id.value) { return false; }
    for (size_t i = 0; i < f->log.count; ++i) {
        if (!same_event(&f->log.events[i], &g->log.events[i])) { return false; }
    }
    for (size_t i = 0; i < f->journal.count; ++i) {
        if (!same_command(&f->journal.commands[i], &g->journal.commands[i])) { return false; }
    }
    RmcMatchFlowStatus x, y;
    if (rmc_match_flow_status(f, &x) != RMC_FLOW_OK || rmc_match_flow_status(g, &y) != RMC_FLOW_OK) { return false; }
    return x.period == y.period && x.mode == y.mode && x.terminal_reason == y.terminal_reason &&
        x.at_second == y.at_second && x.home_goals == y.home_goals && x.away_goals == y.away_goals &&
        x.accepted_commands == y.accepted_commands && x.sporting_operations == y.sporting_operations &&
        x.event_count == y.event_count && x.complete == y.complete && x.needs_instruction == y.needs_instruction;
}

static PlayerStats *actor(Metrics *m, RmcPlayerId id, unsigned *side)
{
    if (id.value == 0) { return NULL; }
    for (unsigned s = 0; s < 2; ++s) {
        for (unsigned i = 0; i < 11; ++i) {
            if (m->team[s].players[i].player->id.value == id.value) {
                if (side != NULL) { *side = s; }
                return &m->team[s].players[i];
            }
        }
    }
    return NULL;
}

static bool init_metrics(const RmcMatchFlow *f, Metrics *m)
{
    memset(m, 0, sizeof(*m));
    const RmcLineup *lineups[] = {&f->state.home_lineup, &f->state.away_lineup};
    const RmcTeamId teams[] = {f->state.fixture.home_team, f->state.fixture.away_team};
    for (unsigned side = 0; side < 2; ++side) {
        const RmcRoster *r = roster(f->world, teams[side]);
        if (r == NULL) { return false; }
        for (unsigned i = 0; i < 11; ++i) {
            for (unsigned j = 0; j < RMC_TEAM_PLAYERS; ++j) {
                const RmcPlayer *p = &r->players[j];
                if (p->id.value == lineups[side]->players[i].value && p->team_id.value == teams[side].value &&
                    (unsigned)p->department < 4) { m->team[side].players[i].player = p; }
            }
            if (m->team[side].players[i].player == NULL) { return false; }
        }
    }
    return true;
}

/* Pure classifier used by observation and isolated reducer vectors. */
static int recovery_source(const RmcMatchEvent *cause)
{
    if (cause->type == RMC_EVENT_ACTION) {
        switch (cause->outcome_code) {
        case RMC_PASS_OUTCOME_LOOSE_BALL: return 0;
        case RMC_TACKLE_OUTCOME_LOOSE_BALL: return 1;
        case RMC_CROSS_OUTCOME_LOOSE_BALL: return 2;
        default: return -1;
        }
    }
    if (cause->type == RMC_EVENT_SHOT &&
        (cause->outcome_code == RMC_SHOT_SAVED_REBOUND || cause->outcome_code == RMC_SHOT_BLOCKED ||
         cause->outcome_code == RMC_SHOT_WOODWORK)) { return 3; }
    return -1;
}

static bool observe(const RmcMatchFlow *f, const RmcMatchFlowCommand *c,
    const RmcMatchFlowStepResult *step, Metrics *m)
{
    if ((unsigned)c->kind >= 12 || step->first_event_index > f->log.count ||
        step->appended_event_count != f->log.count - step->first_event_index) { return false; }
    ++m->commands[c->kind];
    const RmcMatchEvent *first = step->appended_event_count == 0 ? NULL : &f->log.events[step->first_event_index];
    RmcPlayerId roles[4] = {{0}};
    unsigned role_count = 0;
    RmcPlayerId controlled = {0};
    switch (c->kind) {
    case RMC_FLOW_OPENING: roles[role_count++] = c->data.opening.taker; break;
    case RMC_FLOW_RESTART: roles[role_count++] = c->data.restart.taker; break;
    case RMC_FLOW_RECOVERY:
        roles[role_count++] = c->data.recovery.incumbent;
        roles[role_count++] = c->data.recovery.challenger; break;
    case RMC_FLOW_PASS: {
        if (first == NULL || first->type != RMC_EVENT_ACTION ||
            (first->outcome_code != RMC_PASS_OUTCOME_COMPLETED && first->outcome_code != RMC_PASS_OUTCOME_LOOSE_BALL)) { return false; }
        unsigned side;
        controlled = c->data.pass.passer;
        PlayerStats *p = actor(m, controlled, &side);
        if (p == NULL || p->player->team_id.value != c->data.pass.team_id.value) { return false; }
        TeamStats *t = &m->team[side]; ++t->passes;
        roles[role_count++] = controlled; roles[role_count++] = c->data.pass.opponent;
        if (first->outcome_code == RMC_PASS_OUTCOME_COMPLETED) {
            p = actor(m, c->data.pass.recipient, NULL);
            if (p == NULL || first->secondary_actor.value != p->player->id.value) { return false; }
            ++t->completed; ++p->receipts;
            roles[role_count++] = c->data.pass.recipient;
        } else { ++t->failed; }
        break;
    }
    case RMC_FLOW_CARRY:
        controlled = c->data.carry.carrier;
        roles[role_count++] = controlled; roles[role_count++] = c->data.carry.opponent; break;
    case RMC_FLOW_DRIBBLE:
        controlled = c->data.dribble.dribbler;
        roles[role_count++] = controlled; roles[role_count++] = c->data.dribble.opponent; break;
    case RMC_FLOW_TACKLE: {
        unsigned side;
        PlayerStats *p = actor(m, c->data.tackle.defender, &side);
        if (p == NULL || p->player->team_id.value != c->data.tackle.team_id.value || first == NULL ||
            first->type != RMC_EVENT_ACTION || first->primary_actor.value != p->player->id.value ||
            first->outcome_code < 301 || first->outcome_code > 303) { return false; }
        /* event.team_id is resulting possession, NOT attempt ownership. */
        const uint64_t expected = first->outcome_code == RMC_TACKLE_OUTCOME_WON ? c->data.tackle.team_id.value :
            (side == 0 ? f->state.fixture.away_team.value : f->state.fixture.home_team.value);
        if (first->team_id.value != expected) { return false; }
        ++m->team[side].tackles; ++m->team[side].tackle[first->outcome_code - 301];
        roles[role_count++] = c->data.tackle.defender; roles[role_count++] = c->data.tackle.carrier;
        break;
    }
    case RMC_FLOW_CROSS:
        controlled = c->data.cross.crosser;
        roles[role_count++] = controlled; roles[role_count++] = c->data.cross.defender;
        if (first == NULL) { return false; }
        if (first->outcome_code == RMC_CROSS_OUTCOME_COMPLETED) { roles[role_count++] = c->data.cross.recipient; }
        break;
    case RMC_FLOW_SHOT:
        controlled = c->data.shot.shooter;
        roles[role_count++] = controlled; roles[role_count++] = c->data.shot.defender;
        roles[role_count++] = c->data.shot.goalkeeper; break;
    default: break;
    }
    if (controlled.value != 0) {
        PlayerStats *p = actor(m, controlled, NULL);
        if (p == NULL) { return false; }
        ++p->controlled;
    }
    for (unsigned i = 0; i < role_count; ++i) {
        PlayerStats *p = actor(m, roles[i], NULL);
        if (p == NULL) { return false; }
        bool earlier = false;
        for (unsigned j = 0; j < i; ++j) { earlier |= roles[i].value == roles[j].value; }
        if (!earlier) { ++p->participations; }
    }
    for (size_t i = step->first_event_index; i < f->log.count; ++i) {
        const RmcMatchEvent *e = &f->log.events[i];
        if ((unsigned)e->type >= RMC_EVENT_TYPE_COUNT || e->id != i + 1) { return false; }
        ++m->events[e->type];
        if (e->type == RMC_EVENT_ACTION && e->outcome_code == RMC_SHOT_CHANCE) { ++m->chances; }
        if (e->type != RMC_EVENT_SHOT && e->type != RMC_EVENT_GOAL && e->type != RMC_EVENT_RECOVERY) { continue; }
        unsigned side;
        PlayerStats *p = actor(m, e->primary_actor, &side);
        if (p == NULL || p->player->team_id.value != e->team_id.value) { return false; }
        TeamStats *t = &m->team[side];
        if (e->cause_event_id == 0 || e->cause_event_id >= e->id || e->cause_event_id > f->log.count) { return false; }
        const RmcMatchEvent *cause = &f->log.events[(size_t)(e->cause_event_id - 1)];
        if (cause->id != e->cause_event_id) { return false; }
        if (e->type == RMC_EVENT_RECOVERY) {
            const int source = recovery_source(cause);
            if (c->kind != RMC_FLOW_RECOVERY || source < 0 || e->outcome_code < 1 || e->outcome_code > 2) { return false; }
            ++t->recoveries; ++t->recovery[source][e->outcome_code - 1];
        } else if (e->type == RMC_EVENT_GOAL) {
            if (cause->type != RMC_EVENT_SHOT || cause->outcome_code != RMC_SHOT_GOAL ||
                cause->primary_actor.value != e->primary_actor.value || e->outcome_code != RMC_SHOT_GOAL) { return false; }
            ++t->goals; ++p->goals;
        } else {
            const RmcShotEventPayload *q = &e->shot;
            if (c->kind != RMC_FLOW_SHOT || cause->type != RMC_EVENT_ACTION || cause->outcome_code != RMC_SHOT_CHANCE ||
                e->outcome_code < 501 || e->outcome_code > 506 || q->model_version != 1 ||
                q->source_zone.sector < 2 || q->source_zone.sector > 4 || (unsigned)q->source_zone.lane > 2 ||
                q->chance_quality > 100 || q->trajectory < 1 || q->trajectory > 3) { return false; }
            ++t->shots; ++p->shots; ++t->outcomes[e->outcome_code - 501];
            ++t->shot[e->outcome_code - 501][q->source_zone.sector - 2][q->source_zone.lane][q->chance_quality][q->trajectory - 1];
        }
    }
    return true;
}

static bool reconciled(const RmcMatchFlow *f, const Metrics *m)
{
    uint64_t commands = 0, events = 0, sporting = 0;
    for (unsigned i = 0; i < 12; ++i) {
        commands += m->commands[i]; if (i >= RMC_FLOW_RECOVERY) { sporting += m->commands[i]; }
    }
    for (unsigned i = 0; i < RMC_EVENT_TYPE_COUNT; ++i) { events += m->events[i]; }
    if (commands != f->journal.count || events != f->log.count || sporting != f->sporting_count ||
        m->events[RMC_EVENT_SHOT] != m->commands[RMC_FLOW_SHOT] || m->chances != m->events[RMC_EVENT_SHOT] ||
        m->events[RMC_EVENT_ACTION] != sporting - m->commands[RMC_FLOW_RECOVERY] ||
        m->team[0].goals != f->state.home_goals || m->team[1].goals != f->state.away_goals ||
        m->events[RMC_EVENT_GOAL] != m->team[0].goals + m->team[1].goals ||
        m->events[RMC_EVENT_PERIOD_START] != 2 || m->events[RMC_EVENT_PERIOD_END] != 2 ||
        m->events[RMC_EVENT_MATCH_END] != 1) { return false; }
    for (unsigned s = 0; s < 2; ++s) {
        const TeamStats *t = &m->team[s];
        uint64_t shots = 0, recovery = 0, goals = 0, receipts = 0, control = 0, pshots = 0;
        for (unsigned i = 0; i < 6; ++i) { shots += t->outcomes[i]; }
        for (unsigned i = 0; i < 4; ++i) { recovery += t->recovery[i][0] + t->recovery[i][1]; }
        for (unsigned i = 0; i < 11; ++i) {
            goals += t->players[i].goals; receipts += t->players[i].receipts;
            control += t->players[i].controlled; pshots += t->players[i].shots;
        }
        if (shots != t->shots || pshots != shots || goals != t->goals || t->outcomes[0] != goals ||
            receipts != t->completed || t->passes != t->completed + t->failed || recovery != t->recoveries ||
            t->tackles != t->tackle[0] + t->tackle[1] + t->tackle[2] || control > sporting) { return false; }
    }
    return m->commands[RMC_FLOW_TACKLE] == m->team[0].tackles + m->team[1].tackles &&
        m->commands[RMC_FLOW_PASS] == m->team[0].passes + m->team[1].passes &&
        m->commands[RMC_FLOW_RECOVERY] == m->team[0].recoveries + m->team[1].recoveries;
}

static RmcFlowInstructions triple(unsigned bits)
{
    return (RmcFlowInstructions){(RmcFlowApproach)(bits & 1),
        (RmcFlowProgression)((bits >> 1) & 1), (RmcFlowPressure)((bits >> 2) & 1)};
}

static bool execute(Run *run, Run *replay, const RmcCareerFoundation *foundation,
    const RmcFixture *fixture, const RmcLineup lineups[2], const Scenario *s,
    unsigned selector, uint64_t operation_limit, Metrics *m, Attempt *a)
{
    *a = (Attempt){.stage = "init", .replay = replay == NULL ? "none" : "NA"};
    const RmcMatchFlowConfig config = {RMC_MATCH_FLOW_VERSION, selector,
        s->opener == 0 ? fixture->home_team : fixture->away_team, operation_limit};
    RmcFlowError err = rmc_match_flow_init(&foundation->world, fixture, &lineups[0], &lineups[1], &config,
        run->events, RMC_FLOW_DEFAULT_EVENT_CAPACITY, run->commands, RMC_FLOW_DEFAULT_COMMAND_CAPACITY, &run->flow);
    if (err != RMC_FLOW_OK) { a->error = (unsigned)err; return false; }
    RmcMatchFlow *f = &run->flow;
    RmcPcg32 initial;
    if (!rmc_stream_init(&initial, &foundation->world.identity, RMC_STREAM_MATCH, fixture->id.value) ||
        initial.state != f->match_rng.state || initial.increment != f->match_rng.increment || !init_metrics(f, m)) {
        a->error = 100; return false;
    }
    for (uint64_t guard = 0; guard <= RMC_FLOW_DEFAULT_OPERATION_LIMIT; ++guard) {
        RmcMatchFlowNext next;
        a->stage = "next";
        err = rmc_match_flow_next(f, &next);
        if (err != RMC_FLOW_OK) { a->error = (unsigned)err; return false; }
        if (next.kind == RMC_FLOW_COMPLETE) { break; }
        if (guard == RMC_FLOW_DEFAULT_OPERATION_LIMIT) { a->stage = "guard"; a->error = 101; return false; }
        RmcMatchFlowCommand c;
        a->stage = "select";
        if (next.kind == RMC_FLOW_NEED_INSTRUCTION) {
            const unsigned side = next.team_id.value == fixture->home_team.value ? 0 : 1;
            const bool keep = f->state.period == RMC_MATCH_HALF_TIME;
            c = (RmcMatchFlowCommand){.command_id = next.observed_command_count + 1,
                .observed_command_count = next.observed_command_count, .match_id = fixture->id,
                .expected_period = f->state.period, .kind = RMC_FLOW_INSTRUCTION,
                .data.instruction = {next.team_id, f->state.period, keep, keep ? (RmcFlowInstructions){0} : triple(s->tactics[side])}};
        } else {
            err = rmc_match_flow_select(f, &next, &c);
            if (err != RMC_FLOW_OK) { a->error = (unsigned)err; return false; }
        }
        RmcMatchFlowStepResult step;
        const uint32_t previous_second = f->state.elapsed_seconds;
        a->stage = "apply";
        err = rmc_match_flow_apply(f, &c, &step);
        if (err != RMC_FLOW_OK) { a->error = (unsigned)err; return false; }
        a->stage = "reduce";
        if ((c.kind >= RMC_FLOW_RECOVERY && f->state.elapsed_seconds <= previous_second) ||
            !observe(f, &f->journal.commands[f->journal.count - 1], &step, m)) { a->error = 102; return false; }
        if (step.entered_half_time) {
            if (a->half_seen) { a->error = 103; return false; }
            a->half_seen = true; a->half = *f;
        }
    }
    a->stage = "reconcile";
    RmcMatchFlowStatus status;
    if (!a->half_seen || rmc_match_flow_status(f, &status) != RMC_FLOW_OK || !status.complete ||
        !reconciled(f, m)) { a->error = 104; return false; }
    if (replay != NULL) {
        a->stage = "replay"; a->replay = "fail";
        err = rmc_match_flow_init(&foundation->world, fixture, &lineups[0], &lineups[1], &config,
            replay->events, RMC_FLOW_DEFAULT_EVENT_CAPACITY, replay->commands, RMC_FLOW_DEFAULT_COMMAND_CAPACITY, &replay->flow);
        if (err == RMC_FLOW_OK) { err = rmc_match_flow_replay(&replay->flow, f->journal.commands, a->half.journal.count); }
        if (err != RMC_FLOW_OK) { a->error = (unsigned)err; return false; }
        if (!same_flow(&a->half, &replay->flow)) { a->error = 105; return false; }
        err = rmc_match_flow_replay(&replay->flow, &f->journal.commands[a->half.journal.count],
            f->journal.count - a->half.journal.count);
        if (err != RMC_FLOW_OK) { a->error = (unsigned)err; return false; }
        if (!same_flow(f, &replay->flow)) { a->error = 106; return false; }
        a->replay = "pass";
    }
    a->complete = true; a->stage = "complete";
    return true;
}

static void write_report(Reports *r, unsigned file, const char *format, ...)
{
    if (r->file[file] == NULL || ferror(r->file[file])) { r->failed = true; return; }
    va_list args;
    va_start(args, format);
    if (vfprintf(r->file[file], format, args) < 0) { r->failed = true; }
    va_end(args);
}

static void numbers(Reports *r, unsigned file, const uint64_t *values, size_t n)
{
    for (size_t i = 0; i < n; ++i) { write_report(r, file, ",%" U, values[i]); }
}
#define NUMS(r, file, ...) numbers((r), (file), (const uint64_t[]){__VA_ARGS__}, \
    sizeof((const uint64_t[]){__VA_ARGS__}) / sizeof(uint64_t))

static void available_numbers(Reports *r, unsigned file, bool available, const uint64_t *values, size_t n)
{
    if (available) { numbers(r, file, values, n); }
    else { for (size_t i = 0; i < n; ++i) { write_report(r, file, ",NA"); } }
}
#define OBS(r, file, available, ...) available_numbers((r), (file), (available), \
    (const uint64_t[]){__VA_ARGS__}, sizeof((const uint64_t[]){__VA_ARGS__}) / sizeof(uint64_t))

static bool close_report_files(Reports *r, unsigned count)
{
    for (unsigned i = 0; i < count; ++i) {
        if (r->file[i] != NULL) {
            if (ferror(r->file[i])) { r->failed = true; }
            if (fclose(r->file[i]) != 0) { r->failed = true; }
            r->file[i] = NULL;
        }
    }
    return !r->failed;
}

static bool close_reports(Reports *r) { return close_report_files(r, 7); }

static bool open_reports(Reports *r, const char *prefix)
{
    static const char *const suffixes[] = {"manifest.csv", "matches.csv", "team-matches.csv",
        "players.csv", "shot-bins.csv", "recovery-bins.csv", "summary.txt"};
    static const char *const headers[] = {
        "schema_version,world_ordinal,world_seed,generator_version,rules_version,flow_version,event_schema_version,shot_model_version,stream_version,selectors,matrix,half_time,replay,fixtures,command_capacity,event_capacity\n",
        KEY ",fixture_index,matchday,scenario_k,leg,home_team_id,away_team_id,home_formation,away_formation,home_tactics,away_tactics,opener_team_id,status,stage,error_code,accepted_commands,sporting_commands,events,home_goals,away_goals,goal_events,shot_events,chance_events,pass_attempts,pass_completed,pass_failed,tackle_attempts,tackle_retained,tackle_won,tackle_loose,recoveries,half_time_commands,half_home_goals,half_away_goals,half_rng_state,half_rng_increment,final_rng_state,final_rng_increment,replay,period,mode,terminal_reason,elapsed_seconds",
        KEY ",side,team_id,formation,tactics,goals,shots,shot_501,shot_502,shot_503,shot_504,shot_505,shot_506,pass_attempts,pass_completed,pass_failed,tackle_attempts,tackle_retained,tackle_won,tackle_loose,recoveries,participants_num,participants_den,zero_players,controlled_actions,top_one_num,top_one_den,top_two_num,top_two_den,pass_success_num,pass_success_den,conversion_num,conversion_den,receipts_gk,receipts_def,receipts_mid,receipts_att,controlled_gk,controlled_def,controlled_mid,controlled_att,goals_gk,goals_def,goals_mid,goals_att\n",
        KEY ",side,team_id,lineup_index,player_id,slot,department,participations,controlled_actions,pass_receptions,goals,shots\n",
        KEY ",side,team_id,outcome,sector,lane,quality,trajectory,count,shots_den\n",
        KEY ",side,team_id,source,outcome,count,recoveries_den\n"
    };
    char path[PATH_LIMIT + 40];
    for (unsigned i = 0; i < 7; ++i) {
        const int n = snprintf(path, sizeof(path), "%s-%s", prefix, suffixes[i]);
        if (n < 0 || (size_t)n >= sizeof(path) || (r->file[i] = fopen(path, "wb")) == NULL) {
            r->failed = true; return false;
        }
        if (i < 6) { write_report(r, i, "%s", headers[i]); }
    }
    for (unsigned i = 0; i < 12; ++i) { write_report(r, 1, ",cmd_%u", i); }
    for (unsigned i = 0; i < RMC_EVENT_TYPE_COUNT; ++i) { write_report(r, 1, ",evt_%u", i); }
    write_report(r, 1, "\n");
    return !r->failed;
}

static void key(Reports *r, unsigned file, size_t ordinal, uint64_t seed, RmcMatchId match, unsigned selector)
{
    write_report(r, file, "%" U ",%" U ",%" U ",%u", (uint64_t)ordinal, seed, match.value, selector);
}

static unsigned formation_number(unsigned f)
{
    static const unsigned forms[] = {442, 433, 352};
    return forms[f];
}

static void match_row(Reports *r, size_t ordinal, uint64_t seed, const RmcFixture *fixture,
    size_t index, unsigned selector, const Scenario *s, const RmcMatchFlow *f, const Metrics *m, const Attempt *a)
{
    key(r, 1, ordinal, seed, fixture->id, selector);
    NUMS(r, 1, index, fixture->matchday, s->k, s->leg, fixture->home_team.value, fixture->away_team.value,
        formation_number(s->formation[0]), formation_number(s->formation[1]), s->tactics[0], s->tactics[1],
        s->opener == 0 ? fixture->home_team.value : fixture->away_team.value);
    write_report(r, 1, ",%s,%s", a->complete ? "complete" : "incomplete", a->stage);
    NUMS(r, 1, a->error, f->journal.count, f->sporting_count, f->log.count);
    const TeamStats *h = &m->team[0], *v = &m->team[1];
    OBS(r, 1, a->complete, f->state.home_goals, f->state.away_goals,
        h->goals + v->goals, h->shots + v->shots, m->chances,
        h->passes + v->passes, h->completed + v->completed, h->failed + v->failed,
        h->tackles + v->tackles, h->tackle[0] + v->tackle[0], h->tackle[1] + v->tackle[1], h->tackle[2] + v->tackle[2],
        h->recoveries + v->recoveries, a->half.journal.count, a->half.state.home_goals, a->half.state.away_goals,
        a->half.match_rng.state, a->half.match_rng.increment, f->match_rng.state, f->match_rng.increment);
    write_report(r, 1, ",%s", a->replay);
    NUMS(r, 1, f->state.period, f->state.mode, f->state.terminal_reason, f->state.elapsed_seconds);
    available_numbers(r, 1, a->complete, m->commands, 12);
    available_numbers(r, 1, a->complete, m->events, RMC_EVENT_TYPE_COUNT);
    write_report(r, 1, "\n");
}

static void ratio_columns(Reports *r, unsigned file, uint64_t numerator, uint64_t denominator)
{
    if (denominator == 0) { write_report(r, file, ",NA,0"); }
    else { NUMS(r, file, numerator, denominator); }
}

static void concentration(const TeamStats *t, uint64_t *participants, uint64_t *total, uint64_t top[2])
{
    *participants = 0; *total = 0; top[0] = 0; top[1] = 0;
    for (unsigned i = 0; i < 11; ++i) {
        const PlayerStats *p = &t->players[i];
        *participants += p->participations != 0; *total += p->controlled;
        if (p->controlled >= top[0]) { top[1] = top[0]; top[0] = p->controlled; }
        else if (p->controlled > top[1]) { top[1] = p->controlled; }
    }
}

static void detail_rows(Reports *r, size_t ordinal, uint64_t seed, const RmcMatchFlow *f, const Scenario *s,
    const Metrics *m)
{
    const unsigned selector = f->config.selector_version;
    const RmcMatchId match = f->state.fixture.id;
    for (unsigned side = 0; side < 2; ++side) {
        const TeamStats *t = &m->team[side];
        const RmcTeamId team = side == 0 ? f->state.fixture.home_team : f->state.fixture.away_team;
        const RmcLineup *lineup = side == 0 ? &f->state.home_lineup : &f->state.away_lineup;
        uint64_t participants, total, top[2], departments[3][4] = {{0}};
        concentration(t, &participants, &total, top);
        for (unsigned i = 0; i < 11; ++i) {
            const PlayerStats *p = &t->players[i];
            const unsigned d = (unsigned)p->player->department;
            departments[0][d] += p->receipts; departments[1][d] += p->controlled; departments[2][d] += p->goals;
            key(r, 3, ordinal, seed, match, selector);
            NUMS(r, 3, side, team.value, i, p->player->id.value, lineup->slots[i], d,
                p->participations, p->controlled, p->receipts, p->goals, p->shots);
            write_report(r, 3, "\n");
        }
        key(r, 2, ordinal, seed, match, selector);
        NUMS(r, 2, side, team.value, formation_number(s->formation[side]), s->tactics[side], t->goals, t->shots);
        numbers(r, 2, t->outcomes, 6);
        NUMS(r, 2, t->passes, t->completed, t->failed, t->tackles, t->tackle[0], t->tackle[1], t->tackle[2],
            t->recoveries, participants, 11, 11 - participants, total);
        ratio_columns(r, 2, top[0], total); ratio_columns(r, 2, top[0] + top[1], total);
        ratio_columns(r, 2, t->completed, t->passes); ratio_columns(r, 2, t->goals, t->shots);
        for (unsigned i = 0; i < 3; ++i) { numbers(r, 2, departments[i], 4); }
        write_report(r, 2, "\n");
        for (unsigned outcome = 0; outcome < 6; ++outcome) {
            for (unsigned sector = 0; sector < 3; ++sector) {
                for (unsigned lane = 0; lane < 3; ++lane) {
                    for (unsigned quality = 0; quality <= 100; ++quality) {
                        for (unsigned trajectory = 0; trajectory < 3; ++trajectory) {
                            const uint64_t count = t->shot[outcome][sector][lane][quality][trajectory];
                            if (count == 0) { continue; }
                            key(r, 4, ordinal, seed, match, selector);
                            NUMS(r, 4, side, team.value, outcome + 501, sector + 2, lane, quality, trajectory + 1, count, t->shots);
                            write_report(r, 4, "\n");
                        }
                    }
                }
            }
        }
        for (unsigned source = 0; source < 4; ++source) {
            for (unsigned outcome = 0; outcome < 2; ++outcome) {
                if (t->recovery[source][outcome] == 0) { continue; }
                key(r, 5, ordinal, seed, match, selector);
                NUMS(r, 5, side, team.value, source, outcome + 1, t->recovery[source][outcome], t->recoveries);
                write_report(r, 5, "\n");
            }
        }
    }
}

static bool add(uint64_t *destination, uint64_t value)
{
    if (value > UINT64_MAX - *destination) { return false; }
    *destination += value;
    return true;
}

static bool aggregate(Campaign *campaign, const RmcMatchFlow *f, const Metrics *m, bool replay)
{
    Campaign next = *campaign;
    Totals *t = &next.selector[f->config.selector_version - 1];
    const uint64_t goals = m->team[0].goals + m->team[1].goals;
    if (!add(&next.completed, 1) || !add(&t->matches, 1) || !add(&t->goals, goals) ||
        !add(&t->shots, m->team[0].shots + m->team[1].shots) || !add(&t->scoring, goals != 0) ||
        !add(&t->draws, m->team[0].goals == m->team[1].goals) ||
        !add(&t->commands, f->journal.count) || !add(&t->events, f->log.count) || !add(&t->replay, replay)) { return false; }
    *campaign = next;
    return true;
}

/* Single publication boundary: incomplete attempts never reach detail/aggregate. */
static bool publish(Reports *r, Campaign *campaign, size_t ordinal, uint64_t seed,
    const RmcFixture *fixture, size_t index, unsigned selector, const Scenario *s,
    const RmcMatchFlow *f, const Metrics *m, const Attempt *a)
{
    if (!add(&campaign->attempted, 1)) { return false; }
    match_row(r, ordinal, seed, fixture, index, selector, s, f, m, a);
    if (!a->complete) {
        return add(&campaign->incomplete, 1) &&
            add(&campaign->replay_failures, strcmp(a->replay, "fail") == 0) && !r->failed;
    }
    detail_rows(r, ordinal, seed, f, s, m);
    return !r->failed && aggregate(campaign, f, m, strcmp(a->replay, "pass") == 0);
}

static void summary_ratio(Reports *r, const char *name, uint64_t n, uint64_t d)
{
    if (d == 0) { write_report(r, 6, "%s=NA/0\n", name); }
    else { write_report(r, 6, "%s=%" U "/%" U "\n", name, n, d); }
}

static void summary(Reports *r, const Campaign *c, bool success, bool replay)
{
    write_report(r, 6, "Sporting Balance Lab schema=1\n"
        "Exploratory observations only; no completed calibration or statistical qualification claim.\n"
        "Proposed targets (not acceptance results): goals 1.8-4.0; shots 12-36; scoring 65-95%%; draws 15-40%%.\n"
        "Held-out paired 20160-execution campaign is separate and is not certified by this report.\n"
        "campaign=%s\nattempted=%" U "\ncompleted=%" U "\nincomplete=%" U "\n",
        success ? "complete" : "failed", c->attempted, c->completed, c->incomplete);
    write_report(r, 6, "replay_requested_attempts=%" U "\nreplay_failures=%" U "\n",
        replay ? c->attempted : 0, c->replay_failures);
    for (unsigned i = 0; i < 2; ++i) {
        const Totals *t = &c->selector[i];
        write_report(r, 6, "selector=%u completed=%" U " commands=%" U " events=%" U " replay_verified=%" U "\n",
            i + 1, t->matches, t->commands, t->events, t->replay);
        summary_ratio(r, "mean_goals", t->goals, t->matches);
        summary_ratio(r, "mean_shots", t->shots, t->matches);
        summary_ratio(r, "scoring_matches", t->scoring, t->matches);
        summary_ratio(r, "draws", t->draws, t->matches);
    }
    write_report(r, 6, "paired_complete=%" U "\npaired_v1_goals=%" U "\npaired_v2_goals=%" U
        "\npaired_v1_shots=%" U "\npaired_v2_shots=%" U "\n",
        c->pairs, c->paired_goals[0], c->paired_goals[1], c->paired_shots[0], c->paired_shots[1]);
    write_report(r, 6, "event_size=%zu\ncommand_size=%zu\nevent_buffer_bytes=%zu\ncommand_buffer_bytes=%zu\nbuffer_pairs=%u\n"
        "Replay compares every event/cause, typed journal, half-time/final state, metadata and RNG state/increment.\n"
        "No portable replay or hash claim. Ratios are exact numerator/denominator; NA means zero denominator.\n",
        sizeof(RmcMatchEvent), sizeof(RmcMatchFlowCommand),
        RMC_FLOW_DEFAULT_EVENT_CAPACITY * sizeof(RmcMatchEvent),
        RMC_FLOW_DEFAULT_COMMAND_CAPACITY * sizeof(RmcMatchFlowCommand), replay ? 2u : 1u);
}

static void diagnostic(uint64_t seed, RmcMatchId fixture, unsigned selector, const Attempt *a, const RmcMatchFlow *f)
{
    fprintf(stderr, "seed=%" U " fixture=%" U " selector=%u stage=%s error=%u accepted_commands=%zu accepted_events=%zu second=%u\n",
        seed, fixture.value, selector, a->stage, a->error, f->journal.count, f->log.count, f->state.elapsed_seconds);
}

static bool chosen_fixture(const RmcCareerFoundation *f, const Options *o)
{
    if (o->all) { return true; }
    for (size_t i = 0; i < RMC_CALENDAR_FIXTURES; ++i) {
        if (f->calendar.fixtures[i].id.value == o->fixture_id) { return true; }
    }
    return false;
}

static int balance_main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) { help(); return fflush(stdout) == 0 && !ferror(stdout) ? 0 : 1; }
    Options o = {0};
    if (!options(argc, argv, &o)) { fputs("Invalid options; use --help.\n", stderr); return 2; }
    if (SEED_LIMIT > SIZE_MAX / sizeof(uint64_t)) { return 1; }
    uint64_t *seeds = malloc(SEED_LIMIT * sizeof(*seeds));
    if (seeds == NULL) { fputs("Seed storage allocation failed.\n", stderr); return 1; }
    size_t seed_count = 0;
    if (!seed_manifest(o.seeds, seeds, &seed_count)) {
        fputs("Invalid/unreadable seed manifest (empty, duplicate, malformed, overflow or limit).\n", stderr);
        free(seeds); return 2;
    }
    Run live = {0}, replay = {0};
    Metrics *metrics = calloc(1, sizeof(*metrics));
    RmcCareerFoundation *foundation = malloc(sizeof(*foundation));
    Reports reports = {0}; Campaign campaign = {0};
    int result = 1;
    uint64_t current_seed = seeds[0];
    RmcMatchId current_fixture = {0}; unsigned current_selector = 0;
    Attempt attempt = {.stage = "init", .replay = "NA"};
    if (metrics == NULL || foundation == NULL || !allocate_run(&live) || (o.replay && !allocate_run(&replay))) {
        fputs("Bounded campaign storage allocation failed.\n", stderr); goto done;
    }
    const RmcWorldIdentity first_identity = {seeds[0], RMC_GENERATOR_VERSION, RMC_RULES_VERSION};
    if (!rmc_career_foundation_generate(&first_identity, foundation) || !rmc_career_foundation_valid(foundation)) {
        attempt.error = 110; diagnostic(current_seed, current_fixture, 0, &attempt, &live.flow); goto done;
    }
    if (!chosen_fixture(foundation, &o)) { fputs("Unknown calendar fixture MatchId.\n", stderr); result = 2; goto done; }
    if (!open_reports(&reports, o.prefix)) { fputs("Output open/header failure.\n", stderr); goto done; }
    for (size_t s = 0; s < seed_count; ++s) {
        write_report(&reports, 0, "1,%" U ",%" U ",1,1,1,2,1,1,%s,balanced-v1,keep,%s,%s,%u,%u\n",
            (uint64_t)s, seeds[s], o.selector_text, o.replay ? "all" : "none", o.all ? "all" : o.fixture_text,
            RMC_FLOW_DEFAULT_COMMAND_CAPACITY, RMC_FLOW_DEFAULT_EVENT_CAPACITY);
    }
    if (reports.failed) { goto campaign_done; }
    for (size_t s = 0; s < seed_count; ++s) {
        current_seed = seeds[s]; current_fixture.value = 0; current_selector = 0;
        if (s != 0) {
            const RmcWorldIdentity identity = {seeds[s], RMC_GENERATOR_VERSION, RMC_RULES_VERSION};
            if (!rmc_career_foundation_generate(&identity, foundation) || !rmc_career_foundation_valid(foundation)) {
                attempt = (Attempt){.stage = "init", .error = 110};
                diagnostic(current_seed, current_fixture, 0, &attempt, &(RmcMatchFlow){0}); goto campaign_done;
            }
        }
        if (!chosen_fixture(foundation, &o)) {
            fputs("Unknown calendar fixture MatchId in generated world.\n", stderr); goto campaign_done;
        }
        for (size_t i = 0; i < RMC_CALENDAR_FIXTURES; ++i) {
            const RmcFixture *fixture = &foundation->calendar.fixtures[i];
            if (!o.all && fixture->id.value != o.fixture_id) { continue; }
            current_fixture = fixture->id;
            Scenario sc;
            RmcLineup lineups[2];
            const RmcRoster *home = roster(&foundation->world, fixture->home_team);
            const RmcRoster *away = roster(&foundation->world, fixture->away_team);
            if (!scenario(foundation, s, i, &sc) || home == NULL || away == NULL ||
                !rmc_roster_lineup(home, (RmcFormation)sc.formation[0], &lineups[0]) ||
                !rmc_roster_lineup(away, (RmcFormation)sc.formation[1], &lineups[1])) {
                attempt = (Attempt){.stage = "init", .error = 111};
                diagnostic(current_seed, current_fixture, 0, &attempt, &(RmcMatchFlow){0}); goto campaign_done;
            }
            uint64_t goals[2] = {0}, shots[2] = {0};
            for (unsigned version = 1; version <= 2; ++version) {
                if ((o.selectors & (1u << (version - 1))) == 0) { continue; }
                current_selector = version;
                const bool ok = execute(&live, o.replay ? &replay : NULL, foundation, fixture, lineups, &sc,
                    version, 0, metrics, &attempt);
                /* init failure has no prefix belonging to this attempt. */
                const RmcMatchFlow *observed = strcmp(attempt.stage, "init") == 0 && !ok ? &(RmcMatchFlow){0} : &live.flow;
                if (!ok) { diagnostic(current_seed, current_fixture, version, &attempt, observed); }
                if (!publish(&reports, &campaign, s, seeds[s], fixture, i, version, &sc, observed, metrics, &attempt)) {
                    fputs("Report publication/aggregation failure.\n", stderr); goto campaign_done;
                }
                if (!ok) { goto campaign_done; }
                goals[version - 1] = metrics->team[0].goals + metrics->team[1].goals;
                shots[version - 1] = metrics->team[0].shots + metrics->team[1].shots;
            }
            if (o.selectors == 3) {
                if (!add(&campaign.pairs, 1) || !add(&campaign.paired_goals[0], goals[0]) ||
                    !add(&campaign.paired_goals[1], goals[1]) || !add(&campaign.paired_shots[0], shots[0]) ||
                    !add(&campaign.paired_shots[1], shots[1])) { goto campaign_done; }
            }
        }
    }
    result = 0;
campaign_done:
    /* Close CSV streams before claiming publication success in the summary.
     * In particular, a delayed buffered error must not leave campaign=complete. */
    if (!close_report_files(&reports, 6)) { result = 1; }
    summary(&reports, &campaign, result == 0 && !reports.failed, o.replay);
done:
    if (!close_reports(&reports)) {
        fputs("Output write/close failure; report files may end in a partial prefix.\n", stderr);
        attempt.stage = "output"; attempt.error = 112;
        diagnostic(current_seed, current_fixture, current_selector, &attempt, &live.flow); result = 1;
    }
    free_run(&live); free_run(&replay); free(metrics); free(foundation); free(seeds);
    return result;
}

#ifdef RMC_SBL_SELF_CHECK
#define REQUIRE(condition) do { if (!(condition)) { \
    fprintf(stderr, "Balance self-check line %d: %s\n", __LINE__, #condition); return false; } } while (0)

static unsigned oracle_player(const Metrics *m, RmcPlayerId id)
{
    for (unsigned s = 0; s < 2; ++s) {
        for (unsigned p = 0; p < 11; ++p) {
            if (m->team[s].players[p].player->id.value == id.value) { return s * 11 + p; }
        }
    }
    return 22;
}

/* Independent event-only receipts/scorers/tackles and command-only controllers.
 * No generated sporting history is edited or fed fabricated events. */
static bool oracle(const RmcMatchFlow *f, const Metrics *m, unsigned *coverage)
{
    uint64_t receipts[22] = {0}, goals[22] = {0}, shots[22] = {0}, controlled[22] = {0};
    uint64_t tackles[2][3] = {{0}}, recoveries[2][4][2] = {{{0}}};
    uint64_t passes = 0, failed = 0, chances = 0, shot_total = 0, goal_total = 0;
    for (size_t i = 0; i < f->log.count; ++i) {
        const RmcMatchEvent *e = &f->log.events[i];
        const unsigned p = oracle_player(m, e->primary_actor);
        if (e->type == RMC_EVENT_ACTION) {
            if (e->outcome_code == 1) {
                const unsigned recipient = oracle_player(m, e->secondary_actor);
                REQUIRE(recipient < 22); ++receipts[recipient]; ++passes; *coverage |= 1;
            } else if (e->outcome_code == 2) { ++failed; ++passes; *coverage |= 2; }
            else if (e->outcome_code >= 301 && e->outcome_code <= 303) {
                REQUIRE(p < 22); ++tackles[p / 11][e->outcome_code - 301];
                if (e->team_id.value != m->team[p / 11].players[p % 11].player->team_id.value) { *coverage |= 4; }
            } else if (e->outcome_code == 500) { ++chances; }
        } else if (e->type == RMC_EVENT_SHOT) {
            REQUIRE(p < 22); ++shots[p]; ++shot_total;
        } else if (e->type == RMC_EVENT_GOAL) {
            REQUIRE(p < 22); ++goals[p]; ++goal_total;
        } else if (e->type == RMC_EVENT_RECOVERY) {
            REQUIRE(p < 22 && e->cause_event_id > 0 && e->cause_event_id < e->id);
            const RmcMatchEvent *cause = &f->log.events[e->cause_event_id - 1];
            unsigned source = 99;
            if (cause->type == RMC_EVENT_SHOT) { source = 3; *coverage |= 32; }
            else if (cause->type == RMC_EVENT_ACTION && cause->outcome_code == 2) { source = 0; *coverage |= 8; }
            else if (cause->type == RMC_EVENT_ACTION && cause->outcome_code == 303) { source = 1; *coverage |= 16; }
            else if (cause->type == RMC_EVENT_ACTION && cause->outcome_code == 403) { source = 2; }
            REQUIRE(source < 4 && e->outcome_code >= 1 && e->outcome_code <= 2);
            ++recoveries[p / 11][source][e->outcome_code - 1];
        }
    }
    for (size_t i = 0; i < f->journal.count; ++i) {
        const RmcMatchFlowCommand *c = &f->journal.commands[i];
        RmcPlayerId id = {0};
        switch (c->kind) {
        case RMC_FLOW_PASS: id = c->data.pass.passer; break;
        case RMC_FLOW_CARRY: id = c->data.carry.carrier; break;
        case RMC_FLOW_DRIBBLE: id = c->data.dribble.dribbler; break;
        case RMC_FLOW_CROSS: id = c->data.cross.crosser; break;
        case RMC_FLOW_SHOT: id = c->data.shot.shooter; break;
        default: break;
        }
        if (id.value != 0) { const unsigned p = oracle_player(m, id); REQUIRE(p < 22); ++controlled[p]; }
    }
    REQUIRE(chances == shot_total && shot_total == m->commands[RMC_FLOW_SHOT]);
    REQUIRE(goal_total == (uint64_t)f->state.home_goals + f->state.away_goals);
    REQUIRE(passes == m->team[0].passes + m->team[1].passes && failed == m->team[0].failed + m->team[1].failed);
    for (unsigned s = 0; s < 2; ++s) {
        uint64_t zero = 0;
        for (unsigned p = 0; p < 11; ++p) {
            const PlayerStats *x = &m->team[s].players[p]; const unsigned index = s * 11 + p;
            REQUIRE(x->receipts == receipts[index] && x->goals == goals[index] &&
                x->shots == shots[index] && x->controlled == controlled[index]);
            if (x->participations == 0) { ++zero; *coverage |= 64; }
            REQUIRE((unsigned)x->player->department < 4);
        }
        uint64_t participants, total, top[2];
        concentration(&m->team[s], &participants, &total, top);
        REQUIRE(participants + zero == 11 && top[0] + top[1] <= total);
        for (unsigned j = 0; j < 3; ++j) { REQUIRE(tackles[s][j] == m->team[s].tackle[j]); }
        for (unsigned source = 0; source < 4; ++source) {
            for (unsigned outcome = 0; outcome < 2; ++outcome) {
                REQUIRE(recoveries[s][source][outcome] == m->team[s].recovery[source][outcome]);
            }
        }
    }
    return true;
}

static bool self_check(void)
{
    uint64_t v = UINT64_MAX;
    REQUIRE(!add(&v, 1) && v == UINT64_MAX);
    REQUIRE(decimal("18446744073709551615", &v) && v == UINT64_MAX);
    REQUIRE(!decimal("18446744073709551616", &v) && !decimal("-1", &v));
    /* Isolated reducer classification vectors include unreachable automatic CROSS.
     * These records are never submitted to production init/apply/replay. */
    const uint32_t codes[] = {2, 303, RMC_CROSS_OUTCOME_LOOSE_BALL, 503, 504, 506};
    const int sources[] = {0, 1, 2, 3, 3, 3};
    for (unsigned i = 0; i < 6; ++i) {
        const RmcMatchEvent e = {.type = i < 3 ? RMC_EVENT_ACTION : RMC_EVENT_SHOT, .outcome_code = codes[i]};
        REQUIRE(recovery_source(&e) == sources[i]);
    }
    REQUIRE(recovery_source(&(RmcMatchEvent){.type = RMC_EVENT_ACTION, .outcome_code = 500}) == -1);
    REQUIRE(recovery_source(&(RmcMatchEvent){.type = RMC_EVENT_SHOT, .outcome_code = 501}) == -1);
    RmcCareerFoundation *foundation = malloc(sizeof(*foundation));
    Metrics *m = calloc(1, sizeof(*m));
    Run live = {0}, replay = {0};
    REQUIRE(foundation != NULL && m != NULL && allocate_run(&live) && allocate_run(&replay));
    const RmcWorldIdentity identity = {UINT64_C(20261114), 1, 1};
    REQUIRE(rmc_career_foundation_generate(&identity, foundation) && rmc_career_foundation_valid(foundation));
    bool cells[1152] = {false};
    for (size_t s = 0; s < 288; ++s) {
        for (size_t p = 0; p < 28; ++p) {
            Scenario a, b;
            REQUIRE(scenario(foundation, s, p, &a) && scenario(foundation, s, p + 28, &b));
            const unsigned expected = (unsigned)((28 * s + p) % 1152);
            REQUIRE(a.k == expected && a.opener == expected % 2 && a.tactics[0] == (expected / 2) % 8 &&
                a.tactics[1] == (expected / 16) % 8 && a.formation[0] == (expected / 128) % 3 &&
                a.formation[1] == expected / 384);
            REQUIRE(a.k == b.k && a.opener != b.opener && a.tactics[0] == b.tactics[1] &&
                a.tactics[1] == b.tactics[0] && a.formation[0] == b.formation[1] && a.formation[1] == b.formation[0]);
            cells[a.k] = true;
        }
    }
    for (unsigned i = 0; i < 1152; ++i) { REQUIRE(cells[i]); }
    unsigned coverage = 0;
    for (unsigned index = 0; index <= 28; index += 28) {
        const RmcFixture *fixture = &foundation->calendar.fixtures[index];
        Scenario sc; RmcLineup lineups[2];
        REQUIRE(scenario(foundation, 0, index, &sc));
        const RmcRoster *r[] = {roster(&foundation->world, fixture->home_team), roster(&foundation->world, fixture->away_team)};
        REQUIRE(r[0] != NULL && r[1] != NULL);
        for (unsigned form = 0; form < 3; ++form) {
            for (unsigned side = 0; side < 2; ++side) { REQUIRE(rmc_roster_lineup(r[side], (RmcFormation)form, &lineups[side])); }
        }
        for (unsigned side = 0; side < 2; ++side) {
            REQUIRE(rmc_roster_lineup(r[side], (RmcFormation)sc.formation[side], &lineups[side]));
        }
        for (unsigned selector = 1; selector <= 2; ++selector) {
            Attempt a;
            REQUIRE(execute(&live, &replay, foundation, fixture, lineups, &sc, selector, 0, m, &a));
            REQUIRE(a.complete && a.half_seen && strcmp(a.replay, "pass") == 0 && oracle(&live.flow, m, &coverage));
            /* Comparator sensitivity, on read-only observations only. */
            RmcMatchFlow changed = replay.flow;
            changed.match_rng.increment ^= UINT64_C(2); REQUIRE(!same_flow(&live.flow, &changed));
            changed = replay.flow; changed.match_rng.state ^= UINT64_C(1); REQUIRE(!same_flow(&live.flow, &changed));
            changed = replay.flow; ++changed.state.home_goals; REQUIRE(!same_flow(&live.flow, &changed));
            changed = replay.flow; ++changed.reset_count; REQUIRE(!same_flow(&live.flow, &changed));
            RmcMatchEvent e = live.events[2]; ++e.cause_event_id; REQUIRE(!same_event(&live.events[2], &e));
            RmcMatchFlowCommand c = live.commands[0]; c.data.instruction.instructions.pressure = RMC_FLOW_PRESS_HIGH;
            REQUIRE(!same_command(&live.commands[0], &c));
        }
        /* Production failure after accepted prefix: no retries and no aggregate. */
        Attempt a;
        REQUIRE(!execute(&live, NULL, foundation, fixture, lineups, &sc, 1, 4, m, &a));
        REQUIRE(!a.complete && a.error == RMC_FLOW_RESOURCE_EXHAUSTED && strcmp(a.stage, "apply") == 0 &&
            live.flow.journal.count == 4 && live.flow.log.count == 3);
        Reports reports = {0}; Campaign campaign = {0};
        for (unsigned i = 0; i < 7; ++i) { reports.file[i] = tmpfile(); REQUIRE(reports.file[i] != NULL); }
        REQUIRE(publish(&reports, &campaign, 0, identity.world_seed, fixture, index, 1, &sc, &live.flow, m, &a));
        REQUIRE(campaign.attempted == 1 && campaign.completed == 0 && campaign.incomplete == 1 && campaign.selector[0].goals == 0);
        for (unsigned i = 2; i <= 5; ++i) { REQUIRE(ftell(reports.file[i]) == 0); }
        summary(&reports, &campaign, false, false);
        REQUIRE(fflush(reports.file[1]) == 0 && fseek(reports.file[1], 0, SEEK_SET) == 0);
        char line[4096]; REQUIRE(fgets(line, sizeof(line), reports.file[1]) != NULL && strstr(line, ",incomplete,apply,5,4,1,3,NA,NA,") != NULL);
        ratio_columns(&reports, 2, 0, 0);
        REQUIRE(fflush(reports.file[2]) == 0 && fseek(reports.file[2], 0, SEEK_SET) == 0);
        REQUIRE(fgets(line, sizeof(line), reports.file[2]) != NULL && strcmp(line, ",NA,0") == 0);
        REQUIRE(close_reports(&reports));
    }
    REQUIRE(coverage == 127); /* pass success/failure, tackle side, three causes, zero participants */
    free_run(&live); free_run(&replay); free(foundation); free(m);
    puts("Balance self-check PASS: four genuine complete matches; independent metric oracle; all scenario cells; semantic replay/RNG; incomplete-prefix exclusion; reducer vectors.");
    return true;
}
#endif

int main(int argc, char **argv)
{
#ifdef RMC_SBL_SELF_CHECK
    if (argc == 1) { return self_check() ? 0 : 1; }
#endif
    return balance_main(argc, argv);
}
