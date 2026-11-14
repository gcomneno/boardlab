#include "match_cli.h"
#include "career.h"
#include "match_flow_selector.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef struct { int code; char *out, *err; } Capture;

static char *read_all(FILE *f)
{
    assert(fflush(f) == 0 && fseek(f, 0, SEEK_END) == 0);
    long length = ftell(f);
    assert(length >= 0 && (uintmax_t)length < SIZE_MAX);
    char *text = malloc((size_t)length + 1);
    assert(text != NULL && fseek(f, 0, SEEK_SET) == 0);
    assert(fread(text, 1, (size_t)length, f) == (size_t)length);
    text[length] = '\0';
    return text;
}

static Capture invoke(const char *const argv[], size_t count, const char *input)
{
    FILE *in = tmpfile(), *out = tmpfile(), *err = tmpfile();
    assert(in != NULL && out != NULL && err != NULL);
    assert(fputs(input, in) >= 0 && fflush(in) == 0 && fseek(in, 0, SEEK_SET) == 0);
    Capture c = {rmc_match_cli_run((int)count, argv, in, out, err), NULL, NULL};
    /* In auto/help, even a nonempty finite input source must remain unread. */
    for (size_t i = 1; i < count; ++i) {
        if (strcmp(argv[i], "--auto") == 0 || strcmp(argv[i], "--help") == 0) {
            assert(ftell(in) == 0);
        }
    }
    c.out = read_all(out);
    c.err = read_all(err);
    assert(fclose(in) == 0 && fclose(out) == 0 && fclose(err) == 0);
    return c;
}

#define INVOKE(args, input) invoke(args, sizeof(args) / sizeof((args)[0]), input)

static void release(Capture *c)
{
    free(c->out); free(c->err);
}

static size_t occurrences(const char *s, const char *token)
{
    size_t count = 0;
    while ((s = strstr(s, token)) != NULL) { ++count; s += strlen(token); }
    return count;
}

static const RmcRoster *owned_roster(const RmcWorldCore *w, RmcTeamId id)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (w->rosters[i].team_id.value == id.value) { return &w->rosters[i]; }
    }
    assert(false); return NULL;
}

static const RmcTeam *owned_team(const RmcWorldCore *w, RmcTeamId id)
{
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (w->teams[i].id.value == id.value) { return &w->teams[i]; }
    }
    assert(false); return NULL;
}

/* Independent production execution supplies expected semantic facts. It uses
 * the real selector, full instructions and generated fixture, never clocks,
 * possession patches or forced outcomes. Prompt wording is checked elsewhere. */
