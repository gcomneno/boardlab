#include "mission002.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define APPLY(state, action) \
    assert(chrono_m002_apply(&(state), (action)) == CHRONO_M002_OK)

#define END(state, trace) \
    assert(chrono_m002_end_turn(&(state), &(trace)) == CHRONO_M002_OK)

static void nominal(ChronoM002State *s)
{
    ChronoM002Trace t;

    chrono_m002_init(s);
    assert(s->turn == 1);
    assert(s->active_player == CHRONO_PLAYER_A);
    assert(s->action_points == 2);

    APPLY(*s, CHRONO_M002_CONSULT_ARCHIVE);
    APPLY(*s, CHRONO_M002_IDENTIFY_DESIGN);
    END(*s, t);
    assert(s->turn == 2 && s->active_player == CHRONO_PLAYER_B);

    APPLY(*s, CHRONO_M002_INSPECT_CRONOPHONE);
    APPLY(*s, CHRONO_M002_BORROW_MODULE);
    assert(s->module_borrowed && s->debt_open);
    assert(!s->historical_origin_exists);
    assert(!chrono_m002_is_victory(s));
    END(*s, t);
    assert(s->turn == 3 && s->debt_open);

    /* Rejection must preserve the entire state, including PA. */
    ChronoM002State before = *s;
    assert(chrono_m002_apply(s, CHRONO_M002_DESTROY_DESIGN) ==
           CHRONO_M002_CAUSAL_PARADOX);
    assert(memcmp(&before, s, sizeof(before)) == 0);

    APPLY(*s, CHRONO_M002_RECORD_ORIGIN);
    APPLY(*s, CHRONO_M002_APPROVE_PRODUCTION);
    assert(s->debt_open && !s->historical_origin_exists);
    assert(s->production_pending);
    END(*s, t);
    assert(t.changed);
    assert(t.turn == 3);
    assert(t.cause == CHRONO_M002_APPROVE_PRODUCTION);
    assert(t.debt_before && !t.debt_after);
    assert(!t.origin_before && t.origin_after);
    assert(!s->debt_open && s->historical_origin_exists);
    assert(s->turn == 4);

    APPLY(*s, CHRONO_M002_INSTALL_MODULE);
    APPLY(*s, CHRONO_M002_REACTIVATE_CRONOPHONE);
    END(*s, t);
    assert(s->turn == 5 && s->cronophone_active);

    APPLY(*s, CHRONO_M002_VERIFY_DOCUMENTATION);
    APPLY(*s, CHRONO_M002_WAIT);
    END(*s, t);
    assert(s->turn == 6);

    APPLY(*s, CHRONO_M002_CONFIRM_STABILITY);
    APPLY(*s, CHRONO_M002_COMPLETE_MISSION);
    assert(!s->finished);
    END(*s, t);
    assert(s->finished && s->victory && s->turn == 6);
    assert(chrono_m002_is_victory(s));
    assert(chrono_m002_apply(s, CHRONO_M002_WAIT) ==
           CHRONO_M002_MISSION_FINISHED);
    puts("NOMINAL_TURN6_GATE=PASS");
    puts("PARADOX_ATOMICITY_GATE=PASS");
    puts("DEBT_PROPAGATION_GATE=PASS");
}

static void rejection_and_defeat(void)
{
    ChronoM002State s;
    ChronoM002Trace t;
    chrono_m002_init(&s);

    ChronoM002State before = s;
    assert(chrono_m002_apply(&s, CHRONO_M002_BORROW_MODULE) ==
           CHRONO_M002_WRONG_PLAYER);
    assert(memcmp(&before, &s, sizeof(s)) == 0);

    assert(chrono_m002_apply(&s, CHRONO_M002_RECORD_ORIGIN) ==
           CHRONO_M002_PREREQUISITE_MISSING);
    assert(memcmp(&before, &s, sizeof(s)) == 0);

    assert(chrono_m002_apply(&s, (ChronoM002Action)999) ==
           CHRONO_M002_INVALID_ACTION);
    assert(memcmp(&before, &s, sizeof(s)) == 0);

    APPLY(s, CHRONO_M002_CONSULT_ARCHIVE);
    APPLY(s, CHRONO_M002_IDENTIFY_DESIGN);
    assert(s.action_points == 0);
    before = s;
    assert(chrono_m002_apply(&s, CHRONO_M002_WAIT) ==
           CHRONO_M002_NO_ACTION_POINTS);
    assert(memcmp(&before, &s, sizeof(s)) == 0);

    END(s, t);
    APPLY(s, CHRONO_M002_INSPECT_CRONOPHONE);
    APPLY(s, CHRONO_M002_BORROW_MODULE);
    assert(s.debt_open);
    before = s;
    assert(chrono_m002_apply(&s, CHRONO_M002_BORROW_MODULE) ==
           CHRONO_M002_NO_ACTION_POINTS);
    assert(memcmp(&before, &s, sizeof(s)) == 0);

    while (!s.finished) {
        END(s, t);
    }
    assert(s.turn == 8);
    assert(!s.victory && s.debt_open);
    assert(!chrono_m002_is_victory(&s));
    puts("REJECTION_ATOMICITY_GATE=PASS");
    puts("DEFEAT_TURN8_GATE=PASS");
}

int main(void)
{
    ChronoM002State first;
    ChronoM002State second;

    nominal(&first);
    nominal(&second);
    assert(memcmp(&first, &second, sizeof(first)) == 0);
    puts("DETERMINISM_GATE=PASS");

    rejection_and_defeat();
    puts("M002_BEHAVIOR_TEST_GATE=PASS");
    return 0;
}
