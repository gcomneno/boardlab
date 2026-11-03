#include "mission004.h"

#include <assert.h>
#include <stdio.h>

#define APPLY(s, a) \
    assert(chrono_m004_apply(&(s), (a)) == CHRONO_M004_OK)

#define END(s, t) \
    assert(chrono_m004_end_turn(&(s), &(t)) == CHRONO_M004_OK)

static void first_two_turns(ChronoM004State *s)
{
    ChronoM004Trace t;
    chrono_m004_init(s);
    APPLY(*s, CHRONO_M004_INSPECT_DISTRIBUTOR);
    APPLY(*s, CHRONO_M004_IDENTIFY_PULSE);
    END(*s, t);
    APPLY(*s, CHRONO_M004_EXAMINE_BEACON);
    APPLY(*s, CHRONO_M004_EXAMINE_REGISTER);
    END(*s, t);
    assert(s->turn == 3 && s->active_player == CHRONO_PLAYER_A);
}

static void test_nominal_victory(void)
{
    ChronoM004State s;
    ChronoM004Trace t;
    first_two_turns(&s);

    APPLY(s, CHRONO_M004_DISCOVER_DUAL_CONTACT);
    APPLY(s, CHRONO_M004_ENABLE_DUAL);
    assert(s.propagation_pending);
    assert(!s.beacon_active && !s.register_active);
    END(s, t);

    assert(t.turn == 3 && t.changed);
    assert(t.cause == CHRONO_M004_ENABLE_DUAL);
    assert(t.configuration_before == CHRONO_M004_CONFIG_NONE);
    assert(t.configuration_after == CHRONO_M004_CONFIG_DUAL);
    assert(s.beacon_active && s.register_active);
    assert(!s.propagation_pending);

    APPLY(s, CHRONO_M004_VERIFY_BEACON);
    APPLY(s, CHRONO_M004_VERIFY_REGISTER);
    END(s, t);
    APPLY(s, CHRONO_M004_CERTIFY_CONFIGURATION);
    APPLY(s, CHRONO_M004_RECORD_COMMON_ORIGIN);
    END(s, t);

    assert(s.turn == 6 && !chrono_m004_is_victory(&s));
    APPLY(s, CHRONO_M004_VALIDATE_COHERENCE);
    APPLY(s, CHRONO_M004_COMPLETE_MISSION);
    END(s, t);

    assert(s.finished && s.victory && s.turn == 6);
    assert(chrono_m004_is_victory(&s));
    assert(chrono_m004_apply(&s, CHRONO_M004_WAIT) ==
           CHRONO_M004_MISSION_FINISHED);
    puts("NOMINAL_VICTORY_GATE=PASS");
}

static void test_exclusive(ChronoM004Config config)
{
    ChronoM004State s;
    ChronoM004Trace t;
    first_two_turns(&s);

    APPLY(s, config == CHRONO_M004_CONFIG_BEACON
        ? CHRONO_M004_SELECT_BEACON : CHRONO_M004_SELECT_REGISTER);

    assert(!s.beacon_active && !s.register_active);
    END(s, t);

    assert(t.changed);
    assert(s.beacon_active == (config == CHRONO_M004_CONFIG_BEACON));
    assert(s.register_active == (config == CHRONO_M004_CONFIG_REGISTER));
    assert(!s.finished);

    if (config == CHRONO_M004_CONFIG_BEACON) {
        APPLY(s, CHRONO_M004_VERIFY_BEACON);
        assert(chrono_m004_apply(&s, CHRONO_M004_VERIFY_REGISTER) ==
               CHRONO_M004_PREREQUISITE_MISSING);
    } else {
        APPLY(s, CHRONO_M004_VERIFY_REGISTER);
        assert(chrono_m004_apply(&s, CHRONO_M004_VERIFY_BEACON) ==
               CHRONO_M004_PREREQUISITE_MISSING);
    }
    END(s, t);

    assert(s.turn == 5 && !s.finished);
    APPLY(s, CHRONO_M004_DISCOVER_DUAL_CONTACT);
    APPLY(s, CHRONO_M004_ENABLE_DUAL);
    assert(s.propagation_pending);

    /* No premature future update or certification. */
    assert(chrono_m004_apply(&s, CHRONO_M004_CERTIFY_CONFIGURATION) ==
           CHRONO_M004_NO_ACTION_POINTS);
    END(s, t);

    assert(s.configuration == CHRONO_M004_CONFIG_DUAL);
    assert(s.beacon_active && s.register_active);
    assert(!s.beacon_verified && !s.register_verified);
    assert(!s.configuration_certified);
    assert(t.changed && t.cause == CHRONO_M004_ENABLE_DUAL);

    APPLY(s, CHRONO_M004_VERIFY_BEACON);
    APPLY(s, CHRONO_M004_VERIFY_REGISTER);
    END(s, t);

    APPLY(s, CHRONO_M004_CERTIFY_CONFIGURATION);
    APPLY(s, CHRONO_M004_RECORD_COMMON_ORIGIN);
    END(s, t);

    APPLY(s, CHRONO_M004_VALIDATE_COHERENCE);
    APPLY(s, CHRONO_M004_COMPLETE_MISSION);
    END(s, t);

    assert(s.turn == 8 && s.finished && s.victory);
}

