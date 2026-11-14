#ifndef RMC_CANDIDATES_H
#define RMC_CANDIDATES_H

#include "strength.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    RMC_CANDIDATE_COUNT = 3,
    RMC_CANDIDATE_PAIR_COUNT = 3,
    RMC_CANDIDATE_DIMENSIONS = 5
};

/*
 * The five dimensions of candidate diversity:
 *
 * 0. Optimized legal-lineup strength band.
 * 1. Mean roster age band.
 * 2. Mean roster potential band.
 * 3. Initial team cohesion band.
 * 4. Tactical profile.
 *
 * Numerical band boundaries belong to the
 * football-specific selection rules.
 */
typedef enum {
    RMC_CANDIDATE_STRENGTH = 0,
    RMC_CANDIDATE_AGE,
    RMC_CANDIDATE_POTENTIAL,
    RMC_CANDIDATE_COHESION,
    RMC_CANDIDATE_TACTICS
} RmcCandidateDimension;

/*
 * The three selected teams are references into
 * the authoritative World Core, not copies.
 *
 * Team indices are in ascending canonical order.
 *
 * pairwise_differences:
 *   [0] teams 0 and 1
 *   [1] teams 0 and 2
 *   [2] teams 1 and 2
 *
 * fallback_used indicates that no triple satisfied
 * the required minimum pairwise diversity.
 */
typedef struct {
    RmcTeamId team_ids[RMC_CANDIDATE_COUNT];
    size_t team_indices[RMC_CANDIDATE_COUNT];

    uint8_t pairwise_differences[RMC_CANDIDATE_PAIR_COUNT];
    bool fallback_used;
} RmcCandidateSelection;

/*
 * Deterministically select three different teams
 * from the eight teams in a validated World Core.
 *
 * The selection uses the already-qualified initial
 * team-strength evaluation.
 *
 * No World Core mutation or PRNG consumption occurs.
 * On failure, destination remains unchanged.
 */
bool rmc_candidates_select(
    const RmcWorldCore *world,
    RmcCandidateSelection *destination
);

#endif
