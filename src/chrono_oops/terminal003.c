#include "input.h"
#include "mission003.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    ChronoM003Action action;
    const char *label;
    ChronoPlayer player;
} MenuAction;

static const MenuAction menu[] = {
    { CHRONO_M003_INSPECT_EQUIPMENT, "Ispeziona impianto", CHRONO_PLAYER_A },
    { CHRONO_M003_IDENTIFY_RISK, "Individua rischio", CHRONO_PLAYER_A },
    { CHRONO_M003_CONSULT_RECORD, "Consulta registro", CHRONO_PLAYER_B },
    { CHRONO_M003_TRANSMIT_WARNING, "Trasmetti avvertimento", CHRONO_PLAYER_B },
    { CHRONO_M003_RECEIVE_WARNING, "Ricevi avvertimento", CHRONO_PLAYER_A },
    { CHRONO_M003_CANCEL_DEMONSTRATION, "Cancella dimostrazione", CHRONO_PLAYER_A },
    { CHRONO_M003_SECURE_DEMONSTRATION, "Metti in sicurezza", CHRONO_PLAYER_A },
    { CHRONO_M003_VERIFY_TESTIMONY, "Verifica testimonianza", CHRONO_PLAYER_B },
    { CHRONO_M003_CHECK_RECORD, "Controlla registro", CHRONO_PLAYER_B },
    { CHRONO_M003_CERTIFY_DEMONSTRATION, "Certifica dimostrazione", CHRONO_PLAYER_A },
    { CHRONO_M003_ARCHIVE_TESTIMONY, "Archivia testimonianza", CHRONO_PLAYER_A },
    { CHRONO_M003_VALIDATE_PROVENANCE, "Convalida provenienza", CHRONO_PLAYER_B },
    { CHRONO_M003_COMPLETE_MISSION, "Completa missione", CHRONO_PLAYER_B }
};

static const char *yes_no(bool value)
{
    return value ? "SI" : "NO";
}

static void show_state(const ChronoM003State *s)
{
    printf("\n=== TURNO %u/%u — %s (%s) ===\n",
           s->turn, CHRONO_MAX_TURNS,
           s->active_player == CHRONO_PLAYER_A ? "A" : "B",
           s->active_player == CHRONO_PLAYER_A ? "1985" : "2025");
    printf("PA: %u | Messaggio: %s\n", s->action_points,
           s->warning_transmitted ? "TRASMESSO" : "NON TRASMESSO");
    printf("Persone al sicuro: %s | Testimonianza: %s\n",
           yes_no(s->people_safe),
           s->testimony_available ? "VALIDA" : "CANCELLATA");
    printf("Dimostrazione: %s | Provenienza: %s\n",
           s->demonstration_cancelled ? "ANNULLATA" :
           s->people_safe ? "SICURA" :
           s->demonstration_secured ? "IN ATTESA" : "DA VALUTARE",
           s->provenance_validated ? "CONVALIDATA" : "DA VERIFICARE");
}

static void show_hint(const ChronoM003State *s)
{
    const char *hint;

    if (s->active_player == CHRONO_PLAYER_A) {
        if (!s->equipment_inspected)
            hint = "Esamina l'impianto del 1985.";
        else if (!s->risk_identified)
            hint = "Individua il rischio della scarica.";
        else if (!s->warning_received && !s->demonstration_cancelled)
            hint = "Attendi il messaggio dal 2025.";
        else if (!s->demonstration_secured && !s->demonstration_cancelled)
            hint = "Proteggi le persone, ma conserva la fonte storica.";
        else if (!s->demonstration_certified)
            hint = "Certifica la dimostrazione dopo le verifiche.";
        else if (!s->testimony_archived)
            hint = "Archivia la testimonianza.";
        else
            hint = "La parte storica e' completata.";
    } else {
        if (!s->record_consulted)
            hint = "Consulta il registro del 2025.";
        else if (!s->warning_transmitted)
            hint = "Trasmetti l'avvertimento al 1985.";
        else if (!s->testimony_verified)
            hint = "Verifica che la testimonianza sia ancora possibile.";
        else if (!s->record_checked)
            hint = "Controlla che il registro conservi la fonte.";
        else if (!s->provenance_validated)
            hint = "Convalida la provenienza dopo l'archiviazione.";
        else
            hint = "Completa la missione.";
    }
    printf("Suggerimento: %s\n", hint);
}

