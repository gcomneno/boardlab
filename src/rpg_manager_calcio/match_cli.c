#include "match_cli.h"
#include "career.h"
#include "match_flow_selector.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

enum { LIVE_LIMIT = 80 };
enum { OPT_HELP, OPT_AUTO, OPT_VERIFY, OPT_SEED, OPT_TEAM, OPT_FIXTURE,
    OPT_HOME, OPT_AWAY, OPT_OPENER, OPT_PRE, OPT_HALF, OPT_COUNT };

typedef struct {
    FILE *in, *out, *err;
    bool io_failed;
} Console;

typedef struct {
    unsigned seen;
    uint64_t seed, team, fixture;
    RmcFormation home, away;
    bool away_opens, half_keep;
    RmcFlowInstructions pre, half;
} Options;

typedef struct {
    RmcMatchFlow flow;
    RmcMatchEvent *events;
    RmcMatchFlowCommand *commands;
} Run;

typedef struct {
    size_t goals[2], shots[2], outcomes[2][6];
    size_t recoveries[2][2], tackles[2];
    size_t scorers[2][RMC_LINEUP_PLAYERS];
    unsigned live_lines;
    bool suppressed;
} Statistics;

static void write_to(Console *c, FILE *stream, const char *format, ...)
{
    if (c->io_failed) { return; }
    va_list args;
    va_start(args, format);
    if (vfprintf(stream, format, args) < 0) { c->io_failed = true; }
    va_end(args);
}

static int finish(Console *c, int code)
{
    if (fflush(c->out) == EOF) { c->io_failed = true; }
    if (fflush(c->err) == EOF) { c->io_failed = true; }
    return c->io_failed ? 1 : code;
}

static bool has(const Options *o, unsigned flag)
{
    return (o->seen & (1u << flag)) != 0;
}

static bool number(const char *text, uint64_t *value)
{
    uint64_t n = 0;
    if (*text == '\0') { return false; }
    for (; *text != '\0'; ++text) {
        if (*text < '0' || *text > '9') { return false; }
        const unsigned digit = (unsigned)(*text - '0');
        if (n > (UINT64_MAX - digit) / 10) { return false; }
        n = n * 10 + digit;
    }
    *value = n;
    return true;
}

static bool formation(const char *text, RmcFormation *value)
{
    if (strcmp(text, "442") == 0) { *value = RMC_FORMATION_442; }
    else if (strcmp(text, "433") == 0) { *value = RMC_FORMATION_433; }
    else if (strcmp(text, "352") == 0) { *value = RMC_FORMATION_352; }
    else { return false; }
    return true;
}

static const char *formation_text(RmcFormation f)
{
    return f == RMC_FORMATION_442 ? "442" : f == RMC_FORMATION_433 ? "433" : "352";
}

static bool opener(const char *text, bool *away)
{
    if (strcmp(text, "home") == 0) { *away = false; }
    else if (strcmp(text, "away") == 0) { *away = true; }
    else { return false; }
    return true;
}

static bool triple(const char *a, const char *p, const char *r, RmcFlowInstructions *out)
{
    RmcFlowInstructions t;
    if (strcmp(a, "PATIENT") == 0) { t.approach = RMC_FLOW_PATIENT; }
    else if (strcmp(a, "DIRECT") == 0) { t.approach = RMC_FLOW_DIRECT; }
    else { return false; }
    if (strcmp(p, "CARRY_FIRST") == 0) { t.progression = RMC_FLOW_CARRY_FIRST; }
    else if (strcmp(p, "DRIBBLE_FIRST") == 0) { t.progression = RMC_FLOW_DRIBBLE_FIRST; }
    else { return false; }
    if (strcmp(r, "PRESS_NORMAL") == 0) { t.pressure = RMC_FLOW_PRESS_NORMAL; }
    else if (strcmp(r, "PRESS_HIGH") == 0) { t.pressure = RMC_FLOW_PRESS_HIGH; }
    else { return false; }
    *out = t;
    return true;
}

static void instructions_text(char *buffer, size_t size, RmcFlowInstructions t)
{
    /* All tokens are bounded literals; 64 bytes suffice for this triple. */
    (void)snprintf(buffer, size, "%s %s %s",
        t.approach == RMC_FLOW_PATIENT ? "PATIENT" : "DIRECT",
        t.progression == RMC_FLOW_CARRY_FIRST ? "CARRY_FIRST" : "DRIBBLE_FIRST",
        t.pressure == RMC_FLOW_PRESS_NORMAL ? "PRESS_NORMAL" : "PRESS_HIGH");
}

