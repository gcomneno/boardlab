#include "input.h"
#include "mission002.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    ChronoM002Action action;
    const char *label;
    ChronoPlayer player;
} MenuAction;

static const MenuAction menu[] = {
    { CHRONO_M002_CONSULT_ARCHIVE, "Consulta archivio", CHRONO_PLAYER_A },
    { CHRONO_M002_IDENTIFY_DESIGN, "Individua progetto", CHRONO_PLAYER_A },
    { CHRONO_M002_INSPECT_CRONOPHONE, "Esamina Cronofono", CHRONO_PLAYER_B },
    { CHRONO_M002_BORROW_MODULE, "Anticipa modulo", CHRONO_PLAYER_B },
    { CHRONO_M002_RECORD_ORIGIN, "Registra origine", CHRONO_PLAYER_A },
    { CHRONO_M002_APPROVE_PRODUCTION, "Approva produzione", CHRONO_PLAYER_A },
    { CHRONO_M002_DESTROY_DESIGN, "Distruggi progetto", CHRONO_PLAYER_A },
    { CHRONO_M002_INSTALL_MODULE, "Installa modulo", CHRONO_PLAYER_B },
    { CHRONO_M002_REACTIVATE_CRONOPHONE, "Riattiva Cronofono", CHRONO_PLAYER_B },
    { CHRONO_M002_VERIFY_DOCUMENTATION, "Verifica documentazione", CHRONO_PLAYER_A },
    { CHRONO_M002_CONFIRM_STABILITY, "Conferma stabilita'", CHRONO_PLAYER_B },
    { CHRONO_M002_COMPLETE_MISSION, "Completa missione", CHRONO_PLAYER_B }
};

static const char *debt_name(const ChronoM002State *s)
{
    if (!s->module_borrowed) return "NON CONTRATTO";
    return s->debt_open ? "APERTO" : "ESTINTO";
}

static void show_state(const ChronoM002State *s)
{
    printf("\n=== TURNO %u/8 — %s (%s) ===\n",
           s->turn,
           s->active_player == CHRONO_PLAYER_A ? "A" : "B",
           s->active_player == CHRONO_PLAYER_A ? "1985" : "2025");
    printf("PA: %u | Debito causale: %s\n",
           s->action_points, debt_name(s));
    printf("Progetto: %s | Produzione: %s | Origine: %s\n",
           s->design_destroyed ? "DISTRUTTO" :
           s->design_identified ? "INDIVIDUATO" : "IGNOTO",
           s->production_approved ? "APPROVATA" : "NON APPROVATA",
           s->historical_origin_exists ? "STABILITA" : "DA STABILIRE");
    printf("Modulo: %s | Cronofono: %s\n",
           s->module_installed ? "INSTALLATO" :
           s->module_borrowed ? "ANTICIPATO" : "ASSENTE",
           s->cronophone_active ? "ATTIVO" : "SPENTO");
}

static void show_hint(const ChronoM002State *s)
{
    const char *hint;

    if (s->active_player == CHRONO_PLAYER_A) {
        if (!s->archive_consulted)
            hint = "Inizia consultando l'archivio.";
        else if (!s->design_identified)
            hint = "Individua il progetto del modulo.";
        else if (!s->origin_recorded && !s->design_destroyed)
            hint = "Registra l'origine del modulo.";
        else if (!s->production_approved && !s->design_destroyed)
            hint = "Approva la produzione storica.";
        else if (!s->documentation_verified)
            hint = "Verifica la documentazione dopo la propagazione.";
        else
            hint = "Il tuo lavoro storico e' concluso.";
    } else {
        if (!s->cronophone_inspected)
            hint = "Esamina il Cronofono.";
        else if (!s->module_borrowed)
            hint = "Anticipa il modulo dal futuro.";
        else if (!s->module_installed)
            hint = "Installa il modulo.";
        else if (!s->cronophone_active)
            hint = "Riattiva il Cronofono.";
        else if (!s->stability_confirmed)
            hint = "Conferma la stabilita' quando il debito e' estinto.";
        else
            hint = "Completa la missione.";
    }
    printf("Suggerimento: %s\n", hint);
}

