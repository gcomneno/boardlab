#include "mission002.h"

#include <string.h>

void chrono_m002_init(ChronoM002State *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->active_player = CHRONO_PLAYER_A;
    state->turn = 1;
    state->action_points = CHRONO_ACTION_POINTS;
}

bool chrono_m002_is_victory(const ChronoM002State *state)
{
    return state != NULL &&
           state->mission_completed &&
           state->cronophone_active &&
           state->historical_origin_exists &&
           !state->debt_open &&
           state->turn <= CHRONO_MAX_TURNS;
}

static bool action_allowed(
    ChronoPlayer player,
    ChronoM002Action action
)
{
    if (action == CHRONO_M002_WAIT) {
        return true;
    }

    if (player == CHRONO_PLAYER_A) {
        return action == CHRONO_M002_CONSULT_ARCHIVE ||
               action == CHRONO_M002_IDENTIFY_DESIGN ||
               action == CHRONO_M002_RECORD_ORIGIN ||
               action == CHRONO_M002_APPROVE_PRODUCTION ||
               action == CHRONO_M002_DESTROY_DESIGN ||
               action == CHRONO_M002_VERIFY_DOCUMENTATION;
    }

    return action == CHRONO_M002_INSPECT_CRONOPHONE ||
           action == CHRONO_M002_BORROW_MODULE ||
           action == CHRONO_M002_INSTALL_MODULE ||
           action == CHRONO_M002_REACTIVATE_CRONOPHONE ||
           action == CHRONO_M002_CONFIRM_STABILITY ||
           action == CHRONO_M002_COMPLETE_MISSION;
}

ChronoM002Result chrono_m002_apply(
    ChronoM002State *state,
    ChronoM002Action action
)
{
    if (state == NULL) {
        return CHRONO_M002_INVALID_ACTION;
    }
    if (state->finished) {
        return CHRONO_M002_MISSION_FINISHED;
    }
    if (action < CHRONO_M002_WAIT ||
        action > CHRONO_M002_COMPLETE_MISSION) {
        return CHRONO_M002_INVALID_ACTION;
    }
    if (!action_allowed(state->active_player, action)) {
        return CHRONO_M002_WRONG_PLAYER;
    }
    if (state->action_points == 0) {
        return CHRONO_M002_NO_ACTION_POINTS;
    }

    /* Work on a copy: rejected actions are entirely atomic. */
    ChronoM002State next = *state;

    switch (action) {
    case CHRONO_M002_WAIT:
        break;

    case CHRONO_M002_CONSULT_ARCHIVE:
        if (next.archive_consulted)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.archive_consulted = true;
        break;

    case CHRONO_M002_IDENTIFY_DESIGN:
        if (!next.archive_consulted || next.design_identified ||
            next.design_destroyed)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.design_identified = true;
        break;

    case CHRONO_M002_INSPECT_CRONOPHONE:
        if (next.cronophone_inspected)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.cronophone_inspected = true;
        break;

    case CHRONO_M002_BORROW_MODULE:
        if (!next.cronophone_inspected ||
            !next.design_identified ||
            next.design_destroyed ||
            next.module_borrowed)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.module_borrowed = true;
        next.debt_open = !next.historical_origin_exists;
        break;

    case CHRONO_M002_RECORD_ORIGIN:
        if (!next.design_identified || next.design_destroyed ||
            next.origin_recorded)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.origin_recorded = true;
        break;

    case CHRONO_M002_APPROVE_PRODUCTION:
        if (!next.origin_recorded || next.design_destroyed ||
            next.production_approved)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.production_approved = true;
        next.production_pending = true;
        break;

    case CHRONO_M002_DESTROY_DESIGN:
        if (!next.design_identified || next.design_destroyed)
            return CHRONO_M002_PREREQUISITE_MISSING;
        if (next.debt_open)
            return CHRONO_M002_CAUSAL_PARADOX;
        if (next.production_approved)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.design_destroyed = true;
        break;

    case CHRONO_M002_INSTALL_MODULE:
        if (!next.module_borrowed || next.module_installed)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.module_installed = true;
        break;

    case CHRONO_M002_REACTIVATE_CRONOPHONE:
        if (!next.module_installed || next.cronophone_active)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.cronophone_active = true;
        break;

    case CHRONO_M002_VERIFY_DOCUMENTATION:
        if (!next.historical_origin_exists ||
            next.documentation_verified)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.documentation_verified = true;
        break;

    case CHRONO_M002_CONFIRM_STABILITY:
        if (!next.cronophone_active ||
            !next.documentation_verified ||
            next.debt_open ||
            next.stability_confirmed)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.stability_confirmed = true;
        break;

    case CHRONO_M002_COMPLETE_MISSION:
        if (!next.stability_confirmed ||
            !next.cronophone_active ||
            next.debt_open ||
            !next.historical_origin_exists)
            return CHRONO_M002_PREREQUISITE_MISSING;
        next.mission_completed = true;
        break;
    }

    next.action_points--;
    *state = next;
    return CHRONO_M002_OK;
}

ChronoM002Result chrono_m002_end_turn(
    ChronoM002State *state,
    ChronoM002Trace *trace
)
{
    if (state == NULL) {
        return CHRONO_M002_INVALID_ACTION;
    }
    if (state->finished) {
        return CHRONO_M002_MISSION_FINISHED;
    }

    const bool debt_before = state->debt_open;
    const bool origin_before = state->historical_origin_exists;
    ChronoM002Action cause = CHRONO_M002_WAIT;

    if (state->active_player == CHRONO_PLAYER_A &&
        state->production_pending) {
        state->historical_origin_exists = true;
        state->debt_open = false;
        state->production_pending = false;
        cause = CHRONO_M002_APPROVE_PRODUCTION;
    }

    if (trace != NULL) {
        trace->turn = state->turn;
        trace->cause = cause;
        trace->debt_before = debt_before;
        trace->debt_after = state->debt_open;
        trace->origin_before = origin_before;
        trace->origin_after = state->historical_origin_exists;
        trace->changed = debt_before != state->debt_open ||
                         origin_before != state->historical_origin_exists;
    }

    if (chrono_m002_is_victory(state)) {
        state->finished = true;
        state->victory = true;
        return CHRONO_M002_OK;
    }

    /* Only irreversible deadlocks; no speculative search engine. */
    if (state->turn >= CHRONO_MAX_TURNS ||
        (state->design_destroyed &&
         !state->historical_origin_exists)) {
        state->finished = true;
        state->victory = false;
        return CHRONO_M002_OK;
    }

    state->turn++;
    state->active_player =
        state->active_player == CHRONO_PLAYER_A
            ? CHRONO_PLAYER_B
            : CHRONO_PLAYER_A;
    state->action_points = CHRONO_ACTION_POINTS;

    return CHRONO_M002_OK;
}
