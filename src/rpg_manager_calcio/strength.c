#include "strength.h"

#include "../boardlab_c11/weighted_matching.h"

#include <stddef.h>
#include <stdint.h>

#define RMC_STRENGTH_SLOT_BIT(slot) (UINT16_C(1) << (slot))

/*
 * Canonical v1 attribute weights:
 * 30%, 25%, 25%, 20%.
 *
 * Outfield slots use general attributes.
 * Goalkeepers use the four specialist attributes.
 */
static const RmcAttribute slot_attributes[RMC_SLOT_COUNT][4] = {
    [RMC_SLOT_POR] = {
        RMC_ATTR_POSIZIONAMENTO, RMC_ATTR_POSIZIONAMENTO,
        RMC_ATTR_POSIZIONAMENTO, RMC_ATTR_POSIZIONAMENTO
    },
    [RMC_SLOT_TD] = {
        RMC_ATTR_CONTRASTO, RMC_ATTR_VELOCITA,
        RMC_ATTR_MARCATURA, RMC_ATTR_PASSAGGIO
    },
    [RMC_SLOT_DC] = {
        RMC_ATTR_MARCATURA, RMC_ATTR_CONTRASTO,
        RMC_ATTR_FORZA, RMC_ATTR_POSIZIONAMENTO
    },
    [RMC_SLOT_TS] = {
        RMC_ATTR_CONTRASTO, RMC_ATTR_VELOCITA,
        RMC_ATTR_MARCATURA, RMC_ATTR_PASSAGGIO
    },
    [RMC_SLOT_ED] = {
        RMC_ATTR_VELOCITA, RMC_ATTR_CROSS,
        RMC_ATTR_RESISTENZA, RMC_ATTR_CONTROLLO
    },
    [RMC_SLOT_CC] = {
        RMC_ATTR_PASSAGGIO, RMC_ATTR_VISIONE,
        RMC_ATTR_CONTROLLO, RMC_ATTR_POSIZIONAMENTO
    },
    [RMC_SLOT_ES] = {
        RMC_ATTR_VELOCITA, RMC_ATTR_CROSS,
        RMC_ATTR_RESISTENZA, RMC_ATTR_CONTROLLO
    },
    [RMC_SLOT_ATT] = {
        RMC_ATTR_TIRO, RMC_ATTR_POSIZIONAMENTO,
        RMC_ATTR_CONTROLLO, RMC_ATTR_VELOCITA
    },
    [RMC_SLOT_MED] = {
        RMC_ATTR_POSIZIONAMENTO, RMC_ATTR_CONTRASTO,
        RMC_ATTR_MARCATURA, RMC_ATTR_PASSAGGIO
    },
    [RMC_SLOT_AD] = {
        RMC_ATTR_DRIBBLING, RMC_ATTR_VELOCITA,
        RMC_ATTR_TIRO, RMC_ATTR_CONTROLLO
    },
    [RMC_SLOT_AS] = {
        RMC_ATTR_DRIBBLING, RMC_ATTR_VELOCITA,
        RMC_ATTR_TIRO, RMC_ATTR_CONTROLLO
    }
};

static const RmcGoalkeeperAttribute goalkeeper_attributes[4] = {
    RMC_GK_ATTR_RIFLESSI,
    RMC_GK_ATTR_POSIZIONAMENTO,
    RMC_GK_ATTR_PRESA,
    RMC_GK_ATTR_USCITE
};

static const unsigned attribute_weights[4] = {
    30u, 25u, 25u, 20u
};

static uint32_t attribute_effectiveness(
    const RmcPlayer *player,
    RmcSlot slot)
{
    uint32_t weighted_sum = 0;

    for (size_t i = 0; i < 4; ++i) {
        const uint32_t value =
            slot == RMC_SLOT_POR
                ? player->goalkeeper_attributes[
                    goalkeeper_attributes[i]]
                : player->attributes[slot_attributes[slot][i]];

        weighted_sum += attribute_weights[i] * value;
    }

    return weighted_sum / 100u;
}

