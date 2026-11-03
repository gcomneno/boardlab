#ifndef CHRONO_OOPS_MISSION004_H
#define CHRONO_OOPS_MISSION004_H

#include "chrono.h"

/* Mission 004: Two Events, One Cause. */

typedef enum {
    CHRONO_M004_CONFIG_NONE = 0,
    CHRONO_M004_CONFIG_BEACON,
    CHRONO_M004_CONFIG_REGISTER,
    CHRONO_M004_CONFIG_DUAL
} ChronoM004Config;

typedef enum {
    CHRONO_M004_WAIT = 0,
    CHRONO_M004_INSPECT_DISTRIBUTOR,
    CHRONO_M004_IDENTIFY_PULSE,
    CHRONO_M004_EXAMINE_BEACON,
    CHRONO_M004_EXAMINE_REGISTER,
    CHRONO_M004_SELECT_BEACON,
    CHRONO_M004_SELECT_REGISTER,
    CHRONO_M004_DISCOVER_DUAL_CONTACT,
    CHRONO_M004_ENABLE_DUAL,
    CHRONO_M004_VERIFY_BEACON,
    CHRONO_M004_VERIFY_REGISTER,
    CHRONO_M004_CERTIFY_CONFIGURATION,
    CHRONO_M004_RECORD_COMMON_ORIGIN,
    CHRONO_M004_VALIDATE_COHERENCE,
    CHRONO_M004_COMPLETE_MISSION
} ChronoM004Action;

typedef enum {
    CHRONO_M004_OK = 0,
    CHRONO_M004_INVALID_ACTION,
    CHRONO_M004_WRONG_PLAYER,
    CHRONO_M004_NO_ACTION_POINTS,
    CHRONO_M004_PREREQUISITE_MISSING,
    CHRONO_M004_MISSION_FINISHED
} ChronoM004Result;

typedef struct {
    ChronoPlayer active_player;
    unsigned turn;
    unsigned action_points;

    bool distributor_inspected;
    bool pulse_identified;
    bool beacon_examined;
    bool register_examined;
    bool dual_contact_discovered;

    /* A's historical selection and B's propagated observation. */
    ChronoM004Config configuration;
    ChronoM004Config propagated_configuration;
    bool propagation_pending;

    bool beacon_active;
    bool register_active;

    /* Checks are valid only for the current propagated state. */
    bool beacon_verified;
    bool register_verified;
    bool configuration_certified;
    bool common_origin_recorded;
    bool coherence_validated;

    bool mission_completed;
    bool finished;
    bool victory;
} ChronoM004State;

typedef struct {
    unsigned turn;
    ChronoM004Action cause;

    ChronoM004Config configuration_before;
    ChronoM004Config configuration_after;

    bool beacon_before;
    bool beacon_after;
    bool register_before;
    bool register_after;

    bool changed;
} ChronoM004Trace;

void chrono_m004_init(ChronoM004State *state);

ChronoM004Result chrono_m004_apply(
    ChronoM004State *state,
    ChronoM004Action action
);

ChronoM004Result chrono_m004_end_turn(
    ChronoM004State *state,
    ChronoM004Trace *trace
);

bool chrono_m004_is_victory(const ChronoM004State *state);

#endif
