#include "chrono.h"

#ifdef CHRONO_WITH_M002
#include "graphics2d_m002.c"
#include "graphics2d_m003.c"
#include "graphics2d_m004.c"
#include "graphics2d_m005.c"
#endif

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>

enum {
    VIEW_W = 1280,
    VIEW_H = 840,
    ACTION_COUNT = 22
};

typedef struct {
    ChronoAction id;
    const char *label;
} GuiAction;

static const GuiAction actions[ACTION_COUNT] = {
    { CHRONO_ACTION_WAIT, "Attendi" },
    { CHRONO_ACTION_MOVE_ARCHIVE, "Raggiungi archivio" },
    { CHRONO_ACTION_READ_PLANS, "Consulta progetti" },
    { CHRONO_ACTION_INSPECT_RUINS, "Esamina resti ponte" },
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

static const char *locations[] = {
    "Municipio", "Archivio", "Riva Ovest", "Ponte",
    "Papera", "Riva Est", "Torre"
};

static const char *result_label(ChronoResult r)
{
    switch (r) {
    case CHRONO_OK: return "Azione riuscita";
    case CHRONO_INVALID_ACTION: return "Azione non valida";
    case CHRONO_WRONG_PLAYER: return "Giocatore errato";
    case CHRONO_NO_ACTION_POINTS: return "Punti azione esauriti";
    case CHRONO_PREREQUISITE_MISSING: return "Prerequisito mancante";
    case CHRONO_MISSION_FINISHED: return "Missione conclusa";
    }
    return "Risultato sconosciuto";
}

static void rectangle(SDL_Renderer *renderer, SDL_Rect area, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &area);
}

static void label(SDL_Renderer *renderer, TTF_Font *font,
                  int x, int y, SDL_Color color, const char *text)
{
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface)
        return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        SDL_Rect position = { x, y, surface->w, surface->h };
        SDL_RenderCopy(renderer, texture, NULL, &position);
        SDL_DestroyTexture(texture);
    }
    SDL_FreeSurface(surface);
}

static SDL_Rect action_rect(int index)
{
    SDL_Rect r = {
        32 + (index / 11) * 620,
        335 + (index % 11) * 35,
        605,
        31
    };
    return r;
}

static bool contains(SDL_Rect rect, int x, int y)
{
    return x >= rect.x && y >= rect.y &&
           x < rect.x + rect.w && y < rect.y + rect.h;
}

static void draw_world(SDL_Renderer *renderer, TTF_Font *font,
                       const ChronoState *state)
{
    SDL_Color white = { 235, 241, 250, 255 };
    SDL_Color muted = { 155, 179, 205, 255 };
    SDL_Color past = { 62, 104, 154, 255 };
    SDL_Color future = { 54, 130, 114, 255 };
    char info[180];

    rectangle(renderer, (SDL_Rect){ 32, 79, 600, 225 },
              (SDL_Color){ 21, 43, 72, 255 });
    rectangle(renderer, (SDL_Rect){ 648, 79, 600, 225 },
              (SDL_Color){ 20, 59, 61, 255 });

    label(renderer, font, 48, 88, white,
          state->active_player == CHRONO_PLAYER_A
              ? "1985  |  Giocatore A  [ATTIVO]"
              : "1985  |  Giocatore A");
    label(renderer, font, 664, 88, white,
          state->active_player == CHRONO_PLAYER_B
              ? "2025  |  Giocatore B  [ATTIVO]"
              : "2025  |  Giocatore B");

    /* Historical locations belong exclusively to 1985.
     * Future locations use two rows to preserve label readability. */
    for (int i = 0; i < 7; ++i) {
        bool historical = i < 2;
        bool occupied = historical
            ? state->location_a == (ChronoLocation)i
            : state->location_b == (ChronoLocation)i;
        bool active = historical
            ? state->active_player == CHRONO_PLAYER_A
            : state->active_player == CHRONO_PLAYER_B;
        int index = i - 2;
        int x = historical ? 48 + i * 285
                           : 664 + (index % 3) * 190;
        int y = historical ? 145 : 131 + (index / 3) * 57;
        int width = historical ? 270 : 180;
        SDL_Color color = occupied
            ? (active ? (SDL_Color){ 209, 141, 56, 255 }
                      : (SDL_Color){ 117, 130, 159, 255 })
            : (historical ? past : future);

        rectangle(renderer, (SDL_Rect){ x, y, width, 43 }, color);
        label(renderer, font, x + 6, y + 3, white, locations[i]);
        if (occupied)
            label(renderer, font, x + 6, y + 21, white,
                  historical ? "A - QUI" : "B - QUI");
    }

    snprintf(info, sizeof(info), "Posizione A: %s",
             locations[(unsigned)state->location_a]);
    label(renderer, font, 48, 248, muted, info);

    snprintf(info, sizeof(info), "Posizione B: %s",
             locations[(unsigned)state->location_b]);
    label(renderer, font, 664, 248, muted, info);

    snprintf(info, sizeof(info), "Ponte: %s    Papera: %s",
             state->world.bridge_exists ? "ESISTE" : "ASSENTE",
             state->world.duck_exists ? "ESISTE" : "ASSENTE");
    label(renderer, font, 664, 273, white, info);

    snprintf(info, sizeof(info), "Progetto: %s",
             state->selected_plan == CHRONO_PLAN_BRIDGE ? "PONTE" :
             state->selected_plan == CHRONO_PLAN_DUCK ? "PAPERA" : "NESSUNO");
    label(renderer, font, 48, 273, white, info);
}

