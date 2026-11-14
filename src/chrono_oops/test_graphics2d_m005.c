#include "graphics2d_m005.c"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void click(M005Gui *gui, int index)
{
    assert(index >= 0 && index < M005_GUI_ACTION_COUNT);
    SDL_Rect rect = m005_action_rect(index, gui->state.active_player);
    assert(rect.w > 0 && rect.h > 0);
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


static void scene_click(M005Gui *gui, int x, int y)
{
    SDL_Event event = {0};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = x;
    event.button.y = y;
    m005_gui_event(gui, &event);
}

static void test_scene_interaction(void)
{
    M005Gui gui;
    m005_gui_init(&gui);

    scene_click(&gui, 320, 136);
    scene_click(&gui, 320, 136);
    assert(gui.state.clock_inspected);
    assert(gui.state.clock_loss_identified);
    turn(&gui);

    scene_click(&gui, 760, 170);
    scene_click(&gui, 760, 170);
    assert(gui.state.archive_consulted);
    assert(gui.state.inversion_detected);
    turn(&gui);

    scene_click(&gui, 150, 170);
    scene_click(&gui, 500, 240);
    assert(gui.state.real_order_observed);
    assert(gui.state.independent_evidence_preserved);
    turn(&gui);

    scene_click(&gui, 1115, 240);
    assert(gui.state.evidence_compared);
    puts("M005_SDL_SCENE_INTERACTION_GATE=PASS");
}


static void test_hover_feedback(void)
{
    M005Gui gui;
    m005_gui_init(&gui);

    SDL_Event event = {0};
    event.type = SDL_MOUSEMOTION;
    event.motion.x = 320;
    event.motion.y = 136;
    m005_gui_event(&gui, &event);

    assert(gui.hovered_action == 1);
    puts("M005_SDL_CLOCK_HOVER_GATE=PASS");

    scene_click(&gui, 320, 136);
    assert(gui.state.clock_inspected);

    m005_gui_event(&gui, &event);
    assert(gui.hovered_action == 2);
    puts("M005_SDL_CONTEXTUAL_HOVER_GATE=PASS");

    event.motion.x = 10;
    event.motion.y = 10;
    m005_gui_event(&gui, &event);
    assert(gui.hovered_action == -1);
    puts("M005_SDL_HOVER_CLEAR_GATE=PASS");
}

static void test_keyboard_focus(void)
{
    M005Gui gui;
    m005_gui_init(&gui);
    assert(gui.focused_action == -1 && gui.focused_bottom == 0);

    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_DOWN;
    m005_gui_event(&gui, &event);
    assert(gui.focused_action == 0);

    event.key.keysym.sym = SDLK_RIGHT;
    m005_gui_event(&gui, &event);
    assert(gui.focused_action == 11);

    event.key.keysym.sym = SDLK_LEFT;
    m005_gui_event(&gui, &event);
    assert(gui.focused_action == 0);

    event.key.keysym.sym = SDLK_UP;
    m005_gui_event(&gui, &event);
    assert(gui.focused_action == -1);

    scene_click(&gui, 320, 136);
    assert(gui.focused_action == 1);
    assert(gui.state.clock_inspected);

    SDL_Event motion = {0};
    motion.type = SDL_MOUSEMOTION;
    motion.motion.x = 10;
    motion.motion.y = 10;
    m005_gui_event(&gui, &motion);
    assert(gui.focused_action == 1);

    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 100;
    event.button.y = 806;
    m005_gui_event(&gui, &event);
    assert(gui.state.turn == 2);
    assert(gui.focused_action == -1 && gui.focused_bottom == 0);

    event.button.x = 400;
    m005_gui_event(&gui, &event);
    assert(gui.state.turn == 1);
    assert(gui.focused_action == -1);

    puts("M005_KEYBOARD_FOCUS_GATE=PASS");
}

static void test_action_availability(void)
{
    M005Gui gui;
    m005_gui_init(&gui);

    ChronoM005State snapshot = gui.state;

    assert(m005_action_available(&gui.state, 0));
    assert(m005_action_available(&gui.state, 1));
    assert(!m005_action_available(&gui.state, 2));
    assert(!m005_action_available(&gui.state, 3));
    assert(memcmp(&snapshot, &gui.state, sizeof(snapshot)) == 0);

    click(&gui, 1);
    assert(m005_action_available(&gui.state, 2));
    assert(!m005_action_available(&gui.state, 1));

    turn(&gui);
    assert(m005_action_available(&gui.state, 0));
    assert(m005_action_available(&gui.state, 3));
    assert(!m005_action_available(&gui.state, 2));

    while (!gui.state.finished)
        turn(&gui);

    for (int i = 0; i < M005_GUI_ACTION_COUNT; ++i)
        assert(!m005_action_available(&gui.state, i));

    puts("M005_ACTION_AVAILABILITY_GATE=PASS");
}

int main(void)
{
    test_action_availability();
    test_keyboard_focus();
    test_hover_feedback();
    test_scene_interaction();
    test_nominal();
    test_superficial_recovery();
    test_defeat_and_restart();
    puts("CHRONOOOPS_M005_SDL_EVENTS_GATE=PASS");
    return 0;
}
