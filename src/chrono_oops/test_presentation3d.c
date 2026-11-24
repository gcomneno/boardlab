#include "presentation3d.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    ChronoState state;
    Chrono3DScene scene = {0};

    chrono_init(&state);

    assert(!chrono_3d_scene_from_state(NULL, &scene));
    assert(!chrono_3d_scene_from_state(&state, NULL));

    assert(chrono_3d_scene_from_state(&state, &scene));
    assert(scene.turn == state.turn);
    assert(scene.action_points == state.action_points);
    assert(scene.active_player == state.active_player);
    assert(scene.location_a == state.location_a);
    assert(scene.location_b == state.location_b);

    state.plan_approved = true;
    state.selected_plan = CHRONO_PLAN_BRIDGE;

    ChronoTrace trace = {0};
    assert(chrono_end_turn(&state, &trace) == CHRONO_OK);
    assert(trace.changed);
    assert(state.world.bridge_exists);

    assert(chrono_3d_scene_from_state(&state, &scene));
    assert(scene.world.bridge_exists);
    assert(scene.selected_plan == CHRONO_PLAN_BRIDGE);

    /* A snapshot must not track subsequent engine mutations. */
    state.world.bridge_exists = false;
    assert(scene.world.bridge_exists);

    puts("PRESENTATION3D_TEST=PASS");
    return 0;
}