/*
 * UI classification only. chrono_apply() remains authoritative
 * for prerequisites, action points and turn legality.
 */
static bool action_for_player(ChronoAction action, ChronoPlayer player)
{
    if (action == CHRONO_ACTION_WAIT)
        return true;

    switch (action) {
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

    case CHRONO_ACTION_INSPECT_RUINS:
    case CHRONO_ACTION_IDENTIFY_DEFECT:
    case CHRONO_ACTION_MEASURE_RIVER:
    case CHRONO_ACTION_CROSS_BRIDGE:
    case CHRONO_ACTION_REACH_TOWER:
    case CHRONO_ACTION_INSPECT_DUCK:
    case CHRONO_ACTION_PREPARE_DUCK:
    case CHRONO_ACTION_START_DUCK:
    case CHRONO_ACTION_CROSS_RIVER:
    case CHRONO_ACTION_ENTER_TOWER:
    case CHRONO_ACTION_RECOVER_CAPSULE:
        return player == CHRONO_PLAYER_B;

    default:
        return false;
    }
}

typedef struct {
    int focused_action;
    int hovered_action;
    int focused_bottom;
} M001Focus;

static M001Focus m001_focus = {-1, -1, 0};

static void m001_focus_reset(void)
{
    m001_focus = (M001Focus){-1, -1, 0};
}

static int m001_action_at(int x, int y)
{
    for (int i = 0; i < ACTION_COUNT; ++i)
        if (contains(action_rect(i), x, y))
            return i;
    return -1;
}

static bool m001_action_available(const ChronoState *state, int index)
{
    if (state == NULL || index < 0 || index >= ACTION_COUNT)
        return false;

    ChronoState probe = *state;
    return chrono_apply(&probe, actions[index].id) == CHRONO_OK;
}

static const char *m001_action_owner(int index)
{
    if (actions[index].id == CHRONO_ACTION_WAIT)
        return "A/B";

    return action_for_player(actions[index].id, CHRONO_PLAYER_A)
        ? "A" : "B";
}

static void m001_focus_rect(SDL_Renderer *renderer, SDL_Rect rect)
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

