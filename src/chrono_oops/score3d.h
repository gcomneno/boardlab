#ifndef CHRONO_SCORE3D_H
#define CHRONO_SCORE3D_H

#include <stdbool.h>
#include "chrono.h"

/* Pure scoring calculation, independent of rendering and storage. */
unsigned chrono_score3d(bool victory, bool hard_mode,
                        ChronoPlan plan, unsigned turns,
                        unsigned decoys);

#endif
