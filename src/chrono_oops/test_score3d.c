#include "score3d.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    assert(chrono_score3d(true, false, CHRONO_PLAN_BRIDGE, 6, 0, false) == 1200);
    assert(chrono_score3d(true, false, CHRONO_PLAN_DUCK, 8, 0, true) == 1250);
    assert(chrono_score3d(true, false, CHRONO_PLAN_DUCK, 8, 0, false) == 1000);
    assert(chrono_score3d(true, true, CHRONO_PLAN_BRIDGE, 6, 0, false) == 1500);
    assert(chrono_score3d(true, true, CHRONO_PLAN_DUCK, 8, 0, true) == 1550);
    assert(chrono_score3d(true, true, CHRONO_PLAN_DUCK, 8, 2, true) == 1450);
    assert(chrono_score3d(true, false, CHRONO_PLAN_BRIDGE, 5, 0, false) == 1300);
    assert(chrono_score3d(true, false, CHRONO_PLAN_BRIDGE, 6, 0, true) == 1200);
    assert(chrono_score3d(false, true, CHRONO_PLAN_BRIDGE, 8, 0, false) == 0);
    assert(chrono_score3d(true, false, CHRONO_PLAN_NONE, 6, 0, false) == 0);
    assert(chrono_score3d(true, true, CHRONO_PLAN_BRIDGE, 6, 100, false) == 0);
    assert(chrono_score3d(true, true, CHRONO_PLAN_DUCK, 0, 0, true) == 0);
    puts("SCORING_CORE_GATE=PASS");
    return 0;
}
