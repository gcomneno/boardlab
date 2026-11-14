#ifndef RMC_WORLD_H
#define RMC_WORLD_H

#include "streams.h"
#include "identity.h"
#include "roster.h"
#include "social_state.h"

#include <stdbool.h>
#include <stdint.h>

enum {
    RMC_WORLD_TEAMS = 8,
    RMC_WORLD_COACHES = 8,
    RMC_WORLD_PLAYERS = RMC_WORLD_TEAMS * RMC_TEAM_PLAYERS,
    RMC_ENTITY_NAME_CAPACITY = 48
};

/*
 * Team philosophy is distinct from coach personality.
 * These are initial M001 tactical archetypes.
 */
typedef enum {
    RMC_TACTICAL_POSSESSION = 0,
    RMC_TACTICAL_DIRECT,
    RMC_TACTICAL_COUNTER,
    RMC_TACTICAL_COUNT
} RmcTacticalProfile;

typedef struct {
    RmcTeamId id;
    RmcCoachId coach_id;
    char name[RMC_ENTITY_NAME_CAPACITY];
    RmcTacticalProfile tactical_profile;
    uint8_t cohesion;
} RmcTeam;

typedef struct {
    RmcCoachId id;
    RmcTeamId team_id;
    char name[RMC_ENTITY_NAME_CAPACITY];

    /* Four independent personality attributes: 0..100. */
    uint8_t aggression;
    uint8_t caution;
    uint8_t adaptability;
    uint8_t people_management;
} RmcCoach;

/*
 * Pre-calendar, pre-candidate world data.
 *
 * The complete career state will additionally include
 * the calendar, candidate selection and season state.
 *
 * Array indices are storage positions, not entity IDs.
 */
typedef struct {
    RmcWorldIdentity identity;
    RmcIdentityState ids;

    RmcTeam teams[RMC_WORLD_TEAMS];
    RmcCoach coaches[RMC_WORLD_COACHES];
    RmcRoster rosters[RMC_WORLD_TEAMS];
    RmcSocialState social[RMC_WORLD_TEAMS];
} RmcWorldCore;

/*
 * Generate the complete pre-calendar world core.
 *
 * Required logical stages:
 * - Team identities.
 * - Coach identities.
 * - Roster archetypes.
 * - Ages and roles.
 * - Attributes and potential.
 * - Role compatibility.
 * - Initial talents and relationships.
 * - Core validation.
 *
 * This function does not generate the calendar or
 * select the three career candidates.
 *
 * Returns false for invalid arguments or failure.
 * On failure, destination remains unchanged.
 */
bool rmc_world_core_generate(
    const RmcWorldIdentity *identity,
    RmcWorldCore *destination
);

/*
 * Validate the core entity graph, including:
 * - Supported world versions.
 * - Unique and consistent persistent identities.
 * - Eight teams, eight coaches and 144 players.
 * - Correct ownership and roster composition.
 * - Three legal formations for every team.
 * - Required compatible bench alternatives.
 * - Persistent next-ID counters.
 *
 * Calendar and candidate validation belong to
 * subsequent generation stages.
 */
bool rmc_world_core_valid(
    const RmcWorldCore *world
);

#endif