static void draw(SDL_Renderer *renderer, TTF_Font *font,
                 const ChronoState *state, const char *message)
{
    SDL_Color white = { 235, 241, 250, 255 };
    SDL_Color muted = { 155, 179, 205, 255 };
    char header[200];

    SDL_SetRenderDrawColor(renderer, 11, 23, 39, 255);
    SDL_RenderClear(renderer);

    snprintf(header, sizeof(header),
             "ChronoOops! 2D  |  M001  |  Turno %u/%u  |  Giocatore %c  |  PA %u",
             state->turn, CHRONO_MAX_TURNS,
             state->active_player == CHRONO_PLAYER_A ? 'A' : 'B',
             state->action_points);
    label(renderer, font, 32, 28, white, header);

    draw_world(renderer, font, state);
    label(renderer, font, 32, 309, muted, "AZIONI DISPONIBILI");

    for (int i = 0; i < ACTION_COUNT; ++i) {
        SDL_Rect rect = action_rect(i);
        bool available = m001_action_available(state, i);
        bool hovered = !state->finished && m001_focus.hovered_action == i;
        SDL_Color background = available
            ? hovered ? (SDL_Color){55, 124, 169, 255}
                      : (SDL_Color){35, 90, 135, 255}
            : hovered ? (SDL_Color){65, 73, 87, 255}
                      : (SDL_Color){39, 49, 65, 255};

        rectangle(renderer, rect, background);
        if (m001_focus.focused_action == i) {
            m001_focus_rect(renderer, rect);
        } else if (hovered) {
            SDL_Color border = available
                ? (SDL_Color){239, 194, 111, 255}
                : (SDL_Color){130, 145, 160, 255};
            SDL_SetRenderDrawColor(renderer,
                                   border.r, border.g, border.b, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }

        label(renderer, font, rect.x + 10, rect.y + 5,
              available ? white : muted, actions[i].label);
        label(renderer, font, rect.x + rect.w - 48, rect.y + 5,
              available ? white : muted, m001_action_owner(i));
    }

    rectangle(renderer, (SDL_Rect){32, 725, 1216, 33},
              (SDL_Color){29, 49, 72, 255});
    rectangle(renderer, (SDL_Rect){32, 725, 4, 33},
              (SDL_Color){245, 167, 87, 255});
    label(renderer, font, 46, 732,
          (SDL_Color){245, 167, 87, 255}, "EVENTO");
    label(renderer, font, 136, 732, white, message);

    rectangle(renderer, (SDL_Rect){32, 790, 300, 43},
              (SDL_Color){154, 100, 42, 255});
    label(renderer, font, 47, 801, white, "TERMINA TURNO");

    rectangle(renderer, (SDL_Rect){345, 790, 220, 43},
              (SDL_Color){88, 65, 105, 255});
    label(renderer, font, 360, 801, white, "RICOMINCIA");

    if (m001_focus.focused_action < 0) {
        SDL_Rect focus = m001_focus.focused_bottom == 0
            ? (SDL_Rect){32, 790, 300, 43}
            : (SDL_Rect){345, 790, 220, 43};
        m001_focus_rect(renderer, focus);
    }
    SDL_RenderPresent(renderer);
}

static TTF_Font *open_font(void)
{
    const char *paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
    };
    for (unsigned i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        TTF_Font *font = TTF_OpenFont(paths[i], 15);
        if (font)
            return font;
    }
    return NULL;
}