static bool action_visible(
    const ChronoM002State *s,
    ChronoM002Action action
)
{
    switch (action) {
    case CHRONO_M002_CONSULT_ARCHIVE:
        return !s->archive_consulted;
    case CHRONO_M002_IDENTIFY_DESIGN:
        return !s->design_identified && !s->design_destroyed;
    case CHRONO_M002_INSPECT_CRONOPHONE:
        return !s->cronophone_inspected;
    case CHRONO_M002_BORROW_MODULE:
        return !s->module_borrowed;
    case CHRONO_M002_RECORD_ORIGIN:
        return !s->origin_recorded && !s->design_destroyed;
    case CHRONO_M002_APPROVE_PRODUCTION:
        return !s->production_approved && !s->design_destroyed;
    case CHRONO_M002_DESTROY_DESIGN:
        return s->design_identified &&
               !s->design_destroyed &&
               (!s->production_approved || s->debt_open);
    case CHRONO_M002_INSTALL_MODULE:
        return !s->module_installed;
    case CHRONO_M002_REACTIVATE_CRONOPHONE:
        return !s->cronophone_active;
    case CHRONO_M002_VERIFY_DOCUMENTATION:
        return !s->documentation_verified;
    case CHRONO_M002_CONFIRM_STABILITY:
        return !s->stability_confirmed;
    case CHRONO_M002_COMPLETE_MISSION:
        return !s->mission_completed;
    default:
        return false;
    }
}

static void show_menu(const ChronoM002State *s)
{
    puts("\nAzioni disponibili:");
    if (s->action_points > 0) {
        for (size_t i = 0; i < sizeof(menu) / sizeof(menu[0]); i++) {
            if (menu[i].player == s->active_player &&
                action_visible(s, menu[i].action))
                printf(" %2zu. %s\n", i + 1, menu[i].label);
        }
    }
    puts("  0. Termina turno");
    puts("  q. Interrompi partita");
}

static int read_choice(long *choice)
{
    ChronoInputResult result = chrono_input_read(stdin, choice, 1);

    if (result == CHRONO_INPUT_EOF || result == CHRONO_INPUT_QUIT)
        return 0;
    if (result != CHRONO_INPUT_NUMBER)
        return -1;

    return 1;
}

static void explain_prerequisite(
    const ChronoM002State *s,
    ChronoM002Action action
)
{
    const char *reason = NULL;

    switch (action) {
    case CHRONO_M002_CONSULT_ARCHIVE:
        reason = "Archivio gia' consultato.";
        break;
    case CHRONO_M002_IDENTIFY_DESIGN:
        reason = !s->archive_consulted
            ? "Consulta prima l'archivio."
            : s->design_destroyed
            ? "Il progetto e' stato distrutto."
            : "Progetto gia' individuato.";
        break;
    case CHRONO_M002_INSPECT_CRONOPHONE:
        reason = "Cronofono gia' esaminato.";
        break;
    case CHRONO_M002_BORROW_MODULE:
        reason = !s->cronophone_inspected
            ? "Esamina prima il Cronofono."
            : !s->design_identified
            ? "Il progetto storico non e' ancora individuato."
            : s->design_destroyed
            ? "Il progetto storico e' stato distrutto."
            : "Il modulo e' gia' stato anticipato: prestito unico.";
        break;
    case CHRONO_M002_RECORD_ORIGIN:
        reason = !s->design_identified
            ? "Individua prima il progetto."
            : s->design_destroyed
            ? "Il progetto necessario e' distrutto."
            : "Origine gia' registrata.";
        break;
    case CHRONO_M002_APPROVE_PRODUCTION:
        reason = !s->origin_recorded
            ? "Registra prima l'origine del modulo."
            : s->design_destroyed
            ? "Non puoi approvare un progetto distrutto."
            : "Produzione gia' approvata.";
        break;
    case CHRONO_M002_DESTROY_DESIGN:
        reason = !s->design_identified
            ? "Non hai ancora individuato il progetto."
            : s->design_destroyed
            ? "Progetto gia' distrutto."
            : "Produzione gia' approvata: il progetto e' vincolato.";
        break;
    case CHRONO_M002_INSTALL_MODULE:
        reason = !s->module_borrowed
            ? "Anticipa prima il modulo."
            : "Modulo gia' installato.";
        break;
    case CHRONO_M002_REACTIVATE_CRONOPHONE:
        reason = !s->module_installed
            ? "Installa prima il modulo."
            : "Cronofono gia' attivo.";
        break;
    case CHRONO_M002_VERIFY_DOCUMENTATION:
        reason = !s->historical_origin_exists
            ? "Attendi la propagazione dell'approvazione storica."
            : "Documentazione gia' verificata.";
        break;
    case CHRONO_M002_CONFIRM_STABILITY:
        reason = !s->cronophone_active
            ? "Riattiva prima il Cronofono."
            : !s->documentation_verified
            ? "A deve verificare la documentazione."
            : s->debt_open
            ? "Il debito causale deve essere estinto."
            : "Stabilita' gia' confermata.";
        break;
    case CHRONO_M002_COMPLETE_MISSION:
        reason = !s->stability_confirmed
            ? "Conferma prima la stabilita'."
            : !s->cronophone_active
            ? "Il Cronofono non e' attivo."
            : s->debt_open
            ? "Il debito causale e' ancora aperto."
            : !s->historical_origin_exists
            ? "Manca un'origine storica valida."
            : "Missione gia' completata.";
        break;
    case CHRONO_M002_WAIT:
        break;
    }

    if (reason != NULL)
        printf("Motivo: %s\n", reason);
}

