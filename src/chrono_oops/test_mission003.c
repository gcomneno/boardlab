#include "mission003.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define APPLY(state, action) \
    assert(chrono_m003_apply(&(state), (action)) == CHRONO_M003_OK)

#define END(state, trace) \
    assert(chrono_m003_end_turn(&(state), &(trace)) == CHRONO_M003_OK)

static void nominal_prefix(ChronoM003State *s)
{
    ChronoM003Trace trace;

    chrono_m003_init(s);
    APPLY(*s, CHRONO_M003_INSPECT_EQUIPMENT);
    APPLY(*s, CHRONO_M003_IDENTIFY_RISK);
    END(*s, trace);

    assert(s->turn == 2 && s->active_player == CHRONO_PLAYER_B);
    APPLY(*s, CHRONO_M003_CONSULT_RECORD);
    APPLY(*s, CHRONO_M003_TRANSMIT_WARNING);
    END(*s, trace);

    assert(s->turn == 3 && s->active_player == CHRONO_PLAYER_A);
    assert(s->warning_transmitted && s->testimony_available);
}

static void test_nominal_victory(void)
{
    ChronoM003State s;
    ChronoM003Trace trace;

    nominal_prefix(&s);

    APPLY(s, CHRONO_M003_RECEIVE_WARNING);
    APPLY(s, CHRONO_M003_SECURE_DEMONSTRATION);
    assert(!s.people_safe && s.safety_pending);
    END(s, trace);

    assert(trace.turn == 3);
    assert(trace.cause == CHRONO_M003_SECURE_DEMONSTRATION);
    assert(!trace.safety_before && trace.safety_after);
    assert(trace.changed);
    assert(s.people_safe && s.testimony_available);
    assert(!s.safety_pending);

    APPLY(s, CHRONO_M003_VERIFY_TESTIMONY);
    APPLY(s, CHRONO_M003_CHECK_RECORD);
    END(s, trace);

    APPLY(s, CHRONO_M003_CERTIFY_DEMONSTRATION);
    APPLY(s, CHRONO_M003_ARCHIVE_TESTIMONY);
    END(s, trace);

    assert(s.turn == 6 && s.active_player == CHRONO_PLAYER_B);
    assert(!chrono_m003_is_victory(&s));
    APPLY(s, CHRONO_M003_VALIDATE_PROVENANCE);
    APPLY(s, CHRONO_M003_COMPLETE_MISSION);
    assert(chrono_m003_is_victory(&s));
    END(s, trace);

    assert(s.finished && s.victory && s.turn == 6);
    assert(chrono_m003_apply(&s, CHRONO_M003_WAIT) ==
           CHRONO_M003_MISSION_FINISHED);
    assert(chrono_m003_end_turn(&s, NULL) ==
           CHRONO_M003_MISSION_FINISHED);
    puts("NOMINAL_VICTORY_GATE=PASS");
}

static void test_paradox_atomicity(void)
{
    ChronoM003State s;
    ChronoM003State before;
    ChronoM003Trace trace;

    nominal_prefix(&s);
    APPLY(s, CHRONO_M003_RECEIVE_WARNING);
    before = s;

    assert(chrono_m003_apply(
               &s, CHRONO_M003_CANCEL_DEMONSTRATION) ==
           CHRONO_M003_CAUSAL_PARADOX);
    assert(memcmp(&s, &before, sizeof(s)) == 0);
    assert(s.action_points == 1);

    APPLY(s, CHRONO_M003_SECURE_DEMONSTRATION);
    END(s, trace);
    assert(s.people_safe && s.testimony_available);
    puts("PARADOX_ATOMICITY_GATE=PASS");
}

static void test_cancellation_defeat(void)
{
    ChronoM003State s;
    ChronoM003Trace trace;

    chrono_m003_init(&s);
    APPLY(s, CHRONO_M003_INSPECT_EQUIPMENT);
    APPLY(s, CHRONO_M003_IDENTIFY_RISK);
    END(s, trace);

    /* B does not transmit the warning in turn 2. */
    END(s, trace);
    assert(s.turn == 3 && s.active_player == CHRONO_PLAYER_A);

    APPLY(s, CHRONO_M003_CANCEL_DEMONSTRATION);
    assert(s.testimony_available);
    END(s, trace);

    assert(trace.cause == CHRONO_M003_CANCEL_DEMONSTRATION);
    assert(trace.testimony_before && !trace.testimony_after);
    assert(trace.changed);
    assert(s.finished && !s.victory);
    assert(!s.testimony_available);
    puts("CANCELLATION_DEFEAT_GATE=PASS");
}