static void handle_event(const SDL_Event *event, ChronoState *state,
                         char *message, size_t capacity, bool *running)
{
    if (event->type == SDL_QUIT ||
        (event->type == SDL_KEYDOWN &&
         event->key.keysym.sym == SDLK_ESCAPE)) {
        *running = false;
        return;
    }

    bool restart = false;
    bool end_turn = false;
    int clicked = -1;

    if (event->type == SDL_MOUSEMOTION) {
        m001_focus.hovered_action =
            m001_action_at(event->motion.x, event->motion.y);
        return;
    }

    if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        SDL_Keycode key = event->key.keysym.sym;

        if (key == SDLK_LEFT || key == SDLK_RIGHT) {
            int column = key == SDLK_RIGHT ? 1 : 0;
            if (m001_focus.focused_action >= 0) {
                int row = m001_focus.focused_action % 11;
                m001_focus.focused_action = column * 11 + row;
            } else {
                m001_focus.focused_bottom = column;
            }
            return;
        }

        if (key == SDLK_UP || key == SDLK_DOWN) {
            if (m001_focus.focused_action < 0) {
                int row = key == SDLK_UP ? 10 : 0;
                m001_focus.focused_action =
                    m001_focus.focused_bottom * 11 + row;
            } else {
                int column = m001_focus.focused_action / 11;
                int row = m001_focus.focused_action % 11;
                row += key == SDLK_UP ? -1 : 1;
                if (row < 0 || row >= 11) {
                    m001_focus.focused_action = -1;
                    m001_focus.focused_bottom = column;
                } else {
                    m001_focus.focused_action = column * 11 + row;
                }
            }
            return;
        }

        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            if (m001_focus.focused_action >= 0)
                clicked = m001_focus.focused_action;
            else if (m001_focus.focused_bottom == 0)
                end_turn = true;
            else
                restart = true;
        } else {
            restart = key == SDLK_r;
            end_turn = key == SDLK_SPACE;
        }
    }

    if (event->type == SDL_MOUSEBUTTONDOWN &&
        event->button.button == SDL_BUTTON_LEFT) {
        int x = event->button.x;
        int y = event->button.y;
        restart = contains((SDL_Rect){345, 790, 220, 43}, x, y);
        end_turn = contains((SDL_Rect){32, 790, 300, 43}, x, y);

        clicked = m001_action_at(x, y);
        if (clicked >= 0)
            m001_focus.focused_action = clicked;
        else if (end_turn) {
            m001_focus.focused_action = -1;
            m001_focus.focused_bottom = 0;
        }
    }

    if (restart) {
        chrono_init(state);
        m001_focus_reset();
        snprintf(message, capacity, "Nuova partita M001.");
    } else if (end_turn) {
        ChronoTrace trace;
        ChronoResult result = chrono_end_turn(state, &trace);

        if (result != CHRONO_OK) {
            snprintf(message, capacity, "%s", result_label(result));
        } else if (state->finished) {
            snprintf(message, capacity, "%s",
                     state->victory ? "MISSIONE COMPLETATA!" :
                     "MISSIONE TERMINATA: obiettivo non raggiunto.");
        } else {
            const char *cause = "Attendi";
            for (int i = 0; i < ACTION_COUNT; ++i) {
                if (actions[i].id == trace.cause) {
                    cause = actions[i].label;
                    break;
                }
            }

            if (trace.changed) {
                snprintf(message, capacity,
                         "Causa: %s | Ponte %s -> %s | Papera %s -> %s",
                         cause,
                         trace.before.bridge_exists ? "SI" : "NO",
                         trace.after.bridge_exists ? "SI" : "NO",
                         trace.before.duck_exists ? "SI" : "NO",
                         trace.after.duck_exists ? "SI" : "NO");
            } else {
                snprintf(message, capacity,
                         "Turno concluso: nessuna variazione causale.");
            }
        }
    } else if (clicked >= 0) {
        ChronoResult result = chrono_apply(state, actions[clicked].id);
        snprintf(message, capacity, "%s: %s",
                 actions[clicked].label, result_label(result));

        if (state->finished && state->victory)
            snprintf(message, capacity,
                     "MISSIONE COMPLETATA! Capsula recuperata.");
    }
}

static SDL_Rect mission_menu_rect(int index)
{
    return (SDL_Rect){70, 145 + index * 106, 1140, 94};
}

static int mission_menu_pick(int x, int y)
{
    for (int i = 0; i < 5; ++i) {
        if (contains(mission_menu_rect(i), x, y))
            return i + 1;
    }
    return 0;
}

#ifdef CHRONO_WITH_M002
static void mission_menu_symbol(SDL_Renderer *renderer, int index,
                                int cx, int cy, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);

    SDL_Rect frame = {cx - 31, cy - 29, 62, 58};
    SDL_RenderDrawRect(renderer, &frame);

    if (index == 0) {
        SDL_RenderDrawLine(renderer, cx - 23, cy + 12, cx + 23, cy + 12);
        SDL_RenderDrawLine(renderer, cx - 19, cy + 12, cx, cy - 8);
        SDL_RenderDrawLine(renderer, cx, cy - 8, cx + 19, cy + 12);
        SDL_RenderDrawLine(renderer, cx - 19, cy - 8, cx - 19, cy + 20);
        SDL_RenderDrawLine(renderer, cx + 19, cy - 8, cx + 19, cy + 20);
    } else if (index == 1) {
        SDL_RenderDrawLine(renderer, cx - 19, cy - 18, cx + 19, cy - 18);
        SDL_RenderDrawLine(renderer, cx - 19, cy + 18, cx + 19, cy + 18);
        SDL_RenderDrawLine(renderer, cx - 19, cy - 18, cx + 19, cy + 18);
        SDL_RenderDrawLine(renderer, cx + 19, cy - 18, cx - 19, cy + 18);
    } else if (index == 2) {
        SDL_Rect envelope = {cx - 21, cy - 12, 42, 26};
        SDL_RenderDrawRect(renderer, &envelope);
        SDL_RenderDrawLine(renderer, cx - 21, cy - 12, cx, cy + 3);
        SDL_RenderDrawLine(renderer, cx, cy + 3, cx + 21, cy - 12);
    } else if (index == 3) {
        SDL_RenderDrawLine(renderer, cx, cy - 22, cx, cy + 22);
        SDL_RenderDrawLine(renderer, cx - 22, cy - 4, cx + 22, cy - 4);
        SDL_Rect left = {cx - 22, cy + 10, 14, 8};
        SDL_Rect right = {cx + 8, cy + 10, 14, 8};
        SDL_RenderDrawRect(renderer, &left);
        SDL_RenderDrawRect(renderer, &right);
    } else {
        SDL_Rect clock = {cx - 19, cy - 19, 38, 38};
        SDL_RenderDrawRect(renderer, &clock);
        SDL_RenderDrawLine(renderer, cx, cy, cx, cy - 13);
        SDL_RenderDrawLine(renderer, cx, cy, cx + 11, cy + 7);
    }
}