static bool parse(int argc, const char *const argv[], Options *o)
{
    static const char *const flags[OPT_COUNT] = {"--help", "--auto", "--verify-replay",
        "--seed", "--team", "--fixture", "--home-formation", "--away-formation",
        "--opener", "--pre", "--half"};
    *o = (Options){.seed = UINT64_C(20261114), .home = RMC_FORMATION_442,
        .away = RMC_FORMATION_433, .half_keep = true,
        .pre = {RMC_FLOW_PATIENT, RMC_FLOW_CARRY_FIRST, RMC_FLOW_PRESS_NORMAL}};
    for (int i = 1; i < argc; ++i) {
        unsigned f;
        for (f = 0; f < OPT_COUNT; ++f) {
            if (strcmp(argv[i], flags[f]) == 0) { break; }
        }
        if (f == OPT_COUNT || has(o, f)) { return false; }
        o->seen |= 1u << f;
        if (f <= OPT_VERIFY) { continue; }
        if (++i >= argc) { return false; }
        const char *v = argv[i];
        switch (f) {
        case OPT_SEED: if (!number(v, &o->seed)) { return false; } break;
        case OPT_TEAM: if (!number(v, &o->team) || o->team == 0) { return false; } break;
        case OPT_FIXTURE: if (!number(v, &o->fixture) || o->fixture == 0) { return false; } break;
        case OPT_HOME: if (!formation(v, &o->home)) { return false; } break;
        case OPT_AWAY: if (!formation(v, &o->away)) { return false; } break;
        case OPT_OPENER: if (!opener(v, &o->away_opens)) { return false; } break;
        case OPT_HALF:
            if (strcmp(v, "KEEP") == 0) { break; }
            o->half_keep = false;
            /* fall through */
        case OPT_PRE: {
            if (argc - i < 3) { return false; }
            RmcFlowInstructions *t = f == OPT_PRE ? &o->pre : &o->half;
            if (!triple(v, argv[i + 1], argv[i + 2], t)) { return false; }
            i += 2;
            break;
        }
        default: return false;
        }
    }
    return !has(o, OPT_HELP) || argc == 2;
}

static void help(Console *c)
{
    write_to(c, c->out,
        "Uso: rmc-match [--auto] [--verify-replay] [opzioni]\n"
        "     rmc-match --help\n"
        "  --seed UINT64             decimale 0..18446744073709551615; default 20261114\n"
        "  --team TEAM_ID            ID esistente; default minimo TeamId\n"
        "  --fixture MATCH_ID        calendario generato; default primo incontro della squadra\n"
        "  --home-formation 442|433|352   default 442\n"
        "  --away-formation 442|433|352   default 433\n"
        "  --opener home|away        default home\n"
        "  --pre APPROACH PROGRESSION PRESSURE\n"
        "  --half KEEP | APPROACH PROGRESSION PRESSURE\n"
        "  APPROACH: PATIENT | DIRECT (RMC_FLOW_PATIENT/RMC_FLOW_DIRECT)\n"
        "  PROGRESSION: CARRY_FIRST | DRIBBLE_FIRST (RMC_FLOW_CARRY_FIRST/RMC_FLOW_DRIBBLE_FIRST)\n"
        "  PRESSURE: PRESS_NORMAL | PRESS_HIGH (RMC_FLOW_PRESS_NORMAL/RMC_FLOW_PRESS_HIGH)\n"
        "  Default pre e avversario: PATIENT CARRY_FIRST PRESS_NORMAL; intervallo: KEEP.\n"
        "  Token esatti, sensibili alle maiuscole; opzioni in qualsiasi ordine, senza duplicati.\n"
        "  Senza --auto: richieste solo le scelte omesse. Invio accetta il default;\n"
        "  all'intervallo Invio significa KEEP. EOF annulla (codice 3).\n"
        "  --auto non legge stdin. Nessuna scelta di azioni individuali.\n"
        "  --verify-replay verifica il journal in memoria; nessun formato portabile/hash.\n"
        "  Codici: 0 completata/help, 1 errore runtime/risorse/I/O, 2 opzioni/scelta, 3 EOF.\n");
}

static const RmcTeam *team(const RmcWorldCore *w, uint64_t id)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (w->teams[i].id.value == id) { return &w->teams[i]; }
    }
    return NULL;
}

static const RmcRoster *roster(const RmcWorldCore *w, RmcTeamId id)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (w->rosters[i].team_id.value == id.value) { return &w->rosters[i]; }
    }
    return NULL;
}

static const RmcPlayer *player(const RmcWorldCore *w, RmcTeamId owner, RmcPlayerId id)
{
    const RmcRoster *r = roster(w, owner);
    if (r == NULL) { return NULL; }
    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        if (r->players[i].id.value == id.value && r->players[i].team_id.value == owner.value) {
            return &r->players[i];
        }
    }
    return NULL;
}

static const RmcFixture *fixture(const RmcCalendar *cal, uint64_t id, uint64_t managed)
{
    for (size_t i = 0; i < RMC_CALENDAR_FIXTURES; ++i) {
        const RmcFixture *f = &cal->fixtures[i];
        if ((id == 0 || f->id.value == id) &&
            (f->home_team.value == managed || f->away_team.value == managed)) { return f; }
    }
    return NULL;
}

/* 256-byte input, including NUL. Oversized lines are consumed once in full.
 * A final unterminated line reaching EOF is cancellation too. */
static int line(Console *c, const char *label, const char *def, char buffer[256])
{
    write_to(c, c->out, "%s [%s]: ", label, def);
    if (c->io_failed || fflush(c->out) == EOF) { c->io_failed = true; return 1; }
    if (fgets(buffer, 256, c->in) == NULL) { return ferror(c->in) ? 1 : 3; }
    char *newline = strchr(buffer, '\n');
    if (newline == NULL) {
        int ch;
        do { ch = fgetc(c->in); } while (ch != '\n' && ch != EOF);
        if (ferror(c->in)) { return 1; }
        if (ch == EOF) { return 3; }
        write_to(c, c->err, "Input troppo lungo (massimo 254 caratteri).\n");
        return c->io_failed ? 1 : 2;
    }
    *newline = '\0';
    return 0;
}

