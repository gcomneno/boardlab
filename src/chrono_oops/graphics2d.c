#include "replay_store.h"
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "replay_progress.h"
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
#include <stdlib.h>

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

/* UX-02: M001 mouse action feedback, independent from keyboard focus. */
/* UX-02 v2: transient feedback, hover and keyboard focus remain separate. */
static int m001_mouse_feedback_action = -1;
static bool m001_mouse_feedback_accepted = false;
/* UX-03: bottom button hover, 0=end turn, 1=restart. */
static int m001_hovered_control = -1;
static Uint32 m001_mouse_feedback_until = 0;

static bool m001_feedback_visible(void)
{
    return m001_mouse_feedback_action >= 0 &&
           !SDL_TICKS_PASSED(SDL_GetTicks(), m001_mouse_feedback_until);
}

static void m001_reset_mouse_feedback(void)
{
    m001_mouse_feedback_action = -1;
    m001_mouse_feedback_accepted = false;
    m001_mouse_feedback_until = 0;
    m001_hovered_control = -1;
}


/* UX-03: observational classification; never mutate the live state. */
typedef enum {
    M001_ACTION_AVAILABLE,
    M001_ACTION_DONE,
    M001_ACTION_BLOCKED,
    M001_ACTION_OTHER_PLAYER
} M001ActionStatus;

static bool m001_action_done(const ChronoState *s, ChronoAction action)
{
    switch (action) {
    case CHRONO_ACTION_MOVE_ARCHIVE:
        return s->location_a == CHRONO_LOCATION_ARCHIVE;
    case CHRONO_ACTION_READ_PLANS:
        return s->plans_read;
    case CHRONO_ACTION_INSPECT_RUINS:
        return s->ruins_inspected;
    case CHRONO_ACTION_IDENTIFY_DEFECT:
        return s->defect_known;
    case CHRONO_ACTION_MEASURE_RIVER:
        return s->river_measured;
    case CHRONO_ACTION_SELECT_BRIDGE:
        return s->plan_selected &&
               s->selected_plan == CHRONO_PLAN_BRIDGE;
    case CHRONO_ACTION_APPROVE_BRIDGE:
        return s->plan_approved &&
               s->selected_plan == CHRONO_PLAN_BRIDGE;
    case CHRONO_ACTION_SELECT_DUCK:
        return s->plan_selected &&
               s->selected_plan == CHRONO_PLAN_DUCK;
    case CHRONO_ACTION_APPROVE_DUCK:
        return s->plan_approved &&
               s->selected_plan == CHRONO_PLAN_DUCK;
    case CHRONO_ACTION_CROSS_BRIDGE:
        return s->bridge_crossed;
    case CHRONO_ACTION_REACH_TOWER:
        return s->tower_reached;
    case CHRONO_ACTION_VERIFY_PLAN:
        return s->project_verified;
    case CHRONO_ACTION_INSPECT_DUCK:
        return s->duck_inspected;
    case CHRONO_ACTION_PREPARE_DUCK:
        return s->world.duck_prepared;
    case CHRONO_ACTION_READ_MANUAL:
        return s->manual_read;
    case CHRONO_ACTION_CALIBRATE_DUCK:
        return s->world.duck_calibrated ||
               s->calibration_pending;
    case CHRONO_ACTION_START_DUCK:
        return s->world.duck_running;
    case CHRONO_ACTION_CROSS_RIVER:
        return s->river_crossed;
    case CHRONO_ACTION_VERIFY_INTERVENTION:
        return s->intervention_verified;
    case CHRONO_ACTION_ENTER_TOWER:
        return s->tower_entered;
    case CHRONO_ACTION_RECOVER_CAPSULE:
        return s->world.capsule_recovered;
    case CHRONO_ACTION_WAIT:
    default:
        return false;
    }
}

