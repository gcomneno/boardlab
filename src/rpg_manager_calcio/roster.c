#include "roster.h"
#include "../boardlab_c11/matching.h"

#include <stddef.h>
#include <stdint.h>

#define RMC_SLOT_BIT(slot) (UINT16_C(1) << (slot))
#define RMC_VALID_SLOTS ((UINT16_C(1) << RMC_SLOT_COUNT) - 1u)

static const RmcSlot formation_slots[RMC_FORMATION_COUNT]
                                    [RMC_LINEUP_PLAYERS] = {
    {
        RMC_SLOT_POR,
        RMC_SLOT_TD, RMC_SLOT_DC, RMC_SLOT_DC, RMC_SLOT_TS,
        RMC_SLOT_ED, RMC_SLOT_CC, RMC_SLOT_CC, RMC_SLOT_ES,
        RMC_SLOT_ATT, RMC_SLOT_ATT
    },
    {
        RMC_SLOT_POR,
        RMC_SLOT_TD, RMC_SLOT_DC, RMC_SLOT_DC, RMC_SLOT_TS,
        RMC_SLOT_CC, RMC_SLOT_MED, RMC_SLOT_CC,
        RMC_SLOT_AD, RMC_SLOT_ATT, RMC_SLOT_AS
    },
    {
        RMC_SLOT_POR,
        RMC_SLOT_DC, RMC_SLOT_DC, RMC_SLOT_DC,
        RMC_SLOT_ED, RMC_SLOT_CC, RMC_SLOT_MED,
        RMC_SLOT_CC, RMC_SLOT_ES,
        RMC_SLOT_ATT, RMC_SLOT_ATT
    }
};

const RmcSlot *rmc_formation_slots(RmcFormation formation)
{
    if ((unsigned)formation >= RMC_FORMATION_COUNT) {
        return NULL;
    }

    return formation_slots[(unsigned)formation];
}

static bool progression_valid(const RmcPlayer *player)
{
    const RmcTrainingState *training = &player->training;
    const RmcTalentState *talents = &player->talents;
    unsigned season_total = 0;
    unsigned consolidated_count = 0;

    for (size_t i = 0; i < RMC_GENERAL_ATTRIBUTES; ++i) {
        if (training->general_season_gain[i] >
                RMC_MAX_ATTRIBUTE_SEASON_GAIN) {
            return false;
        }
        season_total += training->general_season_gain[i];
    }

    for (size_t i = 0; i < RMC_GOALKEEPER_ATTRIBUTES; ++i) {
        if (training->goalkeeper_season_gain[i] >
                RMC_MAX_ATTRIBUTE_SEASON_GAIN) {
            return false;
        }

        if (player->department != RMC_DEPARTMENT_GOALKEEPER &&
            (training->goalkeeper_season_gain[i] != 0 ||
             training->goalkeeper_progress[i] != 0)) {
            return false;
        }

        season_total += training->goalkeeper_season_gain[i];
    }

    if (season_total > RMC_MAX_PLAYER_SEASON_GAIN ||
        season_total != training->total_season_gain ||
        talents->season_consolidations >
            RMC_MAX_SEASON_CONSOLIDATIONS) {
        return false;
    }

    for (unsigned i = 0; i < RMC_TALENT_COUNT; ++i) {
        if ((talents->consolidated_mask &
             (uint8_t)(UINT8_C(1) << i)) != 0) {
            ++consolidated_count;
        }
    }

    return consolidated_count <= RMC_MAX_CONSOLIDATED_TALENTS &&
           talents->season_consolidations <= consolidated_count;
}

static bool player_valid(
    const RmcPlayer *player,
    uint64_t team_id)
{
    if (player->id.value == 0 ||
        player->team_id.value != team_id ||
        (unsigned)player->department >
            RMC_DEPARTMENT_ATTACKER ||
        player->primary_slots == 0 ||
        ((player->primary_slots | player->secondary_slots) &
         (uint16_t)~RMC_VALID_SLOTS) != 0 ||
        (player->primary_slots & player->secondary_slots) != 0 ||
        player->energy > 100 ||
        player->morale > 100 ||
        player->level < 1 ||
        player->level > 20 ||
        !progression_valid(player)) {
        return false;
    }

    const uint16_t all_slots =
        player->primary_slots | player->secondary_slots;

    if (player->department == RMC_DEPARTMENT_GOALKEEPER) {
        if (all_slots != RMC_SLOT_BIT(RMC_SLOT_POR)) {
            return false;
        }

        for (size_t i = 0; i < RMC_GOALKEEPER_ATTRIBUTES; ++i) {
            if (player->goalkeeper_attributes[i] < 1 ||
                player->goalkeeper_attributes[i] > 100) {
                return false;
            }
        }
    } else {
        if ((all_slots & RMC_SLOT_BIT(RMC_SLOT_POR)) != 0) {
            return false;
        }
    }

    for (size_t i = 0; i < RMC_GENERAL_ATTRIBUTES; ++i) {
        if (player->attributes[i] < 1 ||
            player->attributes[i] > 100) {
            return false;
        }
    }

    return true;
}

bool rmc_roster_valid(const RmcRoster *roster)
{
    if (roster == NULL || roster->team_id.value == 0) {
        return false;
    }

    unsigned counts[4] = {0};

    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        const RmcPlayer *player = &roster->players[i];

        if (!player_valid(player, roster->team_id.value)) {
            return false;
        }

        ++counts[(unsigned)player->department];

        for (size_t j = 0; j < i; ++j) {
            if (player->id.value == roster->players[j].id.value) {
                return false;
            }
        }
    }

    return counts[RMC_DEPARTMENT_GOALKEEPER] == RMC_GOALKEEPERS &&
           counts[RMC_DEPARTMENT_DEFENDER] == RMC_DEFENDERS &&
           counts[RMC_DEPARTMENT_MIDFIELDER] == RMC_MIDFIELDERS &&
           counts[RMC_DEPARTMENT_ATTACKER] == RMC_ATTACKERS;
}

bool rmc_roster_lineup(
    const RmcRoster *roster,
    RmcFormation formation,
    RmcLineup *output)
{
    if (output == NULL || !rmc_roster_valid(roster)) {
        return false;
    }

    const RmcSlot *slots = rmc_formation_slots(formation);

    if (slots == NULL) {
        return false;
    }

    uint8_t edges[RMC_LINEUP_PLAYERS][RMC_TEAM_PLAYERS] = {{0}};
    size_t assignment[RMC_LINEUP_PLAYERS];

    for (size_t slot = 0; slot < RMC_LINEUP_PLAYERS; ++slot) {
        const uint16_t bit = RMC_SLOT_BIT(slots[slot]);

        for (size_t player = 0; player < RMC_TEAM_PLAYERS; ++player) {
            const RmcPlayer *candidate = &roster->players[player];

            if (((candidate->primary_slots |
                  candidate->secondary_slots) & bit) != 0) {
                edges[slot][player] = 1;
            }
        }
    }

    if (!boardlab_bipartite_match(
            &edges[0][0],
            RMC_LINEUP_PLAYERS,
            RMC_TEAM_PLAYERS,
            assignment)) {
        return false;
    }

    RmcLineup next = {0};
    next.formation = formation;

    for (size_t slot = 0; slot < RMC_LINEUP_PLAYERS; ++slot) {
        const RmcPlayer *player =
            &roster->players[assignment[slot]];

        next.players[slot] = player->id;
        next.slots[slot] = slots[slot];

        if ((player->primary_slots &
             RMC_SLOT_BIT(slots[slot])) == 0) {
            ++next.secondary_assignments;
        }
    }

    *output = next;
    return true;
}