static size_t words(char *text, char *tokens[], size_t capacity)
{
    size_t n = 0;
    while (*text != '\0') {
        while (isspace((unsigned char)*text)) { ++text; }
        if (*text == '\0') { break; }
        if (n == capacity) { return capacity + 1; }
        tokens[n++] = text;
        while (*text != '\0' && !isspace((unsigned char)*text)) { ++text; }
        if (*text != '\0') { *text++ = '\0'; }
    }
    return n;
}

static int prompt(Console *c, Options *o, unsigned choice,
    const RmcCareerSetup *career)
{
    const char *label = "";
    char def[64];
    switch (choice) {
    case OPT_TEAM: label = "Squadra TeamId"; (void)snprintf(def, sizeof(def), "%" PRIu64, o->team); break;
    case OPT_FIXTURE: label = "Incontro MatchId"; (void)snprintf(def, sizeof(def), "%" PRIu64, o->fixture); break;
    case OPT_HOME: label = "Modulo casa (442/433/352)"; (void)snprintf(def, sizeof(def), "%s", formation_text(o->home)); break;
    case OPT_AWAY: label = "Modulo trasferta (442/433/352)"; (void)snprintf(def, sizeof(def), "%s", formation_text(o->away)); break;
    case OPT_OPENER: label = "Primo calcio d'inizio (home/away)"; (void)snprintf(def, sizeof(def), "%s", o->away_opens ? "away" : "home"); break;
    case OPT_PRE: label = "Tattica pre-partita (APPROACH PROGRESSION PRESSURE)"; instructions_text(def, sizeof(def), o->pre); break;
    case OPT_HALF: label = "Tattica intervallo (KEEP oppure APPROACH PROGRESSION PRESSURE)"; (void)snprintf(def, sizeof(def), "KEEP"); break;
    default: return 1;
    }
    for (;;) {
        char buffer[256], *tokens[3];
        int rc = line(c, label, def, buffer);
        if (rc == 2) { continue; }
        if (rc != 0) { return rc; }
        const size_t n = words(buffer, tokens, 3);
        if (n == 0) {
            if (choice == OPT_HALF) { o->half_keep = true; }
            return 0;
        }
        uint64_t id;
        bool valid = false;
        if (choice == OPT_TEAM && n == 1 && number(tokens[0], &id) && id != 0 &&
            team(&career->foundation.world, id) != NULL) { o->team = id; valid = true; }
        else if (choice == OPT_FIXTURE && n == 1 && number(tokens[0], &id) && id != 0 &&
            fixture(&career->foundation.calendar, id, o->team) != NULL) { o->fixture = id; valid = true; }
        else if (choice == OPT_HOME && n == 1) { valid = formation(tokens[0], &o->home); }
        else if (choice == OPT_AWAY && n == 1) { valid = formation(tokens[0], &o->away); }
        else if (choice == OPT_OPENER && n == 1) { valid = opener(tokens[0], &o->away_opens); }
        else if (choice == OPT_PRE && n == 3) { valid = triple(tokens[0], tokens[1], tokens[2], &o->pre); }
        else if (choice == OPT_HALF && n == 1 && strcmp(tokens[0], "KEEP") == 0) {
            o->half_keep = true; valid = true;
        } else if (choice == OPT_HALF && n == 3) {
            valid = triple(tokens[0], tokens[1], tokens[2], &o->half);
            if (valid) { o->half_keep = false; }
        }
        if (valid) { return 0; }
        write_to(c, c->err, "Scelta non valida: usare gli ID mostrati o i token di --help.\n");
        if (c->io_failed) { return 1; }
    }
}

