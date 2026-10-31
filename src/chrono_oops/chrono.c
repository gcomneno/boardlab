#include "chrono.h"

#include <string.h>

void chrono_init(ChronoState *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));

    state->active_player = CHRONO_PLAYER_A;
    state->turn = 1;
    state->action_points = CHRONO_ACTION_POINTS;
    state->location_a = CHRONO_LOCATION_MUNICIPALITY;
    state->location_b = CHRONO_LOCATION_WEST_BANK;
    state->selected_plan = CHRONO_PLAN_NONE;
}

bool chrono_is_victory(const ChronoState *state)
{
    return state != NULL &&
           state->world.capsule_recovered &&
           state->turn <= CHRONO_MAX_TURNS;
}

ChronoResult chrono_apply(ChronoState *state, ChronoAction action)
{
    if (state == NULL) {
        return CHRONO_INVALID_ACTION;
    }

    if (state->finished) {
        return CHRONO_MISSION_FINISHED;
    }

    if (state->action_points == 0) {
        return CHRONO_NO_ACTION_POINTS;
    }

    /*
     * Work on a copy. Invalid actions must leave the original
     * state unchanged, including its action points.
     */
    ChronoState next = *state;
    ChronoResult result = CHRONO_OK;

    switch (action) {
    case CHRONO_ACTION_WAIT:
        break;

    case CHRONO_ACTION_MOVE_ARCHIVE:
        if (next.active_player != CHRONO_PLAYER_A) {
            return CHRONO_WRONG_PLAYER;
        }
        if (next.location_a != CHRONO_LOCATION_MUNICIPALITY) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.location_a = CHRONO_LOCATION_ARCHIVE;
        break;

    case CHRONO_ACTION_READ_PLANS:
        if (next.active_player != CHRONO_PLAYER_A) {
            return CHRONO_WRONG_PLAYER;
        }
        if (next.location_a != CHRONO_LOCATION_ARCHIVE) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.plans_read = true;
        break;

    case CHRONO_ACTION_INSPECT_RUINS:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (next.location_b != CHRONO_LOCATION_WEST_BANK) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.ruins_inspected = true;
        break;

    case CHRONO_ACTION_IDENTIFY_DEFECT:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.ruins_inspected) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        /*
         * Mission 001 models the free Cronofono transmission
         * as immediate shared knowledge.
         */
        next.defect_known = true;
        break;

    case CHRONO_ACTION_MEASURE_RIVER:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.ruins_inspected) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.river_measured = true;
        break;

    case CHRONO_ACTION_SELECT_BRIDGE:
        if (next.active_player != CHRONO_PLAYER_A) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.plans_read || !next.defect_known ||
            next.plan_approved) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.selected_plan = CHRONO_PLAN_BRIDGE;
        next.plan_selected = true;
        break;

    case CHRONO_ACTION_APPROVE_BRIDGE:
        if (next.active_player != CHRONO_PLAYER_A) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.plan_selected ||
            next.selected_plan != CHRONO_PLAN_BRIDGE ||
            next.plan_approved) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.plan_approved = true;
        break;

    case CHRONO_ACTION_SELECT_DUCK:
        if (next.active_player != CHRONO_PLAYER_A) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.plans_read || !next.river_measured ||
            next.plan_approved) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.selected_plan = CHRONO_PLAN_DUCK;
        next.plan_selected = true;
        break;

    case CHRONO_ACTION_APPROVE_DUCK:
        if (next.active_player != CHRONO_PLAYER_A) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.plan_selected ||
            next.selected_plan != CHRONO_PLAN_DUCK ||
            next.plan_approved) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.plan_approved = true;
        break;

    case CHRONO_ACTION_CROSS_BRIDGE:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.world.bridge_exists ||
            next.location_b != CHRONO_LOCATION_WEST_BANK) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.location_b = CHRONO_LOCATION_BRIDGE;
        next.bridge_crossed = true;
        break;

    case CHRONO_ACTION_REACH_TOWER:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.bridge_crossed ||
            next.location_b != CHRONO_LOCATION_BRIDGE) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.location_b = CHRONO_LOCATION_TOWER;
        next.tower_reached = true;
        break;

    case CHRONO_ACTION_VERIFY_PLAN:
        if (next.active_player != CHRONO_PLAYER_A) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.plan_approved ||
            next.selected_plan != CHRONO_PLAN_BRIDGE) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.project_verified = true;
        break;

    case CHRONO_ACTION_INSPECT_DUCK:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.world.duck_exists ||
            next.location_b != CHRONO_LOCATION_WEST_BANK) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.duck_inspected = true;
        break;

    case CHRONO_ACTION_PREPARE_DUCK:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.duck_inspected) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.world.duck_prepared = true;
        break;

    case CHRONO_ACTION_READ_MANUAL:
        if (next.active_player != CHRONO_PLAYER_A) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.plan_approved ||
            next.selected_plan != CHRONO_PLAN_DUCK ||
            next.location_a != CHRONO_LOCATION_ARCHIVE) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.manual_read = true;
        break;

    case CHRONO_ACTION_CALIBRATE_DUCK:
        if (next.active_player != CHRONO_PLAYER_A) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.manual_read || !next.river_measured) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        /*
         * Record the historical decision. The future machine
         * remains unchanged until end-of-turn propagation.
         */
        next.calibration_pending = true;
        break;

    case CHRONO_ACTION_START_DUCK:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.world.duck_exists ||
            !next.world.duck_prepared ||
            !next.world.duck_calibrated ||
            next.location_b != CHRONO_LOCATION_WEST_BANK) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.world.duck_running = true;
        break;

    case CHRONO_ACTION_CROSS_RIVER:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.world.duck_running ||
            next.location_b != CHRONO_LOCATION_WEST_BANK) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.location_b = CHRONO_LOCATION_EAST_BANK;
        next.river_crossed = true;
        next.world.tide_register_acquired = true;
        break;

    case CHRONO_ACTION_VERIFY_INTERVENTION:
        if (next.active_player != CHRONO_PLAYER_A) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.plan_approved ||
            next.selected_plan != CHRONO_PLAN_DUCK ||
            !next.world.duck_calibrated) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.intervention_verified = true;
        break;

    case CHRONO_ACTION_ENTER_TOWER:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (next.location_b != CHRONO_LOCATION_TOWER &&
            next.location_b != CHRONO_LOCATION_EAST_BANK) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.location_b = CHRONO_LOCATION_TOWER;
        next.tower_entered = true;
        break;

    case CHRONO_ACTION_RECOVER_CAPSULE:
        if (next.active_player != CHRONO_PLAYER_B) {
            return CHRONO_WRONG_PLAYER;
        }
        if (!next.tower_entered ||
            next.location_b != CHRONO_LOCATION_TOWER) {
            return CHRONO_PREREQUISITE_MISSING;
        }
        next.world.capsule_recovered = true;
        break;

    default:
        result = CHRONO_INVALID_ACTION;
        break;
    }

    if (result != CHRONO_OK) {
        return result;
    }

    next.action_points--;
    *state = next;
    return CHRONO_OK;
}

