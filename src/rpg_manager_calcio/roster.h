#ifndef RMC_ROSTER_H
#define RMC_ROSTER_H

#include "identity.h"
#include "progress_state.h"
#include "attributes.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    RMC_TEAM_PLAYERS = 18,
    RMC_LINEUP_PLAYERS = 11,
    RMC_GOALKEEPERS = 2,
    RMC_DEFENDERS = 5,
    RMC_MIDFIELDERS = 7,
    RMC_ATTACKERS = 4,
    RMC_GENERAL_ATTRIBUTES = 16,
    RMC_GOALKEEPER_ATTRIBUTES = 4
};

/*
 * Persistent roster arrays must match the canonical
 * attribute catalog without changing their layout.
 */
_Static_assert(
    (int)RMC_GENERAL_ATTRIBUTES == (int)RMC_ATTR_COUNT,
    "General attribute catalog mismatch"
);

_Static_assert(
    (int)RMC_GOALKEEPER_ATTRIBUTES == (int)RMC_GK_ATTR_COUNT,
    "Goalkeeper attribute catalog mismatch"
);

typedef enum {
    RMC_DEPARTMENT_GOALKEEPER = 0,
    RMC_DEPARTMENT_DEFENDER,
    RMC_DEPARTMENT_MIDFIELDER,
    RMC_DEPARTMENT_ATTACKER
} RmcDepartment;

typedef enum {
    RMC_SLOT_POR = 0,
    RMC_SLOT_TD,
    RMC_SLOT_DC,
    RMC_SLOT_TS,
    RMC_SLOT_ED,
    RMC_SLOT_CC,
    RMC_SLOT_ES,
    RMC_SLOT_ATT,
    RMC_SLOT_MED,
    RMC_SLOT_AD,
    RMC_SLOT_AS,
    RMC_SLOT_COUNT
} RmcSlot;

typedef enum {
    RMC_FORMATION_442 = 0,
    RMC_FORMATION_433,
    RMC_FORMATION_352,
    RMC_FORMATION_COUNT
} RmcFormation;

typedef struct {
    RmcPlayerId id;
    RmcTeamId team_id;
    RmcDepartment department;
    uint16_t primary_slots;
    uint16_t secondary_slots;
    uint8_t attributes[RMC_GENERAL_ATTRIBUTES];
    uint8_t goalkeeper_attributes[RMC_GOALKEEPER_ATTRIBUTES];
    uint8_t age;
    uint8_t energy;
    uint8_t morale;
    uint8_t fitness;
    uint8_t level;
    uint32_t experience;
    uint8_t potential;
    RmcTrainingState training;
    RmcTalentState talents;
} RmcPlayer;

typedef struct {
    RmcTeamId team_id;
    RmcPlayer players[RMC_TEAM_PLAYERS];
} RmcRoster;

typedef struct {
    RmcFormation formation;
    RmcPlayerId players[RMC_LINEUP_PLAYERS];
    RmcSlot slots[RMC_LINEUP_PLAYERS];
    unsigned secondary_assignments;
} RmcLineup;

/*
 * Return the canonical 11 slot types in formation order.
 * The result is static and must not be modified.
 */
const RmcSlot *rmc_formation_slots(RmcFormation formation);

/*
 * Determine whether a complete legal lineup exists.
 *
 * Uses BoardLab C11 bipartite matching.
 * Returns false for invalid rosters, unsupported
 * formations or impossible assignments.
 *
 * On failure, output remains unchanged.
 */
bool rmc_roster_lineup(
    const RmcRoster *roster,
    RmcFormation formation,
    RmcLineup *output
);

/*
 * Validate the initial roster composition and
 * individual compatibility declarations.
 */
bool rmc_roster_valid(const RmcRoster *roster);

#endif
