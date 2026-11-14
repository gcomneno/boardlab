/*
 * Test the actual GUI button table and hitboxes against M001's
 * qualified engine scenarios. No SDL window is required.
 */
#define main chrono_gui_application_main
#include "graphics2d.c"
#undef main

#include <assert.h>

static void click_action(ChronoState *state, int button)
{
    assert(button >= 0 && button < ACTION_COUNT);

    SDL_Rect rect = action_rect(button);
    int x = rect.x + rect.w / 2;
    int y = rect.y + rect.h / 2;

    assert(contains(rect, x, y));

    for (int i = 0; i < ACTION_COUNT; ++i) {
        if (i != button)
            assert(!contains(action_rect(i), x, y));
    }

    assert(chrono_apply(state, actions[button].id) == CHRONO_OK);
}

static void finish_turn(ChronoState *state, bool changed)
{
    ChronoTrace trace;
    unsigned previous_turn = state->turn;

    assert(chrono_end_turn(state, &trace) == CHRONO_OK);
    assert(trace.changed == changed);

    if (!state->finished)
        assert(state->turn == previous_turn + 1);
}

static void play(ChronoState *state, int first, int second,
                 bool changed)
{
    click_action(state, first);
    if (second >= 0)
        click_action(state, second);
    finish_turn(state, changed);
}

static void test_bridge(void)
{
    ChronoState state;
    chrono_init(&state);

    play(&state, 1, 2, false);
    play(&state, 3, 4, false);

    click_action(&state, 6);
    click_action(&state, 7);
    assert(!state.world.bridge_exists);
    finish_turn(&state, true);
    assert(state.world.bridge_exists);

    play(&state, 10, 11, false);
    play(&state, 12, -1, false);

    click_action(&state, 20);
    click_action(&state, 21);

    /* Victory is finalized at the turn boundary, as in the CLI tests. */
    assert(chrono_end_turn(&state, NULL) == CHRONO_OK);

    assert(state.finished);
    assert(state.victory);
    assert(state.turn == 6);
    assert(state.world.capsule_recovered);
    assert(!state.world.tide_register_acquired);

    puts("GUI_BUTTON_BRIDGE_VICTORY_TURN6=PASS");
}

static void test_duck(void)
{
    ChronoState state;
    chrono_init(&state);

    play(&state, 1, 2, false);
    play(&state, 3, 5, false);

    click_action(&state, 8);
    click_action(&state, 9);
    assert(!state.world.duck_exists);
    finish_turn(&state, true);
    assert(state.world.duck_exists);

    play(&state, 13, 14, false);

    click_action(&state, 15);
    click_action(&state, 16);
    assert(!state.world.duck_calibrated);
    finish_turn(&state, true);
    assert(state.world.duck_calibrated);
    assert(state.world.duck_prepared);

    play(&state, 17, 18, false);
    assert(state.world.tide_register_acquired);

    play(&state, 19, -1, false);

    click_action(&state, 20);
    click_action(&state, 21);

    /* Victory is finalized at the turn boundary, as in the CLI tests. */
    assert(chrono_end_turn(&state, NULL) == CHRONO_OK);

    assert(state.finished);
    assert(state.victory);
    assert(state.turn == 8);
    assert(state.world.capsule_recovered);
    assert(state.world.tide_register_acquired);

    puts("GUI_BUTTON_DUCK_VICTORY_TURN8=PASS");
    puts("GUI_BUTTON_DUCK_REGISTER=PASS");
}


static void send_click(ChronoState *state, char *message,
                       bool *running, int x, int y)
{
    SDL_Event event = {0};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = x;
    event.button.y = y;
    handle_event(&event, state, message, 220, running);
}

static void event_action(ChronoState *state, char *message,
                         bool *running, int index)
{
    SDL_Rect rect = action_rect(index);
    send_click(state, message, running,
               rect.x + rect.w / 2, rect.y + rect.h / 2);
    assert(strstr(message, "Azione riuscita") != NULL);
}

static void event_turn(ChronoState *state, char *message,
                       bool *running)
{
    send_click(state, message, running, 50, 806);
}

static void event_play(ChronoState *state, char *message,
                       bool *running, int first, int second)
{
    event_action(state, message, running, first);
    if (second >= 0)
        event_action(state, message, running, second);
    event_turn(state, message, running);
}

static void test_sdl_event_routes(void)
{
    static const int bridge[][2] = {
        {1, 2}, {3, 4}, {6, 7}, {10, 11}, {12, -1}, {20, 21}
    };
    static const int duck[][2] = {
        {1, 2}, {3, 5}, {8, 9}, {13, 14},
        {15, 16}, {17, 18}, {19, -1}, {20, 21}
    };

    for (int route = 0; route < 2; ++route) {
        ChronoState state;
        char message[220] = "";
        bool running = true;
        chrono_init(&state);

        const int (*steps)[2] = route == 0 ? bridge : duck;
        int count = route == 0 ? 6 : 8;

        for (int i = 0; i < count; ++i) {
            assert(state.turn == (unsigned)(i + 1));
            event_play(&state, message, &running,
                       steps[i][0], steps[i][1]);

            if (i == 2) {
                assert(strstr(message, "Causa:") != NULL);
                assert(strstr(message, route == 0
                              ? "Ponte NO -> SI"
                              : "Papera NO -> SI") != NULL);
                printf("SDL_CAUSAL_%s_FEEDBACK_GATE=PASS\n",
                       route == 0 ? "BRIDGE" : "DUCK");
            }
        }

        assert(running);
        assert(state.finished && state.victory);
        assert(state.world.capsule_recovered);
        assert(strstr(message, "MISSIONE COMPLETATA") != NULL);
        assert(state.world.tide_register_acquired == (route == 1));

        printf("SDL_MOUSE_%s_VICTORY=PASS\n",
               route == 0 ? "BRIDGE" : "DUCK");

        send_click(&state, message, &running, 365, 806);
        assert(!state.finished && state.turn == 1);
        assert(strstr(message, "Nuova partita") != NULL);
        printf("SDL_MOUSE_RESTART_GATE=PASS\n");

        SDL_Event event = {0};
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_SPACE;
        handle_event(&event, &state, message, sizeof(message), &running);
        assert(state.turn == 2);

        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_r;
        handle_event(&event, &state, message, sizeof(message), &running);
        assert(state.turn == 1);
        printf("SDL_KEYBOARD_TURN_RESTART_GATE=PASS\n");

        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_ESCAPE;
        handle_event(&event, &state, message, sizeof(message), &running);
        assert(!running);
    }

    puts("SDL_EVENT_ROUTES_GATE=PASS");
}

