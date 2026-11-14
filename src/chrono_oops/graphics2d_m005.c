#include "mission005.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

enum { M005_GUI_ACTION_COUNT = 15 };

typedef struct {
    ChronoM005Action action;
    const char *label;
    ChronoPlayer player;
} M005GuiAction;

static const M005GuiAction m005_actions[] = {
    {CHRONO_M005_WAIT, "Attendi", CHRONO_PLAYER_A},
    {CHRONO_M005_INSPECT_CLOCK, "Ispeziona orologio", CHRONO_PLAYER_A},
    {CHRONO_M005_IDENTIFY_CLOCK_LOSS, "Identifica perdita di 60 secondi", CHRONO_PLAYER_A},
    {CHRONO_M005_CONSULT_ARCHIVE, "Consulta archivio", CHRONO_PLAYER_B},
    {CHRONO_M005_DETECT_INVERSION, "Rileva inversione apparente", CHRONO_PLAYER_B},
    {CHRONO_M005_SUPERFICIAL_CORRECTION, "Rettifica superficiale", CHRONO_PLAYER_B},
    {CHRONO_M005_RECOGNIZE_INSUFFICIENCY, "Riconosci rettifica insufficiente", CHRONO_PLAYER_B},
    {CHRONO_M005_OBSERVE_REAL_ORDER, "Osserva ordine reale", CHRONO_PLAYER_A},
    {CHRONO_M005_PRESERVE_EVIDENCE, "Preserva prova indipendente", CHRONO_PLAYER_A},
    {CHRONO_M005_COMPARE_EVIDENCE, "Confronta testimonianza", CHRONO_PLAYER_B},
    {CHRONO_M005_RECONSTRUCT_ORDER, "Ricostruisci ordine reale", CHRONO_PLAYER_B},
    {CHRONO_M005_CERTIFY_SOURCE, "Certifica fonte originale", CHRONO_PLAYER_A},
    {CHRONO_M005_RECORD_PROVENANCE, "Registra provenienza", CHRONO_PLAYER_A},
    {CHRONO_M005_VALIDATE_CHRONOLOGY, "Valida cronologia", CHRONO_PLAYER_B},
    {CHRONO_M005_COMPLETE_CAMPAIGN, "Completa campagna", CHRONO_PLAYER_B}
};

typedef struct {
    ChronoM005State state;
    char message[256];
} M005Gui;

static const char *m005_yes_no(bool value)
{
    return value ? "SI" : "NO";
}

static const char *m005_result_name(ChronoM005Result result)
{
    switch (result) {
    case CHRONO_M005_OK: return "Azione riuscita";
    case CHRONO_M005_INVALID_ACTION: return "Azione non valida";
    case CHRONO_M005_WRONG_PLAYER: return "Giocatore errato";
    case CHRONO_M005_NO_ACTION_POINTS: return "Punti azione esauriti";
    case CHRONO_M005_PREREQUISITE_MISSING: return "Prerequisito mancante";
    case CHRONO_M005_MISSION_FINISHED: return "Missione conclusa";
    }
    return "Risultato sconosciuto";
}

static void m005_gui_init(M005Gui *gui)
{
    chrono_m005_init(&gui->state);
    snprintf(gui->message, sizeof(gui->message),
             "M005: il tempo non sbaglia. Verifica le prove, non i timestamp.");
}

static SDL_Rect m005_action_rect(int index)
{
    SDL_Rect rect = {
        32 + (index / 8) * 620,
        350 + (index % 8) * 46,
        605,
        40
    };
    return rect;
}

static bool m005_contains(SDL_Rect r, int x, int y)
{
    return x >= r.x && x < r.x + r.w &&
           y >= r.y && y < r.y + r.h;
}

static void m005_gui_apply(M005Gui *gui, int index)
{
    if (index < 0 || index >= M005_GUI_ACTION_COUNT)
        return;

    ChronoM005Result result =
        chrono_m005_apply(&gui->state, m005_actions[index].action);

    snprintf(gui->message, sizeof(gui->message), "%s: %s",
             m005_actions[index].label, m005_result_name(result));
}

static void m005_gui_end_turn(M005Gui *gui)
{
    ChronoM005Trace trace = {0};
    ChronoM005Result result = chrono_m005_end_turn(&gui->state, &trace);

    if (result != CHRONO_M005_OK) {
        snprintf(gui->message, sizeof(gui->message), "%s",
                 m005_result_name(result));
    } else if (gui->state.finished) {
        snprintf(gui->message, sizeof(gui->message), "%s AL TURNO %u",
                 gui->state.victory ? "CAMPAGNA COMPLETATA" :
                                      "MISSIONE FALLITA",
                 gui->state.turn);
    } else if (trace.changed) {
        snprintf(gui->message, sizeof(gui->message),
                 "PROPAGAZIONE T%u: prova %s -> %s",
                 trace.turn,
                 m005_yes_no(trace.evidence_before),
                 m005_yes_no(trace.evidence_after));
    } else {
        snprintf(gui->message, sizeof(gui->message),
                 "Turno concluso: nessuna nuova prova propagata.");
    }
}

static void m005_gui_event(M005Gui *gui, const SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        if (event->key.keysym.sym == SDLK_r)
            m005_gui_init(gui);
        else if (event->key.keysym.sym == SDLK_SPACE)
            m005_gui_end_turn(gui);
    }

    if (event->type != SDL_MOUSEBUTTONDOWN ||
        event->button.button != SDL_BUTTON_LEFT)
        return;

    int x = event->button.x;
    int y = event->button.y;

    if (m005_contains((SDL_Rect){32, 735, 300, 43}, x, y)) {
        m005_gui_end_turn(gui);
    } else if (m005_contains((SDL_Rect){345, 735, 220, 43}, x, y)) {
        m005_gui_init(gui);
    } else {
        for (int i = 0; i < M005_GUI_ACTION_COUNT; ++i) {
            if (m005_contains(m005_action_rect(i), x, y)) {
                m005_gui_apply(gui, i);
                break;
            }
        }
    }
}

