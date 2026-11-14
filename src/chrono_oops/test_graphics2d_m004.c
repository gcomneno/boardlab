#include "graphics2d_m004.c"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void click(M004Gui *gui, int index)
{
    assert(index >= 0 && index < M004_GUI_ACTION_COUNT);

    SDL_Rect rect = m004_action_rect(index);
    SDL_Event event = {0};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = rect.x + rect.w / 2;
    event.button.y = rect.y + rect.h / 2;
    m004_gui_event(gui, &event);
}

static void turn(M004Gui *gui)
{
    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_SPACE;
    m004_gui_event(gui, &event);
}

static void prefix(M004Gui *gui)
{
    m004_gui_init(gui);

    click(gui, 1);
    click(gui, 2);
    turn(gui);

    click(gui, 3);
    click(gui, 4);
    turn(gui);

    assert(gui->state.turn == 3);
    assert(gui->state.active_player == CHRONO_PLAYER_A);
}

static void test_nominal_victory(void)
{
    M004Gui gui;
    prefix(&gui);

    click(&gui, 7);
    click(&gui, 8);

    assert(gui.state.configuration == CHRONO_M004_CONFIG_DUAL);
    assert(gui.state.propagation_pending);
    assert(!gui.state.beacon_active);
    assert(!gui.state.register_active);

    turn(&gui);

    assert(gui.state.beacon_active && gui.state.register_active);
    assert(gui.state.propagated_configuration == CHRONO_M004_CONFIG_DUAL);
    assert(!gui.state.propagation_pending);
    assert(strstr(gui.message, "NESSUNA -> DOPPIO") != NULL);
    puts("M004_SDL_DUAL_PROPAGATION_GATE=PASS");

    click(&gui, 9);
    click(&gui, 10);
    turn(&gui);

    click(&gui, 11);
    click(&gui, 12);
    turn(&gui);

    assert(gui.state.turn == 6);
    click(&gui, 13);
    click(&gui, 14);
    turn(&gui);

    assert(gui.state.finished && gui.state.victory);
    assert(gui.state.turn == 6);
    assert(chrono_m004_is_victory(&gui.state));
    assert(strstr(gui.message, "MISSIONE COMPLETATA") != NULL);
    puts("M004_SDL_VICTORY_TURN6_GATE=PASS");
}

static void test_recovery(int selected, ChronoM004Config initial)
{
    M004Gui gui;
    prefix(&gui);

    click(&gui, selected);
    turn(&gui);

    assert(gui.state.propagated_configuration == initial);
    assert(gui.state.beacon_active ==
           (initial == CHRONO_M004_CONFIG_BEACON));
    assert(gui.state.register_active ==
           (initial == CHRONO_M004_CONFIG_REGISTER));
    assert(!gui.state.finished);

    click(&gui, initial == CHRONO_M004_CONFIG_BEACON ? 9 : 10);
    turn(&gui);

    assert(gui.state.turn == 5);
    assert(gui.state.beacon_verified != gui.state.register_verified);

    click(&gui, 7);
    click(&gui, 8);
    assert(gui.state.propagation_pending);
    turn(&gui);

    assert(gui.state.propagated_configuration == CHRONO_M004_CONFIG_DUAL);
    assert(gui.state.beacon_active && gui.state.register_active);
    assert(!gui.state.beacon_verified && !gui.state.register_verified);
    assert(!gui.state.configuration_certified);
    assert(strstr(gui.message, "DOPPIO") != NULL);
    puts(initial == CHRONO_M004_CONFIG_BEACON
         ? "M004_SDL_BEACON_INVALIDATION_GATE=PASS"
         : "M004_SDL_REGISTER_INVALIDATION_GATE=PASS");

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
    assert(strstr(gui.message, "MISSIONE COMPLETATA") != NULL);
}

static void test_defeat_and_restart(void)
{
    M004Gui gui;
    m004_gui_init(&gui);

    while (!gui.state.finished)
        turn(&gui);

    assert(gui.state.turn == CHRONO_MAX_TURNS);
    assert(!gui.state.victory);
    assert(strstr(gui.message, "MISSIONE FALLITA") != NULL);
    puts("M004_SDL_TURN_LIMIT_GATE=PASS");

    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_r;
    m004_gui_event(&gui, &event);

    assert(gui.state.turn == 1);
    assert(gui.state.active_player == CHRONO_PLAYER_A);
    assert(gui.state.configuration == CHRONO_M004_CONFIG_NONE);
    assert(!gui.state.finished);
    puts("M004_SDL_RESTART_GATE=PASS");
}

int main(void)
{
    test_nominal_victory();
    test_recovery(5, CHRONO_M004_CONFIG_BEACON);
    test_recovery(6, CHRONO_M004_CONFIG_REGISTER);
    test_defeat_and_restart();
    puts("CHRONOOOPS_M004_SDL_EVENTS_GATE=PASS");
    return 0;
}