/*
 * Compare semantic world state, not raw struct storage:
 * C permits padding bytes with unspecified representations.
 */
static bool chrono_world_changed(
    const ChronoWorld *before,
    const ChronoWorld *after
)
{
    return before->bridge_exists != after->bridge_exists ||
           before->duck_exists != after->duck_exists ||
           before->duck_calibrated != after->duck_calibrated ||
           before->duck_prepared != after->duck_prepared ||
           before->duck_running != after->duck_running ||
           before->capsule_recovered != after->capsule_recovered ||
           before->tide_register_acquired !=
               after->tide_register_acquired;
}

ChronoResult chrono_end_turn(
    ChronoState *state,
    ChronoTrace *trace
)
{
    if (state == NULL) {
        return CHRONO_INVALID_ACTION;
    }

    if (state->finished) {
        return CHRONO_MISSION_FINISHED;
    }

    ChronoWorld before = state->world;
    ChronoAction cause = CHRONO_ACTION_WAIT;

    /*
     * Historical approval reconstructs the future connection.
     * Selection without approval has no causal effect.
     */
    if (state->plan_approved) {
        bool bridge = state->selected_plan == CHRONO_PLAN_BRIDGE;
        bool duck = state->selected_plan == CHRONO_PLAN_DUCK;

        if (before.bridge_exists != bridge ||
            before.duck_exists != duck) {
            cause = bridge
                ? CHRONO_ACTION_APPROVE_BRIDGE
                : CHRONO_ACTION_APPROVE_DUCK;
        }

        state->world.bridge_exists = bridge;
        state->world.duck_exists = duck;
    }

    /*
     * Calibration changes the machine's historical design,
     * but preserves preparation performed in the future.
     */
    if (state->calibration_pending &&
        state->plan_approved &&
        state->selected_plan == CHRONO_PLAN_DUCK &&
        state->world.duck_exists) {
        if (!state->world.duck_calibrated) {
            cause = CHRONO_ACTION_CALIBRATE_DUCK;
        }
        state->world.duck_calibrated = true;
        state->calibration_pending = false;
    }

    if (trace != NULL) {
        trace->before = before;
        trace->after = state->world;
        trace->cause = cause;
        trace->turn = state->turn;
        trace->changed =
            chrono_world_changed(&before, &state->world);
    }

    if (chrono_is_victory(state)) {
        state->victory = true;
        state->finished = true;
        return CHRONO_OK;
    }

    if (state->turn >= CHRONO_MAX_TURNS) {
        state->finished = true;
        state->victory = false;
        return CHRONO_OK;
    }

    state->turn++;
    state->active_player =
        state->active_player == CHRONO_PLAYER_A
            ? CHRONO_PLAYER_B
            : CHRONO_PLAYER_A;
    state->action_points = CHRONO_ACTION_POINTS;

    return CHRONO_OK;
}