static void check_reference(const Capture *c, const RmcCareerSetup *career,
    const RmcFixture *fixture, RmcTeamId managed, bool changed_pre, bool changed_half,
    RmcFormation hf, RmcFormation af, bool away_opens)
{
    assert(c->code == 0 && c->err[0] == '\0');
    const RmcWorldCore *w = &career->foundation.world;
    RmcMatchEvent *events = malloc(RMC_FLOW_DEFAULT_EVENT_CAPACITY * sizeof(*events));
    RmcMatchFlowCommand *commands = malloc(RMC_FLOW_DEFAULT_COMMAND_CAPACITY * sizeof(*commands));
    assert(events != NULL && commands != NULL);
    RmcLineup home, away;
    assert(rmc_roster_lineup(owned_roster(w, fixture->home_team), hf, &home));
    assert(rmc_roster_lineup(owned_roster(w, fixture->away_team), af, &away));
    RmcMatchFlow f;
    const RmcMatchFlowConfig config = {RMC_MATCH_FLOW_VERSION, RMC_MATCH_SELECTOR_VERSION,
        away_opens ? fixture->away_team : fixture->home_team, 0};
    assert(rmc_match_flow_init(w, fixture, &home, &away, &config, events,
        RMC_FLOW_DEFAULT_EVENT_CAPACITY, commands, RMC_FLOW_DEFAULT_COMMAND_CAPACITY, &f) == RMC_FLOW_OK);
    unsigned halves = 0;
    size_t half_count = 0;
    uint16_t half_home = 0, half_away = 0;
    for (;;) {
        RmcMatchFlowNext next;
        assert(rmc_match_flow_next(&f, &next) == RMC_FLOW_OK);
        if (next.kind == RMC_FLOW_COMPLETE) { break; }
        RmcMatchFlowCommand command;
        if (next.kind == RMC_FLOW_NEED_INSTRUCTION) {
            const bool half = f.state.period == RMC_MATCH_HALF_TIME;
            const bool own = next.team_id.value == managed.value;
            const bool change = own && (half ? changed_half : changed_pre);
            const RmcFlowInstructions t = change ?
                (RmcFlowInstructions){RMC_FLOW_DIRECT, RMC_FLOW_DRIBBLE_FIRST, RMC_FLOW_PRESS_HIGH} :
                (RmcFlowInstructions){RMC_FLOW_PATIENT, RMC_FLOW_CARRY_FIRST, RMC_FLOW_PRESS_NORMAL};
            command = (RmcMatchFlowCommand){.command_id = next.observed_command_count + 1,
                .observed_command_count = next.observed_command_count, .match_id = fixture->id,
                .expected_period = f.state.period, .kind = RMC_FLOW_INSTRUCTION,
                .data.instruction = {next.team_id, f.state.period, half && !change, t}};
        } else {
            assert(rmc_match_flow_select(&f, &next, &command) == RMC_FLOW_OK);
        }
        RmcMatchFlowStepResult step;
        assert(rmc_match_flow_apply(&f, &command, &step) == RMC_FLOW_OK);
        if (step.entered_half_time) {
            assert(halves == 0 && f.state.period == RMC_MATCH_HALF_TIME);
            assert(f.state.elapsed_seconds == 2700 && f.state.second_half_start_second == 2700);
            half_home = f.state.home_goals; half_away = f.state.away_goals;
            half_count = f.journal.count;
            ++halves;
        }
    }
    RmcMatchFlowStatus status;
    assert(rmc_match_flow_status(&f, &status) == RMC_FLOW_OK && status.complete);
    assert(halves == 1 && status.period == RMC_MATCH_FINISHED && status.at_second == 5400);
    assert(status.terminal_reason == RMC_MATCH_TERMINAL_COMPLETED);
    assert(commands[0].kind == RMC_FLOW_INSTRUCTION && commands[1].kind == RMC_FLOW_INSTRUCTION);
    assert(commands[0].data.instruction.team_id.value == fixture->home_team.value);
    assert(commands[1].data.instruction.team_id.value == fixture->away_team.value);
    assert(commands[half_count].data.instruction.team_id.value == fixture->home_team.value);
    assert(commands[half_count + 1].data.instruction.team_id.value == fixture->away_team.value);
    char expected[512];
    const RmcTeam *ht = owned_team(w, fixture->home_team), *at = owned_team(w, fixture->away_team);
    (void)snprintf(expected, sizeof(expected), "Intervallo: %s (#%" PRIu64 ") %u - %u %s (#%" PRIu64 ")",
        ht->name, ht->id.value, (unsigned)half_home, (unsigned)half_away, at->name, at->id.value);
    assert(strstr(c->out, expected) != NULL);
    (void)snprintf(expected, sizeof(expected), "Finale: %s (#%" PRIu64 ") %u - %u %s (#%" PRIu64 ")",
        ht->name, ht->id.value, (unsigned)status.home_goals, (unsigned)status.away_goals, at->name, at->id.value);
    assert(strstr(c->out, expected) != NULL);
    (void)snprintf(expected, sizeof(expected), "secondi=%" PRIu32 " comandi=%" PRIu64
        " operazioni=%" PRIu64 " eventi=%zu", status.at_second, status.accepted_commands,
        status.sporting_operations, status.event_count);
    assert(strstr(c->out, expected) != NULL);
    size_t shots[2][6] = {{0}}, goals[2] = {0}, recoveries[2][2] = {{0}}, tackles[2] = {0};
    size_t scorers[2][RMC_LINEUP_PLAYERS] = {{0}};
    unsigned starts = 0, ends = 0, whistles = 0;
    for (size_t i = 0; i < f.log.count; ++i) {
        const RmcMatchEvent *e = &events[i];
        const size_t side = e->team_id.value == fixture->home_team.value ? 0 : 1;
        if (e->type == RMC_EVENT_SHOT) {
            assert(e->outcome_code >= RMC_SHOT_GOAL && e->outcome_code <= RMC_SHOT_WOODWORK);
            ++shots[side][e->outcome_code - RMC_SHOT_GOAL];
            if (e->outcome_code != RMC_SHOT_GOAL) {
                static const char *const outcomes[] = {"GOL", "parato e trattenuto",
                    "parato con respinta", "bloccato", "WIDE (fuori)", "legno"};
                (void)snprintf(expected, sizeof(expected), "LIVE %02" PRIu32 ":%02" PRIu32
                    " %s (#%" PRIu64 ") Player #%" PRIu64 " tiro: %s\n",
                    e->at_second / 60, e->at_second % 60, owned_team(w, e->team_id)->name,
                    e->team_id.value, e->primary_actor.value, outcomes[e->outcome_code - RMC_SHOT_GOAL]);
                assert(strstr(c->out, expected) != NULL);
            }
        }
        if (e->type == RMC_EVENT_GOAL) {
            ++goals[side];
            (void)snprintf(expected, sizeof(expected), "LIVE %02" PRIu32 ":%02" PRIu32
                " GOL %s (#%" PRIu64 ") Player #%" PRIu64 "; score=%zu - %zu\n",
                e->at_second / 60, e->at_second % 60, owned_team(w, e->team_id)->name,
                e->team_id.value, e->primary_actor.value, goals[0], goals[1]);
            assert(strstr(c->out, expected) != NULL);
            const RmcLineup *l = side == 0 ? &home : &away;
            bool found = false;
            for (size_t j = 0; j < RMC_LINEUP_PLAYERS; ++j) {
                if (l->players[j].value == e->primary_actor.value) { ++scorers[side][j]; found = true; }
            }
            assert(found);
        }
        if (e->type == RMC_EVENT_RECOVERY) { ++recoveries[side][e->outcome_code == RMC_RECOVERY_TURNOVER ? 1 : 0]; }
        if (e->type == RMC_EVENT_ACTION && e->outcome_code == RMC_TACKLE_OUTCOME_WON) { ++tackles[side]; }
        if (e->type == RMC_EVENT_PERIOD_START) {
            assert(e->at_second == (starts == 0 ? 0u : 2700u));
            assert(e->team_id.value == (starts == 0 ? config.first_opening_team.value :
                (away_opens ? fixture->home_team.value : fixture->away_team.value)));
            ++starts;
        }
        if (e->type == RMC_EVENT_PERIOD_END) { ++ends; }
        if (e->type == RMC_EVENT_MATCH_END) { ++whistles; }
    }
    assert(starts == 2 && ends == 2 && whistles == 1);
    assert(goals[0] == status.home_goals && goals[1] == status.away_goals);
    for (size_t side = 0; side < 2; ++side) {
        size_t total = 0;
        for (size_t i = 0; i < 6; ++i) { total += shots[side][i]; }
        const RmcTeam *t = side == 0 ? ht : at;
        (void)snprintf(expected, sizeof(expected), "Statistiche %s (#%" PRIu64 "): tiri=%zu gol=%zu "
            "parati-trattenuti=%zu parati-respinti=%zu bloccati=%zu WIDE=%zu legni=%zu; "
            "recuperi-palla-contesa=%zu (mantenuti=%zu cambi-possesso=%zu); contrasti-vinti=%zu",
            t->name, t->id.value, total, goals[side], shots[side][1], shots[side][2], shots[side][3],
            shots[side][4], shots[side][5], recoveries[side][0] + recoveries[side][1],
            recoveries[side][0], recoveries[side][1], tackles[side]);
        assert(strstr(c->out, expected) != NULL);
        const RmcLineup *l = side == 0 ? &home : &away;
        for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
            if (scorers[side][i] != 0) {
                (void)snprintf(expected, sizeof(expected), "Player #%" PRIu64 " (%zu)", l->players[i].value, scorers[side][i]);
                assert(strstr(c->out, expected) != NULL);
            }
        }
    }
    assert(strstr(c->out, "Controllo gol eventi/score: PASS") != NULL);
    assert(occurrences(c->out, "Inizio tempo: apre") == 2);
    assert(strstr(c->out, "LIVE 45:00 Inizio tempo: apre") != NULL);
    assert(occurrences(c->out, "Fine tempo") == 2);
    assert(occurrences(c->out, "Fischio finale") == 1);
    size_t shot_total = 0, routine_total = 0;
    for (size_t side = 0; side < 2; ++side) {
        for (size_t j = 0; j < 6; ++j) { shot_total += shots[side][j]; }
        routine_total += recoveries[side][0] + recoveries[side][1] + tackles[side];
    }
    assert(routine_total > 80); /* Real production volume exceeds the old cap. */
    assert(occurrences(c->out, "Cronaca: limite di 40 routine") == 2);
    assert(occurrences(c->out, " GOL ") == goals[0] + goals[1]);
    assert(occurrences(c->out, " tiro: ") == shot_total - goals[0] - goals[1]);
    assert(occurrences(c->out, "recupera la palla contesa") +
        occurrences(c->out, "vince il contrasto") == 80);
    assert(occurrences(c->out, "Player #") >= 22);
    free(events); free(commands);
}

