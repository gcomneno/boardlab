#ifndef RMC_MATCH_EVENT_LOG_H
#define RMC_MATCH_EVENT_LOG_H

#include "match_state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * M001-B — Structured Event Log v1.
 *
 * Event storage is caller-owned, bounded and append-only.
 * IDs are consecutive, beginning at 1.
 *
 * The log does not resolve actions, mutate match state,
 * allocate memory or consume PRNG.
 *
 * Event-specific outcome semantics, statistics and the
 * canonical replay hash belong to later increments.
 */

#define RMC_EVENT_SCHEMA_VERSION UINT32_C(1)

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

typedef struct {
    uint64_t id;
    RmcMatchId match_id;
    uint32_t at_second;
    RmcMatchEventType type;

    /* Zero denotes an inapplicable or absent identity. */
    RmcTeamId team_id;
    RmcPlayerId primary_actor;
    RmcPlayerId secondary_actor;

    /*
     * Event-specific payload is not yet standardized.
     * Zero is the neutral value in this initial envelope.
     */
    uint32_t outcome_code;

    /* Zero means no causal reference. */
    uint64_t cause_event_id;
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
 * Type-specific event semantics will be added later.
 *
 * On failure, log metadata and storage are unchanged.
 */
bool rmc_match_event_log_append(
    const RmcWorldCore *world,
    const RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcMatchEvent *event
);

#endif
