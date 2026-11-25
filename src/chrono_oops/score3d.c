#include "score3d.h"

unsigned chrono_score3d(bool victory, bool hard_mode,
                        ChronoPlan plan, unsigned turns,
                        unsigned decoys)
{
    if (!victory || turns == 0 || turns > 8)
        return 0;

    unsigned reference;
    if (plan == CHRONO_PLAN_BRIDGE)
        reference = 6;
    else if (plan == CHRONO_PLAN_DUCK)
        reference = 8;
    else
        return 0;

    unsigned score = 1000u + (hard_mode ? 300u : 0u);

    if (turns < reference)
        score += (reference - turns) * 100u;

    /* Avoid unsigned underflow, even for malformed input. */
    if (decoys >= (score + 49u) / 50u)
        return 0;

    return score - decoys * 50u;
}
