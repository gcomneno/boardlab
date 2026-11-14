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
    int hovered_action;
    SDL_Rect click_rect;
    Uint32 click_started_ms;
    bool click_active;
    bool click_success;
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
    gui->hovered_action = -1;
    gui->click_active = false;
    gui->click_rect = (SDL_Rect){0, 0, 0, 0};
    gui->click_started_ms = 0;
    gui->click_success = false;
    snprintf(gui->message, sizeof(gui->message),
             "M005: il tempo non sbaglia. Verifica le prove, non i timestamp.");
}

static bool m005_action_visible(int index, ChronoPlayer player)
{
    return index >= 0 && index < M005_GUI_ACTION_COUNT &&
           (m005_actions[index].action == CHRONO_M005_WAIT ||
            m005_actions[index].player == player);
}

static SDL_Rect m005_action_rect(int index, ChronoPlayer player)
{
    SDL_Rect hidden = {-100, -100, 0, 0};
    if (!m005_action_visible(index, player))
        return hidden;

    int position = 0;
    for (int i = 0; i < index; ++i) {
        if (m005_action_visible(i, player))
            ++position;
    }

    SDL_Rect rect = {
        32 + (position / 5) * 620,
        485 + (position % 5) * 46,
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

    gui->click_success = result == CHRONO_M005_OK;
    gui->click_started_ms = SDL_GetTicks();
    gui->click_active = gui->click_rect.w > 0;

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


static int m005_scene_action_index(const ChronoM005State *s,
                                   int x, int y)
{
    if (m005_contains((SDL_Rect){282, 112, 79, 49}, x, y))
        return s->clock_inspected ? 2 : 1;

    if (m005_contains((SDL_Rect){72, 140, 175, 62}, x, y))
        return 7;

    if (m005_contains((SDL_Rect){482, 220, 45, 55}, x, y))
        return 8;

    if (m005_contains((SDL_Rect){686, 140, 175, 62}, x, y))
        return s->archive_consulted ? 4 : 3;

    if (m005_contains((SDL_Rect){1096, 220, 45, 55}, x, y))
        return 9;

    return -1;
}

static void m005_gui_event(M005Gui *gui, const SDL_Event *event)
{
    if (event->type == SDL_MOUSEMOTION) {
        gui->hovered_action =
            m005_scene_action_index(&gui->state,
                                    event->motion.x, event->motion.y);
        return;
    }

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
        int scene_action = m005_scene_action_index(&gui->state, x, y);
        if (scene_action >= 0) {
            gui->click_rect = (SDL_Rect){x - 12, y - 12, 24, 24};
            m005_gui_apply(gui, scene_action);
            return;
        }
        for (int i = 0; i < M005_GUI_ACTION_COUNT; ++i) {
            if (m005_contains(
                    m005_action_rect(i, gui->state.active_player), x, y)) {
                gui->click_rect =
                    m005_action_rect(i, gui->state.active_player);
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


static void m005_line(SDL_Renderer *r, int x1, int y1,
                      int x2, int y2, SDL_Color c)
{
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDrawLine(r, x1, y1, x2, y2);
}

static void m005_node(SDL_Renderer *r, TTF_Font *font,
                      int x, int y, const char *name, SDL_Color color)
{
    SDL_Rect outer = {x, y, 175, 62};
    SDL_Rect inner = {x + 4, y + 4, 167, 54};
    m005_rect(r, outer, color);
    m005_rect(r, inner, (SDL_Color){16, 31, 49, 255});
    m005_text(r, font, x + 14, y + 20,
              (SDL_Color){235, 241, 250, 255}, name);
}

static void m005_arrow(SDL_Renderer *r, int x1, int x2, int y,
                       SDL_Color color)
{
    m005_line(r, x1, y, x2, y, color);
    m005_line(r, x2 - 12, y - 9, x2, y, color);
    m005_line(r, x2 - 12, y + 9, x2, y, color);
}

static void m005_clock(SDL_Renderer *r, bool identified)
{
    const int cx = 321;
    const int cy = 136;
    const int radius = 22;
    SDL_Color rim = identified
        ? (SDL_Color){125, 219, 172, 255}
        : (SDL_Color){226, 205, 145, 255};

    SDL_Rect background = {296, 111, 50, 50};
    m005_rect(r, background, (SDL_Color){16, 31, 49, 255});

    SDL_SetRenderDrawColor(r, rim.r, rim.g, rim.b, 255);

    int x = radius;
    int y = 0;
    int decision = 1 - radius;

    while (x >= y) {
        SDL_RenderDrawPoint(r, cx + x, cy + y);
        SDL_RenderDrawPoint(r, cx + y, cy + x);
        SDL_RenderDrawPoint(r, cx - y, cy + x);
        SDL_RenderDrawPoint(r, cx - x, cy + y);
        SDL_RenderDrawPoint(r, cx - x, cy - y);
        SDL_RenderDrawPoint(r, cx - y, cy - x);
        SDL_RenderDrawPoint(r, cx + y, cy - x);
        SDL_RenderDrawPoint(r, cx + x, cy - y);

        ++y;
        if (decision < 0) {
            decision += 2 * y + 1;
        } else {
            --x;
            decision += 2 * (y - x) + 1;
        }
    }

    m005_line(r, cx, cy - 18, cx, cy - 14, rim);
    m005_line(r, cx + 15, cy, cx + 19, cy, rim);
    m005_line(r, cx, cy + 14, cx, cy + 18, rim);
    m005_line(r, cx - 19, cy, cx - 15, cy, rim);

    m005_line(r, cx, cy, cx - 9, cy - 10, rim);
    m005_line(r, cx, cy, cx + 12, cy + 9,
              (SDL_Color){245, 167, 87, 255});

    SDL_Rect pivot = {cx - 2, cy - 2, 5, 5};
    m005_rect(r, pivot, rim);
}


static void m005_draw_epoch_backdrops(SDL_Renderer *r,
                                      const ChronoM005State *s)
{
    SDL_Color old_grid = {36, 64, 86, 255};
    SDL_Color new_grid = {33, 75, 75, 255};
    SDL_Color signal = {94, 164, 188, 255};
    SDL_Color evidence = s->independent_evidence_available
        ? (SDL_Color){125, 219, 172, 255}
        : (SDL_Color){73, 85, 102, 255};

    /* 1985: analog instrument panel and waveform. */
    for (int x = 60; x <= 606; x += 30)
        m005_line(r, x, 118, x, 210, old_grid);

    for (int y = 120; y <= 210; y += 30)
        m005_line(r, 48, y, 616, y, old_grid);

    for (int x = 56; x < 600; x += 12) {
        int phase = ((x - 56) / 12) % 6;
        int y = 294 + (phase == 2 ? -7 : phase == 4 ? 7 : 0);
        m005_line(r, x, 294, x + 12, y, signal);
    }

    /* 2025: digital archive grid and evidence scanner. */
    for (int x = 674; x <= 1220; x += 30)
        m005_line(r, x, 118, x, 210, new_grid);

    for (int y = 120; y <= 210; y += 30)
        m005_line(r, 664, y, 1232, y, new_grid);

    for (int i = 0; i < 22; ++i) {
        SDL_Rect pixel = {680 + i * 24, 289, 15, 5};
        SDL_Color pixel_color = s->independent_evidence_available
            ? evidence : new_grid;
        m005_rect(r, pixel, pixel_color);
    }

    /* Temporal evidence bridge: neutral before propagation,
       illuminated only once the historical source is available. */
    m005_line(r, 605, 289, 675, 289, evidence);
    m005_line(r, 665, 283, 675, 289, evidence);
    m005_line(r, 665, 295, 675, 289, evidence);

    if (s->evidence_propagation_pending) {
        SDL_Rect pending = {625, 284, 12, 11};
        m005_rect(r, pending, (SDL_Color){245, 167, 87, 255});
    } else if (s->independent_evidence_available) {
        SDL_Rect delivered = {647, 284, 12, 11};
        m005_rect(r, delivered, evidence);
    }
}

static void m005_draw_causal_scene(SDL_Renderer *r, TTF_Font *font,
                                   const ChronoM005State *s)
{
    SDL_Color good = {125, 219, 172, 255};
    SDL_Color warning = {245, 167, 87, 255};
    SDL_Color muted = {161, 183, 205, 255};
    SDL_Color paper = {226, 205, 145, 255};
    char line[128];

    m005_draw_epoch_backdrops(r, s);

    /* 1985: physical events retain their actual causal order. */
    m005_text(r, font, 48, 88, good, "1985  |  CRONOLOGIA REALE");
    m005_node(r, font, 72, 140, "SEGNALE", good);
    m005_node(r, font, 399, 140, "RISPOSTA", good);
    m005_arrow(r, 260, 382, 171, good);

    /* Analog clock: the defect is in measurement, not event order. */
    m005_clock(r, s->clock_loss_identified);

    snprintf(line, sizeof(line), "Orologio: -%u s  |  Difetto: %s",
             s->clock_loss_seconds,
             s->clock_loss_identified ? "IDENTIFICATO" : "DA INDAGARE");
    m005_text(r, font, 48, 223, muted, line);

    /* Evidence is drawn as a document, not just a status label. */
    SDL_Rect document = {482, 220, 45, 55};
    m005_rect(r, document,
              s->independent_evidence_preserved ? good : paper);
    for (int i = 0; i < 3; ++i)
        m005_line(r, 490, 231 + i * 12, 518, 231 + i * 12,
                  (SDL_Color){28, 45, 62, 255});
    m005_text(r, font, 48, 274, muted,
              s->evidence_propagation_pending ?
              "Prova storica: propagazione in attesa" :
              s->independent_evidence_preserved ?
              "Prova storica: conservata" : "Prova storica: assente");

    /* 2025: the archive's recorded order may contradict causality. */
    m005_text(r, font, 664, 88, (SDL_Color){235, 241, 250, 255}, "2025  |  ARCHIVIO E REGISTRAZIONI");
    if (s->timestamps_show_inversion) {
        m005_node(r, font, 686, 140, "RISPOSTA", warning);
        m005_node(r, font, 1013, 140, "SEGNALE", warning);
        m005_arrow(r, 874, 996, 171, warning);
    } else {
        m005_node(r, font, 686, 140, "SEGNALE", good);
        m005_node(r, font, 1013, 140, "RISPOSTA", good);
        m005_arrow(r, 874, 996, 171, good);
    }

    snprintf(line, sizeof(line), "Archivio: %s",
             s->superficial_correction ? "RETTIFICA NON PROBANTE" :
             s->timestamps_show_inversion ? "ORDINE INVERTITO" :
                                            "ORDINE RICOSTRUITO");
    m005_text(r, font, 664, 223,
              s->superficial_correction ? warning : muted, line);

    SDL_Rect evidence = {1096, 220, 45, 55};
    m005_rect(r, evidence,
              s->independent_evidence_available ? good :
              (SDL_Color){66, 79, 94, 255});
    for (int i = 0; i < 3; ++i)
        m005_line(r, 1104, 231 + i * 12, 1132, 231 + i * 12,
                  (SDL_Color){28, 45, 62, 255});
    m005_text(r, font, 664, 274, muted,
              s->chronology_validated ? "Cronologia: VALIDATA" :
              s->evidence_compared ? "Testimonianza: CONFRONTATA" :
              s->independent_evidence_available ?
              "Testimonianza: DISPONIBILE" : "Testimonianza: ASSENTE");
}

static void m005_gui_draw(SDL_Renderer *renderer, TTF_Font *font,
                          const M005Gui *gui)
{
    const ChronoM005State *s = &gui->state;
    SDL_Color white = {235, 241, 250, 255};
    SDL_Color muted = {161, 183, 205, 255};
    SDL_Color warning = {245, 167, 87, 255};
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
    m005_draw_causal_scene(renderer, font, s);

    if (!s->finished && gui->hovered_action >= 0) {
        int index = gui->hovered_action;
        SDL_Rect highlight = {0, 0, 0, 0};

        switch (index) {
        case 1:
        case 2: highlight = (SDL_Rect){282, 112, 79, 49}; break;
        case 7: highlight = (SDL_Rect){72, 140, 175, 62}; break;
        case 8: highlight = (SDL_Rect){482, 220, 45, 55}; break;
        case 3:
        case 4: highlight = (SDL_Rect){686, 140, 175, 62}; break;
        case 9: highlight = (SDL_Rect){1096, 220, 45, 55}; break;
        default: break;
        }

        if (highlight.w > 0) {
            SDL_SetRenderDrawColor(renderer, 255, 225, 130, 255);
            SDL_RenderDrawRect(renderer, &highlight);
        }
    }

    if (!s->finished && gui->hovered_action >= 0) {
        int index = gui->hovered_action;
        snprintf(line, sizeof(line), "OGGETTO INTERATTIVO | %s",
                 m005_actions[index].label);
        m005_text(renderer, font, 32, 323, warning, line);
    } else {
        m005_text(renderer, font, 32, 323,
                  s->superficial_correction ? warning : muted,
                  s->superficial_correction ?
                  "RETTIFICA INSUFFICIENTE: ripristina le fonti originali." :
                  "AZIONI | Dimostra l'ordine reale senza modificare gli eventi.");
    }

    m005_rect(renderer, (SDL_Rect){32, 350, 1216, 110},
              (SDL_Color){18, 37, 55, 255});
    m005_text(renderer, font, 48, 361, white,
              "PERCORSO DELLA PROVA  |  cronologia documentata");

    const bool milestones[] = {
        s->clock_loss_identified,
        s->inversion_detected,
        s->independent_evidence_available,
        s->evidence_compared,
        s->real_order_reconstructed,
        s->chronology_validated
    };
    const char *milestone_names[] = {
        "OROLOGIO", "ANOMALIA", "PROVA", "CONFRONTO",
        "ORDINE", "VALIDAZIONE"
    };

    for (int i = 0; i < 6; ++i) {
        int x = 62 + i * 200;
        SDL_Color color = milestones[i]
            ? (SDL_Color){125, 219, 172, 255}
            : (SDL_Color){66, 83, 104, 255};
        m005_rect(renderer, (SDL_Rect){x, 403, 168, 12}, color);
        m005_text(renderer, font, x, 430,
                  milestones[i] ? white : muted, milestone_names[i]);
    }

    snprintf(line, sizeof(line),
             "AZIONI DISPONIBILI | Giocatore %c | Epoca %s",
             s->active_player == CHRONO_PLAYER_A ? 'A' : 'B',
             s->active_player == CHRONO_PLAYER_A ? "1985" : "2025");
    m005_text(renderer, font, 32, 465, white, line);

    for (int i = 0; i < M005_GUI_ACTION_COUNT; ++i) {
        if (!m005_action_visible(i, s->active_player))
            continue;

        SDL_Rect rect = m005_action_rect(i, s->active_player);
        SDL_Color color = s->finished
            ? (SDL_Color){43, 50, 65, 255}
            : (SDL_Color){35, 90, 135, 255};

        m005_rect(renderer, rect, color);
        m005_text(renderer, font, rect.x + 10, rect.y + 10,
                  white, m005_actions[i].label);
    }

    /* Non-blocking 200 ms feedback, drawn over the clicked target. */
    if (gui->click_active) {
        Uint32 elapsed = SDL_GetTicks() - gui->click_started_ms;
        if (elapsed < 200) {
            int spread = (int)(elapsed / 25);
            SDL_Rect pulse = gui->click_rect;
            pulse.x -= spread;
            pulse.y -= spread;
            pulse.w += 2 * spread;
            pulse.h += 2 * spread;

            SDL_Color color = gui->click_success
                ? (SDL_Color){125, 219, 172, 255}
                : (SDL_Color){245, 167, 87, 255};

            SDL_SetRenderDrawColor(renderer,
                                   color.r, color.g, color.b, 255);
            SDL_RenderDrawRect(renderer, &pulse);
        }
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
