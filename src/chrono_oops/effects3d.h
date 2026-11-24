#ifndef CHRONO_EFFECTS3D_H
#define CHRONO_EFFECTS3D_H

#include "chrono.h"
#include <stdint.h>

typedef enum {
    CHRONO_3D_EFFECT_NONE = 0,
    CHRONO_3D_EFFECT_BRIDGE = 1u << 0,
    CHRONO_3D_EFFECT_DUCK = 1u << 1,
    CHRONO_3D_EFFECT_DUCK_CALIBRATION = 1u << 2,
    CHRONO_3D_EFFECT_DUCK_PREPARATION = 1u << 3,
    CHRONO_3D_EFFECT_DUCK_RUNNING = 1u << 4,
    CHRONO_3D_EFFECT_CAPSULE = 1u << 5,
    CHRONO_3D_EFFECT_TIDE_REGISTER = 1u << 6
} Chrono3DEffect;

typedef struct {
    uint32_t flags;
    ChronoWorld before;
    ChronoWorld after;
    ChronoAction cause;
    unsigned turn;
} Chrono3DEffects;

/*
 * Read-only causal projection.
 * Only an authoritative successful changed trace may trigger effects.
 */
bool chrono_3d_effects_from_trace(
    ChronoResult result,
    const ChronoTrace *trace,
    Chrono3DEffects *effects
);

#endif
