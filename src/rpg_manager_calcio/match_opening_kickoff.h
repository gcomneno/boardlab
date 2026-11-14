#ifndef RMC_MATCH_OPENING_KICKOFF_H
#define RMC_MATCH_OPENING_KICKOFF_H

#include "match_event_log.h"

typedef struct {
    RmcMatchPeriod opening_period; /* FIRST_HALF or SECOND_HALF expectation. */
    RmcTeamId opening_team; /* Selection in half one, expected award in half two. */
    RmcPlayerId taker;
} RmcOpeningKickoffAction;

typedef struct {
    uint64_t period_start_event_id;
    uint64_t restart_event_id;
    RmcTeamId awarded_team;
    RmcPlayerId taker;
} RmcOpeningKickoffResult;

/*
 * Open an active half at its canonical RESTART_PENDING boundary. Half one
 * requires an empty log; half two authenticates the complete first-half
 * opening, sporting chains and deadline closure before awarding its opponent.
 * Appends PERIOD_START then RESTART/KICKOFF as one schema-v2 batch, creating
 * fresh BUILDUP control at (2,CENTRE), without time, action or RNG charge.
 *
 * Safe lifecycle composition: copy the NOT_STARTED/HALF_TIME state, apply
 * START/SECOND_HALF to that private copy, call this operation on the copy and
 * the live log, then publish the copy only on success (an infallible copy).
 * A failed opening must never publish the staged lifecycle transition. This
 * low-level pending-state interface follows B1.28-B's explicit entry boundary;
 * the enclosing transaction preserves the inactive boundary in contract 5.4.
 *
 * Rejection preserves state, result, log metadata and the ENTIRE backing
 * buffer byte-for-byte. Temporary prefix allocation can fail before commit.
 * Batch append is the last fallible operation. Caller guarantees accessible,
 * nonoverlapping world, state, action, result, log metadata and entire event
 * storage, their lifetimes and exclusive access throughout the transaction.
 * Validation authenticates structural history, not a cryptographic journal
 * or past random draws. World and lineups remain fixed throughout the match.
 */
bool rmc_match_opening_kickoff_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcMatchEventLog *log,
    const RmcOpeningKickoffAction *action,
    RmcOpeningKickoffResult *result
);

#endif
