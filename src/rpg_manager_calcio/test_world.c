#include "world.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static RmcWorldIdentity identity_for(uint64_t seed)
{
    RmcWorldIdentity identity = {
        .world_seed = seed,
        .generator_version = RMC_GENERATOR_VERSION,
        .rules_version = RMC_RULES_VERSION
    };
    return identity;
}

static bool equal_progression(
    const RmcPlayer *a,
    const RmcPlayer *b)
{
    const RmcTrainingState *ta = &a->training;
    const RmcTrainingState *tb = &b->training;
    const RmcTalentState *sa = &a->talents;
    const RmcTalentState *sb = &b->talents;

    if (ta->total_season_gain != tb->total_season_gain ||
        sa->consolidated_mask != sb->consolidated_mask ||
        sa->matured_mask != sb->matured_mask ||
        sa->season_consolidations != sb->season_consolidations) {
        return false;
    }

    for (size_t i = 0; i < RMC_GENERAL_ATTRIBUTES; ++i) {
        if (ta->general_progress[i] != tb->general_progress[i] ||
            ta->general_season_gain[i] != tb->general_season_gain[i]) {
            return false;
        }
    }

    for (size_t i = 0; i < RMC_GOALKEEPER_ATTRIBUTES; ++i) {
        if (ta->goalkeeper_progress[i] != tb->goalkeeper_progress[i] ||
            ta->goalkeeper_season_gain[i] !=
                tb->goalkeeper_season_gain[i]) {
            return false;
        }
    }

    for (size_t i = 0; i < RMC_TALENT_COUNT; ++i) {
        if (sa->candidate_progress[i] !=
            sb->candidate_progress[i]) {
            return false;
        }
    }

    return true;
}

static bool equal_player(const RmcPlayer *a, const RmcPlayer *b)
{
    if (a->id.value != b->id.value ||
        a->team_id.value != b->team_id.value ||
        a->department != b->department ||
        a->primary_slots != b->primary_slots ||
        a->secondary_slots != b->secondary_slots ||
        a->age != b->age ||
        a->energy != b->energy ||
        a->morale != b->morale ||
        a->fitness != b->fitness ||
        a->level != b->level ||
        a->experience != b->experience ||
        a->potential != b->potential) {
        return false;
    }

    for (size_t i = 0; i < RMC_GENERAL_ATTRIBUTES; ++i) {
        if (a->attributes[i] != b->attributes[i]) {
            return false;
        }
    }

    for (size_t i = 0; i < RMC_GOALKEEPER_ATTRIBUTES; ++i) {
        if (a->goalkeeper_attributes[i] !=
            b->goalkeeper_attributes[i]) {
            return false;
        }
    }

    return equal_progression(a, b);
}

static bool equal_social(
    const RmcSocialState *a,
    const RmcSocialState *b)
{
    if (a->team_id.value != b->team_id.value ||
        a->tactical_familiarity != b->tactical_familiarity ||
        a->lineup_continuity != b->lineup_continuity ||
        a->pair_count != b->pair_count) {
        return false;
    }

    for (size_t i = 0; i < RMC_SOCIAL_DEPARTMENTS; ++i) {
        if (a->department_chemistry[i] !=
            b->department_chemistry[i]) {
            return false;
        }
    }

    for (size_t i = 0; i < RMC_MAX_CHEMISTRY_PAIRS; ++i) {
        if (a->pairs[i].first.value != b->pairs[i].first.value ||
            a->pairs[i].second.value != b->pairs[i].second.value ||
            a->pairs[i].chemistry != b->pairs[i].chemistry) {
            return false;
        }
    }

    return true;
}

static bool equal_world(const RmcWorldCore *a, const RmcWorldCore *b)
{
    if (a->identity.world_seed != b->identity.world_seed ||
        a->identity.generator_version !=
            b->identity.generator_version ||
        a->identity.rules_version != b->identity.rules_version) {
        return false;
    }

    for (size_t i = 0; i < RMC_ENTITY_COUNT; ++i) {
        if (a->ids.counters[i].next != b->ids.counters[i].next) {
            return false;
        }
    }

    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        const RmcTeam *ta = &a->teams[i];
        const RmcTeam *tb = &b->teams[i];
        const RmcCoach *ca = &a->coaches[i];
        const RmcCoach *cb = &b->coaches[i];

        if (ta->id.value != tb->id.value ||
            ta->coach_id.value != tb->coach_id.value ||
            strcmp(ta->name, tb->name) != 0 ||
            ta->tactical_profile != tb->tactical_profile ||
            ta->cohesion != tb->cohesion ||
            ca->id.value != cb->id.value ||
            ca->team_id.value != cb->team_id.value ||
            strcmp(ca->name, cb->name) != 0 ||
            ca->aggression != cb->aggression ||
            ca->caution != cb->caution ||
            ca->adaptability != cb->adaptability ||
            ca->people_management != cb->people_management ||
            a->rosters[i].team_id.value !=
                b->rosters[i].team_id.value) {
            return false;
        }

        if (!equal_social(&a->social[i], &b->social[i])) {
            return false;
        }

        for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
            if (!equal_player(
                    &a->rosters[i].players[p],
                    &b->rosters[i].players[p])) {
                return false;
            }
        }
    }

    return true;
}