static int setup(Console *c, Options *o, RmcCareerSetup *career, const RmcFixture **selected)
{
    const RmcWorldIdentity identity = {o->seed, RMC_GENERATOR_VERSION, RMC_RULES_VERSION};
    if (!rmc_career_setup_generate(&identity, career) || !rmc_career_setup_valid(career) ||
        !rmc_world_core_valid(&career->foundation.world) ||
        !rmc_calendar_valid(&career->foundation.world, &career->foundation.world.ids,
            &career->foundation.calendar)) { return 1; }
    const RmcWorldCore *w = &career->foundation.world;
    write_to(c, c->out, "Universo seed=%" PRIu64 " generator=%" PRIu32 " rules=%" PRIu32
        " flow=%" PRIu32 " selector=%" PRIu32 " schema=%" PRIu32 "\n",
        o->seed, RMC_GENERATOR_VERSION, RMC_RULES_VERSION, RMC_MATCH_FLOW_VERSION,
        RMC_MATCH_SELECTOR_VERSION, RMC_EVENT_SCHEMA_VERSION);
    uint64_t lowest = UINT64_MAX;
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        const RmcTeam *t = &w->teams[i];
        bool candidate = false;
        for (size_t j = 0; j < RMC_CANDIDATE_COUNT; ++j) {
            if (career->candidates.team_ids[j].value == t->id.value) { candidate = true; }
        }
        write_to(c, c->out, "Squadra #%" PRIu64 " %s%s\n", t->id.value, t->name,
            candidate ? " [candidata carriera]" : "");
        if (t->id.value < lowest) { lowest = t->id.value; }
    }
    write_to(c, c->out, "Selezione candidate: fallback=%s\n", career->candidates.fallback_used ? "si" : "no");
    if (!has(o, OPT_TEAM)) { o->team = lowest; }
    if (team(w, o->team) == NULL) { return 2; }
    if (!has(o, OPT_AUTO) && !has(o, OPT_TEAM)) {
        int rc = prompt(c, o, OPT_TEAM, career);
        if (rc != 0) { return rc; }
    }
    const RmcFixture *first = fixture(&career->foundation.calendar, 0, o->team);
    if (first == NULL) { return 2; }
    if (!has(o, OPT_FIXTURE)) { o->fixture = first->id.value; }
    for (size_t i = 0; i < RMC_CALENDAR_FIXTURES; ++i) {
        const RmcFixture *f = &career->foundation.calendar.fixtures[i];
        if (f->home_team.value == o->team || f->away_team.value == o->team) {
            write_to(c, c->out, "Calendario Match #%" PRIu64 " giornata %u: %s (#%" PRIu64 ") - %s (#%" PRIu64 ")\n",
                f->id.value, (unsigned)f->matchday, team(w, f->home_team.value)->name, f->home_team.value,
                team(w, f->away_team.value)->name, f->away_team.value);
        }
    }
    const unsigned choices[] = {OPT_FIXTURE, OPT_HOME, OPT_AWAY, OPT_OPENER, OPT_PRE};
    for (size_t i = 0; i < sizeof(choices) / sizeof(choices[0]); ++i) {
        if (c->io_failed) { return 1; }
        if (!has(o, OPT_AUTO) && !has(o, choices[i])) {
            int rc = prompt(c, o, choices[i], career);
            if (rc != 0) { return rc; }
        }
        if (choices[i] == OPT_FIXTURE && fixture(&career->foundation.calendar, o->fixture, o->team) == NULL) {
            return 2;
        }
    }
    *selected = fixture(&career->foundation.calendar, o->fixture, o->team);
    return c->io_failed ? 1 : 0;
}

static void *allocate(size_t count, size_t size)
{
    if (size == 0 || count > SIZE_MAX / size) { return NULL; }
    return malloc(count * size);
}

static void destroy(Run *r)
{
    free(r->events);
    free(r->commands);
}

static bool initialize(Run *r, const RmcWorldCore *w, const RmcFixture *f,
    const RmcLineup *home, const RmcLineup *away, const RmcMatchFlowConfig *config)
{
    r->events = allocate(RMC_FLOW_DEFAULT_EVENT_CAPACITY, sizeof(*r->events));
    if (r->events == NULL) { return false; }
    r->commands = allocate(RMC_FLOW_DEFAULT_COMMAND_CAPACITY, sizeof(*r->commands));
    if (r->commands == NULL) { return false; }
    return rmc_match_flow_init(w, f, home, away, config,
        r->events, RMC_FLOW_DEFAULT_EVENT_CAPACITY, r->commands,
        RMC_FLOW_DEFAULT_COMMAND_CAPACITY, &r->flow) == RMC_FLOW_OK;
}

static bool show_lineup(Console *c, const RmcWorldCore *w, RmcTeamId id, const RmcLineup *l)
{
    static const char *const slots[] = {"POR", "TD", "DC", "TS", "ED", "CC", "ES", "ATT", "MED", "AD", "AS"};
    static const char *const departments[] = {"portiere", "difensore", "centrocampista", "attaccante"};
    write_to(c, c->out, "Formazione %s (#%" PRIu64 ") %s\n", team(w, id.value)->name, id.value, formation_text(l->formation));
    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        const RmcPlayer *p = player(w, id, l->players[i]);
        if (p == NULL || (unsigned)p->department > RMC_DEPARTMENT_ATTACKER ||
            (unsigned)l->slots[i] >= RMC_SLOT_COUNT) { return false; }
        write_to(c, c->out, "  Player #%" PRIu64 " slot=%s reparto=%s\n", p->id.value,
            slots[l->slots[i]], departments[p->department]);
    }
    return !c->io_failed;
}

static bool live_line(Console *c, Statistics *s)
{
    if (s->live_lines < LIVE_LIMIT) { ++s->live_lines; return true; }
    if (!s->suppressed) {
        write_to(c, c->out, "Cronaca: limite di 80 righe raggiunto; statistiche complete a fine partita.\n");
        s->suppressed = true;
    }
    return false;
}

static int side_of(const RmcFixture *f, RmcTeamId id)
{
    if (id.value == f->home_team.value) { return 0; }
    if (id.value == f->away_team.value) { return 1; }
    return -1;
}

static size_t lineup_index(const RmcMatchFlow *f, int side, RmcPlayerId id)
{
    const RmcLineup *l = side == 0 ? &f->state.home_lineup : &f->state.away_lineup;
    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        if (l->players[i].value == id.value) { return i; }
    }
    return RMC_LINEUP_PLAYERS;
}

