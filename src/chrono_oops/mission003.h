#ifndef CHRONO_OOPS_MISSION003_H
#define CHRONO_OOPS_MISSION003_H

#include "chrono.h"

/* Mission 003: The Message That Should Never Have Arrived. */

typedef enum {
    CHRONO_M003_WAIT = 0,
    CHRONO_M003_INSPECT_EQUIPMENT,
    CHRONO_M003_IDENTIFY_RISK,
    CHRONO_M003_CONSULT_RECORD,
    CHRONO_M003_TRANSMIT_WARNING,
    CHRONO_M003_RECEIVE_WARNING,
    CHRONO_M003_CANCEL_DEMONSTRATION,
    CHRONO_M003_SECURE_DEMONSTRATION,
    CHRONO_M003_VERIFY_TESTIMONY,
    CHRONO_M003_CHECK_RECORD,
    CHRONO_M003_CERTIFY_DEMONSTRATION,
    CHRONO_M003_ARCHIVE_TESTIMONY,
    CHRONO_M003_VALIDATE_PROVENANCE,
    CHRONO_M003_COMPLETE_MISSION
} ChronoM003Action;

typedef enum {
    CHRONO_M003_OK = 0,
    CHRONO_M003_INVALID_ACTION,
    CHRONO_M003_WRONG_PLAYER,
    CHRONO_M003_NO_ACTION_POINTS,
    CHRONO_M003_PREREQUISITE_MISSING,
    CHRONO_M003_CAUSAL_PARADOX,
    CHRONO_M003_MISSION_FINISHED
} ChronoM003Result;

typedef struct {
    ChronoPlayer active_player;
    unsigned turn;
    unsigned action_points;

    bool equipment_inspected;
    bool risk_identified;

    bool record_consulted;
    bool warning_transmitted;
    bool warning_received;

    bool demonstration_cancelled;
    bool demonstration_secured;
    bool safety_pending;
    bool people_safe;

    bool testimony_available;
    bool testimony_verified;
    bool record_checked;
    bool demonstration_certified;
    bool testimony_archived;
    bool provenance_validated;

    bool mission_completed;
    bool finished;
    bool victory;
} ChronoM003State;

typedef struct {
    unsigned turn;
    ChronoM003Action cause;
    bool safety_before;
    bool safety_after;
    bool testimony_before;
    bool testimony_after;
    bool changed;
} ChronoM003Trace;

void chrono_m003_init(ChronoM003State *state);

ChronoM003Result chrono_m003_apply(
    ChronoM003State *state,
    ChronoM003Action action
);

ChronoM003Result chrono_m003_end_turn(
    ChronoM003State *state,
    ChronoM003Trace *trace
);

bool chrono_m003_is_victory(const ChronoM003State *state);

#endif
