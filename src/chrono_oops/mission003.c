#include "mission003.h"

#include <string.h>

void chrono_m003_init(ChronoM003State *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->active_player = CHRONO_PLAYER_A;
    state->turn = 1;
    state->action_points = CHRONO_ACTION_POINTS;

    /* The historical testimony is the original source record. */
    state->testimony_available = true;
}

bool chrono_m003_is_victory(const ChronoM003State *state)
{
    return state != NULL &&
           state->mission_completed &&
           state->warning_transmitted &&
           state->warning_received &&
           state->demonstration_secured &&
           state->people_safe &&
           state->testimony_available &&
           state->testimony_verified &&
           state->record_checked &&
           state->demonstration_certified &&
           state->testimony_archived &&
           state->provenance_validated &&
           !state->demonstration_cancelled &&
           state->turn <= CHRONO_MAX_TURNS;
}

static bool action_allowed(
    ChronoPlayer player,
    ChronoM003Action action
)
{
    if (action == CHRONO_M003_WAIT) {
        return true;
    }

    if (player == CHRONO_PLAYER_A) {
        return action == CHRONO_M003_INSPECT_EQUIPMENT ||
               action == CHRONO_M003_IDENTIFY_RISK ||
               action == CHRONO_M003_RECEIVE_WARNING ||
               action == CHRONO_M003_CANCEL_DEMONSTRATION ||
               action == CHRONO_M003_SECURE_DEMONSTRATION ||
               action == CHRONO_M003_CERTIFY_DEMONSTRATION ||
               action == CHRONO_M003_ARCHIVE_TESTIMONY;
    }

    return action == CHRONO_M003_CONSULT_RECORD ||
           action == CHRONO_M003_TRANSMIT_WARNING ||
           action == CHRONO_M003_VERIFY_TESTIMONY ||
           action == CHRONO_M003_CHECK_RECORD ||
           action == CHRONO_M003_VALIDATE_PROVENANCE ||
           action == CHRONO_M003_COMPLETE_MISSION;
}