static bool events(Console *c, const RmcMatchFlow *f, Statistics *s, const RmcMatchFlowStepResult *step)
{
    static const char *const outcome[] = {"GOL", "parato e trattenuto", "parato con respinta", "bloccato", "WIDE (fuori)", "legno"};
    if (step->first_event_index > f->log.count ||
        step->appended_event_count != f->log.count - step->first_event_index) { return false; }
    for (size_t i = step->first_event_index; i < f->log.count; ++i) {
        const RmcMatchEvent *e = &f->log.events[i];
        int side = side_of(&f->state.fixture, e->team_id);
        const RmcTeam *t = side < 0 ? NULL : team(f->world, e->team_id.value);
        const char *message = NULL;
        if (e->type == RMC_EVENT_SHOT || e->type == RMC_EVENT_GOAL || e->type == RMC_EVENT_RECOVERY ||
            (e->type == RMC_EVENT_ACTION && e->outcome_code == RMC_TACKLE_OUTCOME_WON)) {
            if (t == NULL || player(f->world, e->team_id, e->primary_actor) == NULL ||
                lineup_index(f, side, e->primary_actor) == RMC_LINEUP_PLAYERS) { return false; }
        }
        if (e->type == RMC_EVENT_SHOT) {
            if (e->outcome_code < RMC_SHOT_GOAL || e->outcome_code > RMC_SHOT_WOODWORK) { return false; }
            ++s->shots[side];
            ++s->outcomes[side][e->outcome_code - RMC_SHOT_GOAL];
            if (live_line(c, s)) {
                write_to(c, c->out, "LIVE %02" PRIu32 ":%02" PRIu32 " %s (#%" PRIu64 ") Player #%" PRIu64 " tiro: %s\n",
                    e->at_second / 60, e->at_second % 60, t->name, t->id.value, e->primary_actor.value,
                    outcome[e->outcome_code - RMC_SHOT_GOAL]);
            }
        } else if (e->type == RMC_EVENT_GOAL) {
            ++s->goals[side];
            ++s->scorers[side][lineup_index(f, side, e->primary_actor)];
        } else if (e->type == RMC_EVENT_RECOVERY) {
            if (e->outcome_code != RMC_RECOVERY_RETAINED && e->outcome_code != RMC_RECOVERY_TURNOVER) { return false; }
            ++s->recoveries[side][e->outcome_code == RMC_RECOVERY_TURNOVER ? 1 : 0];
            message = e->outcome_code == RMC_RECOVERY_TURNOVER ? "recupera la palla contesa (cambio possesso)" : "recupera la palla contesa (possesso mantenuto)";
        } else if (e->type == RMC_EVENT_ACTION && e->outcome_code == RMC_TACKLE_OUTCOME_WON) {
            ++s->tackles[side]; message = "vince il contrasto";
        } else if (e->type == RMC_EVENT_PERIOD_START) {
            if (t == NULL) { return false; }
            if (live_line(c, s)) {
                write_to(c, c->out, "LIVE %02" PRIu32 ":%02" PRIu32 " Inizio tempo: apre %s (#%" PRIu64 ")\n",
                    e->at_second / 60, e->at_second % 60, t->name, t->id.value);
            }
        } else if (e->type == RMC_EVENT_PERIOD_END || e->type == RMC_EVENT_MATCH_END) {
            if (live_line(c, s)) {
                write_to(c, c->out, "LIVE %02" PRIu32 ":%02" PRIu32 " %s\n",
                    e->at_second / 60, e->at_second % 60,
                    e->type == RMC_EVENT_PERIOD_END ? "Fine tempo" : "Fischio finale");
            }
        }
        if (message != NULL && live_line(c, s)) {
            write_to(c, c->out, "LIVE %02" PRIu32 ":%02" PRIu32 " %s (#%" PRIu64 ") Player #%" PRIu64 " %s\n",
                e->at_second / 60, e->at_second % 60, t->name, t->id.value, e->primary_actor.value, message);
        }
        if (c->io_failed) { return false; }
    }
    return true;
}

static bool scores_match(const RmcMatchFlow *f, const Statistics *s)
{
    return s->goals[0] == f->state.home_goals && s->goals[1] == f->state.away_goals &&
        s->goals[0] == s->outcomes[0][0] && s->goals[1] == s->outcomes[1][0];
}

static void score(Console *c, const RmcMatchFlow *f, const char *label)
{
    write_to(c, c->out, "%s: %s (#%" PRIu64 ") %u - %u %s (#%" PRIu64 ")\n", label,
        team(f->world, f->state.fixture.home_team.value)->name, f->state.fixture.home_team.value,
        (unsigned)f->state.home_goals, (unsigned)f->state.away_goals,
        team(f->world, f->state.fixture.away_team.value)->name, f->state.fixture.away_team.value);
}

static void summary(Console *c, const RmcMatchFlow *f, const Statistics *s)
{
    for (int side = 0; side < 2; ++side) {
        const RmcTeamId id = side == 0 ? f->state.fixture.home_team : f->state.fixture.away_team;
        const RmcLineup *l = side == 0 ? &f->state.home_lineup : &f->state.away_lineup;
        write_to(c, c->out, "Statistiche %s (#%" PRIu64 "): tiri=%zu gol=%zu parati-trattenuti=%zu "
            "parati-respinti=%zu bloccati=%zu WIDE=%zu legni=%zu; recuperi-palla-contesa=%zu "
            "(mantenuti=%zu cambi-possesso=%zu); contrasti-vinti=%zu\n",
            team(f->world, id.value)->name, id.value, s->shots[side], s->goals[side],
            s->outcomes[side][1], s->outcomes[side][2], s->outcomes[side][3], s->outcomes[side][4],
            s->outcomes[side][5], s->recoveries[side][0] + s->recoveries[side][1],
            s->recoveries[side][0], s->recoveries[side][1], s->tackles[side]);
        write_to(c, c->out, "Marcatori %s (#%" PRIu64 "):", team(f->world, id.value)->name, id.value);
        if (s->goals[side] == 0) { write_to(c, c->out, " nessuno"); }
        for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
            if (s->scorers[side][i] != 0) {
                write_to(c, c->out, " Player #%" PRIu64 " (%zu)", l->players[i].value, s->scorers[side][i]);
            }
        }
        write_to(c, c->out, "\n");
    }
    write_to(c, c->out, "Controllo gol eventi/score: PASS\n");
}

