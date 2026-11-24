#ifndef CHRONO_PRESENTATION3D_H
#define CHRONO_PRESENTATION3D_H

#include "chrono.h"

/*
 * Renderer-independent snapshot of Mission 001.
 * The engine remains authoritative for every field.
 */
typedef struct {
    unsigned turn;
    unsigned action_points;
    ChronoPlayer active_player;
    ChronoLocation location_a;
    ChronoLocation location_b;
    ChronoPlan selected_plan;
    ChronoWorld world;
    bool finished;
    bool victory;
} Chrono3DScene;

bool chrono_3d_scene_from_state(
    const ChronoState *state,
    Chrono3DScene *scene
);

#endif