static void invalid_options(void)
{
    static const char *const cases[][7] = {
        {"--wat"}, {"--auto", "junk"}, {"--help", "--auto"}, {"--help", "--help"},
        {"--auto", "--auto"}, {"--verify-replay", "--verify-replay"},
        {"--seed"}, {"--seed", "18446744073709551616"}, {"--seed", "-1"}, {"--seed", "+1"},
        {"--seed", "1x"}, {"--seed", ""}, {"--seed", "0", "--seed", "2"},
        {"--team", "0"}, {"--team", "1x"}, {"--team", "18446744073709551616"},
        {"--team", "1", "--team", "2"}, {"--fixture", "0"}, {"--fixture", "1.0"},
        {"--fixture", "1", "--fixture", "2"}, {"--home-formation", "451"},
        {"--home-formation", "442", "--home-formation", "433"}, {"--away-formation", "0"},
        {"--away-formation", "433", "--away-formation", "352"}, {"--opener", "HOME"},
        {"--opener", "home", "--opener", "away"}, {"--pre", "PATIENT", "CARRY_FIRST"},
        {"--pre", "patient", "CARRY_FIRST", "PRESS_NORMAL"},
        {"--pre", "PATIENT", "PASS_FIRST", "PRESS_NORMAL"},
        {"--pre", "PATIENT", "CARRY_FIRST", "HIGH"}, {"--pre", "KEEP"},
        {"--half"}, {"--half", "keep"}, {"--half", "DIRECT", "DRIBBLE_FIRST", "NORMAL"},
        {"--half", "KEEP", "extra"}, {"--half", "KEEP", "--half", "KEEP"},
        {"--seed", "--auto"}, {"--auto", "--team", "9999"}, {"--auto", "--fixture", "9999"}
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        const char *args[8] = {"rmc-match"};
        size_t n = 1;
        for (size_t j = 0; j < 7 && cases[i][j] != NULL; ++j) { args[n++] = cases[i][j]; }
        Capture c = invoke(args, n, "");
        assert(c.code == 2 && c.err[0] != '\0');
        assert(strstr(c.out, "Partita completata") == NULL && strstr(c.out, "Incontro generato") == NULL);
        release(&c);
    }
    const char *duplicate_pre[] = {"rmc-match", "--pre", "DIRECT", "DRIBBLE_FIRST", "PRESS_HIGH",
        "--pre", "PATIENT", "CARRY_FIRST", "PRESS_NORMAL"};
    Capture c = INVOKE(duplicate_pre, ""); assert(c.code == 2); release(&c);
}

