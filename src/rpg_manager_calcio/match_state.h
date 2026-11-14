#ifndef RMC_MATCH_STATE_H
#define RMC_MATCH_STATE_H

#include "calendar.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * M001-B — Match State Core.
 *
 * Match state is football-specific and independent
 * from the CLI, artificial intelligence and storage.
 *
 * The World Core remains authoritative for player
 * identities, attributes and roster membership.
 */

typedef enum {
    RMC_MATCH_NOT_STARTED = 0,
    RMC_MATCH_FIRST_HALF,
    RMC_MATCH_HALF_TIME,
    RMC_MATCH_SECOND_HALF,
    RMC_MATCH_FINISHED,
    RMC_MATCH_PERIOD_COUNT
} RmcMatchPeriod;

typedef enum {
    RMC_MATCH_BALL_IN_PLAY = 0,
    RMC_MATCH_RESTART_PENDING,
    RMC_MATCH_MANAGEMENT_WINDOW,
    RMC_MATCH_PERIOD_ENDED,
    RMC_MATCH_MODE_COUNT
} RmcMatchMode;

typedef enum {
    RMC_PHASE_BUILDUP = 0,
    RMC_PHASE_PROGRESSION,
    RMC_PHASE_ATTACKING_DEVELOPMENT,
    RMC_PHASE_CHANCE,
    RMC_PHASE_TRANSITION,
    RMC_PHASE_COUNT
} RmcPossessionPhase;

typedef enum {
    RMC_LANE_LEFT = 0,
    RMC_LANE_CENTRE,
    RMC_LANE_RIGHT,
    RMC_LANE_COUNT
} RmcTacticalLane;

enum {
    RMC_FIELD_SECTORS = 5,
    RMC_FIELD_ZONES = RMC_FIELD_SECTORS * RMC_LANE_COUNT
};

/*
 * Coordinates are relative to the attacking
 * direction of the team in possession.
 *
 * A turnover transforms the longitudinal reference.
 * It does not automatically exchange left and right.
 */
typedef struct {
    uint8_t sector;
    RmcTacticalLane lane;
} RmcTacticalZone;

typedef enum {
    RMC_MATCH_TERMINAL_NONE = 0,
    RMC_MATCH_TERMINAL_COMPLETED,
    RMC_MATCH_TERMINAL_SUSPENDED_INSUFFICIENT_PLAYERS,
    RMC_MATCH_TERMINAL_COUNT
} RmcMatchTerminalReason;

/*
 * Active possession context.
 *
 * team_id.value == 0 denotes no active possession.
 * controller.value == 0 denotes an unassigned ball.
 *
 * Counters reset on a newly established possession.
 */
typedef struct {
    RmcTeamId team_id;
    RmcTacticalZone zone;
    RmcPossessionPhase phase;
    RmcPlayerId initiator;
    RmcPlayerId controller;
    uint32_t action_count;
    uint32_t elapsed_seconds;
} RmcMatchPossession;

/*
 * Initial match-state representation.
 *
 * The fixture preserves the existing MatchId.
 * Lineups reference existing players; no roster
 * or World Core is duplicated.
 *
 * elapsed_seconds is the nondecreasing match clock.
 * Stoppage-time policy belongs to a later increment.
 *
 * No persistent action or event log is defined here.
 */
typedef struct {
    RmcFixture fixture;

    RmcLineup home_lineup;
    RmcLineup away_lineup;

    RmcMatchPeriod period;
    RmcMatchMode mode;
    RmcMatchTerminalReason terminal_reason;

    uint32_t elapsed_seconds;

    /* Independent, persistent stoppage accumulators. */
    uint32_t second_half_start_second;
    uint32_t first_half_stoppage_seconds;
    uint32_t second_half_stoppage_seconds;

    uint16_t home_goals;
    uint16_t away_goals;

    RmcMatchPossession possession;
} RmcMatchState;

/*
 * Construct the initial, not-started state from an
 * existing fixture and two complete legal lineups.
 *
 * Validation must use the authoritative World Core.
 * No MatchId allocation or PRNG consumption.
 *
 * On failure, destination remains unchanged.
 */
bool rmc_match_state_init(
    const RmcWorldCore *world,
    const RmcFixture *fixture,
    const RmcLineup *home_lineup,
    const RmcLineup *away_lineup,
    RmcMatchState *destination
);

/*
 * Validate identities, lineups, tactical coordinates,
 * period/mode compatibility and temporal invariants.
 *
 * Additional gameplay invariants will be introduced
 * as the match rules and events are implemented.
 */
bool rmc_match_state_valid(
    const RmcWorldCore *world,
    const RmcMatchState *state
);

#endif