static void test_reversible_paths(void)
{
    test_exclusive(CHRONO_M004_CONFIG_BEACON);
    puts("BEACON_RECOVERY_GATE=PASS");
    test_exclusive(CHRONO_M004_CONFIG_REGISTER);
    puts("REGISTER_RECOVERY_GATE=PASS");
}

static void test_atomic_guards(void)
{
    ChronoM004State s;
    ChronoM004Trace t;
    chrono_m004_init(&s);

    assert(chrono_m004_apply(NULL, CHRONO_M004_WAIT) ==
           CHRONO_M004_INVALID_ACTION);
    assert(chrono_m004_end_turn(NULL, NULL) ==
           CHRONO_M004_INVALID_ACTION);
    assert(chrono_m004_apply(&s, (ChronoM004Action)-1) ==
           CHRONO_M004_INVALID_ACTION);
    assert(chrono_m004_apply(&s, CHRONO_M004_EXAMINE_BEACON) ==
           CHRONO_M004_WRONG_PLAYER);
    assert(chrono_m004_apply(&s, CHRONO_M004_ENABLE_DUAL) ==
           CHRONO_M004_PREREQUISITE_MISSING);
    assert(s.action_points == 2 && s.configuration ==
           CHRONO_M004_CONFIG_NONE);

    APPLY(s, CHRONO_M004_INSPECT_DISTRIBUTOR);
    APPLY(s, CHRONO_M004_IDENTIFY_PULSE);
    assert(chrono_m004_apply(&s, CHRONO_M004_WAIT) ==
           CHRONO_M004_NO_ACTION_POINTS);
    assert(s.action_points == 0);
    END(s, t);
    assert(s.turn == 2 && s.action_points == 2);
    puts("ATOMIC_GUARDS_GATE=PASS");
}

static void test_turn_limit(void)
{
    ChronoM004State s;
    ChronoM004Trace t;
    chrono_m004_init(&s);

    for (unsigned turn = 1; turn <= CHRONO_MAX_TURNS; ++turn) {
        assert(s.turn == turn);
        END(s, t);
    }

    assert(s.finished && !s.victory);
    assert(s.turn == CHRONO_MAX_TURNS);
    puts("TURN_LIMIT_GATE=PASS");
}

static void test_determinism(void)
{
    ChronoM004State a, b;
    ChronoM004Trace ta, tb;
    first_two_turns(&a);
    first_two_turns(&b);

    APPLY(a, CHRONO_M004_DISCOVER_DUAL_CONTACT);
    APPLY(b, CHRONO_M004_DISCOVER_DUAL_CONTACT);
    APPLY(a, CHRONO_M004_ENABLE_DUAL);
    APPLY(b, CHRONO_M004_ENABLE_DUAL);
    END(a, ta);
    END(b, tb);

    assert(a.turn == b.turn);
    assert(a.configuration == b.configuration);
    assert(a.propagated_configuration == b.propagated_configuration);
    assert(a.beacon_active == b.beacon_active);
    assert(a.register_active == b.register_active);
    assert(a.action_points == b.action_points);
    assert(ta.turn == tb.turn);
    assert(ta.cause == tb.cause);
    assert(ta.configuration_before == tb.configuration_before);
    assert(ta.configuration_after == tb.configuration_after);
    assert(ta.beacon_before == tb.beacon_before);
    assert(ta.beacon_after == tb.beacon_after);
    assert(ta.register_before == tb.register_before);
    assert(ta.register_after == tb.register_after);
    assert(ta.changed == tb.changed);
    puts("DETERMINISM_GATE=PASS");
}

int main(void)
{
    test_nominal_victory();
    test_reversible_paths();
    test_atomic_guards();
    test_turn_limit();
    test_determinism();
    puts("M004_ENGINE_TEST_GATE=PASS");
    return 0;
}