static void io_failures(void)
{
    const char *args[] = {"rmc-match", "--auto"};
    FILE *in = tmpfile(), *out = tmpfile(), *err = tmpfile();
    assert(in != NULL && out != NULL && err != NULL);
    /* Writing to an ISO stdio stream opened read-only must fail. */
    FILE *read_only = fopen(__FILE__, "r");
    assert(read_only != NULL);
    assert(rmc_match_cli_run(2, args, in, read_only, err) == 1);
    assert(ferror(read_only));
    assert(fclose(read_only) == 0);
    read_only = fopen(__FILE__, "r");
    assert(read_only != NULL);
    const char *bad[] = {"rmc-match", "--bad"};
    assert(rmc_match_cli_run(2, bad, in, out, read_only) == 1);
    assert(fclose(read_only) == 0);
    assert(rmc_match_cli_run(0, args, in, out, err) == 1);
    assert(rmc_match_cli_run(2, args, NULL, out, err) == 1);
#ifdef __linux__
    /* Buffered /dev/full detects the final flush error, including --help. */
    FILE *full = fopen("/dev/full", "w");
    assert(full != NULL);
    const char *help_args[] = {"rmc-match", "--help"};
    assert(rmc_match_cli_run(2, help_args, in, full, err) == 1);
    (void)fclose(full);
    /* A directory stream supplies an actual read error on this platform. */
    FILE *directory = fopen(".", "r");
    assert(directory != NULL);
    const char *interactive[] = {"rmc-match"};
    assert(rmc_match_cli_run(1, interactive, directory, out, err) == 1);
    assert(ferror(directory));
    assert(fclose(directory) == 0);
#endif
    assert(fclose(in) == 0 && fclose(out) == 0 && fclose(err) == 0);
}

