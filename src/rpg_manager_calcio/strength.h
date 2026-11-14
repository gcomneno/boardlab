#ifndef RMC_STRENGTH_H
#define RMC_STRENGTH_H

#include "world.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Initial football strength assessment.
 *
 * This is an evaluation of actual legal lineups,
 * not an arithmetic average of roster attributes.
 *
 * Scoring is deterministic, integer-only and
 * specific to RPG Manager Calcio.
 */

/*
 * Result of optimizing one legal formation.
 *
 * The lineup contains the eleven selected players
 * and their assigned tactical slots.
 *
 * total_score is the aggregate initial effectiveness
 * under the versioned football scoring rules.
 */
typedef struct {
    RmcFormation formation;
    RmcLineup lineup;
    uint64_t total_score;
} RmcFormationStrength;

/*
 * Result of comparing all supported formations.
 *
 * best contains the strongest legal formation.
 * In case of equal scores, formation enumeration
 * order determines the winner.
 */
typedef struct {
    RmcTeamId team_id;
    RmcFormationStrength best;
    uint64_t formation_scores[RMC_FORMATION_COUNT];
} RmcTeamStrength;

/*
 * Evaluate a team in one specified formation.
 *
 * Players are assigned optimally to legal slots,
 * maximizing the total football-specific score
 * through BoardLab weighted bipartite matching.
 *
 * The World Core is never modified.
 * On failure, destination remains unchanged.
 */
bool rmc_formation_strength(
    const RmcWorldCore *world,
    size_t team_index,
    RmcFormation formation,
    RmcFormationStrength *destination
);

/*
 * Evaluate all three formations and select the
 * strongest legal eleven for the specified team.
 *
 * The result is suitable for comparing generated
 * career candidates, not for predicting a guaranteed
 * match outcome.
 *
 * On failure, destination remains unchanged.
 */
bool rmc_team_strength(
    const RmcWorldCore *world,
    size_t team_index,
    RmcTeamStrength *destination
);

#endif
