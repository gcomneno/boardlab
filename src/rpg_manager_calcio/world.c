#include "world.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

static bool valid_name(const char name[RMC_ENTITY_NAME_CAPACITY])
{
    if (name[0] == '\0') {
        return false;
    }

    for (size_t i = 0; i < RMC_ENTITY_NAME_CAPACITY; ++i) {
        if (name[i] == '\0') {
            return true;
        }
    }

    return false;
}

static bool valid_bench(
    const RmcRoster *roster,
    const RmcLineup *lineup)
{
    unsigned goalkeeper_reserves = 0;
    unsigned movement_alternatives = 0;

    for (size_t i = 0; i < RMC_TEAM_PLAYERS; ++i) {
        const RmcPlayer *player = &roster->players[i];
        bool starter = false;

        for (size_t j = 0; j < RMC_LINEUP_PLAYERS; ++j) {
            if (player->id.value == lineup->players[j].value) {
                starter = true;
                break;
            }
        }

        if (starter) {
            continue;
        }

        if (player->department == RMC_DEPARTMENT_GOALKEEPER) {
            ++goalkeeper_reserves;
            continue;
        }

        const uint16_t compatible =
            player->primary_slots | player->secondary_slots;

        for (size_t j = 0; j < RMC_LINEUP_PLAYERS; ++j) {
            const uint16_t bit =
                (uint16_t)(UINT16_C(1) << lineup->slots[j]);

            if ((compatible & bit) != 0) {
                ++movement_alternatives;
                break;
            }
        }
    }

    return goalkeeper_reserves >= 1 &&
           movement_alternatives >= 2;
}

static bool valid_roster_formations(const RmcRoster *roster)
{
    if (!rmc_roster_valid(roster)) {
        return false;
    }

    for (unsigned f = 0; f < RMC_FORMATION_COUNT; ++f) {
        RmcLineup lineup;

        if (!rmc_roster_lineup(
                roster, (RmcFormation)f, &lineup)) {
            return false;
        }

        if (!valid_bench(roster, &lineup)) {
            return false;
        }
    }

    return true;
}

static bool valid_social_state(
    const RmcSocialState *social,
    const RmcRoster *roster)
{
    if (social->team_id.value != roster->team_id.value ||
        social->tactical_familiarity > 100 ||
        social->lineup_continuity > 100 ||
        social->pair_count == 0 ||
        social->pair_count > RMC_MAX_CHEMISTRY_PAIRS) {
        return false;
    }

    for (size_t d = 0; d < RMC_SOCIAL_DEPARTMENTS; ++d) {
        if (social->department_chemistry[d] > 100) {
            return false;
        }
    }

    for (size_t i = 0; i < RMC_MAX_CHEMISTRY_PAIRS; ++i) {
        const RmcChemistryPair *pair = &social->pairs[i];

        if (i >= social->pair_count) {
            if (pair->first.value != 0 ||
                pair->second.value != 0 ||
                pair->chemistry != 0) {
                return false;
            }
            continue;
        }

        if (pair->first.value == 0 ||
            pair->first.value >= pair->second.value ||
            pair->chemistry > 100) {
            return false;
        }

        bool first_found = false;
        bool second_found = false;

        for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
            const uint64_t player_id = roster->players[p].id.value;

            if (player_id == pair->first.value) {
                first_found = true;
            }
            if (player_id == pair->second.value) {
                second_found = true;
            }
        }

        if (!first_found || !second_found) {
            return false;
        }

        if (i > 0) {
            const RmcChemistryPair *previous = &social->pairs[i - 1];

            if (previous->first.value > pair->first.value ||
                (previous->first.value == pair->first.value &&
                 previous->second.value >= pair->second.value)) {
                return false;
            }
        }
    }

    return true;
}

