#include "mission005.h"

#include <assert.h>
#include <stdio.h>

#define APPLY(s, action) \
    assert(chrono_m005_apply(&(s), (action)) == CHRONO_M005_OK)

#define END(s, trace) \
    assert(chrono_m005_end_turn(&(s), &(trace)) == CHRONO_M005_OK)

static void historical_start(ChronoM005State *s)
{
    chrono_m005_init(s);

    assert(s->signal_precedes_response);
    assert(s->timestamps_show_inversion);
    assert(s->clock_loss_seconds == 60);

    APPLY(*s, CHRONO_M005_INSPECT_CLOCK);
    APPLY(*s, CHRONO_M005_IDENTIFY_CLOCK_LOSS);
}

static void preserve_on_turn_three(ChronoM005State *s)
{
    ChronoM005Trace trace;

    assert(s->turn == 3);
    assert(s->active_player == CHRONO_PLAYER_A);

    APPLY(*s, CHRONO_M005_OBSERVE_REAL_ORDER);
    APPLY(*s, CHRONO_M005_PRESERVE_EVIDENCE);

    assert(s->independent_evidence_preserved);
    assert(s->evidence_propagation_pending);
    assert(!s->independent_evidence_available);

    END(*s, trace);

    assert(trace.turn == 3);
    assert(trace.changed);
    assert(trace.cause == CHRONO_M005_PRESERVE_EVIDENCE);
    assert(!trace.evidence_before);
    assert(trace.evidence_after);
    assert(s->independent_evidence_available);
    assert(!s->evidence_propagation_pending);
    assert(s->signal_precedes_response);
}

static void test_nominal_victory(void)
{
    ChronoM005State s;
    ChronoM005Trace trace;

    historical_start(&s);
    END(s, trace);

    APPLY(s, CHRONO_M005_CONSULT_ARCHIVE);
    APPLY(s, CHRONO_M005_DETECT_INVERSION);
    END(s, trace);

    preserve_on_turn_three(&s);

    APPLY(s, CHRONO_M005_COMPARE_EVIDENCE);
    APPLY(s, CHRONO_M005_RECONSTRUCT_ORDER);
    END(s, trace);

    APPLY(s, CHRONO_M005_CERTIFY_SOURCE);
    APPLY(s, CHRONO_M005_RECORD_PROVENANCE);
    END(s, trace);

    assert(s.turn == 6);
    assert(!chrono_m005_is_victory(&s));

    APPLY(s, CHRONO_M005_VALIDATE_CHRONOLOGY);
    APPLY(s, CHRONO_M005_COMPLETE_CAMPAIGN);
    END(s, trace);

    assert(s.finished && s.victory && s.turn == 6);
    assert(chrono_m005_is_victory(&s));
    assert(s.signal_precedes_response);
    assert(s.timestamps_show_inversion);
    assert(s.clock_loss_seconds == 60);

    puts("NOMINAL_VICTORY_GATE=PASS");
}

static void test_superficial_recovery(void)
{
    ChronoM005State s;
    ChronoM005Trace trace;

    historical_start(&s);
    END(s, trace);

    APPLY(s, CHRONO_M005_CONSULT_ARCHIVE);
    APPLY(s, CHRONO_M005_SUPERFICIAL_CORRECTION);

    assert(s.superficial_correction);
    assert(!s.timestamps_show_inversion);
    assert(!s.inversion_detected);
    assert(s.signal_precedes_response);
    assert(!chrono_m005_is_victory(&s));

    END(s, trace);
    preserve_on_turn_three(&s);

    APPLY(s, CHRONO_M005_RECOGNIZE_INSUFFICIENCY);
    assert(!s.superficial_correction);
    assert(s.timestamps_show_inversion);
    assert(s.insufficiency_recognized);

    APPLY(s, CHRONO_M005_DETECT_INVERSION);
    END(s, trace);

    assert(s.turn == 5);
    END(s, trace);

    assert(s.turn == 6);
    APPLY(s, CHRONO_M005_COMPARE_EVIDENCE);
    APPLY(s, CHRONO_M005_RECONSTRUCT_ORDER);
    END(s, trace);

    assert(s.turn == 7);
    APPLY(s, CHRONO_M005_CERTIFY_SOURCE);
    APPLY(s, CHRONO_M005_RECORD_PROVENANCE);
    END(s, trace);

    assert(s.turn == 8);
    APPLY(s, CHRONO_M005_VALIDATE_CHRONOLOGY);
    APPLY(s, CHRONO_M005_COMPLETE_CAMPAIGN);
    END(s, trace);

    assert(s.finished && s.victory && s.turn == 8);
    assert(chrono_m005_is_victory(&s));
    assert(s.signal_precedes_response);
    assert(s.clock_loss_seconds == 60);

    puts("SUPERFICIAL_RECOVERY_GATE=PASS");
}

