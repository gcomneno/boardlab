#ifndef CHRONO_OOPS_CHRONO_H
#define CHRONO_OOPS_CHRONO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    CHRONO_MAX_TURNS = 8,
    CHRONO_ACTION_POINTS = 2
};

typedef enum {
    CHRONO_PLAYER_A = 0,
    CHRONO_PLAYER_B
} ChronoPlayer;

typedef enum {
    CHRONO_LOCATION_MUNICIPALITY = 0,
    CHRONO_LOCATION_ARCHIVE,
    CHRONO_LOCATION_WEST_BANK,
    CHRONO_LOCATION_BRIDGE,
    CHRONO_LOCATION_DUCK,
    CHRONO_LOCATION_EAST_BANK,
    CHRONO_LOCATION_TOWER
} ChronoLocation;

typedef enum {
    CHRONO_PLAN_NONE = 0,
    CHRONO_PLAN_BRIDGE,
    CHRONO_PLAN_DUCK
} ChronoPlan;

typedef enum {
    CHRONO_ACTION_WAIT = 0,
    CHRONO_ACTION_MOVE_ARCHIVE,
    CHRONO_ACTION_READ_PLANS,
    CHRONO_ACTION_INSPECT_RUINS,
    CHRONO_ACTION_IDENTIFY_DEFECT,
    CHRONO_ACTION_MEASURE_RIVER,
    CHRONO_ACTION_SELECT_BRIDGE,
    CHRONO_ACTION_APPROVE_BRIDGE,
    CHRONO_ACTION_SELECT_DUCK,
    CHRONO_ACTION_APPROVE_DUCK,
    CHRONO_ACTION_CROSS_BRIDGE,
    CHRONO_ACTION_REACH_TOWER,
    CHRONO_ACTION_VERIFY_PLAN,
    CHRONO_ACTION_INSPECT_DUCK,
    CHRONO_ACTION_PREPARE_DUCK,
    CHRONO_ACTION_READ_MANUAL,
    CHRONO_ACTION_CALIBRATE_DUCK,
    CHRONO_ACTION_START_DUCK,
    CHRONO_ACTION_CROSS_RIVER,
    CHRONO_ACTION_VERIFY_INTERVENTION,
    CHRONO_ACTION_ENTER_TOWER,
    CHRONO_ACTION_RECOVER_CAPSULE
} ChronoAction;

typedef enum {
    CHRONO_OK = 0,
    CHRONO_INVALID_ACTION,
    CHRONO_WRONG_PLAYER,
    CHRONO_NO_ACTION_POINTS,
    CHRONO_PREREQUISITE_MISSING,
    CHRONO_MISSION_FINISHED
} ChronoResult;

typedef struct {
    bool bridge_exists;
    bool duck_exists;
    bool duck_calibrated;
    bool duck_prepared;
    bool duck_running;
    bool capsule_recovered;
    bool tide_register_acquired;
} ChronoWorld;

typedef struct {
    ChronoPlayer active_player;
    unsigned turn;
    unsigned action_points;
    ChronoLocation location_a;
    ChronoLocation location_b;
    ChronoPlan selected_plan;
    bool plans_read;
    bool plan_selected;
    bool plan_approved;
    bool ruins_inspected;
    bool duck_inspected;
    bool defect_known;
    bool river_measured;
    bool manual_read;
    bool calibration_pending;
    bool tower_reached;
    bool tower_entered;
    bool bridge_crossed;
    bool river_crossed;
    bool project_verified;
    bool intervention_verified;
    bool finished;
    bool victory;
    ChronoWorld world;
} ChronoState;

typedef struct {
    ChronoWorld before;
    ChronoWorld after;
    ChronoAction cause;
    unsigned turn;
    bool changed;
} ChronoTrace;

/* Initialize a new deterministic Mission 001 state. */
void chrono_init(ChronoState *state);

/* Apply one action without advancing the turn. */
ChronoResult chrono_apply(
    ChronoState *state,
    ChronoAction action
);

/* Propagate causal changes and advance to the next player. */
ChronoResult chrono_end_turn(
    ChronoState *state,
    ChronoTrace *trace
);

/* Return true when the mandatory mission objective is fulfilled. */
bool chrono_is_victory(const ChronoState *state);

#endif
