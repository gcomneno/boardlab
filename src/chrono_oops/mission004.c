#include "mission004.h"

#include <string.h>

void chrono_m004_init(ChronoM004State *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->active_player = CHRONO_PLAYER_A;
    state->turn = 1;
    state->action_points = CHRONO_ACTION_POINTS;
}

bool chrono_m004_is_victory(const ChronoM004State *state)
{
    return state != NULL &&
           state->mission_completed &&
           state->configuration == CHRONO_M004_CONFIG_DUAL &&
           state->propagated_configuration == CHRONO_M004_CONFIG_DUAL &&
           !state->propagation_pending &&
           state->beacon_active &&
           state->register_active &&
           state->beacon_verified &&
           state->register_verified &&
           state->configuration_certified &&
           state->common_origin_recorded &&
           state->coherence_validated &&
           state->turn <= CHRONO_MAX_TURNS;
}

static bool action_allowed(
    ChronoPlayer player,
    ChronoM004Action action
)
{
    if (action == CHRONO_M004_WAIT) {
        return true;
    }

    if (player == CHRONO_PLAYER_A) {
        return action == CHRONO_M004_INSPECT_DISTRIBUTOR ||
               action == CHRONO_M004_IDENTIFY_PULSE ||
               action == CHRONO_M004_SELECT_BEACON ||
               action == CHRONO_M004_SELECT_REGISTER ||
               action == CHRONO_M004_DISCOVER_DUAL_CONTACT ||
               action == CHRONO_M004_ENABLE_DUAL ||
               action == CHRONO_M004_CERTIFY_CONFIGURATION ||
               action == CHRONO_M004_RECORD_COMMON_ORIGIN;
    }

    return action == CHRONO_M004_EXAMINE_BEACON ||
           action == CHRONO_M004_EXAMINE_REGISTER ||
           action == CHRONO_M004_VERIFY_BEACON ||
           action == CHRONO_M004_VERIFY_REGISTER ||
           action == CHRONO_M004_VALIDATE_COHERENCE ||
           action == CHRONO_M004_COMPLETE_MISSION;
}

static void select_configuration(
    ChronoM004State *state,
    ChronoM004Config configuration
)
{
    state->configuration = configuration;
    state->propagation_pending = true;
}

ChronoM004Result chrono_m004_apply(
    ChronoM004State *state,
    ChronoM004Action action
)
{
    if (state == NULL) {
        return CHRONO_M004_INVALID_ACTION;
    }
    if (state->finished) {
        return CHRONO_M004_MISSION_FINISHED;
    }
    if (action < CHRONO_M004_WAIT ||
        action > CHRONO_M004_COMPLETE_MISSION) {
        return CHRONO_M004_INVALID_ACTION;
    }
    if (!action_allowed(state->active_player, action)) {
        return CHRONO_M004_WRONG_PLAYER;
    }
    if (state->action_points == 0) {
        return CHRONO_M004_NO_ACTION_POINTS;
    }

    /* All rejected actions leave state and action points unchanged. */
    ChronoM004State next = *state;

    switch (action) {
    case CHRONO_M004_WAIT:
        break;

    case CHRONO_M004_INSPECT_DISTRIBUTOR:
        if (next.distributor_inspected)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.distributor_inspected = true;
        break;

    case CHRONO_M004_IDENTIFY_PULSE:
        if (!next.distributor_inspected || next.pulse_identified)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.pulse_identified = true;
        break;

    case CHRONO_M004_EXAMINE_BEACON:
        if (next.beacon_examined)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.beacon_examined = true;
        break;

    case CHRONO_M004_EXAMINE_REGISTER:
        if (next.register_examined)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.register_examined = true;
        break;

    case CHRONO_M004_SELECT_BEACON:
        if (!next.pulse_identified ||
            next.configuration == CHRONO_M004_CONFIG_BEACON ||
            next.configuration_certified)
            return CHRONO_M004_PREREQUISITE_MISSING;
        select_configuration(&next, CHRONO_M004_CONFIG_BEACON);
        break;

    case CHRONO_M004_SELECT_REGISTER:
        if (!next.pulse_identified ||
            next.configuration == CHRONO_M004_CONFIG_REGISTER ||
            next.configuration_certified)
            return CHRONO_M004_PREREQUISITE_MISSING;
        select_configuration(&next, CHRONO_M004_CONFIG_REGISTER);
        break;

    case CHRONO_M004_DISCOVER_DUAL_CONTACT:
        if (!next.distributor_inspected ||
            !next.pulse_identified ||
            next.dual_contact_discovered)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.dual_contact_discovered = true;
        break;

    case CHRONO_M004_ENABLE_DUAL:
        if (!next.dual_contact_discovered ||
            !next.pulse_identified ||
            next.configuration == CHRONO_M004_CONFIG_DUAL ||
            next.configuration_certified)
            return CHRONO_M004_PREREQUISITE_MISSING;
        select_configuration(&next, CHRONO_M004_CONFIG_DUAL);
        break;

    case CHRONO_M004_VERIFY_BEACON:
        if (!next.beacon_examined ||
            !next.beacon_active ||
            next.beacon_verified ||
            next.propagation_pending)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.beacon_verified = true;
        break;

    case CHRONO_M004_VERIFY_REGISTER:
        if (!next.register_examined ||
            !next.register_active ||
            next.register_verified ||
            next.propagation_pending)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.register_verified = true;
        break;

    case CHRONO_M004_CERTIFY_CONFIGURATION:
        if (next.configuration != CHRONO_M004_CONFIG_DUAL ||
            next.propagated_configuration != CHRONO_M004_CONFIG_DUAL ||
            next.propagation_pending ||
            !next.beacon_verified ||
            !next.register_verified ||
            next.configuration_certified)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.configuration_certified = true;
        break;

    case CHRONO_M004_RECORD_COMMON_ORIGIN:
        if (!next.configuration_certified ||
            !next.pulse_identified ||
            next.common_origin_recorded)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.common_origin_recorded = true;
        break;

    case CHRONO_M004_VALIDATE_COHERENCE:
        if (!next.beacon_active ||
            !next.register_active ||
            !next.beacon_verified ||
            !next.register_verified ||
            !next.configuration_certified ||
            !next.common_origin_recorded ||
            next.propagation_pending ||
            next.coherence_validated)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.coherence_validated = true;
        break;

    case CHRONO_M004_COMPLETE_MISSION:
        if (!next.coherence_validated ||
            !next.common_origin_recorded ||
            !next.beacon_active ||
            !next.register_active ||
            next.propagation_pending)
            return CHRONO_M004_PREREQUISITE_MISSING;
        next.mission_completed = true;
        break;
    }

    next.action_points--;
    *state = next;
    return CHRONO_M004_OK;
}