ChronoM003Result chrono_m003_apply(
    ChronoM003State *state,
    ChronoM003Action action
)
{
    if (state == NULL) {
        return CHRONO_M003_INVALID_ACTION;
    }
    if (state->finished) {
        return CHRONO_M003_MISSION_FINISHED;
    }
    if (action < CHRONO_M003_WAIT ||
        action > CHRONO_M003_COMPLETE_MISSION) {
        return CHRONO_M003_INVALID_ACTION;
    }
    if (!action_allowed(state->active_player, action)) {
        return CHRONO_M003_WRONG_PLAYER;
    }
    if (state->action_points == 0) {
        return CHRONO_M003_NO_ACTION_POINTS;
    }

    /* A rejected action must leave the entire state untouched. */
    ChronoM003State next = *state;

    switch (action) {
    case CHRONO_M003_WAIT:
        break;

    case CHRONO_M003_INSPECT_EQUIPMENT:
        if (next.equipment_inspected)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.equipment_inspected = true;
        break;

    case CHRONO_M003_IDENTIFY_RISK:
        if (!next.equipment_inspected || next.risk_identified)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.risk_identified = true;
        break;

    case CHRONO_M003_CONSULT_RECORD:
        if (!next.testimony_available ||
            next.demonstration_cancelled ||
            next.record_consulted)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.record_consulted = true;
        break;

    case CHRONO_M003_TRANSMIT_WARNING:
        if (!next.record_consulted ||
            !next.testimony_available ||
            next.demonstration_cancelled ||
            next.warning_transmitted)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.warning_transmitted = true;
        break;

    case CHRONO_M003_RECEIVE_WARNING:
        if (!next.warning_transmitted ||
            !next.risk_identified ||
            next.warning_received)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.warning_received = true;
        break;

    case CHRONO_M003_CANCEL_DEMONSTRATION:
        if (!next.risk_identified ||
            next.demonstration_cancelled ||
            next.demonstration_secured)
            return CHRONO_M003_PREREQUISITE_MISSING;

        /* Erasing the sole source of an already transmitted
           warning would contradict its provenance. */
        if (next.warning_transmitted)
            return CHRONO_M003_CAUSAL_PARADOX;

        next.demonstration_cancelled = true;
        break;

    case CHRONO_M003_SECURE_DEMONSTRATION:
        if (!next.warning_received ||
            !next.risk_identified ||
            next.demonstration_cancelled ||
            next.demonstration_secured)
            return CHRONO_M003_PREREQUISITE_MISSING;

        next.demonstration_secured = true;
        next.safety_pending = true;
        break;

    case CHRONO_M003_VERIFY_TESTIMONY:
        if (!next.people_safe ||
            !next.testimony_available ||
            !next.warning_transmitted ||
            next.testimony_verified)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.testimony_verified = true;
        break;

    case CHRONO_M003_CHECK_RECORD:
        if (!next.testimony_verified ||
            !next.record_consulted ||
            next.record_checked)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.record_checked = true;
        break;

    case CHRONO_M003_CERTIFY_DEMONSTRATION:
        if (!next.people_safe ||
            !next.demonstration_secured ||
            !next.record_checked ||
            next.demonstration_certified)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.demonstration_certified = true;
        break;

    case CHRONO_M003_ARCHIVE_TESTIMONY:
        if (!next.demonstration_certified ||
            !next.testimony_available ||
            next.testimony_archived)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.testimony_archived = true;
        break;

    case CHRONO_M003_VALIDATE_PROVENANCE:
        if (!next.testimony_archived ||
            !next.testimony_verified ||
            !next.record_checked ||
            !next.people_safe ||
            !next.testimony_available ||
            !next.warning_transmitted ||
            next.provenance_validated)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.provenance_validated = true;
        break;

    case CHRONO_M003_COMPLETE_MISSION:
        if (!next.provenance_validated ||
            !next.testimony_available ||
            !next.people_safe ||
            !next.warning_received)
            return CHRONO_M003_PREREQUISITE_MISSING;
        next.mission_completed = true;
        break;
    }

    next.action_points--;
    *state = next;
    return CHRONO_M003_OK;
}

ChronoM003Result chrono_m003_end_turn(
    ChronoM003State *state,
    ChronoM003Trace *trace
)
{
    if (state == NULL) {
        return CHRONO_M003_INVALID_ACTION;
    }
    if (state->finished) {
        return CHRONO_M003_MISSION_FINISHED;
    }

    const bool safety_before = state->people_safe;
    const bool testimony_before = state->testimony_available;
    ChronoM003Action cause = CHRONO_M003_WAIT;

    if (state->active_player == CHRONO_PLAYER_A) {
        if (state->demonstration_cancelled &&
            state->testimony_available) {
            state->testimony_available = false;
            cause = CHRONO_M003_CANCEL_DEMONSTRATION;
        } else if (state->safety_pending) {
            state->people_safe = true;
            state->safety_pending = false;
            cause = CHRONO_M003_SECURE_DEMONSTRATION;
        }
    }

    if (trace != NULL) {
        trace->turn = state->turn;
        trace->cause = cause;
        trace->safety_before = safety_before;
        trace->safety_after = state->people_safe;
        trace->testimony_before = testimony_before;
        trace->testimony_after = state->testimony_available;
        trace->changed =
            safety_before != state->people_safe ||
            testimony_before != state->testimony_available;
    }

    if (chrono_m003_is_victory(state)) {
        state->finished = true;
        state->victory = true;
        return CHRONO_M003_OK;
    }

    /* Only definitive failure states, never speculative search. */
    if (state->turn >= CHRONO_MAX_TURNS ||
        state->demonstration_cancelled) {
        state->finished = true;
        state->victory = false;
        return CHRONO_M003_OK;
    }

    state->turn++;
    state->active_player =
        state->active_player == CHRONO_PLAYER_A
            ? CHRONO_PLAYER_B
            : CHRONO_PLAYER_A;
    state->action_points = CHRONO_ACTION_POINTS;

    return CHRONO_M003_OK;
}