static void test_propagation_and_guards(void)
{
    ChronoM005State s;
    ChronoM005Trace trace;

    chrono_m005_init(&s);

    assert(chrono_m005_apply(NULL, CHRONO_M005_WAIT) ==
           CHRONO_M005_INVALID_ACTION);
    assert(chrono_m005_end_turn(NULL, NULL) ==
           CHRONO_M005_INVALID_ACTION);

    assert(chrono_m005_apply(&s, (ChronoM005Action)-1) ==
           CHRONO_M005_INVALID_ACTION);
    assert(chrono_m005_apply(&s, CHRONO_M005_CONSULT_ARCHIVE) ==
           CHRONO_M005_WRONG_PLAYER);
    assert(chrono_m005_apply(&s, CHRONO_M005_PRESERVE_EVIDENCE) ==
           CHRONO_M005_PREREQUISITE_MISSING);

    assert(s.action_points == CHRONO_ACTION_POINTS);
    assert(!s.independent_evidence_preserved);

    historical_start(&s);

    assert(chrono_m005_apply(&s, CHRONO_M005_WAIT) ==
           CHRONO_M005_NO_ACTION_POINTS);
    assert(s.action_points == 0);

    END(s, trace);

    assert(chrono_m005_apply(&s, CHRONO_M005_COMPARE_EVIDENCE) ==
           CHRONO_M005_PREREQUISITE_MISSING);
    assert(s.action_points == CHRONO_ACTION_POINTS);

    APPLY(s, CHRONO_M005_CONSULT_ARCHIVE);
    APPLY(s, CHRONO_M005_DETECT_INVERSION);
    END(s, trace);

    assert(chrono_m005_apply(&s, CHRONO_M005_CERTIFY_SOURCE) ==
           CHRONO_M005_PREREQUISITE_MISSING);
    assert(s.action_points == CHRONO_ACTION_POINTS);

    preserve_on_turn_three(&s);

    assert(s.active_player == CHRONO_PLAYER_B);
    assert(s.turn == 4);
    assert(chrono_m005_apply(&s, CHRONO_M005_VALIDATE_CHRONOLOGY) ==
           CHRONO_M005_PREREQUISITE_MISSING);

    assert(s.action_points == CHRONO_ACTION_POINTS);
    assert(!s.chronology_validated);
    assert(!s.campaign_completed);

    puts("PROPAGATION_AND_ATOMIC_GUARDS_GATE=PASS");
}

static void test_superficial_only_cannot_win(void)
{
    ChronoM005State s;
    ChronoM005Trace trace;

    historical_start(&s);
    END(s, trace);

    APPLY(s, CHRONO_M005_CONSULT_ARCHIVE);
    APPLY(s, CHRONO_M005_SUPERFICIAL_CORRECTION);

    assert(chrono_m005_apply(&s, CHRONO_M005_DETECT_INVERSION) ==
           CHRONO_M005_NO_ACTION_POINTS);

    END(s, trace);

    preserve_on_turn_three(&s);

    assert(chrono_m005_apply(&s, CHRONO_M005_COMPARE_EVIDENCE) ==
           CHRONO_M005_PREREQUISITE_MISSING);
    assert(chrono_m005_apply(&s, CHRONO_M005_VALIDATE_CHRONOLOGY) ==
           CHRONO_M005_PREREQUISITE_MISSING);

    assert(!chrono_m005_is_victory(&s));
    assert(s.signal_precedes_response);

    puts("NO_TIMESTAMP_ONLY_VICTORY_GATE=PASS");
}

static void test_turn_limit(void)
{
    ChronoM005State s;
    ChronoM005Trace trace;

    chrono_m005_init(&s);

    for (unsigned turn = 1; turn <= CHRONO_MAX_TURNS; ++turn) {
        assert(s.turn == turn);
        END(s, trace);
    }

    assert(s.finished && !s.victory);
    assert(s.turn == CHRONO_MAX_TURNS);
    assert(s.signal_precedes_response);
    assert(s.clock_loss_seconds == 60);
    assert(chrono_m005_apply(&s, CHRONO_M005_WAIT) ==
           CHRONO_M005_MISSION_FINISHED);

    puts("TURN_LIMIT_GATE=PASS");
}

static void test_determinism(void)
{
    ChronoM005State a, b;
    ChronoM005Trace ta, tb;

    historical_start(&a);
    historical_start(&b);

    END(a, ta);
    END(b, tb);

    APPLY(a, CHRONO_M005_CONSULT_ARCHIVE);
    APPLY(b, CHRONO_M005_CONSULT_ARCHIVE);
    APPLY(a, CHRONO_M005_DETECT_INVERSION);
    APPLY(b, CHRONO_M005_DETECT_INVERSION);

    END(a, ta);
    END(b, tb);

    preserve_on_turn_three(&a);
    preserve_on_turn_three(&b);

    assert(a.turn == b.turn);
    assert(a.active_player == b.active_player);
    assert(a.action_points == b.action_points);
    assert(a.signal_precedes_response == b.signal_precedes_response);
    assert(a.timestamps_show_inversion == b.timestamps_show_inversion);
    assert(a.clock_loss_seconds == b.clock_loss_seconds);
    assert(a.independent_evidence_available ==
           b.independent_evidence_available);
    assert(a.evidence_propagation_pending ==
           b.evidence_propagation_pending);
    assert(a.evidence_compared == b.evidence_compared);
    assert(a.real_order_reconstructed == b.real_order_reconstructed);

    puts("DETERMINISM_GATE=PASS");
}

int main(void)
{
    test_nominal_victory();
    test_superficial_recovery();
    test_propagation_and_guards();
    test_superficial_only_cannot_win();
    test_turn_limit();
    test_determinism();

    puts("M005_ENGINE_TEST_GATE=PASS");
    return 0;
}
