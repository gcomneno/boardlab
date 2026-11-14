#ifndef RMC_MATCH_EVENT_LOG_H
#define RMC_MATCH_EVENT_LOG_H

#include "match_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * M001-B — Structured Event Log v2.
 *
 * Event storage is caller-owned, bounded and append-only.
 * IDs are consecutive, beginning at 1.
 *
 * The log does not resolve actions, mutate match state,
 * allocate memory or consume PRNG.
 *
 * Shot v1 payloads are standardized; resolution, statistics and
 * the canonical replay hash belong to later increments.
 */

#define RMC_EVENT_SCHEMA_VERSION UINT32_C(2)
#define RMC_SHOT_MODEL_VERSION UINT32_C(1)

typedef enum {
    RMC_EVENT_PERIOD_START = 0,
    RMC_EVENT_POSSESSION_START,
    RMC_EVENT_ACTION,
    RMC_EVENT_RECOVERY,
    RMC_EVENT_FOUL,
    RMC_EVENT_YELLOW_CARD,
    RMC_EVENT_RED_CARD,
    RMC_EVENT_OFFSIDE,
    RMC_EVENT_RESTART,
    RMC_EVENT_SHOT,
    RMC_EVENT_GOAL,
    RMC_EVENT_SUBSTITUTION,
    RMC_EVENT_TACTICAL_DECISION,
    RMC_EVENT_PERIOD_END,
    RMC_EVENT_MATCH_END,
    RMC_EVENT_TYPE_COUNT
} RmcMatchEventType;

typedef enum {
    RMC_SHOT_CHANCE = 500,
    RMC_SHOT_GOAL = 501,
    RMC_SHOT_SAVED_HELD = 502,
    RMC_SHOT_SAVED_REBOUND = 503,
    RMC_SHOT_BLOCKED = 504,
    RMC_SHOT_WIDE = 505,
    RMC_SHOT_WOODWORK = 506
} RmcShotEventOutcome;

typedef enum {
    RMC_SHOT_TRAJECTORY_NONE = 0,
    RMC_SHOT_TRAJECTORY_ON_TARGET = 1,
    RMC_SHOT_TRAJECTORY_WIDE = 2,
    RMC_SHOT_TRAJECTORY_WOODWORK = 3
} RmcShotTrajectory;

typedef enum {
    RMC_RESTART_NONE = 0,
    RMC_RESTART_GOAL_KICK = 1,
    RMC_RESTART_KICKOFF = 2
} RmcRestartKind;

typedef enum {
    RMC_RESTART_DISPOSITION_NONE = 0,
    RMC_RESTART_PENDING = 1,
    RMC_RESTART_SUPPRESSED_PERIOD_END = 2
} RmcRestartDisposition;

/*
 * model_version is the tag: zero means absence and requires every
 * field to be neutral (zero). Only model 1 is currently supported.
 * Zones and identities are explicit, never packed into outcome_code.
 */
typedef struct {
    uint32_t model_version;
    uint32_t chance_quality;
    RmcTacticalZone source_zone;
    int32_t context;
    uint32_t duration_seconds;
    RmcPlayerId defender_id;
    RmcPlayerId goalkeeper_id;
    uint32_t applied_talent_mask;
    RmcShotTrajectory trajectory;
    RmcRestartKind restart_kind;
    RmcTeamId restart_awarded_team;
    RmcTacticalZone restart_zone;
    RmcRestartDisposition restart_disposition;
} RmcShotEventPayload;

typedef struct {
    uint64_t id;
    RmcMatchId match_id;
    uint32_t at_second;
    RmcMatchEventType type;

    /* Zero denotes an inapplicable or absent identity. */
    RmcTeamId team_id;
    RmcPlayerId primary_actor;
    RmcPlayerId secondary_actor;

    /* Family-specific outcome; zero remains neutral. */
    uint32_t outcome_code;

    /* Zero means no causal reference. */
    uint64_t cause_event_id;

    /* Appended in schema v2; zero-initialized for all other families. */
    RmcShotEventPayload shot;
} RmcMatchEvent;

typedef struct {
    RmcMatchEvent *events;
    size_t count;
    size_t capacity;
    RmcMatchId match_id;
} RmcMatchEventLog;

/*
 * Initialize a log over caller-owned storage.
 *
 * The buffer must remain alive for the entire lifetime
 * of the log. Initialization never writes to the buffer.
 *
 * On failure, destination is unchanged.
 */
bool rmc_match_event_log_init(
    const RmcMatchState *state,
    RmcMatchEvent *storage,
    size_t capacity,
    RmcMatchEventLog *destination
);

/*
 * Validate the append-only envelope and append one event.
 *
 * The state must represent a valid active match period.
 * No new events can be appended in a terminal state.
 *
 * The event must:
 * - belong to the same match;
 * - carry exactly the next consecutive ID;
 * - have a valid type;
 * - not be in the future or earlier than its predecessor;
 * - reference only preceding events;
 * - use match teams and current on-field actors.
 *
 * Shot-specific identity, payload and causal rules are validated.
 *
 * On failure, log metadata and storage are unchanged.
 */
bool rmc_match_event_log_append(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcMatchEvent *event
);

/*
 * Append a nonempty batch as one transaction. All envelope, payload,
 * capacity and integer-headroom checks finish before the first write.
 * Causes may reference the existing prefix or earlier batch members.
 * Rejection leaves metadata and the ENTIRE backing buffer unchanged.
 * Success performs one overlap-safe copy and updates count once.
 * No heap, RNG or fallible operation after the first write.
 *
 * The prefix must have been produced by this API and not modified.
 * Storage, state, world and log metadata must not overlap; the input
 * event array may overlap event storage. Caller has exclusive access.
 */
bool rmc_match_event_log_append_batch(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcMatchEvent *events,
    size_t event_count
);

#endif
