#include "strength.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const RmcAttribute weights_by_slot[RMC_SLOT_COUNT][4] = {
    [RMC_SLOT_POR] = {0, 0, 0, 0},
    [RMC_SLOT_TD] = {RMC_ATTR_CONTRASTO, RMC_ATTR_VELOCITA,
                     RMC_ATTR_MARCATURA, RMC_ATTR_PASSAGGIO},
    [RMC_SLOT_DC] = {RMC_ATTR_MARCATURA, RMC_ATTR_CONTRASTO,
                     RMC_ATTR_FORZA, RMC_ATTR_POSIZIONAMENTO},
    [RMC_SLOT_TS] = {RMC_ATTR_CONTRASTO, RMC_ATTR_VELOCITA,
                     RMC_ATTR_MARCATURA, RMC_ATTR_PASSAGGIO},
    [RMC_SLOT_ED] = {RMC_ATTR_VELOCITA, RMC_ATTR_CROSS,
                     RMC_ATTR_RESISTENZA, RMC_ATTR_CONTROLLO},
    [RMC_SLOT_CC] = {RMC_ATTR_PASSAGGIO, RMC_ATTR_VISIONE,
                     RMC_ATTR_CONTROLLO, RMC_ATTR_POSIZIONAMENTO},
    [RMC_SLOT_ES] = {RMC_ATTR_VELOCITA, RMC_ATTR_CROSS,
                     RMC_ATTR_RESISTENZA, RMC_ATTR_CONTROLLO},
    [RMC_SLOT_ATT] = {RMC_ATTR_TIRO, RMC_ATTR_POSIZIONAMENTO,
                      RMC_ATTR_CONTROLLO, RMC_ATTR_VELOCITA},
    [RMC_SLOT_MED] = {RMC_ATTR_POSIZIONAMENTO, RMC_ATTR_CONTRASTO,
                      RMC_ATTR_MARCATURA, RMC_ATTR_PASSAGGIO},
    [RMC_SLOT_AD] = {RMC_ATTR_DRIBBLING, RMC_ATTR_VELOCITA,
                     RMC_ATTR_TIRO, RMC_ATTR_CONTROLLO},
    [RMC_SLOT_AS] = {RMC_ATTR_DRIBBLING, RMC_ATTR_VELOCITA,
                     RMC_ATTR_TIRO, RMC_ATTR_CONTROLLO}
};

static const unsigned weights[4] = {30, 25, 25, 20};

static uint32_t reference_score(
    const RmcWorldCore *world,
    size_t team_index,
    const RmcPlayer *player,
    RmcSlot slot)
{
    unsigned sum = 0;

    if (slot == RMC_SLOT_POR) {
        const unsigned values[4] = {
            player->goalkeeper_attributes[RMC_GK_ATTR_RIFLESSI],
            player->goalkeeper_attributes[RMC_GK_ATTR_POSIZIONAMENTO],
            player->goalkeeper_attributes[RMC_GK_ATTR_PRESA],
            player->goalkeeper_attributes[RMC_GK_ATTR_USCITE]
        };

        for (size_t i = 0; i < 4; ++i) {
            sum += weights[i] * values[i];
        }
    } else {
        for (size_t i = 0; i < 4; ++i) {
            sum += weights[i] *
                player->attributes[weights_by_slot[slot][i]];
        }
    }

    const unsigned a = sum / 100u;
    const unsigned f =
        ((unsigned)player->energy + player->fitness) / 2u;

    const RmcSocialState *social = &world->social[team_index];

    const unsigned o =
        ((unsigned)world->teams[team_index].cohesion +
         social->tactical_familiarity +
         social->lineup_continuity +
         social->department_chemistry[player->department]) / 4u;

    unsigned result = (70u * a + 15u * f + 15u * o) / 100u;

    const uint16_t bit = (uint16_t)(UINT16_C(1) << slot);

    if ((player->primary_slots & bit) == 0) {
        assert((player->secondary_slots & bit) != 0);
        result = result > 10u ? result - 10u : 0u;
    }

    return result;
}

static const RmcPlayer *find_player(
    const RmcRoster *roster,
    RmcPlayerId id,
    size_t *index)
{
    for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
        if (roster->players[p].id.value == id.value) {
            *index = p;
            return &roster->players[p];
        }
    }

    return NULL;
}

static uint64_t verify_lineup(
    const RmcWorldCore *world,
    size_t team_index,
    const RmcLineup *lineup)
{
    const RmcRoster *roster = &world->rosters[team_index];
    const RmcSlot *expected = rmc_formation_slots(lineup->formation);
    bool used[RMC_TEAM_PLAYERS] = {false};
    unsigned secondary_count = 0;
    uint64_t sum = 0;

    assert(expected != NULL);

    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        size_t index = 0;
        const RmcPlayer *player =
            find_player(roster, lineup->players[i], &index);

        assert(player != NULL);
        assert(!used[index]);
        used[index] = true;

        assert(lineup->slots[i] == expected[i]);

        const uint16_t bit =
            (uint16_t)(UINT16_C(1) << expected[i]);

        assert(((player->primary_slots |
                 player->secondary_slots) & bit) != 0);

        if ((player->primary_slots & bit) == 0) {
            ++secondary_count;
        }

        sum += reference_score(world, team_index, player, expected[i]);
    }

    assert(lineup->secondary_assignments == secondary_count);
    return sum;
}

