#ifndef CHRONO_ACTION_MENU3D_H
#define CHRONO_ACTION_MENU3D_H

#include "chrono.h"

enum { CHRONO_3D_ACTION_COUNT = 26 };

typedef enum {
    CHRONO_3D_ACTION_AVAILABLE,
    CHRONO_3D_ACTION_DONE,
    CHRONO_3D_ACTION_BLOCKED,
    CHRONO_3D_ACTION_OTHER_PLAYER
} Chrono3DActionStatus;

typedef struct {
    ChronoAction id;
    const char *label;
} Chrono3DActionEntry;

const Chrono3DActionEntry *chrono_3d_actions(void);

bool chrono_3d_action_for_player(
    ChronoAction action, ChronoPlayer player
);

Chrono3DActionStatus chrono_3d_action_status(
    const ChronoState *state, ChronoAction action
);

#endif
