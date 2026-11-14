#ifndef RMC_MATCH_TRANSITIONS_H
#define RMC_MATCH_TRANSITIONS_H

#include "match_state.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * Deterministic lifecycle transitions.
 *
 * These transitions establish period boundaries.
 * They do not simulate gameplay, generate events
 * or consume random numbers.
 */
typedef enum {
    RMC_TRANSITION_START = 0,
    RMC_TRANSITION_HALF_TIME,
    RMC_TRANSITION_SECOND_HALF,
    RMC_TRANSITION_FINISH,
    RMC_TRANSITION_COUNT
} RmcMatchTransition;

/*
 * Apply one legal lifecycle transition.
 *
 * at_second must equal the current absolute match clock.
 * Period transitions never advance time.
 *
 * START requires second zero.
 * HALF_TIME requires at least second 2700.
 * SECOND_HALF preserves the clock.
 * FINISH requires at least second 5400.
 *
 * The source state must be valid.
 * Invalid transitions leave state unchanged.
 *
 * Period-boundary transitions explicitly terminate
 * the current possession, if any.
 *
 * This API is not the gameplay time-advance API.
 */
bool rmc_match_transition_apply(
    const RmcWorldCore *world,
    RmcMatchState *state,
    RmcMatchTransition transition,
    uint32_t at_second
);

#endif
