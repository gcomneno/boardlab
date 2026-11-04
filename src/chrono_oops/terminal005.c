#include "mission005.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    ChronoM005Action action;
    const char *label;
    ChronoPlayer player;
} MenuAction;

static const MenuAction menu[] = {
    { CHRONO_M005_INSPECT_CLOCK, "Ispeziona orologio", CHRONO_PLAYER_A },
    { CHRONO_M005_IDENTIFY_CLOCK_LOSS, "Identifica perdita di 60 secondi", CHRONO_PLAYER_A },
    { CHRONO_M005_CONSULT_ARCHIVE, "Consulta archivio", CHRONO_PLAYER_B },
    { CHRONO_M005_DETECT_INVERSION, "Rileva inversione apparente", CHRONO_PLAYER_B },
    { CHRONO_M005_SUPERFICIAL_CORRECTION, "Correggi superficialmente gli orari", CHRONO_PLAYER_B },
    { CHRONO_M005_RECOGNIZE_INSUFFICIENCY, "Riconosci insufficienza della rettifica", CHRONO_PLAYER_B },
    { CHRONO_M005_OBSERVE_REAL_ORDER, "Osserva ordine reale", CHRONO_PLAYER_A },
    { CHRONO_M005_PRESERVE_EVIDENCE, "Preserva testimonianza indipendente", CHRONO_PLAYER_A },
    { CHRONO_M005_COMPARE_EVIDENCE, "Confronta testimonianza", CHRONO_PLAYER_B },
    { CHRONO_M005_RECONSTRUCT_ORDER, "Ricostruisci ordine reale", CHRONO_PLAYER_B },
    { CHRONO_M005_CERTIFY_SOURCE, "Certifica fonte originale", CHRONO_PLAYER_A },
    { CHRONO_M005_RECORD_PROVENANCE, "Registra provenienza storica", CHRONO_PLAYER_A },
    { CHRONO_M005_VALIDATE_CHRONOLOGY, "Valida cronologia", CHRONO_PLAYER_B },
    { CHRONO_M005_COMPLETE_CAMPAIGN, "Completa campagna", CHRONO_PLAYER_B }
};

static const char *yes_no(bool value)
{
    return value ? "SI" : "NO";
}

static void show_state(const ChronoM005State *s)
{
    printf("\n=== TURNO %u/%u — %s (%s) ===\n",
           s->turn, CHRONO_MAX_TURNS,
           s->active_player == CHRONO_PLAYER_A ? "A" : "B",
           s->active_player == CHRONO_PLAYER_A ? "1985" : "2025");

    printf("PA: %u | Orologio: -%u secondi\n",
           s->action_points, s->clock_loss_seconds);
    printf("Ordine reale: %s\n",
           s->signal_precedes_response
               ? "SEGNALE -> RISPOSTA"
               : "NON COERENTE");
    printf("Inversione nei timestamp: %s\n",
           yes_no(s->timestamps_show_inversion));
    printf("Rettifica superficiale attiva: %s\n",
           yes_no(s->superficial_correction));
    printf("Testimonianza preservata: %s | Disponibile nel 2025: %s\n",
           yes_no(s->independent_evidence_preserved),
           yes_no(s->independent_evidence_available));
    printf("Propagazione in attesa: %s\n",
           yes_no(s->evidence_propagation_pending));
    printf("Prova confrontata: %s | Ordine ricostruito: %s\n",
           yes_no(s->evidence_compared),
           yes_no(s->real_order_reconstructed));
    printf("Fonte certificata: %s | Provenienza registrata: %s\n",
           yes_no(s->original_source_certified),
           yes_no(s->historical_provenance_recorded));
    printf("Cronologia validata: %s\n",
           yes_no(s->chronology_validated));

    if (s->superficial_correction) {
        puts("ATTENZIONE: gli orari sembrano corretti,");
        puts("ma manca la prova indipendente dell'ordine reale.");
        puts("Questa scelta e' reversibile e non e' un paradosso.");
    }
}

static void show_hint(const ChronoM005State *s)
{
    const char *hint;

    if (s->active_player == CHRONO_PLAYER_A) {
        if (!s->clock_inspected)
            hint = "Ispeziona l'orologio del 1985.";
        else if (!s->clock_loss_identified)
            hint = "Identifica i sessanta secondi perduti.";
        else if (!s->real_order_observed)
            hint = "Osserva la sequenza reale degli eventi.";
        else if (!s->independent_evidence_preserved)
            hint = "Conserva una prova indipendente dall'orologio.";
        else if (!s->original_source_certified)
            hint = "Attendi la ricostruzione nel 2025, poi certifica.";
        else if (!s->historical_provenance_recorded)
            hint = "Registra la provenienza della fonte.";
        else
            hint = "La fonte storica e' certificata.";
    } else {
        if (!s->archive_consulted)
            hint = "Consulta l'archivio del 2025.";
        else if (s->superficial_correction)
            hint = "Riconosci che correggere gli orari non basta.";
        else if (!s->inversion_detected)
            hint = "Individua l'inversione apparente degli eventi.";
        else if (!s->independent_evidence_available)
            hint = "Attendi la testimonianza dal 1985.";
        else if (!s->evidence_compared)
            hint = "Confronta la prova con l'archivio originale.";
        else if (!s->real_order_reconstructed)
            hint = "Ricostruisci l'ordine SEGNALE -> RISPOSTA.";
        else if (!s->historical_provenance_recorded)
            hint = "Attendi la certificazione storica.";
        else if (!s->chronology_validated)
            hint = "Valida la cronologia documentata.";
        else
            hint = "Completa la campagna!";
    }

    printf("Suggerimento: %s\n", hint);
}

