#include "candidates.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    unsigned category[5];
} ReferenceProfile;

static unsigned reference_band(
    uint64_t value, uint64_t first, uint64_t second)
{
    if (value >= second) return 2;
    if (value >= first) return 1;
    return 0;
}

static void reference_profile(
    const RmcWorldCore *world,
    size_t team,
    ReferenceProfile *out)
{
    RmcTeamStrength strength;
    uint64_t age_total = 0;
    uint64_t potential_total = 0;

    assert(rmc_team_strength(world, team, &strength));

    for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
        age_total += world->rosters[team].players[p].age;
        potential_total += world->rosters[team].players[p].potential;
    }

    out->category[0] =
        reference_band(strength.best.total_score, 660, 695);
    out->category[1] =
        reference_band(age_total * 100,
                       2450 * RMC_TEAM_PLAYERS,
                       2750 * RMC_TEAM_PLAYERS);
    out->category[2] =
        reference_band(potential_total * 100,
                       7200 * RMC_TEAM_PLAYERS,
                       7900 * RMC_TEAM_PLAYERS);
    out->category[3] =
        reference_band(world->teams[team].cohesion, 54, 67);
    out->category[4] =
        (unsigned)world->teams[team].tactical_profile;
}

static unsigned distance(
    const ReferenceProfile *a,
    const ReferenceProfile *b)
{
    unsigned difference = 0;
    for (size_t d = 0; d < 5; ++d) {
        difference += a->category[d] != b->category[d];
    }
    return difference;
}

/*
 * Independent exhaustive reference:
 * determine the global best objective, then take
 * the first lexicographic triple achieving it.
 */
static void verify_optimum(
    const RmcWorldCore *world,
    const RmcCandidateSelection *actual)
{
    ReferenceProfile profile[RMC_WORLD_TEAMS];
    unsigned best_conformity = 0;
    unsigned best_minimum = 0;
    unsigned best_total = 0;
    size_t expected[3] = {0};
    unsigned expected_pairs[3] = {0};
    bool initialized = false;

    for (size_t t = 0; t < RMC_WORLD_TEAMS; ++t) {
        reference_profile(world, t, &profile[t]);
    }

    for (size_t a = 0; a < RMC_WORLD_TEAMS; ++a) {
        for (size_t b = a + 1; b < RMC_WORLD_TEAMS; ++b) {
            for (size_t c = b + 1; c < RMC_WORLD_TEAMS; ++c) {
                const unsigned ab = distance(&profile[a], &profile[b]);
                const unsigned ac = distance(&profile[a], &profile[c]);
                const unsigned bc = distance(&profile[b], &profile[c]);

                unsigned minimum = ab;
                if (ac < minimum) minimum = ac;
                if (bc < minimum) minimum = bc;

                const unsigned conformity = minimum >= 2;
                const unsigned total = ab + ac + bc;

                const bool better =
                    !initialized ||
                    conformity > best_conformity ||
                    (conformity == best_conformity &&
                     minimum > best_minimum) ||
                    (conformity == best_conformity &&
                     minimum == best_minimum &&
                     total > best_total);

                if (!better) continue;

                initialized = true;
                best_conformity = conformity;
                best_minimum = minimum;
                best_total = total;

                expected[0] = a;
                expected[1] = b;
                expected[2] = c;
                expected_pairs[0] = ab;
                expected_pairs[1] = ac;
                expected_pairs[2] = bc;
            }
        }
    }

    assert(initialized);
    assert(actual->fallback_used == (best_conformity == 0));

    for (size_t i = 0; i < 3; ++i) {
        assert(actual->team_indices[i] == expected[i]);
        assert(actual->team_ids[i].value ==
               world->teams[expected[i]].id.value);
        assert(actual->pairwise_differences[i] ==
               expected_pairs[i]);
    }

    assert(actual->team_indices[0] < actual->team_indices[1]);
    assert(actual->team_indices[1] < actual->team_indices[2]);
}

static void test_seed_campaign(void)
{
    unsigned conformant = 0;
    unsigned fallback = 0;

    for (uint64_t seed = 0; seed < 100; ++seed) {
        const RmcWorldIdentity identity = {
            seed * UINT64_C(0x9e3779b97f4a7c15),
            RMC_GENERATOR_VERSION,
            RMC_RULES_VERSION
        };

        RmcWorldCore world;
        RmcCandidateSelection first;
        RmcCandidateSelection second;

        assert(rmc_world_core_generate(&identity, &world));
        assert(rmc_candidates_select(&world, &first));
        assert(rmc_candidates_select(&world, &second));

        verify_optimum(&world, &first);

        for (size_t i = 0; i < RMC_CANDIDATE_COUNT; ++i) {
            assert(first.team_indices[i] == second.team_indices[i]);
            assert(first.team_ids[i].value ==
                   second.team_ids[i].value);
            assert(first.pairwise_differences[i] ==
                   second.pairwise_differences[i]);
        }

        assert(first.fallback_used == second.fallback_used);

        if (first.fallback_used) {
            ++fallback;
        } else {
            ++conformant;
            for (size_t i = 0; i < 3; ++i) {
                assert(first.pairwise_differences[i] >= 2);
            }
        }
    }

    printf("CANDIDATE_CONFORMANT_WORLDS=%u\n", conformant);
    printf("CANDIDATE_FALLBACK_WORLDS=%u\n", fallback);
    puts("CANDIDATE_100_SEEDS=PASS");
    puts("CANDIDATE_EXHAUSTIVE_ORACLE=PASS");
    puts("CANDIDATE_LEXICOGRAPHIC_SELECTION=PASS");
    puts("CANDIDATE_IDENTITIES=PASS");
    puts("CANDIDATE_REPLAY=PASS");
}

