#include "chrono.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * ChronoOops! Terminal Edition.
 *
 * A thin interactive adapter around the Mission 001 engine.
 * The engine remains responsible for all game rules.
 */

typedef struct {
    ChronoAction action;
    const char *name;
} MenuAction;

static const MenuAction actions[] = {
    { CHRONO_ACTION_WAIT, "Attendi" },
    { CHRONO_ACTION_MOVE_ARCHIVE, "Raggiungi archivio" },
    { CHRONO_ACTION_READ_PLANS, "Consulta progetti" },
    { CHRONO_ACTION_INSPECT_RUINS, "Esamina resti del ponte" },
    { CHRONO_ACTION_IDENTIFY_DEFECT, "Identifica difetto" },
    { CHRONO_ACTION_MEASURE_RIVER, "Misura il fiume" },
    { CHRONO_ACTION_SELECT_BRIDGE, "Seleziona ponte" },
    { CHRONO_ACTION_APPROVE_BRIDGE, "Approva ponte" },
    { CHRONO_ACTION_SELECT_DUCK, "Seleziona Papera" },
    { CHRONO_ACTION_APPROVE_DUCK, "Approva Papera" },
    { CHRONO_ACTION_CROSS_BRIDGE, "Attraversa ponte" },
    { CHRONO_ACTION_REACH_TOWER, "Raggiungi torre" },
    { CHRONO_ACTION_VERIFY_PLAN, "Verifica progetto" },
    { CHRONO_ACTION_INSPECT_DUCK, "Esamina Papera" },
    { CHRONO_ACTION_PREPARE_DUCK, "Prepara Papera" },
    { CHRONO_ACTION_READ_MANUAL, "Leggi manuale" },
    { CHRONO_ACTION_CALIBRATE_DUCK, "Tara Papera" },
    { CHRONO_ACTION_START_DUCK, "Avvia Papera" },
    { CHRONO_ACTION_CROSS_RIVER, "Attraversa fiume" },
    { CHRONO_ACTION_VERIFY_INTERVENTION, "Verifica intervento" },
    { CHRONO_ACTION_ENTER_TOWER, "Entra nella torre" },
    { CHRONO_ACTION_RECOVER_CAPSULE, "Recupera capsula" }
};

static const char *result_name(ChronoResult result)
{
    switch (result) {
    case CHRONO_OK: return "Azione riuscita";
    case CHRONO_INVALID_ACTION: return "Azione non valida";
    case CHRONO_WRONG_PLAYER: return "Azione del personaggio sbagliato";
    case CHRONO_NO_ACTION_POINTS: return "Punti azione esauriti";
    case CHRONO_PREREQUISITE_MISSING: return "Prerequisito mancante";
    case CHRONO_MISSION_FINISHED: return "Missione conclusa";
    }
    return "Risultato sconosciuto";
}

static const char *location_name(ChronoLocation location)
{
    switch (location) {
    case CHRONO_LOCATION_MUNICIPALITY: return "Municipio";
    case CHRONO_LOCATION_ARCHIVE: return "Archivio";
    case CHRONO_LOCATION_WEST_BANK: return "Riva occidentale";
    case CHRONO_LOCATION_BRIDGE: return "Ponte";
    case CHRONO_LOCATION_DUCK: return "Papera";
    case CHRONO_LOCATION_EAST_BANK: return "Riva orientale";
    case CHRONO_LOCATION_TOWER: return "Torre dell'Orologio";
    }
    return "Luogo sconosciuto";
}

/*
 * Hints describe observable opportunities, not a complete plan.
 * They never alter the state or bypass the engine's validation.
 */
