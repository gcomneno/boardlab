#include "mission003.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

enum { M003_GUI_ACTION_COUNT = 14 };

typedef struct {
    ChronoM003Action action;
    const char *label;
    ChronoPlayer player;
} M003GuiAction;

static const M003GuiAction m003_actions[] = {
    { CHRONO_M003_WAIT, "Attendi", CHRONO_PLAYER_A },
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

typedef struct {
    ChronoM003State state;
    char message[256];
} M003Gui;

static void m003_gui_init(M003Gui *gui)
{
    chrono_m003_init(&gui->state);
    snprintf(gui->message, sizeof(gui->message),
             "M003: salva le persone senza cancellare la testimonianza.");
}

static const char *m003_result_name(ChronoM003Result result)
{
    switch (result) {
    case CHRONO_M003_OK: return "Azione riuscita";
    case CHRONO_M003_INVALID_ACTION: return "Azione non valida";
    case CHRONO_M003_WRONG_PLAYER: return "Giocatore errato";
    case CHRONO_M003_NO_ACTION_POINTS: return "Punti azione esauriti";
    case CHRONO_M003_PREREQUISITE_MISSING: return "Prerequisito mancante";
    case CHRONO_M003_CAUSAL_PARADOX: return "PARADOSSO CAUSALE!";
    case CHRONO_M003_MISSION_FINISHED: return "Missione conclusa";
    }
    return "Risultato sconosciuto";
}

static SDL_Rect m003_action_rect(int index)
{
    SDL_Rect rect = {
        32 + (index / 7) * 620,
        350 + (index % 7) * 49,
        605,
        43
    };
    return rect;
}

static bool m003_contains(SDL_Rect r, int x, int y)
{
    return x >= r.x && x < r.x + r.w &&
           y >= r.y && y < r.y + r.h;
}

static void m003_gui_apply(M003Gui *gui, int index)
{
    if (index < 0 || index >= M003_GUI_ACTION_COUNT)
        return;

    ChronoM003Result result =
        chrono_m003_apply(&gui->state, m003_actions[index].action);

    snprintf(gui->message, sizeof(gui->message), "%s: %s",
             m003_actions[index].label, m003_result_name(result));
}

static void m003_gui_end_turn(M003Gui *gui)
{
    ChronoM003Trace trace = {0};
    ChronoM003Result result =
        chrono_m003_end_turn(&gui->state, &trace);

    if (result != CHRONO_M003_OK) {
        snprintf(gui->message, sizeof(gui->message), "%s",
                 m003_result_name(result));
    } else if (gui->state.finished) {
        snprintf(gui->message, sizeof(gui->message),
                 "%s AL TURNO %u",
                 gui->state.victory ? "MISSIONE COMPLETATA" :
                                      "MISSIONE FALLITA",
                 gui->state.turn);
    } else if (trace.changed) {
        snprintf(gui->message, sizeof(gui->message),
                 "CAUSA turno %u | Sicurezza %s -> %s | Testimonianza %s -> %s",
                 trace.turn,
                 trace.safety_before ? "SI" : "NO",
                 trace.safety_after ? "SI" : "NO",
                 trace.testimony_before ? "SI" : "NO",
                 trace.testimony_after ? "SI" : "NO");
    } else {
        snprintf(gui->message, sizeof(gui->message),
                 "Turno concluso: nessuna variazione causale.");
    }
}

static void m003_gui_event(M003Gui *gui, const SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        if (event->key.keysym.sym == SDLK_r)
            m003_gui_init(gui);
        else if (event->key.keysym.sym == SDLK_SPACE)
            m003_gui_end_turn(gui);
    }

    if (event->type != SDL_MOUSEBUTTONDOWN ||
        event->button.button != SDL_BUTTON_LEFT)
        return;

    int x = event->button.x;
    int y = event->button.y;

    if (m003_contains((SDL_Rect){32, 735, 300, 43}, x, y)) {
        m003_gui_end_turn(gui);
    } else if (m003_contains((SDL_Rect){345, 735, 220, 43}, x, y)) {
        m003_gui_init(gui);
    } else {
        for (int i = 0; i < M003_GUI_ACTION_COUNT; ++i) {
            if (m003_contains(m003_action_rect(i), x, y)) {
                m003_gui_apply(gui, i);
                break;
            }
        }
    }
}

