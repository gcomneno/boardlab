#ifndef CHRONO_OOPS_MISSION005_H
#define CHRONO_OOPS_MISSION005_H

#include "chrono.h"

/* Mission 005: The Minute That Never Existed. */

typedef enum {
    CHRONO_M005_WAIT = 0,
    CHRONO_M005_INSPECT_CLOCK,
    CHRONO_M005_IDENTIFY_CLOCK_LOSS,
    CHRONO_M005_CONSULT_ARCHIVE,
    CHRONO_M005_DETECT_INVERSION,
    CHRONO_M005_SUPERFICIAL_CORRECTION,
    CHRONO_M005_RECOGNIZE_INSUFFICIENCY,
    CHRONO_M005_OBSERVE_REAL_ORDER,
    CHRONO_M005_PRESERVE_EVIDENCE,
    CHRONO_M005_COMPARE_EVIDENCE,
    CHRONO_M005_RECONSTRUCT_ORDER,
    CHRONO_M005_CERTIFY_SOURCE,
    CHRONO_M005_RECORD_PROVENANCE,
    CHRONO_M005_VALIDATE_CHRONOLOGY,
    CHRONO_M005_COMPLETE_CAMPAIGN
} ChronoM005Action;

typedef enum {
    CHRONO_M005_OK = 0,
    CHRONO_M005_INVALID_ACTION,
    CHRONO_M005_WRONG_PLAYER,
    CHRONO_M005_NO_ACTION_POINTS,
    CHRONO_M005_PREREQUISITE_MISSING,
    CHRONO_M005_MISSION_FINISHED
} ChronoM005Result;

typedef struct {
    ChronoPlayer active_player;
    unsigned turn;
    unsigned action_points;

    /*
     * Actual event order never changes.
     * The defective timestamps initially show the reverse order.
     */
    bool signal_precedes_response;
    bool timestamps_show_inversion;
    unsigned clock_loss_seconds;

    bool clock_inspected;
    bool clock_loss_identified;
    bool archive_consulted;
    bool inversion_detected;

    /*
     * Timestamp correction is not evidence.
     * Recognizing its insufficiency restores reliance
     * on the original archive.
     */
    bool superficial_correction;
    bool insufficiency_recognized;

    bool real_order_observed;
    bool independent_evidence_preserved;
    bool evidence_propagation_pending;
    bool independent_evidence_available;

    /* Checks belong to the currently propagated evidence. */
    bool evidence_compared;
    bool real_order_reconstructed;
    bool original_source_certified;
    bool historical_provenance_recorded;
    bool chronology_validated;

    bool campaign_completed;
    bool finished;
    bool victory;
} ChronoM005State;

typedef struct {
    unsigned turn;
    ChronoM005Action cause;

    bool evidence_before;
    bool evidence_after;

    bool inversion_before;
    bool inversion_after;

    bool changed;
} ChronoM005Trace;

void chrono_m005_init(ChronoM005State *state);

ChronoM005Result chrono_m005_apply(
    ChronoM005State *state,
    ChronoM005Action action
);

ChronoM005Result chrono_m005_end_turn(
    ChronoM005State *state,
    ChronoM005Trace *trace
);

bool chrono_m005_is_victory(const ChronoM005State *state);

#endif
