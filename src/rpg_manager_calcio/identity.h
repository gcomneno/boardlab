#ifndef RMC_IDENTITY_H
#define RMC_IDENTITY_H

#include "../boardlab_c11/ids.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * Logical persistent entity identifiers.
 *
 * Distinct struct types prevent implicit interchange
 * between different entity categories.
 */
typedef struct { uint64_t value; } RmcTeamId;
typedef struct { uint64_t value; } RmcPlayerId;
typedef struct { uint64_t value; } RmcCoachId;
typedef struct { uint64_t value; } RmcMatchId;
typedef struct { uint64_t value; } RmcEventId;
typedef struct { uint64_t value; } RmcSeasonId;

typedef enum {
    RMC_ENTITY_TEAM = 0,
    RMC_ENTITY_PLAYER,
    RMC_ENTITY_COACH,
    RMC_ENTITY_MATCH,
    RMC_ENTITY_EVENT,
    RMC_ENTITY_SEASON,
    RMC_ENTITY_COUNT
} RmcEntityKind;

/*
 * Persistent next-ID counters.
 *
 * The complete structure must be included in the
 * world snapshot and validated when restored.
 */
typedef struct {
    BoardlabIdAllocator counters[RMC_ENTITY_COUNT];
} RmcIdentityState;

/*
 * Initialize all six independent counters at ID 1.
 */
void rmc_identity_init(RmcIdentityState *state);

/*
 * Allocate one identifier in the selected category.
 *
 * Returns false for invalid arguments or exhaustion.
 * On failure, both state and output remain unchanged.
 */
bool rmc_identity_next(
    RmcIdentityState *state,
    RmcEntityKind kind,
    uint64_t *id
);

/*
 * Restore a category's persisted next-ID counter.
 *
 * Validation against existing entity identifiers
 * remains the responsibility of the world validator.
 */
bool rmc_identity_restore(
    RmcIdentityState *state,
    RmcEntityKind kind,
    uint64_t next
);

/*
 * Validate the structural integrity of all counters.
 * This does not validate the world entity graph.
 */
bool rmc_identity_valid(const RmcIdentityState *state);

#endif
