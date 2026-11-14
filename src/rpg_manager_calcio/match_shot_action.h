#ifndef RMC_MATCH_SHOT_ACTION_H
#define RMC_MATCH_SHOT_ACTION_H

#include "match_event_log.h"
#include "match_probability.h"
#include "streams.h"

#define RMC_SHOT_ACTION_VERSION UINT32_C(1)

typedef struct {
    RmcTeamId team_id;
    RmcPlayerId shooter;
    RmcPlayerId defender;
    RmcPlayerId goalkeeper;
    int32_t context;
    uint32_t duration_seconds;
} RmcShotAction;

typedef struct {
    RmcShotEventOutcome outcome;
    RmcShotTrajectory trajectory; /* Latent trajectory, including on a block. */
    uint32_t shooter_effectiveness;     /* S */
    uint32_t positioning_effectiveness; /* J */
    uint32_t defender_effectiveness;    /* D */
    uint32_t goalkeeper_effectiveness;  /* G */
    uint32_t holding_effectiveness;     /* H */
    uint32_t chance_quality;            /* Q, not a goal probability */
    uint32_t on_target_probability;     /* T */
    uint32_t woodwork_probability;      /* K */
    uint32_t wide_probability;          /* W */
    uint32_t block_probability;         /* b */
    uint32_t base_goal_probability;     /* g0 */
    uint32_t goal_probability;          /* g, conditional on unblocked target */
    uint32_t hold_probability;          /* h, conditional on a save */
    uint32_t applied_talent_mask;
    uint32_t bounded_draw_count;
    uint32_t draws[4]; /* Canonical order; unused entries are zero. */
} RmcShotResult;

/*
 * Frozen Shot v1: current outfield controller versus an opposing outfield
 * defender and the opposing POR goalkeeper, from sectors 2..4, any lane.
 * Uses schema-v2 outcome codes and payloads without redefining them.
 * All conditional distributions have scale RMC_PROBABILITY_SCALE.
 *
 * Resolves on a copied match PCG32, requesting 2..4 bounded draws (not a
 * fixed number of raw outputs). Reserves the maximum branch before sampling.
 * Commits SHOT_CHANCE, SHOT, optional GOAL and deadline ending events in one
 * batch. The final SHOT payload describes a pending/suppressed restart;
 * this operation never executes that restart or recovers a loose ball.
 *
 * On any failure, state, RNG, log metadata, entire storage and result are
 * byte-for-byte unchanged. Existing prefix validation reuses the logger on
 * temporary storage; allocation failure is an ordinary atomic rejection.
 * Caller guarantees nonoverlap of world, action, state, RNG, log metadata,
 * event storage and result, and exclusive access for the whole call.
 */
bool rmc_match_shot_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcPcg32 *match_rng,
    RmcMatchEventLog *log,
    const RmcShotAction *action,
    RmcShotResult *result
);

#endif