static void test_100_seed_campaign(void)
{
    for (uint64_t seed = 0; seed < 100; ++seed) {
        const RmcWorldIdentity identity = {
            .world_seed = seed * UINT64_C(0x9e3779b97f4a7c15),
            .generator_version = RMC_GENERATOR_VERSION,
            .rules_version = RMC_RULES_VERSION
        };

        RmcWorldCore world;
        assert(rmc_world_core_generate(&identity, &world));

        for (size_t team = 0; team < RMC_WORLD_TEAMS; ++team) {
            RmcTeamStrength best;
            RmcTeamStrength replay;

            assert(rmc_team_strength(&world, team, &best));
            assert(rmc_team_strength(&world, team, &replay));
            assert(best.team_id.value == world.teams[team].id.value);
            assert(best.team_id.value == replay.team_id.value);

            uint64_t maximum = 0;
            RmcFormation winning = RMC_FORMATION_442;

            for (size_t f = 0; f < RMC_FORMATION_COUNT; ++f) {
                RmcFormationStrength optimized;
                RmcLineup ordinary;

                assert(rmc_formation_strength(
                    &world, team, (RmcFormation)f, &optimized));

                assert(rmc_roster_lineup(
                    &world.rosters[team], (RmcFormation)f, &ordinary));

                assert(optimized.formation == (RmcFormation)f);
                assert(optimized.lineup.formation == (RmcFormation)f);

                const uint64_t verified =
                    verify_lineup(&world, team, &optimized.lineup);
                const uint64_t baseline =
                    verify_lineup(&world, team, &ordinary);

                assert(optimized.total_score == verified);
                assert(optimized.total_score >= baseline);
                assert(best.formation_scores[f] == verified);
                assert(replay.formation_scores[f] == verified);

                if (f == 0 || verified > maximum) {
                    maximum = verified;
                    winning = (RmcFormation)f;
                }
            }

            assert(best.best.total_score == maximum);
            assert(best.best.formation == winning);
            assert(replay.best.total_score == maximum);
            assert(replay.best.formation == winning);

            for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
                assert(best.best.lineup.players[i].value ==
                       replay.best.lineup.players[i].value);
                assert(best.best.lineup.slots[i] ==
                       replay.best.lineup.slots[i]);
            }
        }
    }

    puts("STRENGTH_100_SEEDS=PASS");
    puts("STRENGTH_2400_FORMATIONS=PASS");
    puts("STRENGTH_LINEUP_LEGALITY=PASS");
    puts("STRENGTH_SCORING_FORMULA=PASS");
    puts("STRENGTH_OPTIMIZED_NOT_WORSE=PASS");
    puts("STRENGTH_BEST_FORMATION=PASS");
    puts("STRENGTH_REPLAY=PASS");
}

static void test_failure_atomicity(void)
{
    const RmcWorldIdentity identity = {
        .world_seed = 42,
        .generator_version = RMC_GENERATOR_VERSION,
        .rules_version = RMC_RULES_VERSION
    };

    RmcWorldCore world;
    assert(rmc_world_core_generate(&identity, &world));

    RmcTeamStrength destination;
    RmcTeamStrength baseline;

    memset(&destination, 0xA5, sizeof(destination));
    baseline = destination;

    assert(!rmc_team_strength(NULL, 0, &destination));
    assert(!rmc_team_strength(&world, RMC_WORLD_TEAMS, &destination));
    assert(!rmc_team_strength(&world, 0, NULL));
    assert(memcmp(&destination, &baseline, sizeof(destination)) == 0);

    RmcFormationStrength formation;
    RmcFormationStrength formation_baseline;

    memset(&formation, 0xA5, sizeof(formation));
    formation_baseline = formation;

    assert(!rmc_formation_strength(
        &world, 0, RMC_FORMATION_COUNT, &formation));
    assert(!rmc_formation_strength(&world, 0, RMC_FORMATION_442, NULL));
    assert(memcmp(
        &formation, &formation_baseline, sizeof(formation)) == 0);

    puts("STRENGTH_INVALID_ARGUMENTS=PASS");
    puts("STRENGTH_FAILURE_ATOMICITY=PASS");
}

int main(void)
{
    test_100_seed_campaign();
    test_failure_atomicity();

    puts("TEAM_STRENGTH_QUALIFICATION=PASS");
    return 0;
}