static void draw_mission_menu(SDL_Renderer *renderer, TTF_Font *font)
{
    const SDL_Color white = {235, 241, 250, 255};
    const SDL_Color muted = {155, 179, 205, 255};
    const SDL_Color gold = {239, 194, 111, 255};
    const SDL_Color colors[5] = {
        {35, 90, 135, 255},
        {31, 106, 96, 255},
        {90, 66, 117, 255},
        {110, 81, 44, 255},
        {42, 91, 109, 255}
    };
    const char *titles[5] = {
        "M001  IL PONTE CHE NON C'ERA",
        "M002  IL PRESTITO DI DOMANI",
        "M003  IL MESSAGGIO PROIBITO",
        "M004  DUE EVENTI, UNA CAUSA",
        "M005  IL MINUTO IMPOSSIBILE"
    };
    const char *descriptions[5] = {
        "Un ponte, una papera e una capsula da recuperare.",
        "Il Cronofono e un debito attraverso il tempo.",
        "Salva le persone, preserva la testimonianza.",
        "Ricollega due conseguenze a un solo evento.",
        "Ricostruisci la storia usando prove indipendenti."
    };

    int mouse_x = 0, mouse_y = 0;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    int hovered = mission_menu_pick(mouse_x, mouse_y) - 1;

    SDL_SetRenderDrawColor(renderer, 11, 23, 39, 255);
    SDL_RenderClear(renderer);

    rectangle(renderer, (SDL_Rect){0, 0, VIEW_W, 120},
              (SDL_Color){16, 32, 53, 255});
    label(renderer, font, 70, 20, gold, "CHRONOOOPS!  /  ARCHIVIO TEMPORALE");
    label(renderer, font, 70, 46, white,
          "SCEGLI UNA MISSIONE. CAMBIA IL PASSATO. AFFRONTA IL FUTURO.");

    SDL_SetRenderDrawColor(renderer, 90, 139, 168, 255);
    SDL_RenderDrawLine(renderer, 70, 107, 1210, 107);
    label(renderer, font, 72, 84, gold, "1985");
    label(renderer, font, 1140, 84, gold, "2025");

    for (int i = 0; i < 5; ++i) {
        SDL_Rect card = mission_menu_rect(i);
        bool active = i == hovered;
        SDL_Color c = colors[i];

        rectangle(renderer, card, (SDL_Color){20, 36, 56, 255});
        rectangle(renderer, (SDL_Rect){card.x, card.y, 7, card.h}, c);
        rectangle(renderer, (SDL_Rect){card.x + 8, card.y + 5,
                  card.w - 16, card.h - 10},
                  active ? (SDL_Color){42, 67, 90, 255}
                         : (SDL_Color){26, 45, 65, 255});

        SDL_SetRenderDrawColor(renderer,
                               active ? gold.r : c.r,
                               active ? gold.g : c.g,
                               active ? gold.b : c.b, 255);
        SDL_RenderDrawRect(renderer, &card);

        mission_menu_symbol(renderer, i, 1125, card.y + 47,
                            active ? gold : white);

        label(renderer, font, 102, card.y + 17,
              active ? gold : white, titles[i]);
        label(renderer, font, 102, card.y + 53, muted, descriptions[i]);
        label(renderer, font, 998, card.y + 38, gold,
              active ? "ENTRA" : ">>");
    }

    label(renderer, font, 70, 715, muted,
          "MOUSE / 1-5  SELEZIONA     ESC  ESCI");
    label(renderer, font, 70, 750, gold,
          "1985  >  CAUSA  >  CONSEGUENZA  >  2025");
    SDL_RenderPresent(renderer);
}
#endif

