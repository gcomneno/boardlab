#include "roster.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BIT(slot) (UINT16_C(1) << (slot))

static void make_roster(RmcRoster *roster)
{
    static const RmcDepartment departments[18] = {
        RMC_DEPARTMENT_GOALKEEPER,
        RMC_DEPARTMENT_GOALKEEPER,
        RMC_DEPARTMENT_DEFENDER,
        RMC_DEPARTMENT_DEFENDER,
        RMC_DEPARTMENT_DEFENDER,
        RMC_DEPARTMENT_DEFENDER,
        RMC_DEPARTMENT_DEFENDER,
        RMC_DEPARTMENT_MIDFIELDER,
        RMC_DEPARTMENT_MIDFIELDER,
        RMC_DEPARTMENT_MIDFIELDER,
        RMC_DEPARTMENT_MIDFIELDER,
        RMC_DEPARTMENT_MIDFIELDER,
        RMC_DEPARTMENT_MIDFIELDER,
        RMC_DEPARTMENT_MIDFIELDER,
        RMC_DEPARTMENT_ATTACKER,
        RMC_DEPARTMENT_ATTACKER,
        RMC_DEPARTMENT_ATTACKER,
        RMC_DEPARTMENT_ATTACKER
    };

    static const RmcSlot primary[18] = {
        RMC_SLOT_POR, RMC_SLOT_POR,
        RMC_SLOT_TD, RMC_SLOT_DC, RMC_SLOT_DC,
        RMC_SLOT_TS, RMC_SLOT_DC,
        RMC_SLOT_ED, RMC_SLOT_CC, RMC_SLOT_CC,
        RMC_SLOT_ES, RMC_SLOT_MED, RMC_SLOT_CC,
        RMC_SLOT_CC,
        RMC_SLOT_ATT, RMC_SLOT_ATT,
        RMC_SLOT_AD, RMC_SLOT_AS
    };

    memset(roster, 0, sizeof(*roster));
    roster->team_id.value = 1;

    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        RmcPlayer *p = &roster->players[i];

        p->id.value = i + 1;
        p->team_id = roster->team_id;
        p->department = departments[i];
        p->primary_slots = BIT(primary[i]);
        p->age = 24;
        p->energy = 100;
        p->morale = 50;
        p->fitness = 100;
        p->level = 1;
        p->potential = 75;

        for (size_t j = 0; j < RMC_GENERAL_ATTRIBUTES; ++j) {
            p->attributes[j] = 50;
        }

        if (p->department == RMC_DEPARTMENT_GOALKEEPER) {
            for (size_t j = 0; j < RMC_GOALKEEPER_ATTRIBUTES; ++j) {
                p->goalkeeper_attributes[j] = 50;
            }
        }
    }
}

static void check_lineup(const RmcRoster *roster, RmcFormation formation)
{
    RmcLineup lineup;
    const RmcSlot *expected = rmc_formation_slots(formation);

    assert(expected != NULL);
    assert(rmc_roster_lineup(roster, formation, &lineup));
    assert(lineup.formation == formation);

    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        bool found = false;

        assert(lineup.slots[i] == expected[i]);

        for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
            if (roster->players[p].id.value == lineup.players[i].value) {
                const uint16_t bit = BIT(expected[i]);
                assert((roster->players[p].primary_slots |
                        roster->players[p].secondary_slots) & bit);
                found = true;
            }
        }

        assert(found);

        for (size_t j = 0; j < i; ++j) {
            assert(lineup.players[i].value != lineup.players[j].value);
        }
    }
}

static void test_all_formations(void)
{
    RmcRoster roster;
    make_roster(&roster);

    assert(rmc_roster_valid(&roster));

    for (unsigned f = 0; f < RMC_FORMATION_COUNT; ++f) {
        check_lineup(&roster, (RmcFormation)f);
    }

    puts("ROSTER_FORMATIONS=PASS");
    puts("ROSTER_UNIQUE_STARTERS=PASS");
}

static void test_secondary_compatibility(void)
{
    RmcRoster roster;
    RmcLineup lineup;

    make_roster(&roster);

    roster.players[7].primary_slots = BIT(RMC_SLOT_CC);
    roster.players[7].secondary_slots = BIT(RMC_SLOT_ED);

    assert(rmc_roster_valid(&roster));

    assert(rmc_roster_lineup(
        &roster, RMC_FORMATION_352, &lineup
    ));

    assert(lineup.secondary_assignments >= 1);

    puts("ROSTER_SECONDARY_COMPATIBILITY=PASS");
}

