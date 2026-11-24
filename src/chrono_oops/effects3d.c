#include "effects3d.h"

bool chrono_3d_effects_from_trace(
    ChronoResult result,
    const ChronoTrace *trace,
    Chrono3DEffects *effects
)
{
    if (trace == NULL || effects == NULL) {
        return false;
    }

    Chrono3DEffects next = {0};

    if (result != CHRONO_OK || !trace->changed) {
        *effects = next;
        return true;
    }

#define CHRONO_MARK_EFFECT(field, effect) \
    if (trace->before.field != trace->after.field) { \
        next.flags |= (effect); \
    }

    CHRONO_MARK_EFFECT(bridge_exists, CHRONO_3D_EFFECT_BRIDGE)
    CHRONO_MARK_EFFECT(duck_exists, CHRONO_3D_EFFECT_DUCK)
    CHRONO_MARK_EFFECT(duck_calibrated, CHRONO_3D_EFFECT_DUCK_CALIBRATION)
    CHRONO_MARK_EFFECT(duck_prepared, CHRONO_3D_EFFECT_DUCK_PREPARATION)
    CHRONO_MARK_EFFECT(duck_running, CHRONO_3D_EFFECT_DUCK_RUNNING)
    CHRONO_MARK_EFFECT(capsule_recovered, CHRONO_3D_EFFECT_CAPSULE)
    CHRONO_MARK_EFFECT(tide_register_acquired,
                       CHRONO_3D_EFFECT_TIDE_REGISTER)

#undef CHRONO_MARK_EFFECT

    next.before = trace->before;
    next.after = trace->after;
    next.cause = trace->cause;
    next.turn = trace->turn;

    *effects = next;
    return true;
}