static void m005_rect(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

static void m005_text(SDL_Renderer *renderer, TTF_Font *font,
                      int x, int y, SDL_Color color, const char *value)
{
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, value, color);
    if (!surface)
        return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect rect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

static void m005_gui_draw(SDL_Renderer *renderer, TTF_Font *font,
                          const M005Gui *gui)
{
    const ChronoM005State *s = &gui->state;
    SDL_Color white = {235, 241, 250, 255};
    SDL_Color muted = {161, 183, 205, 255};
    SDL_Color warning = {245, 167, 87, 255};
    SDL_Color good = {125, 219, 172, 255};
    char line[256];

    SDL_SetRenderDrawColor(renderer, 11, 23, 39, 255);
    SDL_RenderClear(renderer);

    snprintf(line, sizeof(line),
             "ChronoOops! 2D | M005 | Turno %u/%u | Giocatore %c | PA %u",
             s->turn, CHRONO_MAX_TURNS,
             s->active_player == CHRONO_PLAYER_A ? 'A' : 'B',
             s->action_points);
    m005_text(renderer, font, 32, 28, white, line);

    m005_rect(renderer, (SDL_Rect){32, 79, 600, 238},
              (SDL_Color){21, 43, 72, 255});
    m005_rect(renderer, (SDL_Rect){648, 79, 600, 238},
              (SDL_Color){20, 59, 61, 255});

    m005_text(renderer, font, 48, 92, white,
              s->active_player == CHRONO_PLAYER_A ?
              "1985 | A [ATTIVO]" : "1985 | A");
    m005_text(renderer, font, 664, 92, white,
              s->active_player == CHRONO_PLAYER_B ?
              "2025 | B [ATTIVO]" : "2025 | B");

    snprintf(line, sizeof(line),
             "Ordine reale: %s | Orologio: -%u s",
             s->signal_precedes_response ?
             "SEGNALE -> RISPOSTA" : "INCOERENTE",
             s->clock_loss_seconds);
    m005_text(renderer, font, 48, 143, white, line);

    snprintf(line, sizeof(line),
             "Difetto identificato: %s | Prova preservata: %s",
             m005_yes_no(s->clock_loss_identified),
             m005_yes_no(s->independent_evidence_preserved));
    m005_text(renderer, font, 48, 188, muted, line);

    snprintf(line, sizeof(line),
             "Propagazione: %s | Fonte certificata: %s",
             s->evidence_propagation_pending ? "IN ATTESA" : "ALLINEATA",
             m005_yes_no(s->original_source_certified));
    m005_text(renderer, font, 48, 237,
              s->evidence_propagation_pending ? warning : good, line);

    snprintf(line, sizeof(line),
             "Timestamp: %s | Rettifica: %s",
             s->timestamps_show_inversion ? "RISPOSTA -> SEGNALE" :
                                            "RIORDINATI",
             m005_yes_no(s->superficial_correction));
    m005_text(renderer, font, 664, 143,
              s->superficial_correction ? warning : white, line);

    snprintf(line, sizeof(line),
             "Prova disponibile: %s | Confrontata: %s",
             m005_yes_no(s->independent_evidence_available),
             m005_yes_no(s->evidence_compared));
    m005_text(renderer, font, 664, 188, muted, line);

    snprintf(line, sizeof(line),
             "Ricostruita: %s | Provenienza: %s | Validata: %s",
             m005_yes_no(s->real_order_reconstructed),
             m005_yes_no(s->historical_provenance_recorded),
             m005_yes_no(s->chronology_validated));
    m005_text(renderer, font, 664, 237, white, line);

    m005_text(renderer, font, 32, 323,
              s->superficial_correction ? warning : muted,
              s->superficial_correction ?
              "RETTIFICA INSUFFICIENTE: ripristina le fonti originali." :
              "AZIONI | Dimostra l'ordine reale senza modificare gli eventi.");

    for (int i = 0; i < M005_GUI_ACTION_COUNT; ++i) {
        SDL_Rect rect = m005_action_rect(i);
        bool active = m005_actions[i].action == CHRONO_M005_WAIT ||
                      m005_actions[i].player == s->active_player;
        SDL_Color color = s->finished
            ? (SDL_Color){43, 50, 65, 255}
            : active ? (SDL_Color){35, 90, 135, 255}
                     : (SDL_Color){39, 49, 65, 255};

        m005_rect(renderer, rect, color);
        m005_text(renderer, font, rect.x + 10, rect.y + 10,
                  white, m005_actions[i].label);
    }

    m005_rect(renderer, (SDL_Rect){32, 735, 300, 43},
              (SDL_Color){154, 100, 42, 255});
    m005_text(renderer, font, 47, 746, white, "TERMINA TURNO");

    m005_rect(renderer, (SDL_Rect){345, 735, 220, 43},
              (SDL_Color){88, 65, 105, 255});
    m005_text(renderer, font, 360, 746, white, "RICOMINCIA");

    m005_text(renderer, font, 32, 790, white, gui->message);
    m005_text(renderer, font, 32, 814, muted,
              "Mouse: azioni | SPAZIO: turno | R: ricomincia | ESC: menu");
}
