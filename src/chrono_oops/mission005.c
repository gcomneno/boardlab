#include "mission005.h"

#include <string.h>

void chrono_m005_init(ChronoM005State *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));

    state->active_player = CHRONO_PLAYER_A;
    state->turn = 1;
    state->action_points = CHRONO_ACTION_POINTS;

    /* The events are correct; the recorded clock is not. */
    state->signal_precedes_response = true;
    state->timestamps_show_inversion = true;
    state->clock_loss_seconds = 60;
}

bool chrono_m005_is_victory(const ChronoM005State *state)
{
    return state != NULL &&
           state->campaign_completed &&
           state->signal_precedes_response &&
           state->clock_loss_identified &&
           state->inversion_detected &&
           !state->superficial_correction &&
           state->real_order_observed &&
           state->independent_evidence_preserved &&
           state->independent_evidence_available &&
           !state->evidence_propagation_pending &&
           state->evidence_compared &&
           state->real_order_reconstructed &&
           state->original_source_certified &&
           state->historical_provenance_recorded &&
           state->chronology_validated &&
           state->turn <= CHRONO_MAX_TURNS;
}

static bool action_allowed(ChronoPlayer player, ChronoM005Action action)
{
    if (action == CHRONO_M005_WAIT) {
        return true;
    }

    if (player == CHRONO_PLAYER_A) {
        return action == CHRONO_M005_INSPECT_CLOCK ||
               action == CHRONO_M005_IDENTIFY_CLOCK_LOSS ||
               action == CHRONO_M005_OBSERVE_REAL_ORDER ||
               action == CHRONO_M005_PRESERVE_EVIDENCE ||
               action == CHRONO_M005_CERTIFY_SOURCE ||
               action == CHRONO_M005_RECORD_PROVENANCE;
    }

    return action == CHRONO_M005_CONSULT_ARCHIVE ||
           action == CHRONO_M005_DETECT_INVERSION ||
           action == CHRONO_M005_SUPERFICIAL_CORRECTION ||
           action == CHRONO_M005_RECOGNIZE_INSUFFICIENCY ||
           action == CHRONO_M005_COMPARE_EVIDENCE ||
           action == CHRONO_M005_RECONSTRUCT_ORDER ||
           action == CHRONO_M005_VALIDATE_CHRONOLOGY ||
           action == CHRONO_M005_COMPLETE_CAMPAIGN;
}

static void invalidate_reconstruction(ChronoM005State *state)
{
    state->evidence_compared = false;
    state->real_order_reconstructed = false;
    state->chronology_validated = false;
}

