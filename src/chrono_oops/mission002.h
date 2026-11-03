#ifndef CHRONO_OOPS_MISSION002_H
#define CHRONO_OOPS_MISSION002_H

#include "chrono.h"

/* Mission 002: Borrowing from Tomorrow. */

typedef enum {
    CHRONO_M002_WAIT = 0,
    CHRONO_M002_CONSULT_ARCHIVE,
    CHRONO_M002_IDENTIFY_DESIGN,
    CHRONO_M002_INSPECT_CRONOPHONE,
    CHRONO_M002_BORROW_MODULE,
    CHRONO_M002_RECORD_ORIGIN,
    CHRONO_M002_APPROVE_PRODUCTION,
    CHRONO_M002_DESTROY_DESIGN,
    CHRONO_M002_INSTALL_MODULE,
    CHRONO_M002_REACTIVATE_CRONOPHONE,
    CHRONO_M002_VERIFY_DOCUMENTATION,
    CHRONO_M002_CONFIRM_STABILITY,
    CHRONO_M002_COMPLETE_MISSION
} ChronoM002Action;

typedef enum {
    CHRONO_M002_OK = 0,
    CHRONO_M002_INVALID_ACTION,
    CHRONO_M002_WRONG_PLAYER,
    CHRONO_M002_NO_ACTION_POINTS,
    CHRONO_M002_PREREQUISITE_MISSING,
    CHRONO_M002_CAUSAL_PARADOX,
    CHRONO_M002_MISSION_FINISHED
} ChronoM002Result;

typedef struct {
    ChronoPlayer active_player;
    unsigned turn;
    unsigned action_points;

    bool archive_consulted;
    bool design_identified;
    bool design_destroyed;
    bool cronophone_inspected;

    bool module_borrowed;
    bool debt_open;
    bool origin_recorded;
    bool production_approved;
    bool production_pending;
    bool historical_origin_exists;

    bool module_installed;
    bool cronophone_active;
    bool documentation_verified;
    bool stability_confirmed;
    bool mission_completed;

    bool finished;
    bool victory;
} ChronoM002State;

typedef struct {
    unsigned turn;
    ChronoM002Action cause;
    bool debt_before;
    bool debt_after;
    bool origin_before;
    bool origin_after;
    bool changed;
} ChronoM002Trace;

void chrono_m002_init(ChronoM002State *state);

ChronoM002Result chrono_m002_apply(
    ChronoM002State *state,
    ChronoM002Action action
);

ChronoM002Result chrono_m002_end_turn(
    ChronoM002State *state,
    ChronoM002Trace *trace
);

bool chrono_m002_is_victory(const ChronoM002State *state);

#endif
