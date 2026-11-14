#include "career.h"

#include <stddef.h>

bool rmc_career_foundation_valid(
    const RmcCareerFoundation *foundation)
{
    if (foundation == NULL) {
        return false;
    }

    /*
     * The World Core owns the persistent counters.
     * Calendar validation checks its MatchIds against
     * those same counters.
     */
    if (!rmc_world_core_valid(&foundation->world)) {
        return false;
    }

    return rmc_calendar_valid(
        &foundation->world,
        &foundation->world.ids,
        &foundation->calendar
    );
}

bool rmc_career_foundation_generate(
    const RmcWorldIdentity *identity,
    RmcCareerFoundation *destination)
{
    if (identity == NULL || destination == NULL) {
        return false;
    }

    /*
     * Construct the entire foundation privately.
     * The caller's destination is never used as
     * intermediate working storage.
     */
    RmcCareerFoundation next = {0};

    /*
     * Contractual phase 1:
     * generate and validate the pre-calendar world.
     */
    if (!rmc_world_core_generate(
            identity, &next.world)) {
        return false;
    }

    if (!rmc_world_core_valid(&next.world)) {
        return false;
    }

    /*
     * Contractual phase 2:
     * generate the calendar and allocate MatchIds.
     *
     * IDs are initially copied so allocation failure
     * cannot alter next.world.ids prematurely.
     */
    RmcIdentityState next_ids = next.world.ids;

    if (!rmc_calendar_generate(
            &next.world,
            &next_ids,
            &next.calendar)) {
        return false;
    }

    /*
     * There is exactly one authoritative counter
     * collection in the finished foundation.
     */
    next.world.ids = next_ids;

    /*
     * Contractual phase 3:
     * validate cross-component consistency before
     * publishing the complete result.
     */
    if (!rmc_career_foundation_valid(&next)) {
        return false;
    }

    /*
     * Atomic publication.
     *
     * No partial world, calendar or counter state
     * becomes visible after a failed generation.
     */
    *destination = next;

    return true;
}

/*
 * A career setup is valid only if its candidate selection
 * is precisely the canonical result for its World Core.
 *
 * Compare semantic fields, never structure padding.
 */
bool rmc_career_setup_valid(const RmcCareerSetup *setup)
{
    if (setup == NULL ||
        !rmc_career_foundation_valid(&setup->foundation)) {
        return false;
    }

    RmcCandidateSelection expected = {0};

    if (!rmc_candidates_select(
            &setup->foundation.world, &expected)) {
        return false;
    }

    const RmcCandidateSelection *actual = &setup->candidates;

    if (actual->fallback_used != expected.fallback_used) {
        return false;
    }

    for (size_t i = 0; i < RMC_CANDIDATE_COUNT; ++i) {
        if (actual->team_indices[i] != expected.team_indices[i] ||
            actual->team_ids[i].value !=
                expected.team_ids[i].value) {
            return false;
        }
    }

    for (size_t i = 0; i < RMC_CANDIDATE_PAIR_COUNT; ++i) {
        if (actual->pairwise_differences[i] !=
            expected.pairwise_differences[i]) {
            return false;
        }
    }

    return true;
}

/*
 * Construct the complete initial setup privately.
 *
 * The foundation retains the sole authoritative
 * World Core and persistent identity counters.
 */
bool rmc_career_setup_generate(
    const RmcWorldIdentity *identity,
    RmcCareerSetup *destination)
{
    if (identity == NULL || destination == NULL) {
        return false;
    }

    RmcCareerSetup next = {0};

    if (!rmc_career_foundation_generate(
            identity, &next.foundation)) {
        return false;
    }

    if (!rmc_candidates_select(
            &next.foundation.world, &next.candidates)) {
        return false;
    }

    if (!rmc_career_setup_valid(&next)) {
        return false;
    }

    /*
     * Publish once, after every component has passed.
     * Failure never alters the caller's destination.
     */
    *destination = next;
    return true;
}
