#ifndef RMC_CAREER_H
#define RMC_CAREER_H

#include "calendar.h"
#include "candidates.h"

#include <stdbool.h>

/*
 * Validated foundation of an M001 career.
 *
 * Contains the generated world and full calendar,
 * before the three candidate teams are selected.
 *
 * This is not yet a complete playable career.
 *
 * Persistent identity counters have one authority:
 * world.ids, including all allocated MatchIds.
 */
typedef struct {
    RmcWorldCore world;
    RmcCalendar calendar;
} RmcCareerFoundation;

/*
 * Generate the career foundation in contractual order:
 *
 * 1. Generate the World Core.
 * 2. Validate the World Core.
 * 3. Generate the double round-robin calendar.
 * 4. Commit the updated MatchId counter to world.ids.
 * 5. Validate the complete foundation.
 *
 * No candidate selection or season simulation occurs.
 *
 * On failure, destination remains unchanged.
 */
bool rmc_career_foundation_generate(
    const RmcWorldIdentity *identity,
    RmcCareerFoundation *destination
);

/*
 * Validate the World Core, calendar, persistent
 * identities and their cross-component consistency.
 *
 * Does not claim candidate diversity or season validity.
 */
bool rmc_career_foundation_valid(
    const RmcCareerFoundation *foundation
);


/*
 * Fully generated initial career setup.
 *
 * The foundation remains authoritative for the
 * World Core, persistent IDs and calendar.
 *
 * Candidates contain references to three existing
 * teams, not copies of their persistent state.
 *
 * No human team has been chosen yet.
 * This is not a complete playable career.
 */
typedef struct {
    RmcCareerFoundation foundation;
    RmcCandidateSelection candidates;
} RmcCareerSetup;

/*
 * Generate world, calendar and candidate selection
 * in contractual order.
 *
 * All intermediate results are private.
 * On failure, destination remains unchanged.
 */
bool rmc_career_setup_generate(
    const RmcWorldIdentity *identity,
    RmcCareerSetup *destination
);

/*
 * Validate the foundation and verify that candidate
 * indices, TeamIds, pairwise diversity and fallback
 * exactly match the canonical deterministic selection.
 *
 * Does not validate season simulation or player choice.
 */
bool rmc_career_setup_valid(
    const RmcCareerSetup *setup
);

#endif
