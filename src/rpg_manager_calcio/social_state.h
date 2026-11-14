#ifndef RMC_SOCIAL_STATE_H
#define RMC_SOCIAL_STATE_H

#include "identity.h"

#include <stdint.h>

enum {
    RMC_SOCIAL_DEPARTMENTS = 4,
    RMC_MAX_CHEMISTRY_PAIRS = 8
};

/*
 * A relevant player pair, not an edge in a
 * general-purpose social graph.
 *
 * Player IDs must be distinct and belong to the team.
 * Canonical ordering requires first.value < second.value.
 */
typedef struct {
    RmcPlayerId first;
    RmcPlayerId second;
    uint8_t chemistry;
} RmcChemistryPair;

/*
 * Persistent simplified dressing-room cohesion.
 *
 * Department order follows RmcDepartment:
 * goalkeeper, defender, midfielder, attacker.
 *
 * All indicators are in [0, 100].
 *
 * Only the first pair_count entries are active.
 * Unused entries must remain zero-initialized.
 */
typedef struct {
    RmcTeamId team_id;
    uint8_t tactical_familiarity;
    uint8_t lineup_continuity;
    uint8_t department_chemistry[RMC_SOCIAL_DEPARTMENTS];

    uint8_t pair_count;
    RmcChemistryPair pairs[RMC_MAX_CHEMISTRY_PAIRS];
} RmcSocialState;

#endif
