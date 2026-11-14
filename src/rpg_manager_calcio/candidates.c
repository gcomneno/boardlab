#include "candidates.h"

#include <stddef.h>
#include <stdint.h>

enum {
    RMC_MIN_PAIRWISE_DIVERSITY = 2
};

typedef struct {
    uint8_t bands[RMC_CANDIDATE_DIMENSIONS];
} RmcCandidateProfile;

static uint8_t classify(
    uint64_t value,
    uint64_t medium_start,
    uint64_t high_start)
{
    if (value < medium_start) {
        return 0;
    }

    if (value < high_start) {
        return 1;
    }

    return 2;
}

static bool build_profile(
    const RmcWorldCore *world,
    size_t team_index,
    RmcCandidateProfile *destination)
{
    RmcTeamStrength strength;

    if (!rmc_team_strength(world, team_index, &strength)) {
        return false;
    }

    uint64_t age_sum = 0;
    uint64_t potential_sum = 0;

    const RmcRoster *roster = &world->rosters[team_index];

    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        age_sum += roster->players[i].age;
        potential_sum += roster->players[i].potential;
    }

    RmcCandidateProfile profile = {{0}};

    profile.bands[RMC_CANDIDATE_STRENGTH] =
        classify(strength.best.total_score, 660u, 695u);

    profile.bands[RMC_CANDIDATE_AGE] =
        classify(
            100u * age_sum,
            2450u * RMC_TEAM_PLAYERS,
            2750u * RMC_TEAM_PLAYERS
        );

    profile.bands[RMC_CANDIDATE_POTENTIAL] =
        classify(
            100u * potential_sum,
            7200u * RMC_TEAM_PLAYERS,
            7900u * RMC_TEAM_PLAYERS
        );

    profile.bands[RMC_CANDIDATE_COHESION] =
        classify(world->teams[team_index].cohesion, 54u, 67u);

    profile.bands[RMC_CANDIDATE_TACTICS] =
        (uint8_t)world->teams[team_index].tactical_profile;

    *destination = profile;
    return true;
}

static uint8_t pairwise_differences(
    const RmcCandidateProfile *first,
    const RmcCandidateProfile *second)
{
    uint8_t count = 0;

    for (size_t i = 0; i < RMC_CANDIDATE_DIMENSIONS; ++i) {
        if (first->bands[i] != second->bands[i]) {
            ++count;
        }
    }

    return count;
}

bool rmc_candidates_select(
    const RmcWorldCore *world,
    RmcCandidateSelection *destination)
{
    if (world == NULL || destination == NULL) {
        return false;
    }

    if (!rmc_world_core_valid(world)) {
        return false;
    }

    RmcCandidateProfile profiles[RMC_WORLD_TEAMS];

    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        if (!build_profile(world, i, &profiles[i])) {
            return false;
        }
    }

    RmcCandidateSelection selected = {0};

    bool found = false;
    bool selected_conformant = false;
    unsigned selected_minimum = 0;
    unsigned selected_total = 0;

    /*
     * Ascending enumeration supplies the canonical
     * lexicographical tie-break without further sorting.
     */
    for (size_t a = 0; a < RMC_WORLD_TEAMS; ++a) {
        for (size_t b = a + 1; b < RMC_WORLD_TEAMS; ++b) {
            for (size_t c = b + 1; c < RMC_WORLD_TEAMS; ++c) {
                const uint8_t ab =
                    pairwise_differences(&profiles[a], &profiles[b]);

                const uint8_t ac =
                    pairwise_differences(&profiles[a], &profiles[c]);

                const uint8_t bc =
                    pairwise_differences(&profiles[b], &profiles[c]);

                unsigned minimum = ab;

                if (ac < minimum) {
                    minimum = ac;
                }

                if (bc < minimum) {
                    minimum = bc;
                }

                const unsigned total =
                    (unsigned)ab + (unsigned)ac + (unsigned)bc;

                const bool conformant =
                    minimum >= RMC_MIN_PAIRWISE_DIVERSITY;

                bool better = !found;

                if (found && conformant != selected_conformant) {
                    better = conformant;
                } else if (found && minimum > selected_minimum) {
                    better = true;
                } else if (found &&
                           minimum == selected_minimum &&
                           total > selected_total) {
                    better = true;
                }

                if (!better) {
                    continue;
                }

                const size_t indices[RMC_CANDIDATE_COUNT] = {
                    a, b, c
                };

                for (size_t i = 0; i < RMC_CANDIDATE_COUNT; ++i) {
                    selected.team_indices[i] = indices[i];
                    selected.team_ids[i] =
                        world->teams[indices[i]].id;
                }

                selected.pairwise_differences[0] = ab;
                selected.pairwise_differences[1] = ac;
                selected.pairwise_differences[2] = bc;

                selected.fallback_used = !conformant;

                selected_conformant = conformant;
                selected_minimum = minimum;
                selected_total = total;
                found = true;
            }
        }
    }

    if (!found) {
        return false;
    }

    *destination = selected;
    return true;
}