static void test_failure_atomicity(void)
{
    const RmcWorldIdentity identity = {
        .world_seed = 42,
        .generator_version = RMC_GENERATOR_VERSION,
        .rules_version = RMC_RULES_VERSION
    };

    RmcWorldCore world;
    RmcCandidateSelection output;
    RmcCandidateSelection initial;

    assert(rmc_world_core_generate(&identity, &world));

    memset(&output, 0xA5, sizeof(output));
    initial = output;

    assert(!rmc_candidates_select(NULL, &output));
    assert(!rmc_candidates_select(&world, NULL));

    world.teams[0].cohesion = 255;
    assert(!rmc_candidates_select(&world, &output));

    assert(memcmp(&output, &initial, sizeof(output)) == 0);

    puts("CANDIDATE_INVALID_ARGUMENTS=PASS");
    puts("CANDIDATE_FAILURE_ATOMICITY=PASS");
}


/*
 * Controlled fallback fixtures preserve all entity IDs,
 * roster coverage and valid persistent relationships.
 */
static void homogenize_fallback_world(RmcWorldCore *world)
{
    const RmcRoster template_roster = world->rosters[0];
    const RmcSocialState template_social = world->social[0];
    const uint8_t cohesion = world->teams[0].cohesion;

    for (size_t t = 0; t < RMC_WORLD_TEAMS; ++t) {
        RmcRoster *roster = &world->rosters[t];
        RmcSocialState *social = &world->social[t];

        for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
            const RmcPlayerId id = roster->players[p].id;
            const RmcTeamId team_id = roster->players[p].team_id;

            roster->players[p] = template_roster.players[p];
            roster->players[p].id = id;
            roster->players[p].team_id = team_id;
        }

        world->teams[t].cohesion = cohesion;
        world->teams[t].tactical_profile = RMC_TACTICAL_POSSESSION;
        social->tactical_familiarity =
            template_social.tactical_familiarity;
        social->lineup_continuity =
            template_social.lineup_continuity;

        for (size_t d = 0; d < RMC_SOCIAL_DEPARTMENTS; ++d) {
            social->department_chemistry[d] =
                template_social.department_chemistry[d];
        }
    }
}

static void verify_fallback_case(
    const RmcWorldCore *world,
    size_t a, size_t b, size_t c,
    unsigned ab, unsigned ac, unsigned bc)
{
    const size_t expected_indices[3] = {a, b, c};
    const unsigned expected_distances[3] = {ab, ac, bc};

    RmcCandidateSelection selected;
    RmcCandidateSelection replay;

    assert(rmc_world_core_valid(world));
    assert(rmc_candidates_select(world, &selected));
    assert(rmc_candidates_select(world, &replay));
    assert(selected.fallback_used);
    assert(replay.fallback_used);

    for (size_t i = 0; i < RMC_CANDIDATE_COUNT; ++i) {
        assert(selected.team_indices[i] == expected_indices[i]);
        assert(selected.team_ids[i].value ==
               world->teams[expected_indices[i]].id.value);
        assert(selected.pairwise_differences[i] ==
               expected_distances[i]);

        assert(replay.team_indices[i] == selected.team_indices[i]);
        assert(replay.team_ids[i].value == selected.team_ids[i].value);
        assert(replay.pairwise_differences[i] ==
               selected.pairwise_differences[i]);
    }
}

static void test_fallback_regression(void)
{
    const RmcWorldIdentity identity = {
        .world_seed = UINT64_C(0x27820261114),
        .generator_version = RMC_GENERATOR_VERSION,
        .rules_version = RMC_RULES_VERSION
    };

    RmcWorldCore world;
    assert(rmc_world_core_generate(&identity, &world));

    homogenize_fallback_world(&world);
    assert(rmc_world_core_valid(&world));

    verify_fallback_case(&world, 0, 1, 2, 0, 0, 0);
    puts("CANDIDATE_FALLBACK_IDENTICAL_PROFILES=PASS");

    for (size_t t = 4; t < RMC_WORLD_TEAMS; ++t) {
        world.teams[t].tactical_profile = RMC_TACTICAL_DIRECT;
    }

    verify_fallback_case(&world, 0, 1, 4, 0, 1, 1);
    puts("CANDIDATE_FALLBACK_TACTICAL_ONLY=PASS");
    puts("CANDIDATE_FALLBACK_REPLAY=PASS");
    puts("CANDIDATE_FALLBACK_REGRESSION=PASS");
}

int main(void)
{
    test_seed_campaign();
    test_failure_atomicity();
    test_fallback_regression();
    puts("CANDIDATE_SELECTOR_QUALIFICATION=PASS");
    return 0;
}
