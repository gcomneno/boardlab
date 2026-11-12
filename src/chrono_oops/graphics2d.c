#include "chrono.h"

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
    label(renderer, font, 32, 309, muted,
          "AZIONI  |  Il motore C11 convalida ogni scelta");

    for (int i = 0; i < ACTION_COUNT; ++i) {
        SDL_Rect rect = action_rect(i);
        SDL_Color background =
            state->finished ? (SDL_Color){ 43, 50, 65, 255 } :
            action_for_player(actions[i].id, state->active_player)
                ? (SDL_Color){ 35, 90, 135, 255 }
                : (SDL_Color){ 39, 49, 65, 255 };
        rectangle(renderer, rect, background);
        label(renderer, font, rect.x + 10, rect.y + 5,
              white, actions[i].label);
    }

    rectangle(renderer, (SDL_Rect){ 32, 730, 300, 43 },
              (SDL_Color){ 154, 100, 42, 255 });
    label(renderer, font, 47, 741, white, "TERMINA TURNO");

    rectangle(renderer, (SDL_Rect){ 345, 730, 220, 43 },
              (SDL_Color){ 88, 65, 105, 255 });
    label(renderer, font, 360, 741, white, "RICOMINCIA");

    label(renderer, font, 32, 790, white, message);
    label(renderer, font, 32, 814, muted,
          "Mouse: seleziona azione  |  SPAZIO: termina turno  |  R: ricomincia  |  ESC: esci");
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

    if (event->type == SDL_KEYDOWN && event->key.repeat == 0) {
        restart = event->key.keysym.sym == SDLK_r;
        end_turn = event->key.keysym.sym == SDLK_SPACE;
    }

    if (event->type == SDL_MOUSEBUTTONDOWN &&
        event->button.button == SDL_BUTTON_LEFT) {
        int x = event->button.x;
        int y = event->button.y;
        restart = contains((SDL_Rect){ 345, 730, 220, 43 }, x, y);
        end_turn = contains((SDL_Rect){ 32, 730, 300, 43 }, x, y);

        for (int i = 0; i < ACTION_COUNT; ++i) {
            if (contains(action_rect(i), x, y)) {
                clicked = i;
                break;
            }
        }
    }

    if (restart) {
        chrono_init(state);
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

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event))
            handle_event(&event, &state, message, sizeof(message), &running);

        draw(renderer, font, &state, message);
        SDL_Delay(30);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
