#include "mission002.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum { M002_GUI_ACTION_COUNT = 13 };

typedef struct {
    ChronoM002Action action;
    const char *label;
    ChronoPlayer player;
} M002GuiAction;

static const M002GuiAction m002_actions[] = {
    { CHRONO_M002_WAIT, "Attendi", CHRONO_PLAYER_A },
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

typedef struct {
    ChronoM002State state;
    char message[256];
} M002Gui;

static void m002_gui_init(M002Gui *gui)
{
    chrono_m002_init(&gui->state);
    snprintf(gui->message, sizeof(gui->message),
             "M002: riattiva il Cronofono e salda il debito causale.");
}

static const char *m002_result_name(ChronoM002Result result)
{
    switch (result) {
    case CHRONO_M002_OK: return "Azione riuscita";
    case CHRONO_M002_INVALID_ACTION: return "Azione non valida";
    case CHRONO_M002_WRONG_PLAYER: return "Giocatore errato";
    case CHRONO_M002_NO_ACTION_POINTS: return "Punti azione esauriti";
    case CHRONO_M002_PREREQUISITE_MISSING: return "Prerequisito mancante";
    case CHRONO_M002_CAUSAL_PARADOX: return "PARADOSSO CAUSALE!";
    case CHRONO_M002_MISSION_FINISHED: return "Missione conclusa";
    }
    return "Risultato sconosciuto";
}

static SDL_Rect m002_action_rect(int index)
{
    SDL_Rect rect = {
        32 + (index / 7) * 620,
        350 + (index % 7) * 49,
        605,
        43
    };
    return rect;
}

static bool m002_contains(SDL_Rect r, int x, int y)
{
    return x >= r.x && x < r.x + r.w &&
           y >= r.y && y < r.y + r.h;
}

static void m002_gui_apply(M002Gui *gui, int index)
{
    ChronoM002Result result =
        chrono_m002_apply(&gui->state, m002_actions[index].action);

    snprintf(gui->message, sizeof(gui->message), "%s: %s",
             m002_actions[index].label, m002_result_name(result));
}

static void m002_gui_end_turn(M002Gui *gui)
{
    ChronoM002Trace trace;
    ChronoM002Result result =
        chrono_m002_end_turn(&gui->state, &trace);

    if (result != CHRONO_M002_OK) {
        snprintf(gui->message, sizeof(gui->message), "%s",
                 m002_result_name(result));
    } else if (gui->state.finished) {
        snprintf(gui->message, sizeof(gui->message),
                 "%s AL TURNO %u",
                 gui->state.victory ? "MISSIONE COMPLETATA" :
                                      "MISSIONE FALLITA",
                 gui->state.turn);
    } else if (trace.changed) {
        snprintf(gui->message, sizeof(gui->message),
                 "CAUSA: approvazione storica | Debito %s -> %s | Origine %s -> %s",
                 trace.debt_before ? "APERTO" : "ESTINTO",
                 trace.debt_after ? "APERTO" : "ESTINTO",
                 trace.origin_before ? "SI" : "NO",
                 trace.origin_after ? "SI" : "NO");
    } else {
        snprintf(gui->message, sizeof(gui->message),
                 "Turno concluso: nessuna variazione causale.");
    }
}

static void m002_gui_event(M002Gui *gui, const SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        if (event->key.keysym.sym == SDLK_r)
            m002_gui_init(gui);
        else if (event->key.keysym.sym == SDLK_SPACE)
            m002_gui_end_turn(gui);
    }

    if (event->type != SDL_MOUSEBUTTONDOWN ||
        event->button.button != SDL_BUTTON_LEFT)
        return;

    int x = event->button.x;
    int y = event->button.y;

    if (m002_contains((SDL_Rect){32, 735, 300, 43}, x, y)) {
        m002_gui_end_turn(gui);
    } else if (m002_contains((SDL_Rect){345, 735, 220, 43}, x, y)) {
        m002_gui_init(gui);
    } else {
        for (int i = 0; i < M002_GUI_ACTION_COUNT; ++i) {
            if (m002_contains(m002_action_rect(i), x, y)) {
                m002_gui_apply(gui, i);
                break;
            }
        }
    }
}