ChronoM005Result chrono_m005_apply(
    ChronoM005State *state,
    ChronoM005Action action
)
{
    if (state == NULL) {
        return CHRONO_M005_INVALID_ACTION;
    }
    if (state->finished) {
        return CHRONO_M005_MISSION_FINISHED;
    }
    if (action < CHRONO_M005_WAIT ||
        action > CHRONO_M005_COMPLETE_CAMPAIGN) {
        return CHRONO_M005_INVALID_ACTION;
    }
    if (!action_allowed(state->active_player, action)) {
        return CHRONO_M005_WRONG_PLAYER;
    }
    if (state->action_points == 0) {
        return CHRONO_M005_NO_ACTION_POINTS;
    }

    /* Reject against a copy: failed actions are atomic. */
    ChronoM005State next = *state;

    switch (action) {
    case CHRONO_M005_WAIT:
        break;

    case CHRONO_M005_INSPECT_CLOCK:
        if (next.clock_inspected)
            return CHRONO_M005_PREREQUISITE_MISSING;
        next.clock_inspected = true;
        break;

    case CHRONO_M005_IDENTIFY_CLOCK_LOSS:
        if (!next.clock_inspected || next.clock_loss_identified)
            return CHRONO_M005_PREREQUISITE_MISSING;
        next.clock_loss_identified = true;
        break;

    case CHRONO_M005_CONSULT_ARCHIVE:
        if (next.archive_consulted)
            return CHRONO_M005_PREREQUISITE_MISSING;
        next.archive_consulted = true;
        break;

    case CHRONO_M005_DETECT_INVERSION:
        if (!next.archive_consulted ||
            !next.timestamps_show_inversion ||
            next.superficial_correction ||
            next.inversion_detected)
            return CHRONO_M005_PREREQUISITE_MISSING;
        next.inversion_detected = true;
        break;

    case CHRONO_M005_SUPERFICIAL_CORRECTION:
        if (!next.archive_consulted ||
            next.superficial_correction ||
            next.original_source_certified ||
            next.chronology_validated)
            return CHRONO_M005_PREREQUISITE_MISSING;

        next.superficial_correction = true;
        next.timestamps_show_inversion = false;
        next.inversion_detected = false;
        next.insufficiency_recognized = false;
        invalidate_reconstruction(&next);
        break;

    case CHRONO_M005_RECOGNIZE_INSUFFICIENCY:
        if (!next.superficial_correction ||
            next.insufficiency_recognized)
            return CHRONO_M005_PREREQUISITE_MISSING;

        next.insufficiency_recognized = true;
        next.superficial_correction = false;
        next.timestamps_show_inversion = true;
        invalidate_reconstruction(&next);
        break;

    case CHRONO_M005_OBSERVE_REAL_ORDER:
        if (!next.clock_loss_identified ||
            next.real_order_observed)
            return CHRONO_M005_PREREQUISITE_MISSING;
        next.real_order_observed = true;
        break;

    case CHRONO_M005_PRESERVE_EVIDENCE:
        if (!next.real_order_observed ||
            next.independent_evidence_preserved)
            return CHRONO_M005_PREREQUISITE_MISSING;

        next.independent_evidence_preserved = true;
        next.evidence_propagation_pending = true;
        break;

    case CHRONO_M005_COMPARE_EVIDENCE:
        if (!next.archive_consulted ||
            !next.inversion_detected ||
            next.superficial_correction ||
            !next.independent_evidence_available ||
            next.evidence_propagation_pending ||
            next.evidence_compared)
            return CHRONO_M005_PREREQUISITE_MISSING;

        next.evidence_compared = true;
        break;

    case CHRONO_M005_RECONSTRUCT_ORDER:
        if (!next.evidence_compared ||
            !next.clock_loss_identified ||
            !next.signal_precedes_response ||
            next.superficial_correction ||
            next.real_order_reconstructed)
            return CHRONO_M005_PREREQUISITE_MISSING;

        next.real_order_reconstructed = true;
        break;

    case CHRONO_M005_CERTIFY_SOURCE:
        if (!next.independent_evidence_preserved ||
            !next.independent_evidence_available ||
            next.evidence_propagation_pending ||
            !next.real_order_reconstructed ||
            next.original_source_certified)
            return CHRONO_M005_PREREQUISITE_MISSING;

        next.original_source_certified = true;
        break;

    case CHRONO_M005_RECORD_PROVENANCE:
        if (!next.original_source_certified ||
            !next.clock_loss_identified ||
            next.historical_provenance_recorded)
            return CHRONO_M005_PREREQUISITE_MISSING;

        next.historical_provenance_recorded = true;
        break;

    case CHRONO_M005_VALIDATE_CHRONOLOGY:
        if (!next.real_order_reconstructed ||
            !next.independent_evidence_available ||
            !next.original_source_certified ||
            !next.historical_provenance_recorded ||
            !next.inversion_detected ||
            next.superficial_correction ||
            next.evidence_propagation_pending ||
            next.chronology_validated)
            return CHRONO_M005_PREREQUISITE_MISSING;

        next.chronology_validated = true;
        break;

    case CHRONO_M005_COMPLETE_CAMPAIGN:
        if (!next.chronology_validated ||
            !next.historical_provenance_recorded ||
            !next.real_order_reconstructed ||
            next.superficial_correction ||
            next.campaign_completed)
            return CHRONO_M005_PREREQUISITE_MISSING;

        next.campaign_completed = true;
        break;
    }

    next.action_points--;
    *state = next;
    return CHRONO_M005_OK;
}

ChronoM005Result chrono_m005_end_turn(
    ChronoM005State *state,
    ChronoM005Trace *trace
)
{
    if (state == NULL) {
        return CHRONO_M005_INVALID_ACTION;
    }
    if (state->finished) {
        return CHRONO_M005_MISSION_FINISHED;
    }

    const bool evidence_before = state->independent_evidence_available;
    const bool inversion_before = state->timestamps_show_inversion;
    ChronoM005Action cause = CHRONO_M005_WAIT;

    if (state->active_player == CHRONO_PLAYER_A &&
        state->evidence_propagation_pending) {
        state->independent_evidence_available =
            state->independent_evidence_preserved;
        state->evidence_propagation_pending = false;

        /* Existing checks must not survive a new historical source. */
        invalidate_reconstruction(state);
        cause = CHRONO_M005_PRESERVE_EVIDENCE;
    }

    if (trace != NULL) {
        trace->turn = state->turn;
        trace->cause = cause;
        trace->evidence_before = evidence_before;
        trace->evidence_after = state->independent_evidence_available;
        trace->inversion_before = inversion_before;
        trace->inversion_after = state->timestamps_show_inversion;
        trace->changed =
            evidence_before != state->independent_evidence_available ||
            inversion_before != state->timestamps_show_inversion;
    }

    if (chrono_m005_is_victory(state)) {
        state->finished = true;
        state->victory = true;
        return CHRONO_M005_OK;
    }

    if (state->turn >= CHRONO_MAX_TURNS) {
        state->finished = true;
        state->victory = false;
        return CHRONO_M005_OK;
    }

    state->turn++;
    state->active_player =
        state->active_player == CHRONO_PLAYER_A
            ? CHRONO_PLAYER_B
            : CHRONO_PLAYER_A;
    state->action_points = CHRONO_ACTION_POINTS;

    return CHRONO_M005_OK;
}
