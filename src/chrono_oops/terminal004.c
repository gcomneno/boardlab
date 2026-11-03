#include "mission004.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    ChronoM004Action action;
    const char *label;
    ChronoPlayer player;
} MenuAction;

static const MenuAction menu[] = {
    { CHRONO_M004_INSPECT_DISTRIBUTOR, "Ispeziona distributore", CHRONO_PLAYER_A },
    { CHRONO_M004_IDENTIFY_PULSE, "Identifica IMPULSO-85", CHRONO_PLAYER_A },
    { CHRONO_M004_EXAMINE_BEACON, "Esamina il Faro", CHRONO_PLAYER_B },
    { CHRONO_M004_EXAMINE_REGISTER, "Esamina il Registro", CHRONO_PLAYER_B },
    { CHRONO_M004_SELECT_BEACON, "Collega solo il Faro", CHRONO_PLAYER_A },
    { CHRONO_M004_SELECT_REGISTER, "Collega solo il Registro", CHRONO_PLAYER_A },
    { CHRONO_M004_DISCOVER_DUAL_CONTACT, "Scopri il doppio contatto", CHRONO_PLAYER_A },
    { CHRONO_M004_ENABLE_DUAL, "Collega Faro e Registro", CHRONO_PLAYER_A },
    { CHRONO_M004_VERIFY_BEACON, "Verifica il Faro", CHRONO_PLAYER_B },
    { CHRONO_M004_VERIFY_REGISTER, "Verifica il Registro", CHRONO_PLAYER_B },
    { CHRONO_M004_CERTIFY_CONFIGURATION, "Certifica configurazione", CHRONO_PLAYER_A },
    { CHRONO_M004_RECORD_COMMON_ORIGIN, "Registra origine comune", CHRONO_PLAYER_A },
    { CHRONO_M004_VALIDATE_COHERENCE, "Convalida coerenza", CHRONO_PLAYER_B },
    { CHRONO_M004_COMPLETE_MISSION, "Completa missione", CHRONO_PLAYER_B }
};

static const char *yes_no(bool value)
{
    return value ? "SI" : "NO";
}

static const char *config_name(ChronoM004Config config)
{
    switch (config) {
    case CHRONO_M004_CONFIG_BEACON: return "FARO";
    case CHRONO_M004_CONFIG_REGISTER: return "REGISTRO";
    case CHRONO_M004_CONFIG_DUAL: return "DOPPIO";
    default: return "NESSUNA";
    }
}

static void show_state(const ChronoM004State *s)
{
    printf("\n=== TURNO %u/%u — %s (%s) ===\n",
           s->turn, CHRONO_MAX_TURNS,
           s->active_player == CHRONO_PLAYER_A ? "A" : "B",
           s->active_player == CHRONO_PLAYER_A ? "1985" : "2025");
    printf("PA: %u | Impulso storico: %s\n",
           s->action_points, s->pulse_identified ? "IDENTIFICATO" : "IGNOTO");
    printf("Configurazione storica: %s | Nel 2025: %s\n",
           config_name(s->configuration),
           config_name(s->propagated_configuration));
    printf("Propagazione in attesa: %s\n", yes_no(s->propagation_pending));
    printf("Faro attivo: %s | Registro attivo: %s\n",
           yes_no(s->beacon_active), yes_no(s->register_active));
    printf("Faro verificato: %s | Registro verificato: %s\n",
           yes_no(s->beacon_verified), yes_no(s->register_verified));
    printf("Certificazione: %s | Origine comune: %s\n",
           yes_no(s->configuration_certified),
           yes_no(s->common_origin_recorded));
    printf("Coerenza: %s\n", yes_no(s->coherence_validated));

    if (s->propagated_configuration == CHRONO_M004_CONFIG_BEACON ||
        s->propagated_configuration == CHRONO_M004_CONFIG_REGISTER) {
        puts("ESITO INCOMPLETO: un solo effetto riceve l'impulso.");
        puts("Non e' un paradosso: A puo' correggere il collegamento.");
    }
}