bool rmc_world_core_valid(const RmcWorldCore *world)
{
    if (world == NULL ||
        world->identity.generator_version !=
            RMC_GENERATOR_VERSION ||
        world->identity.rules_version != RMC_RULES_VERSION ||
        !rmc_identity_valid(&world->ids)) {
        return false;
    }

    uint64_t max_team = 0;
    uint64_t max_coach = 0;
    uint64_t max_player = 0;

    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        const RmcTeam *team = &world->teams[i];
        const RmcCoach *coach = &world->coaches[i];
        const RmcRoster *roster = &world->rosters[i];

        if (team->id.value == 0 ||
            coach->id.value == 0 ||
            team->coach_id.value != coach->id.value ||
            coach->team_id.value != team->id.value ||
            roster->team_id.value != team->id.value ||
            !valid_name(team->name) ||
            !valid_name(coach->name) ||
            (unsigned)team->tactical_profile >=
                RMC_TACTICAL_COUNT ||
            team->cohesion > 100 ||
            coach->aggression > 100 ||
            coach->caution > 100 ||
            coach->adaptability > 100 ||
            coach->people_management > 100 ||
            !valid_roster_formations(roster) ||
            !valid_social_state(&world->social[i], roster)) {
            return false;
        }

        if (team->id.value > max_team) {
            max_team = team->id.value;
        }

        if (coach->id.value > max_coach) {
            max_coach = coach->id.value;
        }

        for (size_t j = 0; j < i; ++j) {
            if (world->teams[j].id.value == team->id.value ||
                world->coaches[j].id.value == coach->id.value) {
                return false;
            }
        }

        for (size_t p = 0; p < RMC_TEAM_PLAYERS; ++p) {
            const uint64_t id = roster->players[p].id.value;

            if (id > max_player) {
                max_player = id;
            }

            for (size_t previous_team = 0;
                 previous_team < i;
                 ++previous_team) {
                for (size_t previous_player = 0;
                     previous_player < RMC_TEAM_PLAYERS;
                     ++previous_player) {
                    if (world->rosters[previous_team]
                            .players[previous_player].id.value == id) {
                        return false;
                    }
                }
            }
        }
    }

    /*
     * Persisted next-ID values must be greater
     * than all existing identifiers of their kind.
     */
    return world->ids.counters[RMC_ENTITY_TEAM].next >
               max_team &&
           world->ids.counters[RMC_ENTITY_COACH].next >
               max_coach &&
           world->ids.counters[RMC_ENTITY_PLAYER].next >
               max_player;
}

/*
 * Deterministic generator v1, initial world-core implementation.
 *
 * Talents, training progress and relationships are not yet
 * represented by RmcPlayer. Their initialization is deferred
 * until the corresponding persistent model is implemented.
 */