static void show_hint(const ChronoState *s)
{
    if (s->active_player == CHRONO_PLAYER_B) {
        if (s->world.bridge_exists &&
            s->location_b == CHRONO_LOCATION_WEST_BANK) {
            puts("Suggerimento: il ponte esiste! Puoi attraversarlo.");
        } else if (s->location_b == CHRONO_LOCATION_BRIDGE) {
            puts("Suggerimento: la torre è raggiungibile da qui.");
        } else if (s->location_b == CHRONO_LOCATION_TOWER &&
                   !s->tower_entered) {
            puts("Suggerimento: sei alla torre. Cerca un ingresso.");
        } else if (s->tower_entered &&
                   !s->world.capsule_recovered) {
            puts("Suggerimento: la capsula è qui vicino.");
        } else if (!s->ruins_inspected) {
            puts("Suggerimento: i resti del ponte possono raccontare qualcosa.");
        }
    } else if (s->active_player == CHRONO_PLAYER_A) {
        if (s->location_a == CHRONO_LOCATION_MUNICIPALITY) {
            puts("Suggerimento: l'archivio conserva i progetti storici.");
        } else if (!s->plans_read) {
            puts("Suggerimento: consulta i progetti disponibili.");
        } else if (!s->plan_approved &&
                   (s->defect_known || s->river_measured)) {
            puts("Suggerimento: le informazioni dal futuro consentono una scelta.");
        }
    }
}

static const char *plan_name(ChronoPlan plan)
{
    switch (plan) {
    case CHRONO_PLAN_NONE: return "NESSUNO";
    case CHRONO_PLAN_BRIDGE: return "PONTE";
    case CHRONO_PLAN_DUCK: return "PAPERA";
    }
    return "SCONOSCIUTO";
}

/* Explain only prerequisites directly supported by engine state. */
static void explain_prerequisite(
    const ChronoState *s,
    ChronoAction action
)
{
    switch (action) {
    case CHRONO_ACTION_MOVE_ARCHIVE:
        if (s->location_a != CHRONO_LOCATION_MUNICIPALITY)
            puts("Motivo: A non si trova al Municipio.");
        break;
    case CHRONO_ACTION_READ_PLANS:
        if (s->location_a != CHRONO_LOCATION_ARCHIVE)
            puts("Motivo: raggiungi prima l'archivio.");
        break;
    case CHRONO_ACTION_SELECT_BRIDGE:
        if (!s->plans_read)
            puts("Motivo: consulta prima i progetti.");
        else if (!s->defect_known)
            puts("Motivo: B deve identificare il difetto.");
        else if (s->plan_approved)
            puts("Motivo: un progetto e' gia' approvato.");
        break;
    case CHRONO_ACTION_APPROVE_BRIDGE:
        if (!s->plan_selected ||
            s->selected_plan != CHRONO_PLAN_BRIDGE)
            puts("Motivo: seleziona prima il ponte.");
        else if (s->plan_approved)
            puts("Motivo: il progetto e' gia' approvato.");
        break;
    case CHRONO_ACTION_SELECT_DUCK:
        if (!s->plans_read)
            puts("Motivo: consulta prima i progetti.");
        else if (!s->river_measured)
            puts("Motivo: B deve misurare il fiume.");
        else if (s->plan_approved)
            puts("Motivo: un progetto e' gia' approvato.");
        break;
    case CHRONO_ACTION_APPROVE_DUCK:
        if (!s->plan_selected ||
            s->selected_plan != CHRONO_PLAN_DUCK)
            puts("Motivo: seleziona prima la Papera.");
        else if (s->plan_approved)
            puts("Motivo: il progetto e' gia' approvato.");
        break;
    case CHRONO_ACTION_CROSS_BRIDGE:
        if (!s->world.bridge_exists)
            puts("Motivo: il ponte non esiste ancora.");
        else if (s->location_b != CHRONO_LOCATION_WEST_BANK)
            puts("Motivo: B non e' sulla riva occidentale.");
        break;
    case CHRONO_ACTION_REACH_TOWER:
        if (!s->bridge_crossed)
            puts("Motivo: attraversa prima il ponte.");
        else if (s->location_b != CHRONO_LOCATION_BRIDGE)
            puts("Motivo: B non si trova sul ponte.");
        break;
    case CHRONO_ACTION_INSPECT_DUCK:
        if (!s->world.duck_exists)
            puts("Motivo: la Papera non esiste ancora.");
        else if (s->location_b != CHRONO_LOCATION_WEST_BANK)
            puts("Motivo: B non e' sulla riva occidentale.");
        break;
    case CHRONO_ACTION_PREPARE_DUCK:
        if (!s->duck_inspected)
            puts("Motivo: esamina prima la Papera.");
        break;
    case CHRONO_ACTION_START_DUCK:
        if (!s->world.duck_exists)
            puts("Motivo: la Papera non esiste.");
        else if (!s->world.duck_prepared)
            puts("Motivo: la Papera non e' preparata.");
        else if (!s->world.duck_calibrated)
            puts("Motivo: manca la taratura storica.");
        else if (s->location_b != CHRONO_LOCATION_WEST_BANK)
            puts("Motivo: B non e' sulla riva occidentale.");
        break;
    case CHRONO_ACTION_CROSS_RIVER:
        if (!s->world.duck_running)
            puts("Motivo: avvia prima la Papera.");
        else if (s->location_b != CHRONO_LOCATION_WEST_BANK)
            puts("Motivo: B non e' sulla riva occidentale.");
        break;
    case CHRONO_ACTION_ENTER_TOWER:
        if (s->location_b != CHRONO_LOCATION_TOWER &&
            s->location_b != CHRONO_LOCATION_EAST_BANK)
            puts("Motivo: raggiungi prima la torre.");
        break;
    case CHRONO_ACTION_RECOVER_CAPSULE:
        if (!s->tower_entered)
            puts("Motivo: entra prima nella torre.");
        else if (s->location_b != CHRONO_LOCATION_TOWER)
            puts("Motivo: B non si trova nella torre.");
        break;
    default:
        break;
    }
}