static void show_hint(const ChronoM004State *s)
{
    const char *hint;

    if (s->active_player == CHRONO_PLAYER_A) {
        if (!s->distributor_inspected)
            hint = "Esamina il distributore storico.";
        else if (!s->pulse_identified)
            hint = "Identifica l'unico impulso del 1985.";
        else if (!s->dual_contact_discovered)
            hint = "Cerca il contatto doppio non documentato.";
        else if (s->configuration != CHRONO_M004_CONFIG_DUAL)
            hint = "Collega entrambi gli effetti alla stessa causa.";
        else if (s->propagation_pending)
            hint = "Termina il turno per propagare la configurazione.";
        else if (!s->configuration_certified)
            hint = "Attendi le verifiche nel 2025, poi certifica.";
        else if (!s->common_origin_recorded)
            hint = "Registra l'origine comune dei due effetti.";
        else
            hint = "La parte storica e' completata.";
    } else {
        if (!s->beacon_examined)
            hint = "Esamina il Faro nel 2025.";
        else if (!s->register_examined)
            hint = "Esamina il Registro nel 2025.";
        else if (s->propagated_configuration != CHRONO_M004_CONFIG_DUAL)
            hint = "I due effetti richiedono un collegamento storico comune.";
        else if (!s->beacon_verified)
            hint = "Verifica il Faro dopo la propagazione.";
        else if (!s->register_verified)
            hint = "Verifica il Registro dopo la propagazione.";
        else if (!s->common_origin_recorded)
            hint = "Attendi la certificazione storica dell'origine comune.";
        else if (!s->coherence_validated)
            hint = "Convalida la coerenza dei due effetti.";
        else
            hint = "Completa la missione.";
    }

    printf("Suggerimento: %s\n", hint);
}

static bool action_visible(const ChronoM004State *s, ChronoM004Action a)
{
    switch (a) {
    case CHRONO_M004_INSPECT_DISTRIBUTOR:
        return !s->distributor_inspected;
    case CHRONO_M004_IDENTIFY_PULSE:
        return !s->pulse_identified;
    case CHRONO_M004_EXAMINE_BEACON:
        return !s->beacon_examined;
    case CHRONO_M004_EXAMINE_REGISTER:
        return !s->register_examined;
    case CHRONO_M004_SELECT_BEACON:
        return s->configuration != CHRONO_M004_CONFIG_BEACON &&
               !s->configuration_certified;
    case CHRONO_M004_SELECT_REGISTER:
        return s->configuration != CHRONO_M004_CONFIG_REGISTER &&
               !s->configuration_certified;
    case CHRONO_M004_DISCOVER_DUAL_CONTACT:
        return !s->dual_contact_discovered;
    case CHRONO_M004_ENABLE_DUAL:
        return s->configuration != CHRONO_M004_CONFIG_DUAL &&
               !s->configuration_certified;
    case CHRONO_M004_VERIFY_BEACON:
        return !s->beacon_verified;
    case CHRONO_M004_VERIFY_REGISTER:
        return !s->register_verified;
    case CHRONO_M004_CERTIFY_CONFIGURATION:
        return !s->configuration_certified;
    case CHRONO_M004_RECORD_COMMON_ORIGIN:
        return !s->common_origin_recorded;
    case CHRONO_M004_VALIDATE_COHERENCE:
        return !s->coherence_validated;
    case CHRONO_M004_COMPLETE_MISSION:
        return !s->mission_completed;
    default:
        return false;
    }
}

static void show_menu(const ChronoM004State *s)
{
    puts("\nAzioni disponibili:");

    if (s->action_points > 0) {
        for (size_t i = 0; i < sizeof(menu) / sizeof(menu[0]); ++i) {
            if (menu[i].player == s->active_player &&
                action_visible(s, menu[i].action)) {
                printf(" %2zu. %s\n", i + 1, menu[i].label);
            }
        }
    }

    puts("  0. Termina turno");
    puts("  q. Interrompi partita");
}

