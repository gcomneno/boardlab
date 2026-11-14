#ifndef RMC_MATCH_STOPPAGE_H
#define RMC_MATCH_STOPPAGE_H

#include <stdbool.h>
#include <stdint.h>

/*
 * M001-B — Stoppage Time Policy v1.
 *
 * Each period has an independent stoppage accumulator.
 * Every registered second authorizes one additional
 * second of play, up to the canonical period cap.
 *
 * This module does not mutate match state, advance
 * the match clock, emit events or consume PRNG.
 */

#define RMC_PERIOD_REGULATION_SECONDS UINT32_C(2700)
#define RMC_STOPPAGE_MAX_SECONDS UINT32_C(900)
#define RMC_STOPPAGE_POLICY_VERSION UINT32_C(1)

/*
 * Add one interruption duration to a period accumulator.
 *
 * Duration must be positive.
 * The cumulative total must not exceed the policy cap.
 *
 * On failure, accumulated_seconds is unchanged.
 */
bool rmc_stoppage_record(
    uint32_t *accumulated_seconds,
    uint32_t duration_seconds
);

/*
 * Calculate the effective absolute end of a period.
 *
 * period_start_second is:
 * - zero for the first half;
 * - the actual first-half end for the second half.
 *
 * The result is:
 * period_start + 2700 + authorized stoppage.
 *
 * Overflow and excessive stoppage are rejected.
 * On failure, deadline_second is unchanged.
 */
bool rmc_stoppage_deadline(
    uint32_t period_start_second,
    uint32_t accumulated_seconds,
    uint32_t *deadline_second
);

#endif