int main(void)
{
    const char *help_args[] = {"rmc-match", "--help"};
    Capture help = INVOKE(help_args, "unread\n");
    assert(help.code == 0 && help.err[0] == '\0' && strstr(help.out, "Universo") == NULL);
    assert(strstr(help.out, "DRIBBLE_FIRST") != NULL && strstr(help.out, "PRESS_HIGH") != NULL);
    release(&help);
    invalid_options();
    io_failures();
    RmcCareerSetup *career = malloc(sizeof(*career));
    assert(career != NULL);
    const RmcWorldIdentity identity = {UINT64_C(20261114), RMC_GENERATOR_VERSION, RMC_RULES_VERSION};
    assert(rmc_career_setup_generate(&identity, career) && rmc_career_setup_valid(career));
    RmcTeamId lowest = {UINT64_MAX};
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (career->foundation.world.teams[i].id.value < lowest.value) { lowest = career->foundation.world.teams[i].id; }
    }
    const RmcFixture *first = NULL;
    for (size_t i = 0; i < RMC_CALENDAR_FIXTURES; ++i) {
        const RmcFixture *f = &career->foundation.calendar.fixtures[i];
        if (f->home_team.value == lowest.value || f->away_team.value == lowest.value) { first = f; break; }
    }
    assert(first != NULL);
    const char *auto_args[] = {"rmc-match", "--auto"};
    Capture automatic = INVOKE(auto_args, "must remain unread\n");
    check_reference(&automatic, career, first, lowest, false, false, RMC_FORMATION_442, RMC_FORMATION_433, false);
    assert(occurrences(automatic.out, "[candidata carriera]") == 3);
    assert(occurrences(automatic.out, "Squadra #") == 8);
    Capture repeat = INVOKE(auto_args, "");
    assert(repeat.code == 0);
    /* Deterministic semantic summary comparison is separate from prompts. */
    assert(strcmp(strstr(automatic.out, "Finale:"), strstr(repeat.out, "Finale:")) == 0);
    release(&repeat);
    const char *verify_args[] = {"rmc-match", "--verify-replay", "--auto"};
    Capture replay = INVOKE(verify_args, "");
    assert(replay.code == 0 && strstr(replay.out, "Replay semantico: PASS") != NULL);
    assert(strcmp(strstr(automatic.out, "Finale:"), strstr(replay.out, "Finale:")) == 0);
    release(&replay);
    const char *interactive[] = {"rmc-match"};
    Capture blanks = INVOKE(interactive, "\n\n\n\n\n\n\n\n\n");
    assert(blanks.code == 0 && strstr(blanks.out, "[1 = KEEP]") != NULL);
    assert(strcmp(strstr(automatic.out, "Finale:"), strstr(blanks.out, "Finale:")) == 0);
    release(&blanks);
    Capture eof = INVOKE(interactive, "");
    assert(eof.code == 3 && strstr(eof.out, "Incontro generato") == NULL);
    assert(strstr(eof.out, "Partita completata") == NULL && strstr(eof.err, "EOF") != NULL);
    release(&eof);
    Capture half_eof = INVOKE(interactive, "\n\n\n\n\n\n\n\n");
    assert(half_eof.code == 3 && strstr(half_eof.out, "Intervallo:") != NULL);
    assert(strstr(half_eof.out, "Finale:") == NULL && strstr(half_eof.out, "Partita completata") == NULL);
    release(&half_eof);
    char long_input[900];
    memset(long_input, 'x', 300);
    (void)snprintf(long_input + 300, sizeof(long_input) - 300,
        "\n0\n9999\n\n9999\n\n451\n\nno\n\nHOME\n\n"
        "DIRECT CARRY_FIRST INVALID\n?\n--help\n1\nBAD\n1\n0\n1\nBAD\n1\n");
    Capture corrected = INVOKE(interactive, long_input);
    assert(corrected.code == 0 && strstr(corrected.err, "Input troppo lungo") != NULL);
    assert(occurrences(corrected.err, "Scelta non valida") >= 7);
    assert(strcmp(strstr(automatic.out, "Finale:"), strstr(corrected.out, "Finale:")) == 0);
    assert(occurrences(corrected.out, "Aiuto: Approach") == 2);
    release(&corrected);
    release(&automatic);
    const RmcFixture *f = &career->foundation.calendar.fixtures[0];
    char home_id[32], away_id[32], match_id[32];
    (void)snprintf(home_id, sizeof(home_id), "%" PRIu64, f->home_team.value);
    (void)snprintf(away_id, sizeof(away_id), "%" PRIu64, f->away_team.value);
    (void)snprintf(match_id, sizeof(match_id), "%" PRIu64, f->id.value);
    const char *home_args[] = {"rmc-match", "--team", home_id, "--fixture", match_id, "--verify-replay"};
    Capture home_play = INVOKE(home_args, "\n\n\n2\n2\n2\n1\n");
    assert(strstr(home_play.out, "Replay semantico: PASS") != NULL);
    check_reference(&home_play, career, f, f->home_team, true, false, RMC_FORMATION_442, RMC_FORMATION_433, false);
    release(&home_play);
    const char *away_args[] = {"rmc-match", "--seed", "20261114", "--team", away_id,
        "--fixture", match_id, "--verify-replay"};
    Capture away_play = INVOKE(away_args, "352\n442\naway\n2\n2\n2\n2\n\n\n\n");
    assert(strstr(away_play.out, "Replay semantico: PASS") != NULL);
    check_reference(&away_play, career, f, f->away_team, true, true, RMC_FORMATION_352, RMC_FORMATION_442, true);
    assert(strstr(away_play.out, "Istruzioni attuali: DIRECT DRIBBLE_FIRST PRESS_HIGH") != NULL);
    assert(strstr(away_play.out, "Approach [2 = DIRECT]") != NULL);
    assert(strstr(away_play.out, "Progression [2 = DRIBBLE_FIRST]") != NULL);
    assert(strstr(away_play.out, "Pressure [2 = PRESS_HIGH]") != NULL);
    release(&away_play);
    const char *explicit_args[] = {"rmc-match", "--half", "DIRECT", "DRIBBLE_FIRST", "PRESS_HIGH",
        "--auto", "--team", home_id, "--fixture", match_id, "--seed", "20261114",
        "--home-formation", "433", "--away-formation", "352", "--opener", "away",
        "--pre", "DIRECT", "DRIBBLE_FIRST", "PRESS_HIGH", "--verify-replay"};
    Capture explicit_play = INVOKE(explicit_args, "unread\n");
    check_reference(&explicit_play, career, f, f->home_team, true, true, RMC_FORMATION_433, RMC_FORMATION_352, true);
    release(&explicit_play);
    /* Omitted pre is default; the complete triple changes only at actual half-time. */
    const char *half_args[] = {"rmc-match", "--auto", "--team", away_id, "--fixture", match_id,
        "--half", "DIRECT", "DRIBBLE_FIRST", "PRESS_HIGH", "--verify-replay"};
    Capture half_play = INVOKE(half_args, "");
    check_reference(&half_play, career, f, f->away_team, false, true, RMC_FORMATION_442, RMC_FORMATION_433, false);
    release(&half_play);
    /* All explicit setup choices bypass setup input; EOF occurs only at half. */
    const char *half_only[] = {"rmc-match", "--team", home_id, "--fixture", match_id,
        "--home-formation", "442", "--away-formation", "433", "--opener", "home",
        "--pre", "PATIENT", "CARRY_FIRST", "PRESS_NORMAL"};
    Capture stopped = INVOKE(half_only, "");
    assert(stopped.code == 3 && strstr(stopped.out, "Finale:") == NULL);
    release(&stopped);
    /* Every partial menu/CHANGE boundary must cancel, including unterminated input. */
    const char *tactical[] = {"rmc-match", "--team", home_id, "--fixture", match_id,
        "--home-formation", "442", "--away-formation", "433", "--opener", "home"};
    const char *cancel[] = {"", "2\n", "2\n2\n", "2\n2\n2\n",
        "2\n2\n2\n2\n", "2\n2\n2\n2\n1\n", "2\n2\n2\n2\n1\n1\n", "1"};
    for (size_t i = 0; i < sizeof(cancel) / sizeof(cancel[0]); ++i) {
        Capture cancelled = INVOKE(tactical, cancel[i]);
        assert(cancelled.code == 3 && strstr(cancelled.out, "Finale:") == NULL);
        release(&cancelled);
    }
    /* Mixed triples, both managed sides, both instruction windows, help and long
     * lines: accepted sporting summaries equal the unchanged flag workflow.
     * Replay additionally compares every active command/event field, causal ID,
     * participant assignment, clock and exact RNG pair at half-time and final. */
    for (unsigned bits = 0; bits < 8; ++bits) {
        const char *a = bits & 1 ? "DIRECT" : "PATIENT";
        const char *p = bits & 2 ? "DRIBBLE_FIRST" : "CARRY_FIRST";
        const char *r = bits & 4 ? "PRESS_HIGH" : "PRESS_NORMAL";
        const char *manager = bits & 1 ? away_id : home_id;
        const char *flags[] = {"rmc-match", "--auto", "--team", manager, "--fixture", match_id,
            "--pre", a, p, r, "--half", a, p, r, "--verify-replay"};
        const char *menus[] = {"rmc-match", "--team", manager, "--fixture", match_id,
            "--home-formation", "442", "--away-formation", "433", "--opener", "home", "--verify-replay"};
        char input[1024];
        memset(input, 'x', 300);
        (void)snprintf(input + 300, sizeof(input) - 300,
            "\n?\n9\n%u\n--help\n0\n%u\n?\n3\n%u\n--help\n9\n2\n?\n%u\n--help\n%u\n?\n%u\n",
            1 + (bits & 1), 1 + ((bits >> 1) & 1), 1 + ((bits >> 2) & 1),
            1 + (bits & 1), 1 + ((bits >> 1) & 1), 1 + ((bits >> 2) & 1));
        Capture flagged = INVOKE(flags, "unread\n");
        Capture numbered = INVOKE(menus, input);
        assert(flagged.code == 0 && numbered.code == 0);
        assert(strstr(numbered.err, "Input troppo lungo") != NULL);
        assert(occurrences(numbered.out, "Aiuto:") == 7);
        assert(strstr(numbered.out, "Replay semantico: PASS") != NULL);
        assert(strcmp(strstr(flagged.out, "Finale:"), strstr(numbered.out, "Finale:")) == 0);
        /* Compare all event commentary, independent of setup/half-time prompts. */
        const char *x = flagged.out, *y = numbered.out;
        while ((x = strstr(x, "LIVE ")) != NULL) {
            y = strstr(y, "LIVE ");
            assert(y != NULL);
            size_t length = (size_t)(strchr(x, '\n') - x);
            assert(strncmp(x, y, length) == 0 && y[length] == '\n');
            x += length; y += length;
        }
        assert(strstr(y, "LIVE ") == NULL);
        release(&flagged); release(&numbered);
    }
    const RmcFixture *unrelated = NULL;
    for (size_t i = 0; i < RMC_CALENDAR_FIXTURES; ++i) {
        const RmcFixture *candidate = &career->foundation.calendar.fixtures[i];
        if (candidate->home_team.value != f->home_team.value && candidate->away_team.value != f->home_team.value) {
            unrelated = candidate; break;
        }
    }
    assert(unrelated != NULL);
    char unrelated_id[32];
    (void)snprintf(unrelated_id, sizeof(unrelated_id), "%" PRIu64, unrelated->id.value);
    const char *unrelated_args[] = {"rmc-match", "--auto", "--team", home_id, "--fixture", unrelated_id};
    Capture rejected = INVOKE(unrelated_args, "");
    assert(rejected.code == 2 && strstr(rejected.out, "Incontro generato") == NULL);
    release(&rejected);
    const char *zero_seed[] = {"rmc-match", "--auto", "--seed", "0", "--half", "KEEP", "--verify-replay"};
    Capture zero = INVOKE(zero_seed, "");
    assert(zero.code == 0 && strstr(zero.out, "Replay semantico: PASS") != NULL);
    release(&zero);
    const char *max_seed[] = {"rmc-match", "--auto", "--seed", "18446744073709551615"};
    Capture maximum = INVOKE(max_seed, "");
    assert(maximum.code == 0 && strstr(maximum.out, "FINISHED/COMPLETED") != NULL);
    release(&maximum);
    free(career);
    puts("CLI: generated matches, semantic references, home/away tactics, replay, input/EOF and I/O checks PASS.");
    return 0;
}
