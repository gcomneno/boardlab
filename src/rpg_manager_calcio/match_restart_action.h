#ifndef RMC_MATCH_RESTART_ACTION_H
#define RMC_MATCH_RESTART_ACTION_H

#include "match_event_log.h"

typedef struct {
    RmcRestartKind restart_kind;
    RmcTeamId awarded_team;
    RmcPlayerId taker;
    uint64_t source_shot_event_id;
} RmcRestartAction;

typedef struct {
    uint64_t restart_event_id;
    uint64_t source_shot_event_id;
    uint64_t source_goal_event_id; /* Zero for GOAL_KICK. */
} RmcRestartResult;

/*
 * Execute the complete, pending Shot v1 GOAL/WIDE cause at the log tail.
 * The kind and award are expectations; the explicit taker must be an on-field
 * outfielder for KICKOFF or the genuine current POR goalkeeper for GOAL_KICK.
 * Creates one neutral-payload RESTART and a fresh BUILDUP possession, without
 * time, stoppage, action charge or RNG consumption.
 *
 * Rejection preserves state, result, log metadata and the entire backing
 * storage byte-for-byte. Prefix validation uses temporary allocation; failure
 * is an atomic rejection. Append is the last fallible operation.
 * Caller guarantees lifetime, accessibility, exclusive access and nonoverlap
 * of world, action, state, log metadata, entire event storage and result.
 */
bool rmc_match_restart_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcRestartAction *action,
    RmcRestartResult *result
);

#endif