static void test_action_player_highlighting(void)
{
    for (int i = 0; i < ACTION_COUNT; ++i) {
        bool a = action_for_player(actions[i].id, CHRONO_PLAYER_A);
        bool b = action_for_player(actions[i].id, CHRONO_PLAYER_B);
        assert(a || b);
        assert((a && b) == (actions[i].id == CHRONO_ACTION_WAIT));
    }

    assert(action_for_player(CHRONO_ACTION_MOVE_ARCHIVE,
                             CHRONO_PLAYER_A));
    assert(action_for_player(CHRONO_ACTION_INSPECT_RUINS,
                             CHRONO_PLAYER_B));
    puts("GUI_PLAYER_ACTION_HIGHLIGHT_GATE=PASS");
}

static void test_mission_menu_hitboxes(void)
{
    for (int i = 0; i < 5; ++i) {
        SDL_Rect r = mission_menu_rect(i);
        int mission = i + 1;

        assert(r.w > 0 && r.h > 0);
        assert(mission_menu_pick(r.x, r.y) == mission);
        assert(mission_menu_pick(r.x + r.w - 1,
                                 r.y + r.h - 1) == mission);
        assert(mission_menu_pick(r.x + r.w / 2,
                                 r.y + r.h / 2) == mission);

        assert(mission_menu_pick(r.x - 1, r.y) == 0);
        assert(mission_menu_pick(r.x + r.w, r.y) == 0);
        assert(mission_menu_pick(r.x, r.y + r.h) != mission);

        for (int j = 0; j < 5; ++j) {
            if (j != i) {
                SDL_Rect other = mission_menu_rect(j);
                assert(!contains(other, r.x + r.w / 2,
                                       r.y + r.h / 2));
            }
        }
    }

    assert(mission_menu_pick(0, 0) == 0);
    assert(mission_menu_pick(1279, 839) == 0);
    assert(mission_menu_pick(70, 134) == 0);
    assert(mission_menu_pick(70, 239) == 0);

    puts("MISSION_MENU_HITBOX_GATE=PASS");
}

static void test_m001_focus_and_availability(void)
{
    ChronoState state;
    chrono_init(&state);
    m001_focus_reset();

    char message[220] = "";
    bool running = true;
    ChronoState snapshot = state;

    assert(m001_focus.focused_action == -1);
    assert(m001_focus.focused_bottom == 0);
    assert(m001_action_available(&state, 0));
    assert(m001_action_available(&state, 1));
    assert(!m001_action_available(&state, 2));
    assert(!m001_action_available(&state, 3));
    assert(memcmp(&snapshot, &state, sizeof(state)) == 0);
    assert(strcmp(m001_action_owner(0), "A/B") == 0);
    assert(strcmp(m001_action_owner(3), "B") == 0);

    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_DOWN;
    handle_event(&event, &state, message, sizeof(message), &running);
    assert(m001_focus.focused_action == 0);

    event.key.keysym.sym = SDLK_RIGHT;
    handle_event(&event, &state, message, sizeof(message), &running);
    assert(m001_focus.focused_action == 11);

    event.key.keysym.sym = SDLK_LEFT;
    handle_event(&event, &state, message, sizeof(message), &running);
    assert(m001_focus.focused_action == 0);

    event.key.keysym.sym = SDLK_UP;
    handle_event(&event, &state, message, sizeof(message), &running);
    assert(m001_focus.focused_action == -1);

    SDL_Rect rect = action_rect(1);
    send_click(&state, message, &running, rect.x + 20, rect.y + 10);
    assert(m001_focus.focused_action == 1);
    assert(m001_action_available(&state, 2));

    SDL_Event motion = {0};
    motion.type = SDL_MOUSEMOTION;
    motion.motion.x = 0;
    motion.motion.y = 0;
    handle_event(&motion, &state, message, sizeof(message), &running);
    assert(m001_focus.focused_action == 1);

    send_click(&state, message, &running, 50, 806);
    assert(m001_focus.focused_action == -1);
    assert(m001_focus.focused_bottom == 0);
    assert(state.turn == 2);

    send_click(&state, message, &running, 365, 806);
    assert(m001_focus.focused_action == -1);
    assert(state.turn == 1);
    assert(running);
    puts("M001_FOCUS_AVAILABILITY_GATE=PASS");
}

int main(void)
{
    test_m001_focus_and_availability();
    test_mission_menu_hitboxes();
    test_action_player_highlighting();
    test_bridge();
    test_duck();
    test_sdl_event_routes();
    puts("GUI_BUTTON_M001_ROUTES_GATE=PASS");
    return 0;
}