static M001ActionStatus m001_action_status(
    const ChronoState *s, ChronoAction action
)
{
    if (!action_for_player(action, s->active_player))
        return M001_ACTION_OTHER_PLAYER;

    if (m001_action_done(s, action))
        return M001_ACTION_DONE;

    if (s->finished || s->action_points == 0)
        return M001_ACTION_BLOCKED;

    ChronoState probe = *s;
    return chrono_apply(&probe, action) == CHRONO_OK
        ? M001_ACTION_AVAILABLE : M001_ACTION_BLOCKED;
}

/* UX-03: compact status glyphs, right-aligned inside each action. */
static void m001_draw_action_icon(
    SDL_Renderer *renderer, SDL_Rect rect, M001ActionStatus status
)
{
    int x = rect.x + rect.w - 23;
    int y = rect.y + rect.h / 2;
    SDL_SetRenderDrawColor(renderer, 235, 241, 250, 255);

    switch (status) {
    case M001_ACTION_AVAILABLE: {
        SDL_Point triangle[] = {
            {x - 5, y - 7}, {x + 6, y},
            {x - 5, y + 7}, {x - 5, y - 7}
        };
        SDL_RenderDrawLines(renderer, triangle, 4);
        break;
    }
    case M001_ACTION_DONE:
        SDL_RenderDrawLine(renderer, x - 7, y,
                          x - 2, y + 5);
        SDL_RenderDrawLine(renderer, x - 2, y + 5,
                          x + 8, y - 6);
        break;

    case M001_ACTION_OTHER_PLAYER:
        /* Two people, not a text label. */
        SDL_RenderDrawRect(renderer,
                           &(SDL_Rect){x - 7, y - 7, 5, 5});
        SDL_RenderDrawRect(renderer,
                           &(SDL_Rect){x + 2, y - 7, 5, 5});
        SDL_RenderFillRect(renderer,
                           &(SDL_Rect){x - 9, y, 9, 7});
        SDL_RenderFillRect(renderer,
                           &(SDL_Rect){x + 1, y, 9, 7});
        break;

    case M001_ACTION_BLOCKED:
        /* Lock body and shackle. */
        SDL_RenderDrawRect(renderer,
                           &(SDL_Rect){x - 7, y - 1, 14, 10});
        SDL_RenderDrawLine(renderer, x - 4, y - 1,
                          x - 4, y - 6);
        SDL_RenderDrawLine(renderer, x + 4, y - 1,
                          x + 4, y - 6);
        SDL_RenderDrawLine(renderer, x - 4, y - 6,
                          x + 4, y - 6);
        break;
    }
}