static void test_action_guards(void)
{
    ChronoM003State s;
    ChronoM003State before;
    ChronoM003Trace trace;

    chrono_m003_init(&s);
    before = s;
    assert(chrono_m003_apply(NULL, CHRONO_M003_WAIT) ==
           CHRONO_M003_INVALID_ACTION);
    assert(chrono_m003_end_turn(NULL, NULL) ==
           CHRONO_M003_INVALID_ACTION);
    assert(chrono_m003_apply(&s, (ChronoM003Action)-1) ==
           CHRONO_M003_INVALID_ACTION);
    assert(chrono_m003_apply(&s, CHRONO_M003_CONSULT_RECORD) ==
           CHRONO_M003_WRONG_PLAYER);
    assert(chrono_m003_apply(&s, CHRONO_M003_IDENTIFY_RISK) ==
           CHRONO_M003_PREREQUISITE_MISSING);
    assert(memcmp(&s, &before, sizeof(s)) == 0);

    APPLY(s, CHRONO_M003_INSPECT_EQUIPMENT);
    APPLY(s, CHRONO_M003_IDENTIFY_RISK);
    before = s;
    assert(chrono_m003_apply(&s, CHRONO_M003_WAIT) ==
           CHRONO_M003_NO_ACTION_POINTS);
    assert(memcmp(&s, &before, sizeof(s)) == 0);

    END(s, trace);
    APPLY(s, CHRONO_M003_CONSULT_RECORD);
    APPLY(s, CHRONO_M003_TRANSMIT_WARNING);
    before = s;
    assert(chrono_m003_apply(&s, CHRONO_M003_TRANSMIT_WARNING) ==
           CHRONO_M003_NO_ACTION_POINTS);
    assert(memcmp(&s, &before, sizeof(s)) == 0);

    puts("ACTION_GUARDS_GATE=PASS");
}

static void test_turn_limit(void)
{
    ChronoM003State s;
    ChronoM003Trace trace;

    chrono_m003_init(&s);
    for (unsigned turn = 1; turn <= CHRONO_MAX_TURNS; ++turn) {
        assert(s.turn == turn);
        END(s, trace);
    }

    assert(s.finished && !s.victory);
    assert(s.turn == CHRONO_MAX_TURNS);
    puts("TURN_LIMIT_GATE=PASS");
}

static void test_determinism(void)
{
    ChronoM003State a;
    ChronoM003State b;
    ChronoM003Trace ta;
    ChronoM003Trace tb;

    nominal_prefix(&a);
    nominal_prefix(&b);

    assert(memcmp(&a, &b, sizeof(a)) == 0);
    APPLY(a, CHRONO_M003_RECEIVE_WARNING);
    APPLY(b, CHRONO_M003_RECEIVE_WARNING);
    APPLY(a, CHRONO_M003_SECURE_DEMONSTRATION);
    APPLY(b, CHRONO_M003_SECURE_DEMONSTRATION);
    END(a, ta);
    END(b, tb);

    assert(memcmp(&a, &b, sizeof(a)) == 0);
    assert(ta.turn == tb.turn);
    assert(ta.cause == tb.cause);
    assert(ta.safety_before == tb.safety_before);
    assert(ta.safety_after == tb.safety_after);
    assert(ta.testimony_before == tb.testimony_before);
    assert(ta.testimony_after == tb.testimony_after);
    assert(ta.changed == tb.changed);
    puts("DETERMINISM_GATE=PASS");
}

int main(void)
{
    test_nominal_victory();
    test_paradox_atomicity();
    test_cancellation_defeat();
    test_action_guards();
    test_turn_limit();
    test_determinism();
    puts("M003_ENGINE_TEST_GATE=PASS");
    return 0;
}
