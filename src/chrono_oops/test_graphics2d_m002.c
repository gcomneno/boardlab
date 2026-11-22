#include "graphics2d_m002.c"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void click(M002Gui *gui, int index)
{
    assert(index >= 0 && index < M002_GUI_ACTION_COUNT);

    SDL_Rect rect = m002_action_rect(index);
    SDL_Event event = {0};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = rect.x + rect.w / 2;
    event.button.y = rect.y + rect.h / 2;

    m002_gui_event(gui, &event);
}

static void turn(M002Gui *gui)
{
    SDL_Event event = {0};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 50;
    event.button.y = 806;
    m002_gui_event(gui, &event);
}

static void test_victory_and_paradox(void)
{
    M002Gui gui;
    m002_gui_init(&gui);

    assert(gui.state.turn == 1);
    click(&gui, 1);
    click(&gui, 2);
    turn(&gui);

    assert(gui.state.turn == 2);
    click(&gui, 3);
    click(&gui, 4);
    assert(gui.state.debt_open);
    turn(&gui);

    assert(gui.state.turn == 3);
    ChronoM002State before = gui.state;
    click(&gui, 7);
    assert(strstr(gui.message, "PARADOSSO") != NULL);
    assert(memcmp(&before, &gui.state, sizeof(before)) == 0);
    puts("M002_SDL_PARADOX_ATOMICITY_GATE=PASS");

    click(&gui, 5);
    click(&gui, 6);
    assert(gui.state.debt_open);
    turn(&gui);

    assert(gui.state.turn == 4);
    assert(!gui.state.debt_open);
    assert(gui.state.historical_origin_exists);
    assert(strstr(gui.message, "Debito APERTO -> ESTINTO") != NULL);
    assert(strstr(gui.message, "Origine NO -> SI") != NULL);
    puts("M002_SDL_CAUSAL_PROPAGATION_GATE=PASS");

    click(&gui, 8);
    click(&gui, 9);
    turn(&gui);

    click(&gui, 10);
    click(&gui, 0);
    turn(&gui);

    assert(gui.state.turn == 6);
    click(&gui, 11);
    click(&gui, 12);
    turn(&gui);

    assert(gui.state.finished);
    assert(gui.state.victory);
    assert(gui.state.turn == 6);
    assert(chrono_m002_is_victory(&gui.state));
    assert(strstr(gui.message, "MISSIONE COMPLETATA") != NULL);
    puts("M002_SDL_VICTORY_TURN6_GATE=PASS");
}

static void test_defeat_and_restart(void)
{
    M002Gui gui;
    m002_gui_init(&gui);

    while (!gui.state.finished)
        turn(&gui);

    assert(gui.state.turn == 8);
    assert(!gui.state.victory);
    assert(strstr(gui.message, "MISSIONE FALLITA") != NULL);
    puts("M002_SDL_DEFEAT_TURN8_GATE=PASS");

    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_r;
    m002_gui_event(&gui, &event);

    assert(gui.state.turn == 1);
    assert(!gui.state.finished);
    puts("M002_SDL_RESTART_GATE=PASS");
}

static void test_m002_hover_routing(void)
{
    M002Gui gui;
    m002_gui_init(&gui);
    assert(gui.hovered_action == -1);

    for (int i = 0; i < M002_GUI_ACTION_COUNT; ++i) {
        SDL_Rect rect = m002_action_rect(i);
        SDL_Event event = {0};
        event.type = SDL_MOUSEMOTION;
        event.motion.x = rect.x + rect.w / 2;
        event.motion.y = rect.y + rect.h / 2;

        assert(m002_action_at(event.motion.x,
                              event.motion.y) == i);
        m002_gui_event(&gui, &event);
        assert(gui.hovered_action == i);
        assert(gui.state.turn == 1);
    }

    SDL_Event outside = {0};
    outside.type = SDL_MOUSEMOTION;
    outside.motion.x = 0;
    outside.motion.y = 0;
    m002_gui_event(&gui, &outside);
    assert(gui.hovered_action == -1);

    SDL_Event restart = {0};
    restart.type = SDL_KEYDOWN;
    restart.key.keysym.sym = SDLK_r;
    m002_gui_event(&gui, &restart);
    assert(gui.hovered_action == -1);

    puts("M002_HOVER_ROUTING_GATE=PASS");
}

static void test_m002_focus_and_availability(void)
{
    M002Gui gui;
    m002_gui_init(&gui);
    assert(gui.focused_action == -1 && gui.focused_bottom == 0);

    ChronoM002State original = gui.state;
    assert(m002_action_available(&gui.state, 0));
    assert(m002_action_available(&gui.state, 1));
    assert(!m002_action_available(&gui.state, 2));
    assert(!m002_action_available(&gui.state, 3));
    assert(memcmp(&original, &gui.state, sizeof(original)) == 0);
    assert(strcmp(m002_action_owner(0), "A/B") == 0);
    assert(strcmp(m002_action_owner(3), "B") == 0);

    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_DOWN;
    m002_gui_event(&gui, &event);
    assert(gui.focused_action == 0);

    event.key.keysym.sym = SDLK_RIGHT;
    m002_gui_event(&gui, &event);
    assert(gui.focused_action == 7);

    event.key.keysym.sym = SDLK_UP;
    m002_gui_event(&gui, &event);
    assert(gui.focused_action == -1 && gui.focused_bottom == 1);

    event.key.keysym.sym = SDLK_UP;
    m002_gui_event(&gui, &event);
    assert(gui.focused_action == 12);

    event.key.keysym.sym = SDLK_LEFT;
    m002_gui_event(&gui, &event);
    assert(gui.focused_action == 5);

    event.key.keysym.sym = SDLK_DOWN;
    m002_gui_event(&gui, &event);
    assert(gui.focused_action == 6);

    event.key.keysym.sym = SDLK_RIGHT;
    m002_gui_event(&gui, &event);
    assert(gui.focused_action == 12);

    click(&gui, 1);
    assert(gui.focused_action == 1);
    assert(m002_action_available(&gui.state, 2));

    SDL_Event motion = {0};
    motion.type = SDL_MOUSEMOTION;
    motion.motion.x = 0;
    motion.motion.y = 0;
    m002_gui_event(&gui, &motion);
    assert(gui.focused_action == 1);

    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 50;
    event.button.y = 806;
    m002_gui_event(&gui, &event);
    assert(gui.state.turn == 2);
    assert(gui.focused_action == -1 && gui.focused_bottom == 0);

    event.button.x = 400;
    m002_gui_event(&gui, &event);
    assert(gui.state.turn == 1);
    assert(gui.focused_action == -1);

    puts("M002_FOCUS_AVAILABILITY_GATE=PASS");
}

int main(void)
{
    test_m002_focus_and_availability();
    test_m002_hover_routing();
    test_victory_and_paradox();
    test_defeat_and_restart();
    puts("CHRONOOOPS_M002_SDL_EVENTS_GATE=PASS");
    return 0;
}
