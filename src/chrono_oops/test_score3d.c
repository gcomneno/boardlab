#include "score3d.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    assert(chrono_score3d(true, false, CHRONO_PLAN_BRIDGE, 6, 0) == 1000);
    assert(chrono_score3d(true, false, CHRONO_PLAN_DUCK, 8, 0) == 1000);
    assert(chrono_score3d(true, true, CHRONO_PLAN_BRIDGE, 6, 0) == 1300);
    assert(chrono_score3d(true, true, CHRONO_PLAN_DUCK, 8, 0) == 1300);
    assert(chrono_score3d(true, true, CHRONO_PLAN_DUCK, 8, 2) == 1200);
    assert(chrono_score3d(true, false, CHRONO_PLAN_BRIDGE, 5, 0) == 1100);
    assert(chrono_score3d(false, true, CHRONO_PLAN_BRIDGE, 8, 0) == 0);
    assert(chrono_score3d(true, false, CHRONO_PLAN_NONE, 6, 0) == 0);
    assert(chrono_score3d(true, true, CHRONO_PLAN_BRIDGE, 6, 100) == 0);
    assert(chrono_score3d(true, true, CHRONO_PLAN_DUCK, 0, 0) == 0);
    puts("SCORING_CORE_GATE=PASS");
    return 0;
}
