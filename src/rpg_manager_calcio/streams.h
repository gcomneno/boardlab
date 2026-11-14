#ifndef RMC_STREAMS_H
#define RMC_STREAMS_H

#include "../boardlab_c11/rng.h"

#include <stdbool.h>
#include <stdint.h>

#define RMC_GENERATOR_VERSION UINT32_C(1)
#define RMC_RULES_VERSION UINT32_C(1)

typedef enum {
    RMC_STREAM_WORLD = 1,
    RMC_STREAM_ATTRIBUTES = 2,
    RMC_STREAM_CALENDAR = 3,
    RMC_STREAM_MATCH = 4,
    RMC_STREAM_AI = 5,
    RMC_STREAM_RPG = 6
} RmcStreamDomain;

typedef struct {
    uint64_t world_seed;
    uint32_t generator_version;
    uint32_t rules_version;
} RmcWorldIdentity;

/*
 * Canonical stream derivation v1.
 *
 * entity_id is zero for global streams.
 * Match-specific streams require a nonzero MatchId.
 *
 * Returns false for unsupported versions, invalid domains,
 * invalid entity IDs, or null pointers.
 *
 * On failure, the destination is not modified.
 */
bool rmc_stream_init(
    RmcPcg32 *destination,
    const RmcWorldIdentity *identity,
    RmcStreamDomain domain,
    uint64_t entity_id
);

#endif