static void m002_rect(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

static void m002_text(SDL_Renderer *renderer, TTF_Font *font,
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

static void m002_gui_draw(SDL_Renderer *renderer, TTF_Font *font,
                          const M002Gui *gui)
{
    const ChronoM002State *s = &gui->state;
    SDL_Color white = {235, 241, 250, 255};
    SDL_Color muted = {161, 183, 205, 255};
    char line[256];

    SDL_SetRenderDrawColor(renderer, 11, 23, 39, 255);
    SDL_RenderClear(renderer);

    snprintf(line, sizeof(line),
             "ChronoOops! 2D | M002 | Turno %u/8 | Giocatore %c | PA %u",
             s->turn, s->active_player == CHRONO_PLAYER_A ? 'A' : 'B',
             s->action_points);
    m002_text(renderer, font, 32, 28, white, line);

    m002_rect(renderer, (SDL_Rect){32, 79, 600, 238},
              (SDL_Color){21, 43, 72, 255});
    m002_rect(renderer, (SDL_Rect){648, 79, 600, 238},
              (SDL_Color){20, 59, 61, 255});

    m002_text(renderer, font, 48, 92, white,
              s->active_player == CHRONO_PLAYER_A ?
              "1985 | A [ATTIVO]" : "1985 | A");
    m002_text(renderer, font, 664, 92, white,
              s->active_player == CHRONO_PLAYER_B ?
              "2025 | B [ATTIVO]" : "2025 | B");

    snprintf(line, sizeof(line), "Archivio: %s | Progetto: %s",
             s->archive_consulted ? "CONSULTATO" : "IGNOTO",
             s->design_destroyed ? "DISTRUTTO" :
             s->design_identified ? "IDENTIFICATO" : "IGNOTO");
    m002_text(renderer, font, 48, 145, white, line);

    snprintf(line, sizeof(line), "Produzione: %s",
             s->production_approved ? "APPROVATA" : "NON APPROVATA");
    m002_text(renderer, font, 48, 190, muted, line);

    snprintf(line, sizeof(line), "Origine storica: %s",
             s->historical_origin_exists ? "STABILITA" : "ASSENTE");
    m002_text(renderer, font, 48, 240, white, line);

    snprintf(line, sizeof(line), "Modulo: %s",
             s->module_installed ? "INSTALLATO" :
             s->module_borrowed ? "ANTICIPATO" : "ASSENTE");
    m002_text(renderer, font, 664, 145, white, line);

    snprintf(line, sizeof(line), "Cronofono: %s",
             s->cronophone_active ? "ATTIVO" : "SPENTO");
    m002_text(renderer, font, 664, 190, white, line);

    snprintf(line, sizeof(line), "Debito causale: %s",
             !s->module_borrowed ? "NON CONTRATTO" :
             s->debt_open ? "APERTO" : "ESTINTO");
    m002_text(renderer, font, 664, 240,
              s->debt_open ? (SDL_Color){245, 167, 87, 255} : white,
              line);

    m002_text(renderer, font, 32, 323, muted,
              "AZIONI | Il motore C11 verifica ogni scelta");

    for (int i = 0; i < M002_GUI_ACTION_COUNT; ++i) {
        SDL_Rect rect = m002_action_rect(i);
        bool active = m002_actions[i].action == CHRONO_M002_WAIT ||
            m002_actions[i].player == s->active_player;
        SDL_Color color = s->finished
            ? (SDL_Color){43, 50, 65, 255}
            : active ? (SDL_Color){35, 90, 135, 255}
                     : (SDL_Color){39, 49, 65, 255};

        m002_rect(renderer, rect, color);
        m002_text(renderer, font, rect.x + 10, rect.y + 12,
                  white, m002_actions[i].label);
    }

    m002_rect(renderer, (SDL_Rect){32, 735, 300, 43},
              (SDL_Color){154, 100, 42, 255});
    m002_text(renderer, font, 47, 746, white, "TERMINA TURNO");

    m002_rect(renderer, (SDL_Rect){345, 735, 220, 43},
              (SDL_Color){88, 65, 105, 255});
    m002_text(renderer, font, 360, 746, white, "RICOMINCIA");

    m002_text(renderer, font, 32, 790, white, gui->message);
    m002_text(renderer, font, 32, 814, muted,
              "Mouse: azioni | SPAZIO: turno | R: ricomincia | ESC: menu");
}