static bool same_instructions(RmcFlowInstructions a, RmcFlowInstructions b)
{
    return a.approach == b.approach && a.progression == b.progression && a.pressure == b.pressure;
}

static bool same_state(const RmcMatchState *a, const RmcMatchState *b)
{
    if (a->fixture.id.value != b->fixture.id.value || a->fixture.matchday != b->fixture.matchday ||
        a->fixture.home_team.value != b->fixture.home_team.value || a->fixture.away_team.value != b->fixture.away_team.value ||
        a->period != b->period || a->mode != b->mode || a->terminal_reason != b->terminal_reason ||
        a->elapsed_seconds != b->elapsed_seconds || a->second_half_start_second != b->second_half_start_second ||
        a->first_half_stoppage_seconds != b->first_half_stoppage_seconds || a->second_half_stoppage_seconds != b->second_half_stoppage_seconds ||
        a->home_goals != b->home_goals || a->away_goals != b->away_goals) { return false; }
    const RmcLineup *x[] = {&a->home_lineup, &a->away_lineup};
    const RmcLineup *y[] = {&b->home_lineup, &b->away_lineup};
    for (size_t side = 0; side < 2; ++side) {
        if (x[side]->formation != y[side]->formation || x[side]->secondary_assignments != y[side]->secondary_assignments) { return false; }
        for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
            if (x[side]->players[i].value != y[side]->players[i].value || x[side]->slots[i] != y[side]->slots[i]) { return false; }
        }
    }
    const RmcMatchPossession *p = &a->possession, *q = &b->possession;
    return p->team_id.value == q->team_id.value && p->initiator.value == q->initiator.value &&
        p->controller.value == q->controller.value && p->zone.sector == q->zone.sector &&
        p->zone.lane == q->zone.lane && p->phase == q->phase &&
        p->action_count == q->action_count && p->elapsed_seconds == q->elapsed_seconds;
}

static bool same_event(const RmcMatchEvent *a, const RmcMatchEvent *b)
{
    if (a->id != b->id || a->match_id.value != b->match_id.value || a->at_second != b->at_second ||
        a->type != b->type || a->team_id.value != b->team_id.value ||
        a->primary_actor.value != b->primary_actor.value || a->secondary_actor.value != b->secondary_actor.value ||
        a->outcome_code != b->outcome_code || a->cause_event_id != b->cause_event_id) { return false; }
    const RmcShotEventPayload *x = &a->shot, *y = &b->shot;
    return x->model_version == y->model_version && x->chance_quality == y->chance_quality &&
        x->source_zone.sector == y->source_zone.sector && x->source_zone.lane == y->source_zone.lane &&
        x->context == y->context && x->duration_seconds == y->duration_seconds &&
        x->defender_id.value == y->defender_id.value && x->goalkeeper_id.value == y->goalkeeper_id.value &&
        x->applied_talent_mask == y->applied_talent_mask && x->trajectory == y->trajectory &&
        x->restart_kind == y->restart_kind && x->restart_awarded_team.value == y->restart_awarded_team.value &&
        x->restart_zone.sector == y->restart_zone.sector && x->restart_zone.lane == y->restart_zone.lane &&
        x->restart_disposition == y->restart_disposition;
}

static bool same_command(const RmcMatchFlowCommand *a, const RmcMatchFlowCommand *b)
{
    if (a->command_id != b->command_id || a->observed_command_count != b->observed_command_count ||
        a->match_id.value != b->match_id.value || a->expected_period != b->expected_period ||
        a->kind != b->kind) { return false; }
#define FIELD(member, field) (a->data.member.field == b->data.member.field)
#define TIMING(member) (FIELD(member, context) && FIELD(member, duration_seconds))
    switch (a->kind) {
    case RMC_FLOW_INSTRUCTION:
        return FIELD(instruction, team_id.value) && FIELD(instruction, window) &&
            FIELD(instruction, keep) && (a->data.instruction.keep ||
                same_instructions(a->data.instruction.instructions, b->data.instruction.instructions));
    case RMC_FLOW_OPENING:
        return FIELD(opening, opening_period) && FIELD(opening, opening_team.value) && FIELD(opening, taker.value);
    case RMC_FLOW_CLOSE_PERIOD: case RMC_FLOW_RESET_WINDOW: return true;
    case RMC_FLOW_RESTART:
        return FIELD(restart, restart_kind) && FIELD(restart, awarded_team.value) &&
            FIELD(restart, taker.value) && FIELD(restart, source_shot_event_id);
    case RMC_FLOW_RECOVERY:
        return FIELD(recovery, incumbent.value) && FIELD(recovery, challenger.value) && TIMING(recovery);
    case RMC_FLOW_PASS:
        return FIELD(pass, team_id.value) && FIELD(pass, passer.value) &&
            FIELD(pass, recipient.value) && FIELD(pass, opponent.value) && TIMING(pass);
    case RMC_FLOW_CARRY:
        return FIELD(carry, team_id.value) && FIELD(carry, carrier.value) &&
            FIELD(carry, opponent.value) && TIMING(carry);
    case RMC_FLOW_DRIBBLE:
        return FIELD(dribble, team_id.value) && FIELD(dribble, dribbler.value) &&
            FIELD(dribble, opponent.value) && TIMING(dribble);
    case RMC_FLOW_TACKLE:
        return FIELD(tackle, team_id.value) && FIELD(tackle, carrier.value) &&
            FIELD(tackle, defender.value) && TIMING(tackle);
    case RMC_FLOW_CROSS:
        return FIELD(cross, team_id.value) && FIELD(cross, crosser.value) &&
            FIELD(cross, recipient.value) && FIELD(cross, defender.value) && TIMING(cross);
    case RMC_FLOW_SHOT:
        return FIELD(shot, team_id.value) && FIELD(shot, shooter.value) && FIELD(shot, defender.value) &&
            FIELD(shot, goalkeeper.value) && TIMING(shot);
    }
#undef TIMING
#undef FIELD
    return false;
}

