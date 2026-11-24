#include "input3d.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    ChronoState direct;
    ChronoState adapted;

    chrono_init(&direct);
    chrono_init(&adapted);

    assert(chrono_3d_apply(NULL, CHRONO_ACTION_WAIT) ==
           chrono_apply(NULL, CHRONO_ACTION_WAIT));

    assert(chrono_3d_apply(&adapted, CHRONO_ACTION_WAIT) ==
           chrono_apply(&direct, CHRONO_ACTION_WAIT));

    assert(adapted.action_points == direct.action_points);
    assert(adapted.turn == direct.turn);
    assert(adapted.active_player == direct.active_player);

    ChronoTrace direct_trace = {0};
    ChronoTrace adapted_trace = {0};

    assert(chrono_3d_end_turn(&adapted, &adapted_trace) ==
           chrono_end_turn(&direct, &direct_trace));

    assert(adapted.turn == direct.turn);
    assert(adapted.active_player == direct.active_player);
    assert(adapted_trace.changed == direct_trace.changed);

    adapted_trace.changed = true;

    assert(chrono_3d_end_turn(NULL, &adapted_trace) ==
           CHRONO_INVALID_ACTION);
    assert(!adapted_trace.changed);

    puts("INPUT3D_TEST=PASS");
    return 0;
}