ChronoM004Result chrono_m004_end_turn(
    ChronoM004State *state,
    ChronoM004Trace *trace
)
{
    if (state == NULL) {
        return CHRONO_M004_INVALID_ACTION;
    }
    if (state->finished) {
        return CHRONO_M004_MISSION_FINISHED;
    }

    const ChronoM004Config configuration_before =
        state->propagated_configuration;
    const bool beacon_before = state->beacon_active;
    const bool register_before = state->register_active;
    ChronoM004Action cause = CHRONO_M004_WAIT;

    if (state->active_player == CHRONO_PLAYER_A &&
        state->propagation_pending) {
        state->propagated_configuration = state->configuration;

        state->beacon_active =
            state->configuration == CHRONO_M004_CONFIG_BEACON ||
            state->configuration == CHRONO_M004_CONFIG_DUAL;

        state->register_active =
            state->configuration == CHRONO_M004_CONFIG_REGISTER ||
            state->configuration == CHRONO_M004_CONFIG_DUAL;

        /*
         * A new historical configuration invalidates checks
         * and certificates tied to the previous future state.
         */
        state->beacon_verified = false;
        state->register_verified = false;
        state->configuration_certified = false;
        state->common_origin_recorded = false;
        state->coherence_validated = false;

        state->propagation_pending = false;

        switch (state->configuration) {
        case CHRONO_M004_CONFIG_BEACON:
            cause = CHRONO_M004_SELECT_BEACON;
            break;
        case CHRONO_M004_CONFIG_REGISTER:
            cause = CHRONO_M004_SELECT_REGISTER;
            break;
        case CHRONO_M004_CONFIG_DUAL:
            cause = CHRONO_M004_ENABLE_DUAL;
            break;
        case CHRONO_M004_CONFIG_NONE:
            break;
        }
    }

    if (trace != NULL) {
        trace->turn = state->turn;
        trace->cause = cause;
        trace->configuration_before = configuration_before;
        trace->configuration_after = state->propagated_configuration;
        trace->beacon_before = beacon_before;
        trace->beacon_after = state->beacon_active;
        trace->register_before = register_before;
        trace->register_after = state->register_active;
        trace->changed =
            configuration_before != state->propagated_configuration ||
            beacon_before != state->beacon_active ||
            register_before != state->register_active;
    }

    if (chrono_m004_is_victory(state)) {
        state->finished = true;
        state->victory = true;
        return CHRONO_M004_OK;
    }

    if (state->turn >= CHRONO_MAX_TURNS) {
        state->finished = true;
        state->victory = false;
        return CHRONO_M004_OK;
    }

    state->turn++;
    state->active_player =
        state->active_player == CHRONO_PLAYER_A
            ? CHRONO_PLAYER_B
            : CHRONO_PLAYER_A;
    state->action_points = CHRONO_ACTION_POINTS;

    return CHRONO_M004_OK;
}