static const RmcDepartment generator_departments[RMC_TEAM_PLAYERS] = {
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

static const RmcSlot generator_slots[RMC_TEAM_PLAYERS] = {
    RMC_SLOT_POR, RMC_SLOT_POR,
    RMC_SLOT_TD, RMC_SLOT_DC, RMC_SLOT_DC,
    RMC_SLOT_TS, RMC_SLOT_DC,
    RMC_SLOT_ED, RMC_SLOT_CC, RMC_SLOT_CC,
    RMC_SLOT_ES, RMC_SLOT_MED, RMC_SLOT_CC,
    RMC_SLOT_CC,
    RMC_SLOT_ATT, RMC_SLOT_ATT,
    RMC_SLOT_AD, RMC_SLOT_AS
};

static bool generator_range(
    RmcPcg32 *rng,
    uint32_t minimum,
    uint32_t maximum,
    uint8_t *destination)
{
    uint32_t value = 0;

    if (!rmc_pcg32_bounded(
            rng, maximum - minimum + UINT32_C(1), &value)) {
        return false;
    }

    *destination = (uint8_t)(minimum + value);
    return true;
}

static bool generator_coach(
    RmcWorldCore *world,
    size_t index,
    RmcPcg32 *rng)
{
    RmcCoach *coach = &world->coaches[index];
    RmcTeam *team = &world->teams[index];
    uint64_t id = 0;

    if (!rmc_identity_next(
            &world->ids, RMC_ENTITY_COACH, &id)) {
        return false;
    }

    coach->id.value = id;
    coach->team_id = team->id;
    team->coach_id = coach->id;

    if (snprintf(coach->name, sizeof(coach->name),
                 "Allenatore %u", (unsigned)(index + 1)) <= 0) {
        return false;
    }

    return generator_range(rng, 20, 80, &coach->aggression) &&
           generator_range(rng, 20, 80, &coach->caution) &&
           generator_range(rng, 20, 80, &coach->adaptability) &&
           generator_range(rng, 20, 80, &coach->people_management);
}

static bool generator_player(
    RmcWorldCore *world,
    size_t team_index,
    size_t player_index,
    RmcPcg32 *rng)
{
    RmcRoster *roster = &world->rosters[team_index];
    RmcPlayer *player = &roster->players[player_index];
    uint64_t id = 0;

    if (!rmc_identity_next(
            &world->ids, RMC_ENTITY_PLAYER, &id)) {
        return false;
    }

    player->id.value = id;
    player->team_id = roster->team_id;
    player->department = generator_departments[player_index];
    player->primary_slots =
        (uint16_t)(UINT16_C(1) << generator_slots[player_index]);

    if (!generator_range(rng, 18, 34, &player->age) ||
        !generator_range(rng, 70, 100, &player->energy) ||
        !generator_range(rng, 40, 80, &player->morale) ||
        !generator_range(rng, 75, 100, &player->fitness) ||
        !generator_range(rng, 55, 95, &player->potential)) {
        return false;
    }

    player->level = 1;
    player->experience = 0;

    for (size_t a = 0; a < RMC_GENERAL_ATTRIBUTES; ++a) {
        if (!generator_range(
                rng, 35, 75, &player->attributes[a])) {
            return false;
        }
    }

    if (player->department == RMC_DEPARTMENT_GOALKEEPER) {
        for (size_t a = 0; a < RMC_GOALKEEPER_ATTRIBUTES; ++a) {
            if (!generator_range(
                    rng, 35, 75, &player->goalkeeper_attributes[a])) {
                return false;
            }
        }
    }

    return true;
}

static bool generator_social(
    RmcWorldCore *world,
    size_t team_index,
    RmcPcg32 *rng)
{
    static const size_t pair_indices[3][2] = {
        {2, 3},
        {7, 8},
        {14, 15}
    };

    RmcSocialState *social = &world->social[team_index];
    const RmcRoster *roster = &world->rosters[team_index];

    social->team_id = roster->team_id;

    if (!generator_range(
            rng, 40, 80, &social->tactical_familiarity) ||
        !generator_range(
            rng, 40, 80, &social->lineup_continuity)) {
        return false;
    }

    for (size_t d = 0; d < RMC_SOCIAL_DEPARTMENTS; ++d) {
        if (!generator_range(
                rng, 40, 80, &social->department_chemistry[d])) {
            return false;
        }
    }

    social->pair_count = 3;

    for (size_t i = 0; i < social->pair_count; ++i) {
        RmcChemistryPair *pair = &social->pairs[i];

        pair->first = roster->players[pair_indices[i][0]].id;
        pair->second = roster->players[pair_indices[i][1]].id;

        if (!generator_range(rng, 40, 80, &pair->chemistry)) {
            return false;
        }
    }

    return true;
}

bool rmc_world_core_generate(
    const RmcWorldIdentity *identity,
    RmcWorldCore *destination)
{
    if (identity == NULL || destination == NULL) {
        return false;
    }

    RmcPcg32 world_rng;
    RmcPcg32 attributes_rng;

    if (!rmc_stream_init(
            &world_rng, identity, RMC_STREAM_WORLD, 0) ||
        !rmc_stream_init(
            &attributes_rng, identity, RMC_STREAM_ATTRIBUTES, 0)) {
        return false;
    }

    RmcWorldCore next = {0};
    next.identity = *identity;
    rmc_identity_init(&next.ids);

    /*
     * Phase 1: team identities.
     */
    for (size_t i = 0; i < RMC_WORLD_TEAMS; ++i) {
        RmcTeam *team = &next.teams[i];
        uint64_t id = 0;

        if (!rmc_identity_next(
                &next.ids, RMC_ENTITY_TEAM, &id)) {
            return false;
        }

        team->id.value = id;

        if (snprintf(team->name, sizeof(team->name),
                     "Club %u", (unsigned)(i + 1)) <= 0) {
            return false;
        }

        uint32_t profile = 0;

        if (!rmc_pcg32_bounded(
                &world_rng, RMC_TACTICAL_COUNT, &profile) ||
            !generator_range(
                &world_rng, 40, 80, &team->cohesion)) {
            return false;
        }

        team->tactical_profile = (RmcTacticalProfile)profile;
        next.rosters[i].team_id = team->id;
    }

    /*
     * Phase 2: coach identities and personalities.
     */
    for (size_t i = 0; i < RMC_WORLD_COACHES; ++i) {
        if (!generator_coach(&next, i, &world_rng)) {
            return false;
        }
    }

    /*
     * Phases 3-6: initial roster archetypes,
     * ages, roles, attributes and compatibility.
     */
    for (size_t team = 0; team < RMC_WORLD_TEAMS; ++team) {
        for (size_t player = 0; player < RMC_TEAM_PLAYERS; ++player) {
            if (!generator_player(
                    &next, team, player, &attributes_rng)) {
                return false;
            }
        }
    }

    /*
     * Phase 7: initial simplified team relationships.
     */
    for (size_t team = 0; team < RMC_WORLD_TEAMS; ++team) {
        if (!generator_social(&next, team, &world_rng)) {
            return false;
        }
    }

    /*
     * Phase 8: validate before publication.
     * Phase 7 (talents and relationships) is pending.
     * Calendar and candidate selection are separate stages.
     */
    if (!rmc_world_core_valid(&next)) {
        return false;
    }

    *destination = next;
    return true;
}