static int read_choice(long *choice)
{
    char buffer[80];
    char *end;

    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        return 0;
    if (buffer[0] == 'q' && (buffer[1] == '\n' || buffer[1] == '\0'))
        return 0;

    errno = 0;
    *choice = strtol(buffer, &end, 10);

    if (errno != 0 || end == buffer ||
        (*end != '\n' && *end != '\0'))
        return -1;

    return 1;
}

static void show_result(ChronoM004Result result)
{
    switch (result) {
    case CHRONO_M004_OK:
        puts("Azione eseguita.");
        break;
    case CHRONO_M004_PREREQUISITE_MISSING:
        puts("Prerequisito mancante o azione gia' eseguita.");
        puts("Controlla configurazione, propagazione e verifiche.");
        puts("Stato e punti azione invariati.");
        break;
    case CHRONO_M004_WRONG_PLAYER:
        puts("Azione riservata all'altro personaggio.");
        break;
    case CHRONO_M004_NO_ACTION_POINTS:
        puts("PA esauriti: termina il turno.");
        break;
    case CHRONO_M004_MISSION_FINISHED:
        puts("Missione gia' conclusa.");
        break;
    default:
        puts("Azione non valida.");
        break;
    }
}

int chrono_m004_run(void)
{
    ChronoM004State state;
    chrono_m004_init(&state);

    puts("ChronoOops! — Missione 004");
    puts("Due eventi, una sola causa");
    puts("Un solo impulso nel 1985 deve alimentare");
    puts("Faro e Registro nel 2025. Limite: 8 turni.");
    puts("Le configurazioni esclusive sono correggibili.");
    puts("Ogni modifica storica invalida le verifiche precedenti.");

    while (!state.finished) {
        show_state(&state);
        show_hint(&state);
        show_menu(&state);

        printf("\nScelta: ");
        fflush(stdout);

        long choice;
        int input = read_choice(&choice);

        if (input == 0) {
            puts("\nPartita interrotta.");
            return 0;
        }
        if (input < 0) {
            puts("Inserisci un numero valido.");
            continue;
        }

        if (choice == 0) {
            ChronoM004Trace trace;
            ChronoM004Result result =
                chrono_m004_end_turn(&state, &trace);

            if (result != CHRONO_M004_OK) {
                show_result(result);
                continue;
            }

            if (trace.changed) {
                printf("\nEFFETTO CAUSALE (turno %u):\n", trace.turn);
                printf("Configurazione: %s -> %s\n",
                       config_name(trace.configuration_before),
                       config_name(trace.configuration_after));
                printf("Faro attivo: %s -> %s\n",
                       yes_no(trace.beacon_before),
                       yes_no(trace.beacon_after));
                printf("Registro attivo: %s -> %s\n",
                       yes_no(trace.register_before),
                       yes_no(trace.register_after));
                puts("Le verifiche precedenti sono state invalidate.");
            }
            continue;
        }

        if (choice < 1 ||
            (unsigned long)choice > sizeof(menu) / sizeof(menu[0])) {
            puts("Scelta fuori intervallo.");
            continue;
        }

        const MenuAction *selected = &menu[(size_t)choice - 1];

        if (selected->player != state.active_player ||
            !action_visible(&state, selected->action)) {
            puts("Azione non disponibile in questo momento.");
            continue;
        }

        show_result(chrono_m004_apply(&state, selected->action));
    }

    printf("\n%s al turno %u.\n",
           state.victory ? "VITTORIA!" : "SCONFITTA", state.turn);
    puts("\n--- RIEPILOGO MISSIONE 004 ---");
    printf("Configurazione: %s\n", config_name(state.configuration));
    printf("Faro attivo: %s\n", yes_no(state.beacon_active));
    printf("Registro attivo: %s\n", yes_no(state.register_active));
    printf("Faro verificato: %s\n", yes_no(state.beacon_verified));
    printf("Registro verificato: %s\n", yes_no(state.register_verified));
    printf("Origine comune registrata: %s\n",
           yes_no(state.common_origin_recorded));
    printf("Coerenza convalidata: %s\n", yes_no(state.coherence_validated));

    return 0;
}

#ifndef CHRONO_UNIFIED_CLI
int main(void)
{
    return chrono_m004_run();
}
#endif