static bool action_visible(const ChronoM003State *s, ChronoM003Action a)
{
    switch (a) {
    case CHRONO_M003_INSPECT_EQUIPMENT: return !s->equipment_inspected;
    case CHRONO_M003_IDENTIFY_RISK: return !s->risk_identified;
    case CHRONO_M003_CONSULT_RECORD: return !s->record_consulted;
    case CHRONO_M003_TRANSMIT_WARNING: return !s->warning_transmitted;
    case CHRONO_M003_RECEIVE_WARNING: return !s->warning_received;
    case CHRONO_M003_CANCEL_DEMONSTRATION:
        return s->risk_identified && !s->demonstration_cancelled &&
               !s->demonstration_secured;
    case CHRONO_M003_SECURE_DEMONSTRATION:
        return !s->demonstration_secured && !s->demonstration_cancelled;
    case CHRONO_M003_VERIFY_TESTIMONY: return !s->testimony_verified;
    case CHRONO_M003_CHECK_RECORD: return !s->record_checked;
    case CHRONO_M003_CERTIFY_DEMONSTRATION:
        return !s->demonstration_certified;
    case CHRONO_M003_ARCHIVE_TESTIMONY: return !s->testimony_archived;
    case CHRONO_M003_VALIDATE_PROVENANCE: return !s->provenance_validated;
    case CHRONO_M003_COMPLETE_MISSION: return !s->mission_completed;
    default: return false;
    }
}

