#include "input3d.h"

ChronoResult chrono_3d_apply(
    ChronoState *state,
    ChronoAction action
)
{
    return chrono_apply(state, action);
}

ChronoResult chrono_3d_end_turn(
    ChronoState *state,
    ChronoTrace *trace
)
{
    if (trace != NULL) {
        *trace = (ChronoTrace){0};
    }

    return chrono_end_turn(state, trace);
}
