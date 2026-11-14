#include "mission004.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

enum { M004_GUI_ACTION_COUNT = 15 };

typedef struct {
    ChronoM004Action action;
    const char *label;
    ChronoPlayer player;
} M004GuiAction;

static const M004GuiAction m004_actions[] = {
    { CHRONO_M004_WAIT, "Attendi", CHRONO_PLAYER_A },
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

typedef struct {
    ChronoM004State state;
    char message[256];
    int hovered_action;
    int focused_action;
    int focused_bottom;
} M004Gui;

static const char *m004_config_name(ChronoM004Config config)
{
    switch (config) {
    case CHRONO_M004_CONFIG_NONE: return "NESSUNA";
    case CHRONO_M004_CONFIG_BEACON: return "FARO";
    case CHRONO_M004_CONFIG_REGISTER: return "REGISTRO";
    case CHRONO_M004_CONFIG_DUAL: return "DOPPIO";
    }
    return "SCONOSCIUTA";
}

static const char *m004_yes_no(bool value)
{
    return value ? "SI" : "NO";
}

static void m004_gui_init(M004Gui *gui)
{
    chrono_m004_init(&gui->state);
    gui->hovered_action = -1;
    gui->focused_action = -1;
    gui->focused_bottom = 0;
    snprintf(gui->message, sizeof(gui->message),
             "M004: due effetti, una sola causa. Trova il doppio contatto.");
}

static const char *m004_result_name(ChronoM004Result result)
{
    switch (result) {
    case CHRONO_M004_OK: return "Azione riuscita";
    case CHRONO_M004_INVALID_ACTION: return "Azione non valida";
    case CHRONO_M004_WRONG_PLAYER: return "Giocatore errato";
    case CHRONO_M004_NO_ACTION_POINTS: return "Punti azione esauriti";
    case CHRONO_M004_PREREQUISITE_MISSING: return "Prerequisito mancante";
    case CHRONO_M004_MISSION_FINISHED: return "Missione conclusa";
    }
    return "Risultato sconosciuto";
}

static SDL_Rect m004_action_rect(int index)
{
    SDL_Rect rect = {
        32 + (index / 8) * 620,
        350 + (index % 8) * 46,
        605,
        40
    };
    return rect;
}

static bool m004_contains(SDL_Rect r, int x, int y)
{
    return x >= r.x && x < r.x + r.w &&
           y >= r.y && y < r.y + r.h;
}

static void m004_gui_apply(M004Gui *gui, int index)
{
    if (index < 0 || index >= M004_GUI_ACTION_COUNT)
        return;

    ChronoM004Result result =
        chrono_m004_apply(&gui->state, m004_actions[index].action);

    snprintf(gui->message, sizeof(gui->message),
             "%s: %s",
             m004_actions[index].label,
             m004_result_name(result));
}

static void m004_gui_end_turn(M004Gui *gui)
{
    ChronoM004Trace trace = {0};
    ChronoM004Result result =
        chrono_m004_end_turn(&gui->state, &trace);

    if (result != CHRONO_M004_OK) {
        snprintf(gui->message, sizeof(gui->message), "%s",
                 m004_result_name(result));
    } else if (gui->state.finished) {
        snprintf(gui->message, sizeof(gui->message),
                 "%s AL TURNO %u",
                 gui->state.victory ? "MISSIONE COMPLETATA" :
                                      "MISSIONE FALLITA",
                 gui->state.turn);
    } else if (trace.changed) {
        snprintf(gui->message, sizeof(gui->message),
                 "CAUSA T%u: %s -> %s | Faro %s -> %s | Registro %s -> %s",
                 trace.turn,
                 m004_config_name(trace.configuration_before),
                 m004_config_name(trace.configuration_after),
                 m004_yes_no(trace.beacon_before),
                 m004_yes_no(trace.beacon_after),
                 m004_yes_no(trace.register_before),
                 m004_yes_no(trace.register_after));
    } else {
        snprintf(gui->message, sizeof(gui->message),
                 "Turno concluso: nessuna variazione causale.");
    }
}

static int m004_action_at(int x, int y)
{
    for (int i = 0; i < M004_GUI_ACTION_COUNT; ++i) {
        if (m004_contains(m004_action_rect(i), x, y))
            return i;
    }
    return -1;
}

static void m004_gui_event(M004Gui *gui, const SDL_Event *event)
{
    if (event->type == SDL_MOUSEMOTION) {
        gui->hovered_action =
            m004_action_at(event->motion.x, event->motion.y);
        return;
    }

    if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        SDL_Keycode key = event->key.keysym.sym;

        if (key == SDLK_LEFT || key == SDLK_RIGHT) {
            int column = key == SDLK_RIGHT ? 1 : 0;
            if (gui->focused_action >= 0) {
                int row = gui->focused_action % 8;
                if (column == 1 && row > 6)
                    row = 6;
                gui->focused_action = column * 8 + row;
            } else {
                gui->focused_bottom = column;
            }
            return;
        }

        if (key == SDLK_UP || key == SDLK_DOWN) {
            if (gui->focused_action < 0) {
                int count = gui->focused_bottom == 0 ? 8 : 7;
                int row = key == SDLK_UP ? count - 1 : 0;
                gui->focused_action = gui->focused_bottom * 8 + row;
            } else {
                int column = gui->focused_action / 8;
                int row = gui->focused_action % 8;
                int count = column == 0 ? 8 : 7;
                row += key == SDLK_UP ? -1 : 1;

                if (row < 0 || row >= count) {
                    gui->focused_action = -1;
                    gui->focused_bottom = column;
                } else {
                    gui->focused_action = column * 8 + row;
                }
            }
            return;
        }

        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            if (gui->focused_action >= 0)
                m004_gui_apply(gui, gui->focused_action);
            else if (gui->focused_bottom == 0)
                m004_gui_end_turn(gui);
            else
                m004_gui_init(gui);
            return;
        }

        if (key == SDLK_r)
            m004_gui_init(gui);
        else if (key == SDLK_SPACE)
            m004_gui_end_turn(gui);
    }

    if (event->type != SDL_MOUSEBUTTONDOWN ||
        event->button.button != SDL_BUTTON_LEFT)
        return;

    int x = event->button.x;
    int y = event->button.y;

    if (m004_contains((SDL_Rect){32, 790, 300, 43}, x, y)) {
        gui->focused_action = -1;
        gui->focused_bottom = 0;
        m004_gui_end_turn(gui);
    } else if (m004_contains((SDL_Rect){345, 790, 220, 43}, x, y)) {
        m004_gui_init(gui);
    } else {
        int index = m004_action_at(x, y);
        if (index >= 0) {
            gui->focused_action = index;
            m004_gui_apply(gui, index);
        }
    }
}