int main(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "ChronoOops! 2D - Missione 001",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        VIEW_W, VIEW_H, SDL_WINDOW_SHOWN
    );
    SDL_Renderer *renderer = window
        ? SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE)
        : NULL;
    TTF_Font *font = open_font();

    if (!window || !renderer || !font) {
        fprintf(stderr, "Frontend 2D: inizializzazione fallita: %s / %s\n",
                SDL_GetError(), TTF_GetError());
        if (font)
            TTF_CloseFont(font);
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (window)
            SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    ChronoState state;
    chrono_init(&state);
    char message[220] = "Missione 001 iniziata: recupera la capsula temporale.";
    bool running = true;
#ifdef CHRONO_WITH_M002
    M002Gui m002;
    M003Gui m003;
    M004Gui m004;
    M005Gui m005;
    m002_gui_init(&m002);
    m003_gui_init(&m003);
    m004_gui_init(&m004);
    m005_gui_init(&m005);
    int selected_mission = 0;
#endif


    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
#ifdef CHRONO_WITH_M002
            if (event.type == SDL_QUIT) {
                running = false;
                continue;
            }
            if (event.type == SDL_KEYDOWN &&
                event.key.keysym.sym == SDLK_ESCAPE) {
                if (selected_mission == 0)
                    running = false;
                else {
                    selected_mission = 0;
                    SDL_SetWindowTitle(window, "ChronoOops! 2D - Selezione missione");
                }
                continue;
            }
            if (selected_mission == 0) {
                int choice = 0;
                if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                    if (event.key.keysym.sym == SDLK_1)
                        choice = 1;
                    else if (event.key.keysym.sym == SDLK_2)
                        choice = 2;
                    else if (event.key.keysym.sym == SDLK_3)
                        choice = 3;
                    else if (event.key.keysym.sym == SDLK_4)
                        choice = 4;
                    else if (event.key.keysym.sym == SDLK_5)
                        choice = 5;
                } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                           event.button.button == SDL_BUTTON_LEFT) {
                    int x = event.button.x, y = event.button.y;
                    choice = mission_menu_pick(x, y);
                }
                if (choice == 1) {
                    chrono_init(&state);
                    m001_focus_reset();
                    snprintf(message, sizeof(message),
                             "Missione 001 iniziata: recupera la capsula.");
                    selected_mission = 1;
                    SDL_SetWindowTitle(window, "ChronoOops! 2D - Missione 001");
                } else if (choice == 2) {
                    m002_gui_init(&m002);
                    selected_mission = 2;
                    SDL_SetWindowTitle(window, "ChronoOops! 2D - Missione 002");
                } else if (choice == 3) {
                    m003_gui_init(&m003);
                    selected_mission = 3;
                    SDL_SetWindowTitle(window, "ChronoOops! 2D - Missione 003");
                } else if (choice == 4) {
                    m004_gui_init(&m004);
                    selected_mission = 4;
                    SDL_SetWindowTitle(window, "ChronoOops! 2D - Missione 004");
                } else if (choice == 5) {
                    m005_gui_init(&m005);
                    selected_mission = 5;
                    SDL_SetWindowTitle(window, "ChronoOops! 2D - Missione 005");
                }
                continue;
            }
            if (selected_mission == 2) {
                m002_gui_event(&m002, &event);
                continue;
            }
            if (selected_mission == 3) {
                m003_gui_event(&m003, &event);
                continue;
            }
            if (selected_mission == 4) {
                m004_gui_event(&m004, &event);
                continue;
            }
            if (selected_mission == 5) {
                m005_gui_event(&m005, &event);
                continue;
            }
#endif
            handle_event(&event, &state, message, sizeof(message), &running);
        }

#ifdef CHRONO_WITH_M002
        if (selected_mission == 0) {
            draw_mission_menu(renderer, font);
        } else if (selected_mission == 2) {
            m002_gui_draw(renderer, font, &m002);
            SDL_RenderPresent(renderer);
        } else if (selected_mission == 3) {
            m003_gui_draw(renderer, font, &m003);
            SDL_RenderPresent(renderer);
        } else if (selected_mission == 4) {
            m004_gui_draw(renderer, font, &m004);
            SDL_RenderPresent(renderer);
        } else if (selected_mission == 5) {
            m005_gui_draw(renderer, font, &m005);
            SDL_RenderPresent(renderer);
        } else {
            draw(renderer, font, &state, message);
        }
#else
        draw(renderer, font, &state, message);
#endif
        SDL_Delay(30);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