static void show_state(const ChronoState *s)
{
    printf("\n=== TURNO %u/8 — %s (%s) ===\n",
           s->turn,
           s->active_player == CHRONO_PLAYER_A ? "A" : "B",
           s->active_player == CHRONO_PLAYER_A ? "1985" : "2025");
    printf("PA disponibili: %u\n", s->action_points);
    printf("Progetto: %s | Stato: %s\n",
           plan_name(s->selected_plan),
           s->plan_approved ? "APPROVATO" :
           s->plan_selected ? "SELEZIONATO, NON APPROVATO" :
           "NON SELEZIONATO");
    printf("Posizioni: A = %s | B = %s\n",
           location_name(s->location_a),
           location_name(s->location_b));
    printf("Ponte: %s | Papera: %s | Tarata: %s\n",
           s->world.bridge_exists ? "SI" : "NO",
           s->world.duck_exists ? "SI" : "NO",
           s->world.duck_calibrated ? "SI" : "NO");
    printf("Capsula: %s | Registro delle Maree: %s\n",
           s->world.capsule_recovered ? "RECUPERATA" : "DA RECUPERARE",
           s->world.tide_register_acquired ? "ACQUISITO" : "NON ACQUISITO");
}

/*
 * The menu filters actions by character, not by prerequisites.
 * Only chrono_apply() decides whether an action is legal.
 */
static bool action_belongs_to_player(
    ChronoAction action,
    ChronoPlayer player
)
{
    switch (action) {
    case CHRONO_ACTION_WAIT:
        return true;

    case CHRONO_ACTION_MOVE_ARCHIVE:
    case CHRONO_ACTION_READ_PLANS:
    case CHRONO_ACTION_SELECT_BRIDGE:
    case CHRONO_ACTION_APPROVE_BRIDGE:
    case CHRONO_ACTION_SELECT_DUCK:
    case CHRONO_ACTION_APPROVE_DUCK:
    case CHRONO_ACTION_VERIFY_PLAN:
    case CHRONO_ACTION_READ_MANUAL:
    case CHRONO_ACTION_CALIBRATE_DUCK:
    case CHRONO_ACTION_VERIFY_INTERVENTION:
        return player == CHRONO_PLAYER_A;

    default:
        return player == CHRONO_PLAYER_B;
    }
}