static bool same_flow(const RmcMatchFlow *f, const RmcMatchFlow *g)
{
    if (!same_state(&f->state, &g->state) || f->match_rng.state != g->match_rng.state ||
        f->match_rng.increment != g->match_rng.increment || f->journal.count != g->journal.count ||
        f->sporting_count != g->sporting_count || f->reset_count != g->reset_count ||
        f->home_window_submitted != g->home_window_submitted || f->away_window_submitted != g->away_window_submitted ||
        !same_instructions(f->home_instructions, g->home_instructions) ||
        !same_instructions(f->away_instructions, g->away_instructions) || f->log.count != g->log.count ||
        f->log.match_id.value != g->log.match_id.value ||
        f->config.flow_version != g->config.flow_version || f->config.selector_version != g->config.selector_version ||
        f->config.first_opening_team.value != g->config.first_opening_team.value ||
        f->config.operation_limit != g->config.operation_limit) { return false; }
    for (size_t i = 0; i < f->log.count; ++i) {
        if (!same_event(&f->log.events[i], &g->log.events[i])) { return false; }
    }
    /* Enumerate active fields only: padding and inactive union members have
     * no semantic meaning and are not a portable serialization. */
    for (size_t i = 0; i < f->journal.count; ++i) {
        if (!same_command(&f->journal.commands[i], &g->journal.commands[i])) { return false; }
    }
    return true;
}

static bool verify(const RmcMatchFlow *live, const RmcMatchFlow *half)
{
    Run replay = {0};
    bool ok = initialize(&replay, live->world, &live->state.fixture,
        &live->state.home_lineup, &live->state.away_lineup, &live->config);
    if (ok) {
        const size_t prefix = half->journal.count;
        ok = rmc_match_flow_replay(&replay.flow, live->journal.commands, prefix) == RMC_FLOW_OK &&
            same_flow(half, &replay.flow) &&
            rmc_match_flow_replay(&replay.flow, live->journal.commands + prefix,
                live->journal.count - prefix) == RMC_FLOW_OK && same_flow(live, &replay.flow);
        RmcMatchFlowStatus status;
        ok = ok && rmc_match_flow_status(&replay.flow, &status) == RMC_FLOW_OK && status.complete;
    }
    destroy(&replay);
    return ok;
}