static void test_impossible_and_atomicity(void)
{
    RmcRoster roster;
    RmcLineup output;
    RmcLineup before;

    make_roster(&roster);

    memset(&output, 0xA5, sizeof(output));
    before = output;

    roster.players[7].primary_slots = BIT(RMC_SLOT_CC);

    assert(rmc_roster_valid(&roster));

    assert(!rmc_roster_lineup(
        &roster, RMC_FORMATION_442, &output
    ));

    assert(memcmp(&output, &before, sizeof(output)) == 0);

    assert(!rmc_roster_lineup(
        &roster, RMC_FORMATION_352, &output
    ));

    assert(memcmp(&output, &before, sizeof(output)) == 0);

    puts("ROSTER_IMPOSSIBLE=PASS");
    puts("ROSTER_FAILURE_ATOMICITY=PASS");
}

static void test_invalid_rosters(void)
{
    RmcRoster roster;
    RmcLineup output;

    make_roster(&roster);

    roster.players[1].id = roster.players[0].id;
    assert(!rmc_roster_valid(&roster));

    make_roster(&roster);
    roster.players[2].secondary_slots =
        roster.players[2].primary_slots;
    assert(!rmc_roster_valid(&roster));

    make_roster(&roster);
    assert(!rmc_roster_lineup(
        &roster, RMC_FORMATION_COUNT, &output
    ));

    assert(!rmc_roster_lineup(
        NULL, RMC_FORMATION_442, &output
    ));

    assert(!rmc_roster_lineup(
        &roster, RMC_FORMATION_442, NULL
    ));

    puts("ROSTER_INVALID_INPUT=PASS");
}

static void test_progression_state(void)
{
    RmcRoster roster;
    RmcPlayer *player;

    make_roster(&roster);
    player = &roster.players[8];

    /* Valid nonzero persistent progression. */
    player->training.general_progress[0] = 125;
    player->training.general_season_gain[0] = 3;
    player->training.total_season_gain = 3;
    player->talents.consolidated_mask = UINT8_C(0x03);
    player->talents.matured_mask = UINT8_C(0x04);
    player->talents.candidate_progress[2] = 90;
    player->talents.season_consolidations = 1;
    assert(rmc_roster_valid(&roster));

    make_roster(&roster);
    player = &roster.players[8];
    player->training.general_season_gain[0] = 5;
    player->training.total_season_gain = 5;
    assert(!rmc_roster_valid(&roster));

    make_roster(&roster);
    player = &roster.players[8];
    for (size_t i = 0; i < 4; ++i) {
        player->training.general_season_gain[i] = 4;
    }
    player->training.total_season_gain = 16;
    assert(!rmc_roster_valid(&roster));

    make_roster(&roster);
    player = &roster.players[8];
    player->training.general_season_gain[0] = 2;
    player->training.total_season_gain = 1;
    assert(!rmc_roster_valid(&roster));

    make_roster(&roster);
    player = &roster.players[8];
    player->training.goalkeeper_progress[0] = 1;
    assert(!rmc_roster_valid(&roster));

    make_roster(&roster);
    player = &roster.players[8];
    player->training.goalkeeper_season_gain[0] = 1;
    player->training.total_season_gain = 1;
    assert(!rmc_roster_valid(&roster));

    make_roster(&roster);
    player = &roster.players[8];
    player->talents.consolidated_mask = UINT8_C(0x0F);
    assert(!rmc_roster_valid(&roster));

    make_roster(&roster);
    player = &roster.players[8];
    player->talents.consolidated_mask = UINT8_C(0x07);
    player->talents.season_consolidations = 3;
    assert(!rmc_roster_valid(&roster));

    make_roster(&roster);
    player = &roster.players[8];
    player->talents.consolidated_mask = UINT8_C(0x01);
    player->talents.season_consolidations = 2;
    assert(!rmc_roster_valid(&roster));

    puts("RPG_VALID_PERSISTENT_STATE=PASS");
    puts("RPG_ATTRIBUTE_SEASON_LIMITS=PASS");
    puts("RPG_SEASON_TOTAL_CONSISTENCY=PASS");
    puts("RPG_GOALKEEPER_SPECIALIZATION=PASS");
    puts("RPG_TALENT_CONSOLIDATION_LIMITS=PASS");
}

int main(void)
{
    test_all_formations();
    test_secondary_compatibility();
    test_impossible_and_atomicity();
    test_invalid_rosters();
    test_progression_state();

    puts("ROSTER_QUALIFICATION=PASS");
    return 0;
}