static uint32_t slot_effectiveness(
    const RmcWorldCore *world,
    size_t team_index,
    const RmcPlayer *player,
    RmcSlot slot,
    bool primary)
{
    const RmcTeam *team = &world->teams[team_index];
    const RmcSocialState *social = &world->social[team_index];

    const uint32_t attributes =
        attribute_effectiveness(player, slot);

    const uint32_t condition =
        ((uint32_t)player->energy + player->fitness) / 2u;

    const uint32_t organization =
        ((uint32_t)team->cohesion +
         social->tactical_familiarity +
         social->lineup_continuity +
         social->department_chemistry[player->department]) / 4u;

    uint32_t effectiveness =
        (70u * attributes +
         15u * condition +
         15u * organization) / 100u;

    if (!primary) {
        effectiveness =
            effectiveness > 10u ? effectiveness - 10u : 0u;
    }

    return effectiveness;
}

bool rmc_formation_strength(
    const RmcWorldCore *world,
    size_t team_index,
    RmcFormation formation,
    RmcFormationStrength *destination)
{
    if (world == NULL ||
        destination == NULL ||
        team_index >= RMC_WORLD_TEAMS ||
        !rmc_world_core_valid(world)) {
        return false;
    }

    const RmcSlot *slots = rmc_formation_slots(formation);

    if (slots == NULL) {
        return false;
    }

    const RmcRoster *roster = &world->rosters[team_index];

    uint8_t compatibility[RMC_LINEUP_PLAYERS][RMC_TEAM_PLAYERS]
        = {{0}};
    uint32_t scores[RMC_LINEUP_PLAYERS][RMC_TEAM_PLAYERS]
        = {{0}};

    for (size_t slot_index = 0;
         slot_index < RMC_LINEUP_PLAYERS;
         ++slot_index) {
        const RmcSlot slot = slots[slot_index];
        const uint16_t bit = RMC_STRENGTH_SLOT_BIT(slot);

        for (size_t player_index = 0;
             player_index < RMC_TEAM_PLAYERS;
             ++player_index) {
            const RmcPlayer *player =
                &roster->players[player_index];

            const bool primary =
                (player->primary_slots & bit) != 0;

            const bool secondary =
                (player->secondary_slots & bit) != 0;

            if (!primary && !secondary) {
                continue;
            }

            compatibility[slot_index][player_index] = 1;

            scores[slot_index][player_index] =
                slot_effectiveness(
                    world, team_index, player, slot, primary
                );
        }
    }

    size_t assignment[RMC_LINEUP_PLAYERS];
    uint64_t total_score = 0;

    if (!boardlab_weighted_match(
            &compatibility[0][0],
            &scores[0][0],
            RMC_LINEUP_PLAYERS,
            RMC_TEAM_PLAYERS,
            assignment,
            &total_score)) {
        return false;
    }

    RmcFormationStrength next = {0};
    next.formation = formation;
    next.lineup.formation = formation;
    next.total_score = total_score;

    for (size_t i = 0; i < RMC_LINEUP_PLAYERS; ++i) {
        const RmcPlayer *player =
            &roster->players[assignment[i]];
        const RmcSlot slot = slots[i];

        next.lineup.players[i] = player->id;
        next.lineup.slots[i] = slot;

        if ((player->primary_slots &
             RMC_STRENGTH_SLOT_BIT(slot)) == 0) {
            ++next.lineup.secondary_assignments;
        }
    }

    *destination = next;
    return true;
}

bool rmc_team_strength(
    const RmcWorldCore *world,
    size_t team_index,
    RmcTeamStrength *destination)
{
    if (world == NULL ||
        destination == NULL ||
        team_index >= RMC_WORLD_TEAMS ||
        !rmc_world_core_valid(world)) {
        return false;
    }

    RmcTeamStrength next = {0};
    next.team_id = world->teams[team_index].id;

    bool found = false;

    for (size_t f = 0; f < RMC_FORMATION_COUNT; ++f) {
        RmcFormationStrength current;

        if (!rmc_formation_strength(
                world, team_index,
                (RmcFormation)f, &current)) {
            return false;
        }

        next.formation_scores[f] = current.total_score;

        if (!found ||
            current.total_score > next.best.total_score) {
            next.best = current;
            found = true;
        }
    }

    if (!found) {
        return false;
    }

    *destination = next;
    return true;
}
