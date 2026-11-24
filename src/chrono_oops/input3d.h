#ifndef CHRONO_INPUT3D_H
#define CHRONO_INPUT3D_H

#include "chrono.h"

/*
 * Thin input boundary for Mission 001.
 * Gameplay validation remains entirely in the C11 engine.
 */
ChronoResult chrono_3d_apply(
    ChronoState *state,
    ChronoAction action
);

/*
 * End the current turn and return its authoritative causal trace.
 * A failed transition must not expose a stale trace.
 */
ChronoResult chrono_3d_end_turn(
    ChronoState *state,
    ChronoTrace *trace
);

#endif
