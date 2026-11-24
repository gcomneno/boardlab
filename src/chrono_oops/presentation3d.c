#include "presentation3d.h"

bool chrono_3d_scene_from_state(
    const ChronoState *state,
    Chrono3DScene *scene
)
{
    if (state == NULL || scene == NULL) {
        return false;
    }

    Chrono3DScene next = {0};

    next.turn = state->turn;
    next.action_points = state->action_points;
    next.active_player = state->active_player;
    next.location_a = state->location_a;
    next.location_b = state->location_b;
    next.selected_plan = state->selected_plan;
    next.world = state->world;
    next.finished = state->finished;
    next.victory = state->victory;

    *scene = next;
    return true;
}
