#include "graphics2d_m005.c"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void click(M005Gui *gui, int index)
{
    assert(index >= 0 && index < M005_GUI_ACTION_COUNT);
    SDL_Rect rect = m005_action_rect(index);
    SDL_Event event = {0};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = rect.x + rect.w / 2;
    event.button.y = rect.y + rect.h / 2;
    m005_gui_event(gui, &event);
}

static void turn(M005Gui *gui)
{
    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_SPACE;
    m005_gui_event(gui, &event);
}

static void prefix(M005Gui *gui, bool superficial)
{
    m005_gui_init(gui);

    click(gui, 1);
    click(gui, 2);
    turn(gui);

    click(gui, 3);
    click(gui, superficial ? 5 : 4);
    turn(gui);

    assert(gui->state.turn == 3);
    assert(gui->state.signal_precedes_response);

    click(gui, 7);
    click(gui, 8);

    assert(gui->state.evidence_propagation_pending);
    assert(!gui->state.independent_evidence_available);

    turn(gui);

    assert(gui->state.independent_evidence_available);
    assert(!gui->state.evidence_propagation_pending);
    assert(strstr(gui->message, "PROPAGAZIONE") != NULL);
}

static void test_nominal(void)
{
    M005Gui gui;
    prefix(&gui, false);
    puts("M005_SDL_EVIDENCE_PROPAGATION_GATE=PASS");

    click(&gui, 9);
    click(&gui, 10);
    turn(&gui);

    click(&gui, 11);
    click(&gui, 12);
    turn(&gui);

    click(&gui, 13);
    click(&gui, 14);
    turn(&gui);

    assert(gui.state.turn == 6);
    assert(gui.state.finished && gui.state.victory);
    assert(chrono_m005_is_victory(&gui.state));
    assert(gui.state.signal_precedes_response);
    assert(strstr(gui.message, "CAMPAGNA COMPLETATA") != NULL);
    puts("M005_SDL_NOMINAL_TURN6_GATE=PASS");
}

static void test_superficial_recovery(void)
{
    M005Gui gui;
    prefix(&gui, true);

    assert(gui.state.superficial_correction);
    assert(!gui.state.timestamps_show_inversion);
    assert(!gui.state.inversion_detected);

    click(&gui, 9);
    assert(strstr(gui.message, "Prerequisito mancante") != NULL);
    assert(gui.state.action_points == CHRONO_ACTION_POINTS);
    assert(!gui.state.evidence_compared);
    puts("M005_SDL_ATOMIC_REJECTION_GATE=PASS");

    click(&gui, 6);
    click(&gui, 4);
    assert(!gui.state.superficial_correction);
    assert(gui.state.timestamps_show_inversion);
    assert(gui.state.inversion_detected);
    turn(&gui);

    turn(&gui);

    click(&gui, 9);
    click(&gui, 10);
    turn(&gui);

    click(&gui, 11);
    click(&gui, 12);
    turn(&gui);

    click(&gui, 13);
    click(&gui, 14);
    turn(&gui);

    assert(gui.state.finished && gui.state.victory);
    assert(gui.state.turn == 8);
    assert(chrono_m005_is_victory(&gui.state));
    puts("M005_SDL_SUPERFICIAL_RECOVERY_TURN8_GATE=PASS");
}

static void test_defeat_and_restart(void)
{
    M005Gui gui;
    m005_gui_init(&gui);

    while (!gui.state.finished)
        turn(&gui);

    assert(gui.state.turn == CHRONO_MAX_TURNS);
    assert(!gui.state.victory);
    assert(strstr(gui.message, "MISSIONE FALLITA") != NULL);
    puts("M005_SDL_TURN_LIMIT_GATE=PASS");

    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_r;
    m005_gui_event(&gui, &event);

    assert(gui.state.turn == 1);
    assert(gui.state.active_player == CHRONO_PLAYER_A);
    assert(gui.state.action_points == CHRONO_ACTION_POINTS);
    assert(gui.state.timestamps_show_inversion);
    assert(!gui.state.finished);
    puts("M005_SDL_RESTART_GATE=PASS");
}

int main(void)
{
    test_nominal();
    test_superficial_recovery();
    test_defeat_and_restart();
    puts("CHRONOOOPS_M005_SDL_EVENTS_GATE=PASS");
    return 0;
}