static int play(Console *c, Options *o, const RmcCareerSetup *career, const RmcFixture *selected)
{
    const RmcWorldCore *w = &career->foundation.world;
    RmcLineup home, away;
    if (!rmc_roster_lineup(roster(w, selected->home_team), o->home, &home) ||
        !rmc_roster_lineup(roster(w, selected->away_team), o->away, &away)) { return 1; }
    const RmcMatchFlowConfig config = {RMC_MATCH_FLOW_VERSION, RMC_MATCH_SELECTOR_VERSION,
        o->away_opens ? selected->away_team : selected->home_team, 0};
    Run run = {0};
    int rc = 1;
    if (!initialize(&run, w, selected, &home, &away, &config)) {
        write_to(c, c->err, "Inizializzazione flow/allocazione fallita.\n");
        destroy(&run);
        return 1;
    }
    RmcMatchFlow *f = &run.flow;
    /* Read-only observation of the half-time boundary; never executed as a
     * shallow copy. Its log/journal prefix remains immutable until verify ends. */
    RmcMatchFlow half = {0};
    bool half_seen = false;
    Statistics stats = {0};
    write_to(c, c->out, "Incontro generato Match #%" PRIu64 " giornata %u: %s (#%" PRIu64 ") - %s (#%" PRIu64 "); manager=#%" PRIu64 "; opener=%s\n",
        selected->id.value, (unsigned)selected->matchday, team(w, selected->home_team.value)->name,
        selected->home_team.value, team(w, selected->away_team.value)->name, selected->away_team.value,
        o->team, o->away_opens ? "away" : "home");
    if (!show_lineup(c, w, selected->home_team, &home) || !show_lineup(c, w, selected->away_team, &away)) { goto done; }
    for (;;) {
        RmcMatchFlowNext next;
        RmcFlowError err = rmc_match_flow_next(f, &next);
        if (err != RMC_FLOW_OK) { write_to(c, c->err, "Flow next fallito: %d.\n", (int)err); goto done; }
        if (next.kind == RMC_FLOW_COMPLETE) { break; }
        RmcMatchFlowCommand command;
        if (next.kind == RMC_FLOW_NEED_INSTRUCTION) {
            const bool at_half = f->state.period == RMC_MATCH_HALF_TIME;
            const bool managed = next.team_id.value == o->team;
            if (managed && at_half && !has(o, OPT_AUTO) && !has(o, OPT_HALF)) {
                rc = prompt(c, o, OPT_HALF, career);
                if (rc != 0) { goto done; }
                rc = 1;
            }
            const bool keep = at_half && (!managed || o->half_keep);
            const RmcFlowInstructions defaults = {RMC_FLOW_PATIENT, RMC_FLOW_CARRY_FIRST, RMC_FLOW_PRESS_NORMAL};
            const RmcFlowInstructions t = managed ? (at_half ? o->half : o->pre) : defaults;
            command = (RmcMatchFlowCommand){.command_id = next.observed_command_count + 1,
                .observed_command_count = next.observed_command_count, .match_id = selected->id,
                .expected_period = f->state.period, .kind = RMC_FLOW_INSTRUCTION,
                .data.instruction = {next.team_id, f->state.period, keep, keep ? defaults : t}};
        } else {
            err = rmc_match_flow_select(f, &next, &command);
            if (err != RMC_FLOW_OK) { write_to(c, c->err, "Flow select fallito: %d.\n", (int)err); goto done; }
        }
        if (c->io_failed) { goto done; }
        RmcMatchFlowStepResult step;
        err = rmc_match_flow_apply(f, &command, &step);
        if (err != RMC_FLOW_OK) { write_to(c, c->err, "Flow apply fallito: %d; partita incompleta.\n", (int)err); goto done; }
        if (command.kind == RMC_FLOW_INSTRUCTION) {
            char text[64];
            const RmcFlowInstructions t = next.team_id.value == selected->home_team.value ? f->home_instructions : f->away_instructions;
            instructions_text(text, sizeof(text), t);
            write_to(c, c->out, "Istruzioni %s %s (#%" PRIu64 "): %s%s\n",
                f->state.period == RMC_MATCH_NOT_STARTED ? "pre" : "intervallo",
                team(w, next.team_id.value)->name, next.team_id.value,
                command.data.instruction.keep ? "KEEP -> " : "", text);
        }
        if (!events(c, f, &stats, &step)) { goto done; }
        if (step.entered_half_time) {
            if (half_seen || !scores_match(f, &stats)) { goto done; }
            half_seen = true;
            half = *f;
            score(c, f, "Intervallo");
            char h[64], a[64];
            instructions_text(h, sizeof(h), f->home_instructions);
            instructions_text(a, sizeof(a), f->away_instructions);
            write_to(c, c->out, "Tattiche correnti: casa=%s; trasferta=%s\n", h, a);
        }
        if (c->io_failed) { goto done; }
    }
    RmcMatchFlowStatus status;
    if (!half_seen || rmc_match_flow_status(f, &status) != RMC_FLOW_OK || !status.complete ||
        status.period != RMC_MATCH_FINISHED || status.terminal_reason != RMC_MATCH_TERMINAL_COMPLETED ||
        !scores_match(f, &stats)) { goto done; }
    if (has(o, OPT_VERIFY)) {
        const bool ok = verify(f, &half);
        write_to(c, c->out, "Replay semantico: %s (eventi/cause, intervallo/finale, RNG, comandi, operazioni)\n", ok ? "PASS" : "FAIL");
        if (!ok || c->io_failed) { goto done; }
    }
    score(c, f, "Finale");
    summary(c, f, &stats);
    write_to(c, c->out, "Partita completata: FINISHED/COMPLETED; secondi=%" PRIu32
        " comandi=%" PRIu64 " operazioni=%" PRIu64 " eventi=%zu\n",
        status.at_second, status.accepted_commands, status.sporting_operations, status.event_count);
    rc = c->io_failed ? 1 : 0;
done:
    destroy(&run);
    return rc;
}

int rmc_match_cli_run(int argc, const char *const argv[], FILE *input, FILE *output, FILE *error)
{
    if (argc < 1 || argv == NULL || input == NULL || output == NULL || error == NULL) { return 1; }
    Console c = {input, output, error, false};
    for (int i = 0; i < argc; ++i) { if (argv[i] == NULL) { return finish(&c, 1); } }
    Options options;
    if (!parse(argc, argv, &options)) {
        write_to(&c, error, "Opzioni non valide, duplicate o valori mancanti. Usare rmc-match --help.\n");
        return finish(&c, 2);
    }
    if (has(&options, OPT_HELP)) { help(&c); return finish(&c, 0); }
    RmcCareerSetup *career = allocate(1, sizeof(*career));
    int rc = 1;
    if (career != NULL) {
        const RmcFixture *selected = NULL;
        rc = setup(&c, &options, career, &selected);
        if (rc == 0) { rc = play(&c, &options, career, selected); }
        free(career);
    }
    if (rc == 3) { write_to(&c, error, "Annullata: EOF al prompt; nessun risultato finale ufficiale.\n"); }
    else if (rc == 2) { write_to(&c, error, "Selezione non valida: squadra o incontro non disponibile per il manager.\n"); }
    else if (rc == 1) { write_to(&c, error, "Errore runtime/risorse/I/O; esecuzione interrotta.\n"); }
    return finish(&c, rc);
}
