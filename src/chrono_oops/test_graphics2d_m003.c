#include "graphics2d_m003.c"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void click(M003Gui *gui, int index)
{
    assert(index >= 0 && index < M003_GUI_ACTION_COUNT);
    SDL_Rect rect = m003_action_rect(index);
    SDL_Event event = {0};

    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = rect.x + rect.w / 2;
    event.button.y = rect.y + rect.h / 2;
    m003_gui_event(gui, &event);
}

static void turn(M003Gui *gui)
{
    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_SPACE;
    m003_gui_event(gui, &event);
}

static void restart(M003Gui *gui)
{
    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_r;
    m003_gui_event(gui, &event);
}

static void nominal_prefix(M003Gui *gui)
{
    m003_gui_init(gui);
    click(gui, 1);
    click(gui, 2);
    turn(gui);
    assert(gui->state.turn == 2);

    click(gui, 3);
    click(gui, 4);
    turn(gui);

    assert(gui->state.turn == 3);
    assert(gui->state.warning_transmitted);
    assert(gui->state.testimony_available);
}

static void test_victory_paradox_and_propagation(void)
{
    M003Gui gui;
    nominal_prefix(&gui);

    click(&gui, 5);
    ChronoM003State before = gui.state;
    click(&gui, 6);

    assert(strstr(gui.message, "PARADOSSO") != NULL);
    assert(memcmp(&before, &gui.state, sizeof(before)) == 0);
    puts("M003_SDL_PARADOX_ATOMICITY_GATE=PASS");

    click(&gui, 7);
    assert(gui.state.safety_pending);
    assert(!gui.state.people_safe);
    turn(&gui);

    assert(gui.state.people_safe);
    assert(!gui.state.safety_pending);
    assert(gui.state.testimony_available);
    assert(strstr(gui.message, "Sicurezza NO -> SI") != NULL);
    puts("M003_SDL_CAUSAL_PROPAGATION_GATE=PASS");

    click(&gui, 8);
    click(&gui, 9);
    turn(&gui);

    click(&gui, 10);
    click(&gui, 11);
    turn(&gui);

    assert(gui.state.turn == 6);
    click(&gui, 12);
    click(&gui, 13);
    assert(chrono_m003_is_victory(&gui.state));
    turn(&gui);

    assert(gui.state.finished);
    assert(gui.state.victory);
    assert(gui.state.turn == 6);
    assert(strstr(gui.message, "MISSIONE COMPLETATA") != NULL);
    puts("M003_SDL_VICTORY_TURN6_GATE=PASS");
}

static void test_cancellation_defeat(void)
{
    M003Gui gui;
    m003_gui_init(&gui);

    click(&gui, 1);
    click(&gui, 2);
    turn(&gui);
    turn(&gui);

    assert(gui.state.turn == 3);
    click(&gui, 6);
    turn(&gui);

    assert(gui.state.finished);
    assert(!gui.state.victory);
    assert(!gui.state.testimony_available);
    assert(strstr(gui.message, "MISSIONE FALLITA") != NULL);
    puts("M003_SDL_CANCELLATION_DEFEAT_GATE=PASS");
}

static void test_turn_limit_and_restart(void)
{
    M003Gui gui;
    m003_gui_init(&gui);

    while (!gui.state.finished)
        turn(&gui);

    assert(gui.state.turn == CHRONO_MAX_TURNS);
    assert(!gui.state.victory);
    puts("M003_SDL_TURN_LIMIT_GATE=PASS");

    restart(&gui);
    assert(gui.state.turn == 1);
    assert(!gui.state.finished);
    assert(gui.state.active_player == CHRONO_PLAYER_A);
    puts("M003_SDL_RESTART_GATE=PASS");
}

int main(void)
{
    test_victory_paradox_and_propagation();
    test_cancellation_defeat();
    test_turn_limit_and_restart();
    puts("CHRONOOOPS_M003_SDL_EVENTS_GATE=PASS");
    return 0;
}
