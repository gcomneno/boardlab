#include "effects3d.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    Chrono3DEffects effects = {0};
    ChronoTrace trace = {0};

    assert(!chrono_3d_effects_from_trace(
        CHRONO_OK, NULL, &effects));
    assert(!chrono_3d_effects_from_trace(
        CHRONO_OK, &trace, NULL));

    trace.changed = true;
    trace.before.bridge_exists = false;
    trace.after.bridge_exists = true;
    trace.cause = CHRONO_ACTION_APPROVE_BRIDGE;
    trace.turn = 2;

    assert(chrono_3d_effects_from_trace(
        CHRONO_OK, &trace, &effects));
    assert(effects.flags == CHRONO_3D_EFFECT_BRIDGE);
    assert(effects.after.bridge_exists);
    assert(effects.cause == CHRONO_ACTION_APPROVE_BRIDGE);
    assert(effects.turn == 2);

    trace.before.duck_exists = true;
    trace.after.duck_exists = false;

    assert(chrono_3d_effects_from_trace(
        CHRONO_OK, &trace, &effects));
    assert(effects.flags ==
        (CHRONO_3D_EFFECT_BRIDGE | CHRONO_3D_EFFECT_DUCK));

    trace.changed = false;
    assert(chrono_3d_effects_from_trace(
        CHRONO_OK, &trace, &effects));
    assert(effects.flags == CHRONO_3D_EFFECT_NONE);

    trace.changed = true;
    assert(chrono_3d_effects_from_trace(
        CHRONO_INVALID_ACTION, &trace, &effects));
    assert(effects.flags == CHRONO_3D_EFFECT_NONE);

    puts("EFFECTS3D_TEST=PASS");
    return 0;
}
