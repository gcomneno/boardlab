/*
 * Exercise REPLAY-02B's actual GUI helper functions.
 * No SDL window or filesystem persistence required.
 */
#define CHRONO_WITH_M002
#define main chrono_gui_application_main
#include "graphics2d.c"
#undef main

#include <assert.h>
#include <stdio.h>

static void test_restart_events(void)
{
    SDL_Event event = {0};

    event.type = SDL_KEYDOWN;
    event.key.keysym.sym = SDLK_r;
    assert(replay_restart_event(&event, 1));
    assert(replay_restart_event(&event, 5));

    event.key.repeat = 1;
    assert(!replay_restart_event(&event, 1));

    event.key.repeat = 0;
    event.key.keysym.sym = SDLK_SPACE;
    assert(!replay_restart_event(&event, 1));

    event = (SDL_Event){0};
    event.type = SDL_MOUSEBUTTONDOWN;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = 365;
    event.button.y = 750;
    assert(replay_restart_event(&event, 1));
    assert(replay_restart_event(&event, 4));

    event.button.y = 730;
    assert(replay_restart_event(&event, 1));
    assert(!replay_restart_event(&event, 4));

    puts("REPLAY_RESTART_EVENTS_GATE=PASS");
}

static void test_observation(void)
{
    ChronoReplayProgress progress;
    chrono_replay_init(&progress);

    bool recorded = false;
    const ChronoReplayMission *m;

    replay_observe(&progress, 1, 1, &recorded,
                   false, false, 6, 0);
    assert(!recorded);

    replay_observe(&progress, 1, 1, &recorded,
                   true, true, 8, CHRONO_REPLAY_TIDE_REGISTER);
    assert(recorded);

    m = chrono_replay_get(&progress, 1);
    assert(m->victories == 1 && m->best_turn == 8);
    assert(m->achievements == CHRONO_REPLAY_TIDE_REGISTER);

    replay_observe(&progress, 1, 1, &recorded,
                   true, true, 8, CHRONO_REPLAY_TIDE_REGISTER);
    assert(m->victories == 1);

    recorded = false;
    replay_observe(&progress, 1, 2, &recorded,
                   true, true, 6, 0);
    assert(recorded && m->victories == 2 && m->best_turn == 6);

    recorded = false;
    replay_observe(&progress, 2, 3, &recorded,
                   true, false, 8, 0);
    assert(recorded);
    assert(chrono_replay_get(&progress, 2)->victories == 0);

    recorded = false;
    replay_observe(&progress, 4, 4, &recorded,
                   true, true, 8, CHRONO_REPLAY_RECONFIGURED);
    assert(recorded);
    assert(chrono_replay_get(&progress, 4)->achievements ==
           CHRONO_REPLAY_RECONFIGURED);

    recorded = false;
    replay_observe(&progress, 5, 5, &recorded,
                   true, true, 8, CHRONO_REPLAY_CORRECTION_RECOVERED);
    assert(recorded);
    assert(chrono_replay_get(&progress, 5)->achievements ==
           CHRONO_REPLAY_CORRECTION_RECOVERED);

    puts("REPLAY_OBSERVATION_GATE=PASS");
}

int main(void)
{
    test_restart_events();
    test_observation();
    puts("REPLAY_GUI_HOOKS_GATE=PASS");
    return 0;
}