static void show_menu(const ChronoM003State *s)
{
    puts("\nAzioni disponibili:");
    if (s->action_points > 0) {
        for (size_t i = 0; i < sizeof(menu) / sizeof(menu[0]); ++i) {
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
    const ChronoM003State *s,
    ChronoM003Action a
)
{
    const char *reason = "Azione gia' eseguita o non disponibile.";

    switch (a) {
    case CHRONO_M003_IDENTIFY_RISK:
        reason = "Devi prima ispezionare l'impianto."; break;
    case CHRONO_M003_CONSULT_RECORD:
        reason = "Il registro richiede una testimonianza valida."; break;
    case CHRONO_M003_TRANSMIT_WARNING:
        reason = "Consulta il registro prima di inviare il messaggio."; break;
    case CHRONO_M003_RECEIVE_WARNING:
        reason = "Il messaggio non e' stato ancora trasmesso."; break;
    case CHRONO_M003_CANCEL_DEMONSTRATION:
        reason = "L'intervento storico non e' piu' modificabile."; break;
    case CHRONO_M003_SECURE_DEMONSTRATION:
        reason = "Occorre conoscere il rischio e ricevere il messaggio."; break;
    case CHRONO_M003_VERIFY_TESTIMONY:
        reason = "Attendi la sicurezza delle persone e la propagazione."; break;
    case CHRONO_M003_CHECK_RECORD:
        reason = "Verifica prima la testimonianza."; break;
    case CHRONO_M003_CERTIFY_DEMONSTRATION:
        reason = "Occorrono sicurezza e registro controllato."; break;
    case CHRONO_M003_ARCHIVE_TESTIMONY:
        reason = "Certifica prima la dimostrazione."; break;
    case CHRONO_M003_VALIDATE_PROVENANCE:
        reason = "Occorrono testimonianza archiviata e fonte verificata."; break;
    case CHRONO_M003_COMPLETE_MISSION:
        reason = "Convalida prima la provenienza del messaggio."; break;
    default: break;
    }
    printf("Motivo: %s\n", reason);
    (void)s;
}

static void show_result(ChronoM003Result result)
{
    switch (result) {
    case CHRONO_M003_OK:
        puts("Azione eseguita."); break;
    case CHRONO_M003_CAUSAL_PARADOX:
        puts("PARADOSSO IMPEDITO!");
        puts("Il messaggio del 2025 proviene dal registro di una");
        puts("testimonianza del 1985. Annullare la dimostrazione");
        puts("cancellerebbe l'unica fonte di quell'informazione.");
        puts("Stato e punti azione invariati."); break;
    case CHRONO_M003_PREREQUISITE_MISSING:
        puts("Prerequisito mancante."); break;
    case CHRONO_M003_WRONG_PLAYER:
        puts("Azione riservata all'altro personaggio."); break;
    case CHRONO_M003_NO_ACTION_POINTS:
        puts("PA esauriti: termina il turno."); break;
    case CHRONO_M003_MISSION_FINISHED:
        puts("Missione gia' conclusa."); break;
    default:
        puts("Azione non valida."); break;
    }
}

int chrono_m003_run(void)
{
    ChronoM003State state;
    chrono_m003_init(&state);

    puts("ChronoOops! — Missione 003");
    puts("Il messaggio che non doveva arrivare");
    puts("Obiettivo: salvare le persone senza cancellare");
    puts("la fonte dell'avvertimento. Limite: 8 turni.");

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
            ChronoM003Trace trace;
            ChronoM003Result result = chrono_m003_end_turn(&state, &trace);
            if (result != CHRONO_M003_OK) {
                show_result(result);
                continue;
            }
            if (trace.changed) {
                printf("\nEFFETTO CAUSALE (turno %u):\n", trace.turn);
                printf("Persone al sicuro: %s -> %s\n",
                       yes_no(trace.safety_before),
                       yes_no(trace.safety_after));
                printf("Testimonianza valida: %s -> %s\n",
                       yes_no(trace.testimony_before),
                       yes_no(trace.testimony_after));
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

        ChronoM003Result result =
            chrono_m003_apply(&state, selected->action);
        show_result(result);
        if (result == CHRONO_M003_PREREQUISITE_MISSING)
            explain_prerequisite(&state, selected->action);
    }

    printf("\n%s al turno %u.\n",
           state.victory ? "VITTORIA!" : "SCONFITTA", state.turn);
    if (state.victory) {
        puts("\n1985 — La dimostrazione era terminata.");
        puts("Nessun ferito. Nessuna tragedia.");
        puts("A osservo' il registro delle testimonianze.");
        puts("\"Abbiamo evitato un incidente grazie a un messaggio");
        puts("che non avremmo dovuto ricevere.\"");

        puts("\n2025 — B controllo' la provenienza dell'avvertimento.");
        puts("\"Correzione: lo avete ricevuto perche'");
        puts("avete evitato l'incidente.\"");
        puts("Segui' un silenzio piuttosto lungo.");
        puts("\"Quindi chi ci ha avvertiti?\"");
        puts("\"Preferirei non aprire un'indagine.\"");

        puts("\nMISSIONE COMPIUTA — Tutti salvi.");
        puts("La logica chiede un giorno di ferie.");
    } else if (state.demonstration_cancelled) {
        puts("\n1985 — A annullo' la dimostrazione.");
        puts("Nessuna dimostrazione. Nessuna testimonianza.");
        puts("Nessun registro.");

        puts("\n2025 — B cerco' l'origine del messaggio.");
        puts("Il documento non esisteva piu'.");
        puts("\"Ottimo. Abbiamo eliminato il problema.\"");
        puts("\"Quale problema?\"");
        puts("\"Precisamente.\"");

        puts("\nMISSIONE FALLITA — Cancellata la testimonianza,");
        puts("cancellata la soluzione.");
    } else {
        puts("\n2025 — B osservo' l'orologio.");
        puts("\"Il messaggio era arrivato in anticipo.");
        puts("Noi, invece, siamo riusciti ad arrivare in ritardo.\"");

        puts("\n1985 — A sfoglio' inutilmente il registro.");
        puts("\"Possiamo tornare indietro?\"");
        puts("\"Siamo gia' indietro.\"");

        puts("\nMISSIONE FALLITA — Il tempo non aspetta");
        puts("nemmeno chi lo attraversa.");
    }

    puts("\n--- RIEPILOGO MISSIONE 003 ---");
    printf("Persone al sicuro: %s\n", yes_no(state.people_safe));
    printf("Testimonianza valida: %s\n",
           yes_no(state.testimony_available));
    printf("Messaggio trasmesso: %s\n",
           yes_no(state.warning_transmitted));
    printf("Provenienza convalidata: %s\n",
           yes_no(state.provenance_validated));
    return 0;
}

#ifndef CHRONO_UNIFIED_CLI
int main(void)
{
    return chrono_m003_run();
}
#endif