static void m004_rect(SDL_Renderer *renderer,
                      SDL_Rect rect, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer,
                           color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

static void m004_text(SDL_Renderer *renderer, TTF_Font *font,
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

static bool m004_action_available(const ChronoM004State *state,
                                  int index)
{
    if (state == NULL || index < 0 || index >= M004_GUI_ACTION_COUNT)
        return false;

    ChronoM004State probe = *state;
    return chrono_m004_apply(&probe, m004_actions[index].action) ==
           CHRONO_M004_OK;
}

static const char *m004_action_owner(int index)
{
    if (index == 0)
        return "A/B";

    return m004_actions[index].player == CHRONO_PLAYER_A ? "A" : "B";
}

static void m004_focus_rect(SDL_Renderer *renderer, SDL_Rect rect)
{
    SDL_SetRenderDrawColor(renderer, 245, 72, 72, 255);
    for (int thickness = 0; thickness < 3; ++thickness) {
        SDL_Rect border = {
            rect.x + thickness,
            rect.y + thickness,
            rect.w - 2 * thickness,
            rect.h - 2 * thickness
        };
        SDL_RenderDrawRect(renderer, &border);
    }
}

static void m004_gui_draw(SDL_Renderer *renderer, TTF_Font *font,
                          const M004Gui *gui)
{
    const ChronoM004State *s = &gui->state;
    SDL_Color white = {235, 241, 250, 255};
    SDL_Color muted = {161, 183, 205, 255};
    SDL_Color warning = {245, 167, 87, 255};
    SDL_Color good = {125, 219, 172, 255};
    char line[256];

    SDL_SetRenderDrawColor(renderer, 11, 23, 39, 255);
    SDL_RenderClear(renderer);

    snprintf(line, sizeof(line),
             "ChronoOops! 2D | M004 | Turno %u/%u | Giocatore %c | PA %u",
             s->turn, CHRONO_MAX_TURNS,
             s->active_player == CHRONO_PLAYER_A ? 'A' : 'B',
             s->action_points);
    m004_text(renderer, font, 32, 28, white, line);

    m004_rect(renderer, (SDL_Rect){32, 79, 600, 238},
              (SDL_Color){21, 43, 72, 255});
    m004_rect(renderer, (SDL_Rect){648, 79, 600, 238},
              (SDL_Color){20, 59, 61, 255});

    m004_text(renderer, font, 48, 92, white,
              s->active_player == CHRONO_PLAYER_A ?
              "1985 | A [ATTIVO]" : "1985 | A");
    m004_text(renderer, font, 664, 92, white,
              s->active_player == CHRONO_PLAYER_B ?
              "2025 | B [ATTIVO]" : "2025 | B");

    snprintf(line, sizeof(line), "Impulso: %s | Doppio contatto: %s",
             s->pulse_identified ? "IDENTIFICATO" : "IGNOTO",
             s->dual_contact_discovered ? "SCOPERTO" : "IGNOTO");
    m004_text(renderer, font, 48, 143, muted, line);

    snprintf(line, sizeof(line), "Configurazione storica: %s",
             m004_config_name(s->configuration));
    m004_text(renderer, font, 48, 188, white, line);

    snprintf(line, sizeof(line), "Propagazione: %s",
             s->propagation_pending ? "IN ATTESA" : "ALLINEATA");
    m004_text(renderer, font, 48, 237,
              s->propagation_pending ? warning : good, line);

    snprintf(line, sizeof(line), "Configurazione osservata: %s",
             m004_config_name(s->propagated_configuration));
    m004_text(renderer, font, 664, 143, white, line);

    snprintf(line, sizeof(line), "Faro: %s (%s) | Registro: %s (%s)",
             s->beacon_active ? "ON" : "OFF",
             s->beacon_verified ? "VERIFICATO" : "DA VERIFICARE",
             s->register_active ? "ON" : "OFF",
             s->register_verified ? "VERIFICATO" : "DA VERIFICARE");
    m004_text(renderer, font, 664, 188, muted, line);

    snprintf(line, sizeof(line), "Certificata: %s | Origine: %s | Coerenza: %s",
             m004_yes_no(s->configuration_certified),
             m004_yes_no(s->common_origin_recorded),
             m004_yes_no(s->coherence_validated));
    m004_text(renderer, font, 664, 237, white, line);

    if (s->propagated_configuration == CHRONO_M004_CONFIG_BEACON ||
        s->propagated_configuration == CHRONO_M004_CONFIG_REGISTER) {
        m004_text(renderer, font, 32, 323, warning,
                  "ESITO INCOMPLETO: correggi il collegamento e ripeti le verifiche.");
    } else {
        m004_text(renderer, font, 32, 323, muted,
                  "AZIONI | Una causa storica deve spiegare entrambi gli effetti.");
    }

    for (int i = 0; i < M004_GUI_ACTION_COUNT; ++i) {
        SDL_Rect rect = m004_action_rect(i);
        bool available = m004_action_available(s, i);
        bool hovered = !s->finished && gui->hovered_action == i;
        SDL_Color color = available
            ? hovered ? (SDL_Color){55, 124, 169, 255}
                      : (SDL_Color){35, 90, 135, 255}
            : hovered ? (SDL_Color){65, 73, 87, 255}
                      : (SDL_Color){39, 49, 65, 255};

        m004_rect(renderer, rect, color);
        if (gui->focused_action == i) {
            m004_focus_rect(renderer, rect);
        } else if (hovered) {
            SDL_Color border = available
                ? (SDL_Color){239, 194, 111, 255}
                : (SDL_Color){130, 145, 160, 255};
            SDL_SetRenderDrawColor(renderer,
                                   border.r, border.g, border.b, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
        m004_text(renderer, font, rect.x + 10, rect.y + 10,
                  available ? white : muted, m004_actions[i].label);
        m004_text(renderer, font, rect.x + rect.w - 48,
                  rect.y + 10, available ? white : muted,
                  m004_action_owner(i));
    }

    m004_rect(renderer, (SDL_Rect){32, 720, 1216, 33},
              (SDL_Color){29, 49, 72, 255});
    m004_rect(renderer, (SDL_Rect){32, 720, 4, 33},
              (SDL_Color){245, 167, 87, 255});
    m004_text(renderer, font, 46, 727, warning, "EVENTO");
    m004_text(renderer, font, 136, 727, white, gui->message);

    m004_rect(renderer, (SDL_Rect){32, 790, 300, 43},
              (SDL_Color){154, 100, 42, 255});
    m004_text(renderer, font, 47, 801, white, "TERMINA TURNO");

    m004_rect(renderer, (SDL_Rect){345, 790, 220, 43},
              (SDL_Color){88, 65, 105, 255});
    m004_text(renderer, font, 360, 801, white, "RICOMINCIA");

    if (gui->focused_action < 0) {
        SDL_Rect focus = gui->focused_bottom == 0
            ? (SDL_Rect){32, 790, 300, 43}
            : (SDL_Rect){345, 790, 220, 43};
        m004_focus_rect(renderer, focus);
    }
}
