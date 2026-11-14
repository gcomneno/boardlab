#include "calendar.h"

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
    for (uint64_t seed = 0; seed < 100; ++seed) {
        const RmcWorldIdentity identity = identity_for(
            seed * UINT64_C(0x9e3779b97f4a7c15)
        );
        RmcWorldCore world;
        RmcCalendar first = {0};
        RmcCalendar replay = {0};

        assert(rmc_world_core_generate(&identity, &world));

        RmcIdentityState first_ids = world.ids;
        RmcIdentityState replay_ids = world.ids;

        assert(rmc_calendar_generate(&world, &first_ids, &first));
        assert(rmc_calendar_generate(&world, &replay_ids, &replay));

        assert(rmc_calendar_valid(&world, &first_ids, &first));
        assert(rmc_calendar_valid(&world, &replay_ids, &replay));

        for (size_t i = 0; i < RMC_CALENDAR_FIXTURES; ++i) {
            const RmcFixture *a = &first.fixtures[i];
            const RmcFixture *b = &replay.fixtures[i];

            assert(a->id.value == b->id.value);
            assert(a->home_team.value == b->home_team.value);
            assert(a->away_team.value == b->away_team.value);
            assert(a->matchday == b->matchday);

            assert(a->id.value ==
                world.ids.counters[RMC_ENTITY_MATCH].next + i);
            assert(a->matchday ==
                (uint8_t)(i / RMC_CALENDAR_MATCHES_PER_DAY + 1));
        }

        assert(first_ids.counters[RMC_ENTITY_MATCH].next ==
            world.ids.counters[RMC_ENTITY_MATCH].next +
            RMC_CALENDAR_FIXTURES);

        for (size_t kind = 0; kind < RMC_ENTITY_COUNT; ++kind) {
            assert(first_ids.counters[kind].next ==
                replay_ids.counters[kind].next);

            if (kind != RMC_ENTITY_MATCH) {
                assert(first_ids.counters[kind].next ==
                    world.ids.counters[kind].next);
            }
        }

        for (size_t i = 0; i < RMC_CALENDAR_FIXTURES / 2;
             ++i) {
            const RmcFixture *outbound = &first.fixtures[i];
            const RmcFixture *inbound =
                &first.fixtures[i + RMC_CALENDAR_FIXTURES / 2];

            assert(outbound->home_team.value ==
                inbound->away_team.value);
            assert(outbound->away_team.value ==
                inbound->home_team.value);
        }
    }

    puts("CALENDAR_100_SEEDS_VALID=PASS");
    puts("CALENDAR_100_SEEDS_REPLAY=PASS");
    puts("CALENDAR_56_MATCH_IDS=PASS");
    puts("CALENDAR_HOME_AWAY_REVERSAL=PASS");
}

static void test_invalid_calendar(void)
{
    const RmcWorldIdentity identity = identity_for(42);
    RmcWorldCore world;
    RmcCalendar original = {0};
    RmcCalendar changed;
    RmcIdentityState ids;

    assert(rmc_world_core_generate(&identity, &world));
    ids = world.ids;
    assert(rmc_calendar_generate(&world, &ids, &original));

    changed = original;
    changed.fixtures[1].id = changed.fixtures[0].id;
    assert(!rmc_calendar_valid(&world, &ids, &changed));

    changed = original;
    changed.fixtures[0].home_team =
        changed.fixtures[0].away_team;
    assert(!rmc_calendar_valid(&world, &ids, &changed));

    changed = original;
    changed.fixtures[0].matchday = 14;
    assert(!rmc_calendar_valid(&world, &ids, &changed));

    changed = original;
    changed.fixtures[0].home_team.value = UINT64_MAX;
    assert(!rmc_calendar_valid(&world, &ids, &changed));

    changed = original;
    changed.fixtures[28].home_team =
        changed.fixtures[0].home_team;
    changed.fixtures[28].away_team =
        changed.fixtures[0].away_team;
    assert(!rmc_calendar_valid(&world, &ids, &changed));

    puts("CALENDAR_INVALID_STATE_REJECTED=PASS");
}

static void test_failure_atomicity(void)
{
    const RmcWorldIdentity identity = identity_for(43);
    RmcWorldCore world;
    RmcCalendar destination;
    RmcCalendar baseline;
    RmcIdentityState ids;
    RmcIdentityState before;

    assert(rmc_world_core_generate(&identity, &world));

    memset(&destination, 0xA5, sizeof(destination));
    baseline = destination;

    ids = world.ids;
    ids.counters[RMC_ENTITY_MATCH].next = UINT64_MAX - 20;
    before = ids;

    assert(!rmc_calendar_generate(&world, &ids, &destination));
    assert(memcmp(&destination, &baseline, sizeof(destination)) == 0);
    assert(memcmp(&ids, &before, sizeof(ids)) == 0);

    ids = world.ids;
    before = ids;

    assert(!rmc_calendar_generate(NULL, &ids, &destination));
    assert(!rmc_calendar_generate(&world, NULL, &destination));
    assert(!rmc_calendar_generate(&world, &ids, NULL));

    assert(memcmp(&destination, &baseline, sizeof(destination)) == 0);
    assert(memcmp(&ids, &before, sizeof(ids)) == 0);

    puts("CALENDAR_FAILURE_ATOMICITY=PASS");
    puts("CALENDAR_ID_EXHAUSTION_REJECTED=PASS");
}

int main(void)
{
    test_100_seed_campaign();
    test_invalid_calendar();
    test_failure_atomicity();

    puts("CALENDAR_QUALIFICATION=PASS");
    return 0;
}
