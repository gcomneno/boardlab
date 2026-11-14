#include "career.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static RmcWorldIdentity identity_for(uint64_t seed)
{
    const RmcWorldIdentity identity = {
        .world_seed = seed,
        .generator_version = RMC_GENERATOR_VERSION,
        .rules_version = RMC_RULES_VERSION
    };

    return identity;
}

static void test_100_seed_campaign(void)
{
    for (uint64_t seed = 0; seed < UINT64_C(100); ++seed) {
        const RmcWorldIdentity identity = identity_for(
            seed * UINT64_C(0x9e3779b97f4a7c15)
        );

        RmcCareerFoundation first;
        RmcCareerFoundation replay;

        assert(rmc_career_foundation_generate(
            &identity, &first));
        assert(rmc_career_foundation_generate(
            &identity, &replay));

        assert(rmc_career_foundation_valid(&first));
        assert(rmc_career_foundation_valid(&replay));

        assert(first.world.identity.world_seed ==
               identity.world_seed);
        assert(first.world.identity.generator_version ==
               identity.generator_version);
        assert(first.world.identity.rules_version ==
               identity.rules_version);

        for (size_t kind = 0; kind < RMC_ENTITY_COUNT; ++kind) {
            const uint64_t first_next =
                first.world.ids.counters[kind].next;
            const uint64_t replay_next =
                replay.world.ids.counters[kind].next;

            assert(first_next == replay_next);

            if (kind == RMC_ENTITY_MATCH) {
                assert(first_next ==
                       UINT64_C(1) + RMC_CALENDAR_FIXTURES);
            }
        }

        for (size_t t = 0; t < RMC_WORLD_TEAMS; ++t) {
            assert(first.world.teams[t].id.value ==
                   replay.world.teams[t].id.value);
            assert(first.world.coaches[t].id.value ==
                   replay.world.coaches[t].id.value);

            for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
                assert(
                    first.world.rosters[t].players[p].id.value ==
                    replay.world.rosters[t].players[p].id.value
                );
            }
        }

        for (size_t i = 0; i < RMC_CALENDAR_FIXTURES; ++i) {
            const RmcFixture *a = &first.calendar.fixtures[i];
            const RmcFixture *b = &replay.calendar.fixtures[i];

            assert(a->id.value == b->id.value);
            assert(a->home_team.value == b->home_team.value);
            assert(a->away_team.value == b->away_team.value);
            assert(a->matchday == b->matchday);

            assert(a->id.value == UINT64_C(1) + i);
        }
    }

    puts("CAREER_100_SEEDS_VALID=PASS");
    puts("CAREER_100_SEEDS_REPLAY=PASS");
    puts("CAREER_5600_FIXTURES=PASS");
    puts("CAREER_PERSISTENT_COUNTERS=PASS");
}

static void test_invalid_foundations(void)
{
    const RmcWorldIdentity identity = identity_for(42);
    RmcCareerFoundation original;
    RmcCareerFoundation changed;

    assert(rmc_career_foundation_generate(
        &identity, &original));
    assert(rmc_career_foundation_valid(&original));

    changed = original;
    changed.calendar.fixtures[0].id.value = 0;
    assert(!rmc_career_foundation_valid(&changed));

    changed = original;
    changed.calendar.fixtures[1].id =
        changed.calendar.fixtures[0].id;
    assert(!rmc_career_foundation_valid(&changed));

    changed = original;
    changed.calendar.fixtures[0].home_team =
        changed.calendar.fixtures[0].away_team;
    assert(!rmc_career_foundation_valid(&changed));

    changed = original;
    changed.calendar.fixtures[0].matchday = 14;
    assert(!rmc_career_foundation_valid(&changed));

    changed = original;
    changed.world.ids.counters[RMC_ENTITY_MATCH].next = 1;
    assert(!rmc_career_foundation_valid(&changed));

    changed = original;
    changed.world.teams[0].id.value = 0;
    assert(!rmc_career_foundation_valid(&changed));

    assert(!rmc_career_foundation_valid(NULL));

    puts("CAREER_INVALID_CALENDAR_REJECTED=PASS");
    puts("CAREER_INVALID_COUNTERS_REJECTED=PASS");
    puts("CAREER_INVALID_WORLD_REJECTED=PASS");
}

static void test_failure_atomicity(void)
{
    const RmcWorldIdentity valid_identity = identity_for(43);

    RmcWorldIdentity invalid_identity = valid_identity;
    invalid_identity.generator_version =
        RMC_GENERATOR_VERSION + 1;

    RmcCareerFoundation destination;
    RmcCareerFoundation baseline;

    memset(&destination, 0xA5, sizeof(destination));
    baseline = destination;

    assert(!rmc_career_foundation_generate(
        &invalid_identity, &destination));

    assert(memcmp(
        &destination, &baseline, sizeof(destination)) == 0);

    assert(!rmc_career_foundation_generate(
        NULL, &destination));

    assert(!rmc_career_foundation_generate(
        &valid_identity, NULL));

    assert(memcmp(
        &destination, &baseline, sizeof(destination)) == 0);

    puts("CAREER_INVALID_IDENTITY_REJECTED=PASS");
    puts("CAREER_FAILURE_ATOMICITY=PASS");
}


/*
 * Complete initial career setup: World Core, calendar
 * and three canonically selected candidate teams.
 */
