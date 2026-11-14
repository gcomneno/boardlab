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
    int hovered_action;
    int focused_action;
    int focused_bottom;
} M002Gui;

static void m002_gui_init(M002Gui *gui)
{
    chrono_m002_init(&gui->state);
    gui->hovered_action = -1;
    gui->focused_action = -1;
    gui->focused_bottom = 0;
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

static int m002_action_at(int x, int y)
{
    for (int i = 0; i < M002_GUI_ACTION_COUNT; ++i) {
        if (m002_contains(m002_action_rect(i), x, y))
            return i;
    }
    return -1;
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

static bool m002_action_available(const ChronoM002State *state,
                                  int index)
{
    if (state == NULL || index < 0 || index >= M002_GUI_ACTION_COUNT)
        return false;

    ChronoM002State probe = *state;
    return chrono_m002_apply(&probe, m002_actions[index].action) ==
           CHRONO_M002_OK;
}

static const char *m002_action_owner(int index)
{
    if (index == 0)
        return "A/B";
    return m002_actions[index].player == CHRONO_PLAYER_A ? "A" : "B";
}

static void m002_gui_event(M002Gui *gui, const SDL_Event *event)
{
    if (event->type == SDL_MOUSEMOTION) {
        gui->hovered_action =
            m002_action_at(event->motion.x, event->motion.y);
        return;
    }

    if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        SDL_Keycode key = event->key.keysym.sym;

        if (key == SDLK_LEFT || key == SDLK_RIGHT) {
            int column = key == SDLK_RIGHT ? 1 : 0;
            if (gui->focused_action >= 0) {
                int row = gui->focused_action % 7;
                if (column == 1 && row > 5)
                    row = 5;
                gui->focused_action = column * 7 + row;
            } else {
                gui->focused_bottom = column;
            }
            return;
        }

        if (key == SDLK_UP || key == SDLK_DOWN) {
            if (gui->focused_action < 0) {
                int last = gui->focused_bottom == 0 ? 6 : 5;
                int row = key == SDLK_UP ? last : 0;
                gui->focused_action = gui->focused_bottom * 7 + row;
            } else {
                int column = gui->focused_action / 7;
                int row = gui->focused_action % 7;
                int count = column == 0 ? 7 : 6;
                row += key == SDLK_UP ? -1 : 1;

                if (row < 0 || row >= count) {
                    gui->focused_action = -1;
                    gui->focused_bottom = column;
                } else {
                    gui->focused_action = column * 7 + row;
                }
            }
            return;
        }

        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            if (gui->focused_action >= 0)
                m002_gui_apply(gui, gui->focused_action);
            else if (gui->focused_bottom == 0)
                m002_gui_end_turn(gui);
            else
                m002_gui_init(gui);
            return;
        }

        if (key == SDLK_r)
            m002_gui_init(gui);
        else if (key == SDLK_SPACE)
            m002_gui_end_turn(gui);
    }

    if (event->type != SDL_MOUSEBUTTONDOWN ||
        event->button.button != SDL_BUTTON_LEFT)
        return;

    int x = event->button.x;
    int y = event->button.y;

    if (m002_contains((SDL_Rect){32, 790, 300, 43}, x, y)) {
        gui->focused_action = -1;
        gui->focused_bottom = 0;
        m002_gui_end_turn(gui);
    } else if (m002_contains((SDL_Rect){345, 790, 220, 43}, x, y)) {
        m002_gui_init(gui);
    } else {
        int index = m002_action_at(x, y);
        if (index >= 0) {
            gui->focused_action = index;
            m002_gui_apply(gui, index);
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

static void m002_focus_rect(SDL_Renderer *renderer, SDL_Rect rect)
{
    SDL_SetRenderDrawColor(renderer, 245, 72, 72, 255);
    for (int thickness = 0; thickness < 3; ++thickness) {
        SDL_Rect border = {
            rect.x + thickness, rect.y + thickness,
            rect.w - 2 * thickness, rect.h - 2 * thickness
        };
        SDL_RenderDrawRect(renderer, &border);
    }
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
              "AZIONI DISPONIBILI");

    for (int i = 0; i < M002_GUI_ACTION_COUNT; ++i) {
        SDL_Rect rect = m002_action_rect(i);
        bool available = m002_action_available(s, i);
        bool hovered = !s->finished && gui->hovered_action == i;
        SDL_Color color = available
            ? hovered ? (SDL_Color){55, 124, 169, 255}
                      : (SDL_Color){35, 90, 135, 255}
            : hovered ? (SDL_Color){65, 73, 87, 255}
                      : (SDL_Color){39, 49, 65, 255};

        m002_rect(renderer, rect, color);
        if (gui->focused_action == i) {
            m002_focus_rect(renderer, rect);
        } else if (hovered) {
            SDL_Color border = available
                ? (SDL_Color){239, 194, 111, 255}
                : (SDL_Color){130, 145, 160, 255};
            SDL_SetRenderDrawColor(renderer,
                                   border.r, border.g, border.b, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
        m002_text(renderer, font, rect.x + 10, rect.y + 12,
                  available ? white : muted, m002_actions[i].label);
        m002_text(renderer, font, rect.x + rect.w - 48,
                  rect.y + 12, available ? white : muted,
                  m002_action_owner(i));
    }

    m002_rect(renderer, (SDL_Rect){32, 704, 1216, 33},
              (SDL_Color){29, 49, 72, 255});
    m002_rect(renderer, (SDL_Rect){32, 704, 4, 33},
              (SDL_Color){245, 167, 87, 255});
    m002_text(renderer, font, 46, 711,
              (SDL_Color){245, 167, 87, 255}, "EVENTO");
    m002_text(renderer, font, 136, 711, white, gui->message);

    m002_rect(renderer, (SDL_Rect){32, 790, 300, 43},
              (SDL_Color){154, 100, 42, 255});
    m002_text(renderer, font, 47, 801, white, "TERMINA TURNO");

    m002_rect(renderer, (SDL_Rect){345, 790, 220, 43},
              (SDL_Color){88, 65, 105, 255});
    m002_text(renderer, font, 360, 801, white, "RICOMINCIA");

    if (gui->focused_action < 0) {
        SDL_Rect focus = gui->focused_bottom == 0
            ? (SDL_Rect){32, 790, 300, 43}
            : (SDL_Rect){345, 790, 220, 43};
        m002_focus_rect(renderer, focus);
    }
}