static void show_result(ChronoM002Result result)
{
    switch (result) {
    case CHRONO_M002_OK:
        puts("Azione eseguita."); break;
    case CHRONO_M002_CAUSAL_PARADOX:
        puts("PARADOSSO IMPEDITO: il modulo e' stato anticipato.");
        puts("Debito ancora aperto: distruggere l'unico progetto");
        puts("renderebbe impossibile restituire l'origine dovuta.");
        puts("Stato e punti azione invariati."); break;
    case CHRONO_M002_PREREQUISITE_MISSING:
        puts("Prerequisito mancante o azione gia' eseguita."); break;
    case CHRONO_M002_WRONG_PLAYER:
        puts("Azione riservata all'altro personaggio."); break;
    case CHRONO_M002_NO_ACTION_POINTS:
        puts("PA esauriti: termina il turno."); break;
    case CHRONO_M002_MISSION_FINISHED:
        puts("Missione gia' conclusa."); break;
    default:
        puts("Azione non valida."); break;
    }
}

int chrono_m002_run(void)
{
    ChronoM002State state;
    chrono_m002_init(&state);

    puts("ChronoOops! — Missione 002");
    puts("Il prestito di domani");
    puts("Obiettivo: riattivare il Cronofono e saldare il debito entro 8 turni.");

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
            ChronoM002Trace trace;
            ChronoM002Result result = chrono_m002_end_turn(&state, &trace);
            if (result != CHRONO_M002_OK) {
                show_result(result);
                continue;
            }
            if (trace.changed) {
                printf("\nEFFETTO CAUSALE (turno %u):\n", trace.turn);
                printf("Debito: %s -> %s\n",
                       trace.debt_before ? "APERTO" : "ESTINTO",
                       trace.debt_after ? "APERTO" : "ESTINTO");
                printf("Origine storica: %s -> %s\n",
                       trace.origin_before ? "SI" : "NO",
                       trace.origin_after ? "SI" : "NO");
            }
            continue;
        }
        if (choice < 1 ||
            (unsigned long)choice > sizeof(menu) / sizeof(menu[0])) {
            puts("Scelta fuori intervallo.");
            continue;
        }

        const MenuAction *selected = &menu[(size_t)choice - 1];
        if (selected->player != state.active_player) {
            puts("Azione non disponibile per questo personaggio.");
            continue;
        }
        ChronoM002Result result =
            chrono_m002_apply(&state, selected->action);
        show_result(result);
        if (result == CHRONO_M002_PREREQUISITE_MISSING)
            explain_prerequisite(&state, selected->action);
    }

    if (state.victory) {
        printf("\nVITTORIA! Cronofono riattivato al turno %u.\n",
               state.turn);
        puts("Debito causale ESTINTO. Origine storica confermata.");

        puts("\n2025 — Il Cronofono riprese a funzionare.");
        puts("B osservo' il modulo, poi la ricevuta del prestito.");
        puts("\"Tecnicamente, l'ho preso prima che esistesse.\"");

        puts("\n1985 — A contemplo' il progetto appena approvato.");
        puts("\"Tecnicamente, l'ho inventato dopo che");
        puts("qualcuno l'aveva gia' usato.\"");

        puts("\nIl debito causale era estinto.");
        puts("L'origine storica confermata.");
        puts("La contabilita' temporale finalmente in ordine.");
        puts("Nessuno dei due oso' chiedere gli interessi.");

        puts("\nMISSIONE COMPIUTA — Il futuro ha restituito");
        puts("il passato. Senza ricevuta fiscale.");
    } else {
        printf("\nSCONFITTA al turno %u.\n", state.turn);

        puts("\n2025 — Il Cronofono rimase spento.");
        puts("B fisso' il calendario.");
        puts("Otto turni, un prestito temporale e nessuna soluzione.");
        puts("\"Possiamo chiedere una proroga?\"");

        puts("\n1985 — A consulto' il regolamento.");
        puts("\"Certamente. Devi presentare la domanda ieri.\"");

        puts("\nMISSIONE FALLITA — Il tempo e' scaduto.");
        puts("Il debito, eventualmente, no.");
    }

    puts("\n--- RIEPILOGO MISSIONE 002 ---");
    printf("Esito: %s | Turno: %u/%u\n",
           state.victory ? "VITTORIA" : "SCONFITTA",
           state.turn, CHRONO_MAX_TURNS);
    printf("Cronofono: %s\n",
           state.cronophone_active ? "ATTIVO" : "SPENTO");
    printf("Debito causale: %s\n", debt_name(&state));
    printf("Origine storica: %s\n",
           state.historical_origin_exists ? "STABILITA" : "ASSENTE");
    return 0;
}

#ifndef CHRONO_UNIFIED_CLI
int main(void)
{
    return chrono_m002_run();
}
#endif
