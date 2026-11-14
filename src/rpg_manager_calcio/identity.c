#include "identity.h"

#include <stddef.h>

static bool rmc_kind_valid(RmcEntityKind kind)
{
    return kind >= RMC_ENTITY_TEAM &&
           kind < RMC_ENTITY_COUNT;
}

void rmc_identity_init(RmcIdentityState *state)
{
    if (state == NULL) {
        return;
    }

    for (unsigned i = 0; i < RMC_ENTITY_COUNT; ++i) {
        boardlab_ids_init(&state->counters[i]);
    }
}

bool rmc_identity_next(
    RmcIdentityState *state,
    RmcEntityKind kind,
    uint64_t *id)
{
    if (state == NULL || id == NULL ||
        !rmc_kind_valid(kind)) {
        return false;
    }

    return boardlab_ids_next(
        &state->counters[(unsigned)kind],
        id
    );
}

bool rmc_identity_restore(
    RmcIdentityState *state,
    RmcEntityKind kind,
    uint64_t next)
{
    if (state == NULL || !rmc_kind_valid(kind)) {
        return false;
    }

    return boardlab_ids_restore(
        &state->counters[(unsigned)kind],
        next
    );
}

bool rmc_identity_valid(const RmcIdentityState *state)
{
    if (state == NULL) {
        return false;
    }

    for (unsigned i = 0; i < RMC_ENTITY_COUNT; ++i) {
        if (state->counters[i].next ==
            BOARDLAB_INVALID_ID) {
            return false;
        }
    }

    return true;
}