static void test_world_generation(void)
{
    RmcWorldCore world;
    RmcWorldIdentity identity = identity_for(UINT64_C(42));

    assert(rmc_world_core_generate(&identity, &world));
    assert(rmc_world_core_valid(&world));

    assert(world.ids.counters[RMC_ENTITY_TEAM].next == 9);
    assert(world.ids.counters[RMC_ENTITY_COACH].next == 9);
    assert(world.ids.counters[RMC_ENTITY_PLAYER].next == 145);

    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        assert(world.teams[i].id.value == i + 1);
        assert(world.coaches[i].id.value == i + 1);
        assert(world.rosters[i].team_id.value == i + 1);

        assert(world.teams[i].coach_id.value ==
               world.coaches[i].id.value);
        assert(world.coaches[i].team_id.value ==
               world.teams[i].id.value);
        assert(rmc_roster_valid(&world.rosters[i]));

        for (unsigned f = 0; f < RMC_FORMATION_COUNT; ++f) {
            RmcLineup lineup;
            assert(rmc_roster_lineup(
                &world.rosters[i], (RmcFormation)f, &lineup
            ));
        }

        for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
            const RmcPlayer *player = &world.rosters[i].players[p];
            assert(player->id.value == i * RMC_TEAM_PLAYERS + p + 1);
            assert(player->team_id.value == world.teams[i].id.value);
        }
    }

    puts("WORLD_ENTITY_GRAPH=PASS");
    puts("WORLD_144_IDENTITIES=PASS");
    puts("WORLD_FORMATION_COVERAGE=PASS");
}

static void test_repeated_generation(void)
{
    const uint64_t seeds[] = {
        UINT64_C(0),
        UINT64_C(1),
        UINT64_C(42),
        UINT64_MAX
    };

    for (size_t i = 0; i < sizeof(seeds) / sizeof(seeds[0]); ++i) {
        RmcWorldIdentity identity = identity_for(seeds[i]);
        RmcWorldCore first;
        RmcWorldCore second;

        assert(rmc_world_core_generate(&identity, &first));
        assert(rmc_world_core_generate(&identity, &second));
        assert(equal_world(&first, &second));
    }

    puts("WORLD_REPEATABILITY=PASS");
}

static void test_seed_sensitivity(void)
{
    RmcWorldIdentity first_id = identity_for(UINT64_C(100));
    RmcWorldIdentity second_id = identity_for(UINT64_C(101));
    RmcWorldCore first;
    RmcWorldCore second;

    assert(rmc_world_core_generate(&first_id, &first));
    assert(rmc_world_core_generate(&second_id, &second));
    assert(!equal_world(&first, &second));

    bool attributes_differ = false;

    for (size_t t = 0; t < RMC_WORLD_TEAMS; ++t) {
        for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
            if (!equal_player(
                    &first.rosters[t].players[p],
                    &second.rosters[t].players[p])) {
                attributes_differ = true;
            }
        }
    }

    assert(attributes_differ);
    puts("WORLD_SEED_SENSITIVITY=PASS");
}

static void test_invalid_and_atomicity(void)
{
    RmcWorldIdentity identity = identity_for(UINT64_C(7));
    RmcWorldCore world;
    RmcWorldCore baseline;

    assert(rmc_world_core_generate(&identity, &world));
    baseline = world;

    identity.generator_version = UINT32_MAX;
    assert(!rmc_world_core_generate(&identity, &world));
    assert(equal_world(&world, &baseline));

    assert(!rmc_world_core_generate(NULL, &world));
    assert(equal_world(&world, &baseline));

    assert(!rmc_world_core_generate(&identity, NULL));
    assert(!rmc_world_core_valid(NULL));

    world.teams[1].id = world.teams[0].id;
    assert(!rmc_world_core_valid(&world));

    world = baseline;
    world.rosters[1].players[0].id =
        world.rosters[0].players[0].id;
    assert(!rmc_world_core_valid(&world));

    world = baseline;
    world.ids.counters[RMC_ENTITY_PLAYER].next = 1;
    assert(!rmc_world_core_valid(&world));

    puts("WORLD_INVALID_STATE=PASS");
    puts("WORLD_GENERATION_ATOMICITY=PASS");
}