static bool action_visible(const ChronoM005State *s, ChronoM005Action a)
{
    switch (a) {
    case CHRONO_M005_INSPECT_CLOCK:
        return !s->clock_inspected;
    case CHRONO_M005_IDENTIFY_CLOCK_LOSS:
        return !s->clock_loss_identified;
    case CHRONO_M005_CONSULT_ARCHIVE:
        return !s->archive_consulted;
    case CHRONO_M005_DETECT_INVERSION:
        return !s->inversion_detected && !s->superficial_correction;
    case CHRONO_M005_SUPERFICIAL_CORRECTION:
        return !s->superficial_correction &&
               !s->original_source_certified;
    case CHRONO_M005_RECOGNIZE_INSUFFICIENCY:
        return s->superficial_correction &&
               !s->insufficiency_recognized;
    case CHRONO_M005_OBSERVE_REAL_ORDER:
        return !s->real_order_observed;
    case CHRONO_M005_PRESERVE_EVIDENCE:
        return !s->independent_evidence_preserved;
    case CHRONO_M005_COMPARE_EVIDENCE:
        return !s->evidence_compared;
    case CHRONO_M005_RECONSTRUCT_ORDER:
        return !s->real_order_reconstructed;
    case CHRONO_M005_CERTIFY_SOURCE:
        return !s->original_source_certified;
    case CHRONO_M005_RECORD_PROVENANCE:
        return !s->historical_provenance_recorded;
    case CHRONO_M005_VALIDATE_CHRONOLOGY:
        return !s->chronology_validated;
    case CHRONO_M005_COMPLETE_CAMPAIGN:
        return !s->campaign_completed;
    default:
        return false;
    }
}

static void show_menu(const ChronoM005State *s)
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

static void show_result(ChronoM005Result result)
{
    switch (result) {
    case CHRONO_M005_OK:
        puts("Azione eseguita.");
        break;
    case CHRONO_M005_PREREQUISITE_MISSING:
        puts("Prerequisito mancante o azione gia' eseguita.");
        puts("Le prove devono essere disponibili e verificate.");
        puts("Stato e punti azione invariati.");
        break;
    case CHRONO_M005_WRONG_PLAYER:
        puts("Azione riservata all'altro personaggio.");
        break;
    case CHRONO_M005_NO_ACTION_POINTS:
        puts("PA esauriti: termina il turno.");
        break;
    case CHRONO_M005_MISSION_FINISHED:
        puts("Missione gia' conclusa.");
        break;
    default:
        puts("Azione non valida.");
        break;
    }
}

int chrono_m005_run(void)
{
    ChronoM005State state;
    chrono_m005_init(&state);

    puts("ChronoOops! — Missione 005");
    puts("Il minuto che non e' mai esistito");
    puts("Il tempo non ha sbagliato: l'orologio ha perso 60 secondi.");
    puts("Ricostruisci l'ordine reale con prove indipendenti.");
    puts("Limite: 8 turni, 2 PA per turno.");

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
            ChronoM005Trace trace;
            ChronoM005Result result =
                chrono_m005_end_turn(&state, &trace);

            if (result != CHRONO_M005_OK) {
                show_result(result);
                continue;
            }

            if (trace.changed) {
                printf("\nEFFETTO CAUSALE (turno %u):\n", trace.turn);
                printf("Prova disponibile: %s -> %s\n",
                       yes_no(trace.evidence_before),
                       yes_no(trace.evidence_after));
                printf("Inversione registrata: %s -> %s\n",
                       yes_no(trace.inversion_before),
                       yes_no(trace.inversion_after));
                puts("Le verifiche obsolete sono state invalidate.");
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

        show_result(chrono_m005_apply(&state, selected->action));
    }

    printf("\n%s al turno %u.\n",
           state.victory ? "VITTORIA!" : "SCONFITTA", state.turn);

    puts("\n--- RIEPILOGO MISSIONE 005 ---");
    printf("Ordine reale preservato: %s\n",
           yes_no(state.signal_precedes_response));
    printf("Testimonianza disponibile: %s\n",
           yes_no(state.independent_evidence_available));
    printf("Ordine ricostruito: %s\n",
           yes_no(state.real_order_reconstructed));
    printf("Provenienza certificata: %s\n",
           yes_no(state.historical_provenance_recorded));
    printf("Cronologia validata: %s\n",
           yes_no(state.chronology_validated));

    if (state.victory) {
        puts("\n====================================");
        puts("       CHRONOOOPS! — FINALE");
        puts("====================================");
        puts("L'Archivio delle Anomalie e' finalmente coerente.");
        puts("Le cinque missioni hanno trovato una conclusione.");
        puts("Il tempo non aveva sbagliato nulla.");
        puts("Erano gli orologi a raccontare frottole.");
    }

    return 0;
}

#ifndef CHRONO_UNIFIED_CLI
int main(void)
{
    return chrono_m005_run();
}
#endif