static void test_career_setup_regression(void)
{
    for (uint64_t seed = 0; seed < UINT64_C(100); ++seed) {
        const RmcWorldIdentity identity = identity_for(seed);
        RmcCareerSetup first;
        RmcCareerSetup replay;
        RmcCandidateSelection canonical;

        assert(rmc_career_setup_generate(&identity, &first));
        assert(rmc_career_setup_generate(&identity, &replay));
        assert(rmc_career_setup_valid(&first));
        assert(rmc_career_setup_valid(&replay));

        assert(rmc_candidates_select(
            &first.foundation.world, &canonical));

        assert(first.foundation.world.identity.world_seed ==
               identity.world_seed);

        assert(first.foundation.world.ids
                   .counters[RMC_ENTITY_MATCH].next ==
               UINT64_C(1) + RMC_CALENDAR_FIXTURES);

        for (size_t kind = 0; kind < RMC_ENTITY_COUNT; ++kind) {
            assert(first.foundation.world.ids.counters[kind].next ==
                   replay.foundation.world.ids.counters[kind].next);
        }

        for (size_t i = 0; i < RMC_CALENDAR_FIXTURES; ++i) {
            const RmcFixture *a = &first.foundation.calendar.fixtures[i];
            const RmcFixture *b = &replay.foundation.calendar.fixtures[i];

            assert(a->id.value == b->id.value);
            assert(a->home_team.value == b->home_team.value);
            assert(a->away_team.value == b->away_team.value);
            assert(a->matchday == b->matchday);
        }

        assert(first.candidates.fallback_used ==
               canonical.fallback_used);
        assert(first.candidates.fallback_used ==
               replay.candidates.fallback_used);

        for (size_t i = 0; i < RMC_CANDIDATE_COUNT; ++i) {
            const size_t index = first.candidates.team_indices[i];

            assert(index < RMC_WORLD_TEAMS);
            assert(first.candidates.team_ids[i].value ==
                   first.foundation.world.teams[index].id.value);

            assert(index == canonical.team_indices[i]);
            assert(index == replay.candidates.team_indices[i]);

            assert(first.candidates.team_ids[i].value ==
                   canonical.team_ids[i].value);
            assert(first.candidates.team_ids[i].value ==
                   replay.candidates.team_ids[i].value);

            assert(first.candidates.pairwise_differences[i] ==
                   canonical.pairwise_differences[i]);
            assert(first.candidates.pairwise_differences[i] ==
                   replay.candidates.pairwise_differences[i]);
        }

        assert(first.candidates.team_indices[0] <
               first.candidates.team_indices[1]);
        assert(first.candidates.team_indices[1] <
               first.candidates.team_indices[2]);
    }

    puts("CAREER_SETUP_100_SEEDS=PASS");
    puts("CAREER_SETUP_CALENDAR_AND_IDENTITIES=PASS");
    puts("CAREER_SETUP_CANONICAL_CANDIDATES=PASS");
    puts("CAREER_SETUP_REPLAY=PASS");
}

static void test_career_setup_validation(void)
{
    const RmcWorldIdentity identity = identity_for(42);
    RmcCareerSetup original;
    RmcCareerSetup changed;

    assert(rmc_career_setup_generate(&identity, &original));
    assert(rmc_career_setup_valid(&original));

    changed = original;
    changed.candidates.team_indices[0] = RMC_WORLD_TEAMS;
    assert(!rmc_career_setup_valid(&changed));

    changed = original;
    changed.candidates.team_ids[0].value = 0;
    assert(!rmc_career_setup_valid(&changed));

    changed = original;
    changed.candidates.pairwise_differences[0] = 255;
    assert(!rmc_career_setup_valid(&changed));

    changed = original;
    changed.candidates.fallback_used =
        !changed.candidates.fallback_used;
    assert(!rmc_career_setup_valid(&changed));

    changed = original;
    changed.foundation.calendar.fixtures[0].id.value = 0;
    assert(!rmc_career_setup_valid(&changed));

    assert(!rmc_career_setup_valid(NULL));

    puts("CAREER_SETUP_TAMPERING_REJECTED=PASS");
    puts("CAREER_SETUP_INVALID_FOUNDATION_REJECTED=PASS");
}

static void test_career_setup_atomicity(void)
{
    const RmcWorldIdentity valid_identity = identity_for(43);
    RmcWorldIdentity invalid_identity = valid_identity;
    RmcCareerSetup destination;
    RmcCareerSetup baseline;

    invalid_identity.generator_version = 0;

    memset(&destination, 0xA5, sizeof(destination));
    baseline = destination;

    assert(!rmc_career_setup_generate(
        &invalid_identity, &destination));

    assert(memcmp(
        &destination, &baseline, sizeof(destination)) == 0);

    assert(!rmc_career_setup_generate(NULL, &destination));
    assert(!rmc_career_setup_generate(&valid_identity, NULL));

    assert(memcmp(
        &destination, &baseline, sizeof(destination)) == 0);

    puts("CAREER_SETUP_INVALID_ARGUMENTS=PASS");
    puts("CAREER_SETUP_FAILURE_ATOMICITY=PASS");
}

int main(void)
{
    test_100_seed_campaign();
    test_invalid_foundations();
    test_failure_atomicity();
    test_career_setup_regression();
    test_career_setup_validation();
    test_career_setup_atomicity();

    puts("CAREER_FOUNDATION_QUALIFICATION=PASS");
    return 0;
}