static void test_100_seed_campaign(void)
{
    for (uint64_t index = 0; index < UINT64_C(100); ++index) {
        const uint64_t seed =
            index * UINT64_C(0x9e3779b97f4a7c15) +
            UINT64_C(0xd1b54a32d192ed03);

        const RmcWorldIdentity identity = identity_for(seed);
        RmcWorldCore first;
        RmcWorldCore replay;

        assert(rmc_world_core_generate(&identity, &first));
        assert(rmc_world_core_generate(&identity, &replay));

        assert(rmc_world_core_valid(&first));
        assert(rmc_world_core_valid(&replay));
        assert(equal_world(&first, &replay));

        for (size_t team = 0; team < RMC_WORLD_TEAMS; ++team) {
            for (unsigned formation = 0;
                 formation < RMC_FORMATION_COUNT;
                 ++formation) {
                RmcLineup lineup;

                assert(rmc_roster_lineup(
                    &first.rosters[team],
                    (RmcFormation)formation,
                    &lineup
                ));
            }
        }
    }

    puts("WORLD_100_SEEDS_VALID=PASS");
    puts("WORLD_100_SEEDS_REPLAY=PASS");
    puts("WORLD_100_SEEDS_FORMATIONS=PASS");
}

static void test_progression_replay_detection(void)
{
    const RmcWorldIdentity identity = identity_for(UINT64_C(42));
    RmcWorldCore original;
    RmcWorldCore changed;

    assert(rmc_world_core_generate(&identity, &original));

    changed = original;
    changed.rosters[0].players[0]
        .training.general_progress[0] = 17;
    assert(!equal_world(&original, &changed));

    changed = original;
    changed.rosters[0].players[0]
        .talents.candidate_progress[0] = 31;
    assert(!equal_world(&original, &changed));

    changed = original;
    changed.rosters[0].players[0]
        .talents.consolidated_mask = 1;
    assert(!equal_world(&original, &changed));

    puts("WORLD_RPG_REPLAY_DETECTION=PASS");
}

static void test_social_integrity_and_replay(void)
{
    const RmcWorldIdentity identity = identity_for(UINT64_C(42));
    RmcWorldCore original;
    RmcWorldCore changed;

    assert(rmc_world_core_generate(&identity, &original));
    assert(rmc_world_core_valid(&original));

    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        assert(original.social[i].team_id.value ==
               original.teams[i].id.value);
        assert(original.social[i].pair_count == 3);
    }

    changed = original;
    changed.social[0].tactical_familiarity = 0;
    assert(!equal_world(&original, &changed));

    changed = original;
    changed.social[0].department_chemistry[0] = 0;
    assert(!equal_world(&original, &changed));

    changed = original;
    changed.social[0].pairs[0].chemistry = 0;
    assert(!equal_world(&original, &changed));

    changed = original;
    changed.social[0].pairs[1] = changed.social[0].pairs[0];
    assert(!rmc_world_core_valid(&changed));

    changed = original;
    changed.social[0].pairs[0].first =
        original.rosters[1].players[0].id;
    assert(!rmc_world_core_valid(&changed));

    changed = original;
    changed.social[0].pairs[0].first =
        changed.social[0].pairs[0].second;
    assert(!rmc_world_core_valid(&changed));

    changed = original;
    changed.social[0].pairs[3].chemistry = 50;
    assert(!rmc_world_core_valid(&changed));

    changed = original;
    changed.social[0].pair_count = 9;
    assert(!rmc_world_core_valid(&changed));

    puts("WORLD_SOCIAL_INTEGRITY=PASS");
    puts("WORLD_SOCIAL_INVALID_RELATIONS=PASS");
    puts("WORLD_SOCIAL_REPLAY_DETECTION=PASS");
}

static void test_initial_rpg_state(void)
{
    for (uint64_t seed = 0; seed < UINT64_C(100); ++seed) {
        const RmcWorldIdentity identity = identity_for(
            seed * UINT64_C(0x9e3779b97f4a7c15)
        );
        RmcWorldCore world;

        assert(rmc_world_core_generate(&identity, &world));

        for (size_t t = 0; t < RMC_WORLD_TEAMS; ++t) {
            for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
                const RmcPlayer *player = &world.rosters[t].players[p];
                const RmcTrainingState *training = &player->training;
                const RmcTalentState *talents = &player->talents;

                assert(player->level == 1);
                assert(player->experience == 0);
                assert(training->total_season_gain == 0);
                assert(talents->consolidated_mask == 0);
                assert(talents->matured_mask == 0);
                assert(talents->season_consolidations == 0);

                for (size_t i = 0;
                     i < RMC_GENERAL_ATTRIBUTES; ++i) {
                    assert(training->general_progress[i] == 0);
                    assert(training->general_season_gain[i] == 0);
                }

                for (size_t i = 0;
                     i < RMC_GOALKEEPER_ATTRIBUTES; ++i) {
                    assert(training->goalkeeper_progress[i] == 0);
                    assert(training->goalkeeper_season_gain[i] == 0);
                }

                for (size_t i = 0; i < RMC_TALENT_COUNT; ++i) {
                    assert(talents->candidate_progress[i] == 0);
                }
            }
        }
    }

    puts("WORLD_100_SEEDS_INITIAL_RPG_STATE=PASS");
}

int main(void)
{
    test_world_generation();
    test_repeated_generation();
    test_seed_sensitivity();
    test_invalid_and_atomicity();
    test_100_seed_campaign();
    test_progression_replay_detection();
    test_social_integrity_and_replay();
    test_initial_rpg_state();

    puts("WORLD_CORE_QUALIFICATION=PASS");
    return 0;
}