static void show_menu(const ChronoState *state)
{
    if (state->action_points == 0) {
        puts("\nPA esauriti: termina il turno.");
    } else {
        size_t count = sizeof(actions) / sizeof(actions[0]);

        puts("\nAzioni disponibili per questo personaggio:");
        for (size_t i = 0; i < count; ++i) {
            if (action_belongs_to_player(
                    actions[i].action, state->active_player)) {
                printf("%2zu. %s\n", i + 1, actions[i].name);
            }
        }
    }

    puts(" 0. Termina turno");
    puts(" q. Esci dal gioco");
}

static int read_choice(long *choice)
{
    char line[64];
    char *end;

    if (!fgets(line, sizeof(line), stdin)) {
        return 0;
    }
    if (line[0] == 'q' || line[0] == 'Q') {
        return -1;
    }

    *choice = strtol(line, &end, 10);
    if (end == line || (*end != '\n' && *end != '\0')) {
        return 2;
    }
    return 1;
}

int chrono_m001_run(void)
{
    ChronoState state;
    chrono_init(&state);

    puts("ChronoOops! — Missione 001");
    puts("Il ponte che non c'era. Poi c'era. Poi era un'anatra.");
    puts("Obiettivo: recuperare la capsula entro 8 turni.");

    while (!state.finished) {
        show_state(&state);
        show_hint(&state);
        show_menu(&state);

        printf("\nScelta: ");
        fflush(stdout);

        long choice = -1;
        int input = read_choice(&choice);

        if (input == 0 || input == -1) {
            puts("\nPartita interrotta.");
            return 0;
        }
        if (input == 2) {
            puts("Inserisci un numero valido.");
            continue;
        }

        if (choice == 0) {
            ChronoTrace trace;
            ChronoResult result = chrono_end_turn(&state, &trace);

            if (result != CHRONO_OK) {
                printf("%s\n", result_name(result));
                continue;
            }

            if (trace.changed) {
                printf("\nEFFETTO CAUSALE (turno %u):\n", trace.turn);
                printf("Ponte: %s -> %s\n",
                       trace.before.bridge_exists ? "SI" : "NO",
                       trace.after.bridge_exists ? "SI" : "NO");
                printf("Papera: %s -> %s\n",
                       trace.before.duck_exists ? "SI" : "NO",
                       trace.after.duck_exists ? "SI" : "NO");
                printf("Taratura: %s -> %s\n",
                       trace.before.duck_calibrated ? "SI" : "NO",
                       trace.after.duck_calibrated ? "SI" : "NO");
            }
            continue;
        }

        size_t count = sizeof(actions) / sizeof(actions[0]);
        if (choice < 1 || (unsigned long)choice > count) {
            puts("Scelta fuori intervallo.");
            continue;
        }

        ChronoAction action = actions[(size_t)choice - 1].action;

        if (state.action_points == 0) {
            puts("PA esauriti: premi 0 per terminare il turno.");
            continue;
        }

        if (!action_belongs_to_player(action, state.active_player)) {
            puts("Azione non disponibile per questo personaggio.");
            continue;
        }

        ChronoResult result = chrono_apply(&state, action);
        printf("%s\n", result_name(result));
        if (result == CHRONO_PREREQUISITE_MISSING) {
            explain_prerequisite(&state, action);
        }
    }

    if (state.victory) {
        printf("\nVITTORIA! Capsula recuperata al turno %u.\n",
               state.turn);
        if (state.world.tide_register_acquired) {
            puts("BONUS: Registro delle Maree acquisito!");
        }
    } else {
        puts("\nSCONFITTA! La torre e' stata demolita.");
    }

    return 0;
}

#ifndef CHRONO_UNIFIED_CLI
int main(void)
{
    return chrono_m001_run();
}
#endif