static const char *m001_action_hint(
    const ChronoState *s, ChronoAction action, M001ActionStatus status
)
{
    if (status == M001_ACTION_OTHER_PLAYER)
        return "Azione dell'altra epoca: cambia turno.";
    if (status == M001_ACTION_DONE)
        return "Obiettivo gia' raggiunto: ripetere puo' consumare PA.";
    if (s->action_points == 0)
        return "PA esauriti: premi TERMINA TURNO (SPAZIO).";
    if (status == M001_ACTION_AVAILABLE)
        return "Azione disponibile: clicca per eseguirla.";
    if (action == CHRONO_ACTION_READ_PLANS &&
        s->location_a != CHRONO_LOCATION_ARCHIVE)
        return "Devi prima raggiungere l'archivio.";
    if (action == CHRONO_ACTION_IDENTIFY_DEFECT &&
        !s->ruins_inspected)
        return "Devi prima esaminare i resti del ponte.";
    return "Azione bloccata: manca un prerequisito.";
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
    if (state->action_points == 0 && !state->finished) {
        label(renderer, font, 32, 309,
              (SDL_Color){255, 185, 110, 255},
              "PA ESAURITI | Premi TERMINA TURNO (SPAZIO)");
    } else if (m001_focus.hovered_action >= 0 && !state->finished) {
        ChronoAction hovered = actions[m001_focus.hovered_action].id;
        M001ActionStatus status = m001_action_status(state, hovered);
        label(renderer, font, 32, 309, muted,
              m001_action_hint(state, hovered, status));
    } else {
        label(renderer, font, 32, 309, muted,
              "AZIONI | 2 PA per turno | A (1985) e B (2025) alternati");
    }

    for (int i = 0; i < ACTION_COUNT; ++i) {
        SDL_Rect rect = action_rect(i);
        M001ActionStatus status =
            m001_action_status(state, actions[i].id);

        SDL_Color background;
        if (state->finished)
            background = (SDL_Color){43, 50, 65, 255};
        else if (status == M001_ACTION_AVAILABLE)
            background = (SDL_Color){35, 90, 135, 255};
        else if (status == M001_ACTION_DONE)
            background = (SDL_Color){35, 88, 76, 255};
        else if (status == M001_ACTION_OTHER_PLAYER)
            background = (SDL_Color){33, 41, 54, 255};
        else
            background = (SDL_Color){45, 52, 64, 255};

        if (!state->finished && m001_focus.hovered_action == i &&
            status == M001_ACTION_AVAILABLE)
            background = (SDL_Color){53, 118, 170, 255};

        rectangle(renderer, rect, background);

        label(renderer, font, rect.x + 10, rect.y + 5,
              status == M001_ACTION_OTHER_PLAYER ? muted : white,
              actions[i].label);
        m001_draw_action_icon(renderer, rect, status);
        if (m001_focus.focused_action == i)
            m001_focus_rect(renderer, rect);
        else if (!state->finished &&
                 m001_focus.hovered_action == i) {
            SDL_SetRenderDrawColor(renderer, 239, 194, 111, 255);
            SDL_RenderDrawRect(renderer, &rect);
        }
    }


    if (m001_feedback_visible() && !state->finished) {
        SDL_Rect selected = action_rect(m001_mouse_feedback_action);

        if (m001_mouse_feedback_accepted)
            SDL_SetRenderDrawColor(renderer, 105, 230, 156, 255);
        else
            SDL_SetRenderDrawColor(renderer, 255, 159, 72, 255);

        for (int border = 0; border < 3; ++border) {
            SDL_Rect outline = {
                selected.x + border,
                selected.y + border,
                selected.w - 2 * border,
                selected.h - 2 * border
            };
            SDL_RenderDrawRect(renderer, &outline);
        }
    }

    /* REPLAY-01A: M001 mission result overlay */
    if (state->finished) {
        SDL_Color gold = {255, 220, 80, 255};
        SDL_Color white_result = {235, 241, 250, 255};
        SDL_Color detail = {175, 202, 227, 255};
        char result[180];

        rectangle(renderer, (SDL_Rect){120, 365, 1040, 290},
                  (SDL_Color){21, 40, 60, 255});

        label(renderer, font, 155, 390, gold,
              state->victory ? "MISSIONE COMPLETATA" :
                               "MISSIONE NON COMPLETATA");

        snprintf(result, sizeof(result),
                 "Turni impiegati: %u / %u",
                 state->turn, CHRONO_MAX_TURNS);
        label(renderer, font, 155, 430, white_result, result);

        const char *strategy =
            state->selected_plan == CHRONO_PLAN_BRIDGE
                ? "Ponte riparato" :
            state->selected_plan == CHRONO_PLAN_DUCK
                ? "Papera a Pendolo" : "Nessuna strategia completata";

        snprintf(result, sizeof(result),
                 "Strategia: %s", strategy);
        label(renderer, font, 155, 466, white_result, result);

        snprintf(result, sizeof(result),
                 "Capsula: %s | Registro delle Maree: %s",
                 state->world.capsule_recovered ? "RECUPERATA" : "MANCANTE",
                 state->world.tide_register_acquired ? "OTTENUTO" : "MANCANTE");
        label(renderer, font, 155, 502, white_result, result);

        if (state->victory) {
            label(renderer, font, 155, 548, detail,
                  state->world.tide_register_acquired
                      ? "Bonus acquisito! Prova anche il percorso piu' rapido."
                      : "Obiettivo raggiunto! Scopri il Registro delle Maree.");
        } else {
            label(renderer, font, 155, 548, detail,
                  "Riprova: ogni scelta modifica il futuro.");
        }

        label(renderer, font, 155, 607, detail,
              "R: rigioca la missione | ESC: selezione missioni");
    }

    rectangle(renderer, (SDL_Rect){32, 725, 1216, 33},
              (SDL_Color){29, 49, 72, 255});
    rectangle(renderer, (SDL_Rect){32, 725, 4, 33},
              (SDL_Color){245, 167, 87, 255});
    label(renderer, font, 46, 732,
          (SDL_Color){245, 167, 87, 255}, "EVENTO");
    label(renderer, font, 136, 732, white, message);

    rectangle(renderer, (SDL_Rect){32, 790, 300, 43},
              m001_hovered_control == 0
                   ? (SDL_Color){207, 139, 58, 255}
                   : (SDL_Color){154, 100, 42, 255});
    label(renderer, font, 47, 801, white, "TERMINA TURNO");

    rectangle(renderer, (SDL_Rect){345, 790, 220, 43},
              m001_hovered_control == 1
                   ? (SDL_Color){132, 91, 158, 255}
                   : (SDL_Color){88, 65, 105, 255});
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
    const char *override = getenv("CHRONO_FONT_PATH");
    if (override != NULL && override[0] != '\0') {
        TTF_Font *font = TTF_OpenFont(override, 15);
        if (font)
            return font;
    }

    const char *paths[] = {
#if defined(_WIN32)
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf",
#elif defined(__APPLE__)
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
#else
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
#endif
    };

    for (unsigned i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        TTF_Font *font = TTF_OpenFont(paths[i], 15);
        if (font)
            return font;
    }

#if defined(_WIN32)
    const char *windows_dir = getenv("WINDIR");
    if (windows_dir != NULL && windows_dir[0] != '\0') {
        char path[1024];
        int written = snprintf(path, sizeof(path),
                               "%s\\Fonts\\arial.ttf", windows_dir);
        if (written > 0 && (size_t)written < sizeof(path))
            return TTF_OpenFont(path, 15);
    }
#endif

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
    bool mouse_clicked_action = false;

    if (event->type == SDL_MOUSEMOTION) {
        int x = event->motion.x;
        int y = event->motion.y;

        m001_hovered_control = -1;
        if (contains((SDL_Rect){32, 790, 300, 43}, x, y))
            m001_hovered_control = 0;
        else if (contains((SDL_Rect){345, 790, 220, 43}, x, y))
            m001_hovered_control = 1;

        m001_focus.hovered_action = state->finished
            ? -1 : m001_action_at(x, y);
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
        if (clicked >= 0) {
            m001_focus.focused_action = clicked;
            mouse_clicked_action = true;
        }
        else if (end_turn) {
            m001_focus.focused_action = -1;
            m001_focus.focused_bottom = 0;
        }
    }

    if (restart) {
        chrono_init(state);
        m001_focus_reset();
        m001_reset_mouse_feedback();
        snprintf(message, capacity, "Nuova partita M001.");
    } else if (end_turn) {
        m001_reset_mouse_feedback();
        ChronoTrace trace;
        ChronoResult result = chrono_end_turn(state, &trace);

        if (m001_focus.focused_action >= 0 &&
            !action_for_player(
                actions[m001_focus.focused_action].id,
                state->active_player))
            m001_focus.focused_action = -1;

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
        if (mouse_clicked_action) {
            m001_mouse_feedback_action = clicked;
            m001_mouse_feedback_accepted = result == CHRONO_OK;
            m001_mouse_feedback_until = SDL_GetTicks() + 700u;
        }

        if (result == CHRONO_NO_ACTION_POINTS)
            snprintf(message, capacity,
                     "PA esauriti: premi TERMINA TURNO (SPAZIO).");
        else
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


#ifdef CHRONO_WITH_M002

/* REPLAY-02C GUI persistence: XDG data directory, no game-state writes. */
static bool replay_ensure_directory(const char *path)
{
    struct stat info;
    if (mkdir(path, 0700) != 0 && errno != EEXIST)
        return false;
    return stat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

static bool replay_prepare_save_path(char *path, size_t capacity)
{
    char base[4096];
    char directory[4096];
    const char *xdg = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");
    int count;

    if (xdg != NULL && *xdg == '/') {
        count = snprintf(base, sizeof(base), "%s", xdg);
        if (count < 0 || (size_t)count >= sizeof(base))
            return false;
    } else {
        if (home == NULL || *home != '/')
            return false;

        count = snprintf(base, sizeof(base), "%s/.local", home);
        if (count < 0 || (size_t)count >= sizeof(base))
            return false;
        if (!replay_ensure_directory(base))
            return false;

        count = snprintf(base, sizeof(base),
                         "%s/.local/share", home);
        if (count < 0 || (size_t)count >= sizeof(base))
            return false;
    }

    if (!replay_ensure_directory(base))
        return false;

    count = snprintf(directory, sizeof(directory),
                     "%s/chrono-oops", base);
    if (count < 0 || (size_t)count >= sizeof(directory))
        return false;
    if (!replay_ensure_directory(directory))
        return false;

    count = snprintf(path, capacity,
                     "%s/progress.v1", directory);
    return count >= 0 && (size_t)count < capacity;
}

static void replay_save_if_changed(
    const ChronoReplayProgress *progress,
    const char *path, bool *enabled, bool changed
)
{
    if (!changed || !*enabled)
        return;

    ChronoStoreResult result = chrono_store_save(path, progress);
    if (result != CHRONO_STORE_OK) {
        fprintf(stderr,
                "ChronoOops: salvataggio non riuscito (%d); "
                "progressi precedenti preservati.\n", (int)result);
        *enabled = false;
    }
}

/* REPLAY-02B GUI integration: observation only, no causal mutations. */
static bool replay_restart_event(const SDL_Event *event, int mission)
{
    if (event->type == SDL_KEYDOWN)
        return event->key.repeat == 0 &&
               event->key.keysym.sym == SDLK_r;

    if (event->type != SDL_MOUSEBUTTONDOWN ||
        event->button.button != SDL_BUTTON_LEFT)
        return false;

    SDL_Rect rect = {345, mission == 1 ? 730 : 735, 220, 43};
    return contains(rect, event->button.x, event->button.y);
}

static bool replay_observe(ChronoReplayProgress *progress,
                           unsigned mission, uint64_t run_id,
                           bool *recorded, bool finished,
                           bool victory, unsigned turns,
                           unsigned achievements)
{
    if (!finished || *recorded || run_id == 0)
        return false;

    ChronoReplayResult result = chrono_replay_record(
        progress, mission, run_id, victory, turns,
        victory ? achievements : 0
    );

    if (result == CHRONO_REPLAY_RECORDED ||
        result == CHRONO_REPLAY_DUPLICATE)
        *recorded = true;

    return result == CHRONO_REPLAY_RECORDED;
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
    ChronoReplayProgress replay_progress;
    chrono_replay_init(&replay_progress);

    char replay_save_path[4096] = {0};
    bool replay_store_enabled =
        replay_prepare_save_path(replay_save_path,
                                 sizeof(replay_save_path));

    if (replay_store_enabled) {
        ChronoStoreResult loaded =
            chrono_store_load(replay_save_path, &replay_progress);
        if (loaded != CHRONO_STORE_OK &&
            loaded != CHRONO_STORE_NOT_FOUND) {
            fprintf(stderr,
                    "ChronoOops: progressi non caricabili (%d); "
                    "salvataggio disabilitato per questa sessione.\n",
                    (int)loaded);
            replay_store_enabled = false;
        }
    } else {
        fprintf(stderr,
                "ChronoOops: directory dati non disponibile; "
                "salvataggio disabilitato.\n");
    }

    uint64_t replay_next_run_id = 0;
    uint64_t replay_run_ids[5] = {0};
    bool replay_recorded[5] = {false};
    bool replay_m004_exclusive = false;
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
                if (choice >= 1 && choice <= 5) {
                    unsigned index = (unsigned)(choice - 1);
                    replay_run_ids[index] = chrono_replay_next_run_id(&replay_progress, &replay_next_run_id);
                    replay_recorded[index] = false;
                    if (choice == 4)
                        replay_m004_exclusive = false;
                }
                if (choice == 1) {
                    chrono_init(&state);
                    m001_focus_reset();
                    m001_reset_mouse_feedback();
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
                if (replay_restart_event(&event, 2)) {
                    replay_run_ids[1] = chrono_replay_next_run_id(&replay_progress, &replay_next_run_id);
                    replay_recorded[1] = false;
                }
                m002_gui_event(&m002, &event);
                bool replay_changed = replay_observe(&replay_progress,
                    2, replay_run_ids[1],
                    &replay_recorded[1],
                    m002.state.finished,
                    m002.state.victory,
                    m002.state.turn, 0);
                replay_save_if_changed(
                    &replay_progress, replay_save_path,
                    &replay_store_enabled, replay_changed);
                continue;
            }
            if (selected_mission == 3) {
                if (replay_restart_event(&event, 3)) {
                    replay_run_ids[2] = chrono_replay_next_run_id(&replay_progress, &replay_next_run_id);
                    replay_recorded[2] = false;
                }
                m003_gui_event(&m003, &event);
                bool replay_changed = replay_observe(&replay_progress,
                    3, replay_run_ids[2],
                    &replay_recorded[2],
                    m003.state.finished,
                    m003.state.victory,
                    m003.state.turn, 0);
                replay_save_if_changed(
                    &replay_progress, replay_save_path,
                    &replay_store_enabled, replay_changed);
                continue;
            }
            if (selected_mission == 4) {
                if (replay_restart_event(&event, 4)) {
                    replay_run_ids[3] = chrono_replay_next_run_id(&replay_progress, &replay_next_run_id);
                    replay_recorded[3] = false;
                }
                if (replay_restart_event(&event, 4))
                    replay_m004_exclusive = false;
                m004_gui_event(&m004, &event);
                if (m004.state.beacon_active !=
                    m004.state.register_active)
                    replay_m004_exclusive = true;
                bool replay_changed = replay_observe(&replay_progress,
                    4, replay_run_ids[3],
                    &replay_recorded[3],
                    m004.state.finished,
                    m004.state.victory,
                    m004.state.turn, (replay_m004_exclusive ? CHRONO_REPLAY_RECONFIGURED : 0));
                replay_save_if_changed(
                    &replay_progress, replay_save_path,
                    &replay_store_enabled, replay_changed);
                continue;
            }
            if (selected_mission == 5) {
                if (replay_restart_event(&event, 5)) {
                    replay_run_ids[4] = chrono_replay_next_run_id(&replay_progress, &replay_next_run_id);
                    replay_recorded[4] = false;
                }
                m005_gui_event(&m005, &event);
                bool replay_changed = replay_observe(&replay_progress,
                    5, replay_run_ids[4],
                    &replay_recorded[4],
                    m005.state.finished,
                    m005.state.victory,
                    m005.state.turn, (m005.state.insufficiency_recognized ? CHRONO_REPLAY_CORRECTION_RECOVERED : 0));
                replay_save_if_changed(
                    &replay_progress, replay_save_path,
                    &replay_store_enabled, replay_changed);
                continue;
            }
#endif
#ifdef CHRONO_WITH_M002
            if (replay_restart_event(&event, 1)) {
                replay_run_ids[0] = chrono_replay_next_run_id(&replay_progress, &replay_next_run_id);
                replay_recorded[0] = false;
            }
#endif
            handle_event(&event, &state, message, sizeof(message), &running);
#ifdef CHRONO_WITH_M002
            bool replay_changed = replay_observe(&replay_progress, 1, replay_run_ids[0],
                           &replay_recorded[0], state.finished,
                           state.victory, state.turn,
                           state.world.tide_register_acquired
                               ? CHRONO_REPLAY_TIDE_REGISTER : 0);
            replay_save_if_changed(
                &replay_progress, replay_save_path,
                &replay_store_enabled, replay_changed);
#endif
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