static void m003_rect(SDL_Renderer *renderer,
                      SDL_Rect rect, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer,
                           color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

static void m003_text(SDL_Renderer *renderer, TTF_Font *font,
                      int x, int y, SDL_Color color, const char *value)
{
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, value, color);
    if (!surface)
        return;

    SDL_Texture *texture =
        SDL_CreateTextureFromSurface(renderer, surface);

    if (texture) {
        SDL_Rect rect = {x, y, surface->w, surface->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
        SDL_DestroyTexture(texture);
    }

    SDL_FreeSurface(surface);
}

static void m003_gui_draw(SDL_Renderer *renderer, TTF_Font *font,
                          const M003Gui *gui)
{
    const ChronoM003State *s = &gui->state;
    SDL_Color white = {235, 241, 250, 255};
    SDL_Color muted = {161, 183, 205, 255};
    SDL_Color warning = {245, 167, 87, 255};
    char line[256];

    SDL_SetRenderDrawColor(renderer, 11, 23, 39, 255);
    SDL_RenderClear(renderer);

    snprintf(line, sizeof(line),
             "ChronoOops! 2D | M003 | Turno %u/8 | Giocatore %c | PA %u",
             s->turn,
             s->active_player == CHRONO_PLAYER_A ? 'A' : 'B',
             s->action_points);
    m003_text(renderer, font, 32, 28, white, line);

    m003_rect(renderer, (SDL_Rect){32, 79, 600, 238},
              (SDL_Color){21, 43, 72, 255});
    m003_rect(renderer, (SDL_Rect){648, 79, 600, 238},
              (SDL_Color){20, 59, 61, 255});

    m003_text(renderer, font, 48, 92, white,
              s->active_player == CHRONO_PLAYER_A ?
              "1985 | A [ATTIVO]" : "1985 | A");
    m003_text(renderer, font, 664, 92, white,
              s->active_player == CHRONO_PLAYER_B ?
              "2025 | B [ATTIVO]" : "2025 | B");

    snprintf(line, sizeof(line), "Impianto: %s | Rischio: %s",
             s->equipment_inspected ? "ISPEZIONATO" : "IGNOTO",
             s->risk_identified ? "IDENTIFICATO" : "IGNOTO");
    m003_text(renderer, font, 48, 145, white, line);

    snprintf(line, sizeof(line), "Avvertimento: %s",
             s->warning_received ? "RICEVUTO" :
             s->warning_transmitted ? "TRASMESSO" : "ASSENTE");
    m003_text(renderer, font, 48, 190, muted, line);

    snprintf(line, sizeof(line), "Persone al sicuro: %s",
             s->people_safe ? "SI" :
             s->safety_pending ? "IN ATTESA" : "NO");
    m003_text(renderer, font, 48, 240,
              s->people_safe ? white : warning, line);

    snprintf(line, sizeof(line), "Testimonianza: %s",
             s->testimony_available ? "PRESENTE" : "ASSENTE");
    m003_text(renderer, font, 664, 145,
              s->testimony_available ? white : warning, line);

    snprintf(line, sizeof(line), "Registro: %s | Archivio: %s",
             s->record_checked ? "VERIFICATO" :
             s->record_consulted ? "CONSULTATO" : "IGNOTO",
             s->testimony_archived ? "COMPLETO" : "INCOMPLETO");
    m003_text(renderer, font, 664, 190, muted, line);

    snprintf(line, sizeof(line), "Provenienza: %s",
             s->provenance_validated ? "CONVALIDATA" : "DA VERIFICARE");
    m003_text(renderer, font, 664, 240, white, line);

    m003_text(renderer, font, 32, 323, muted,
              "AZIONI | Il motore C11 verifica ogni scelta");

    for (int i = 0; i < M003_GUI_ACTION_COUNT; ++i) {
        SDL_Rect rect = m003_action_rect(i);
        bool active = m003_actions[i].action == CHRONO_M003_WAIT ||
            m003_actions[i].player == s->active_player;

        SDL_Color color = s->finished
            ? (SDL_Color){43, 50, 65, 255}
            : active ? (SDL_Color){35, 90, 135, 255}
                     : (SDL_Color){39, 49, 65, 255};

        m003_rect(renderer, rect, color);
        m003_text(renderer, font, rect.x + 10, rect.y + 12,
                  white, m003_actions[i].label);
    }

    m003_rect(renderer, (SDL_Rect){32, 735, 300, 43},
              (SDL_Color){154, 100, 42, 255});
    m003_text(renderer, font, 47, 746, white, "TERMINA TURNO");

    m003_rect(renderer, (SDL_Rect){345, 735, 220, 43},
              (SDL_Color){88, 65, 105, 255});
    m003_text(renderer, font, 360, 746, white, "RICOMINCIA");

    m003_text(renderer, font, 32, 790, white, gui->message);
    m003_text(renderer, font, 32, 814, muted,
              "Mouse: azioni | SPAZIO: turno | R: ricomincia | ESC: menu");
}
