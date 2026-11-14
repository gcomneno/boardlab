#include "identity.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_independent_counters(void)
{
    RmcIdentityState state;
    uint64_t id = 0;

    rmc_identity_init(&state);
    assert(rmc_identity_valid(&state));

    for (unsigned kind = 0; kind < RMC_ENTITY_COUNT; ++kind) {
        assert(state.counters[kind].next == UINT64_C(1));

        assert(rmc_identity_next(
            &state, (RmcEntityKind)kind, &id
        ));
        assert(id == UINT64_C(1));

        assert(rmc_identity_next(
            &state, (RmcEntityKind)kind, &id
        ));
        assert(id == UINT64_C(2));
    }

    for (unsigned kind = 0; kind < RMC_ENTITY_COUNT; ++kind) {
        assert(state.counters[kind].next == UINT64_C(3));
    }
}

static void test_restore(void)
{
    RmcIdentityState state;
    uint64_t id = 0;

    rmc_identity_init(&state);

    assert(rmc_identity_restore(
        &state, RMC_ENTITY_PLAYER, UINT64_C(145)
    ));

    assert(rmc_identity_next(
        &state, RMC_ENTITY_PLAYER, &id
    ));
    assert(id == UINT64_C(145));

    assert(rmc_identity_next(
        &state, RMC_ENTITY_TEAM, &id
    ));
    assert(id == UINT64_C(1));

    assert(state.counters[RMC_ENTITY_PLAYER].next == 146);
    assert(state.counters[RMC_ENTITY_TEAM].next == 2);
}

static void test_exhaustion(void)
{
    RmcIdentityState state;
    uint64_t id = UINT64_C(999);

    rmc_identity_init(&state);

    assert(rmc_identity_restore(
        &state, RMC_ENTITY_EVENT, UINT64_MAX
    ));

    assert(rmc_identity_valid(&state));

    assert(!rmc_identity_next(
        &state, RMC_ENTITY_EVENT, &id
    ));

    assert(id == UINT64_C(999));
    assert(state.counters[RMC_ENTITY_EVENT].next ==
           UINT64_MAX);
}

static void test_invalid_inputs(void)
{
    RmcIdentityState state;
    uint64_t id = UINT64_C(777);

    rmc_identity_init(&state);

    assert(!rmc_identity_next(
        NULL, RMC_ENTITY_TEAM, &id
    ));

    assert(!rmc_identity_next(
        &state, RMC_ENTITY_TEAM, NULL
    ));

    assert(!rmc_identity_next(
        &state, (RmcEntityKind)-1, &id
    ));

    assert(!rmc_identity_next(
        &state, RMC_ENTITY_COUNT, &id
    ));

    assert(!rmc_identity_restore(
        &state, RMC_ENTITY_TEAM, 0
    ));

    assert(!rmc_identity_restore(
        &state, RMC_ENTITY_COUNT, 10
    ));

    assert(id == UINT64_C(777));
    assert(state.counters[RMC_ENTITY_TEAM].next == 1);
    assert(rmc_identity_valid(&state));

    state.counters[RMC_ENTITY_COACH].next = 0;
    assert(!rmc_identity_valid(&state));
    assert(!rmc_identity_valid(NULL));
}

int main(void)
{
    test_independent_counters();
    test_restore();
    test_exhaustion();
    test_invalid_inputs();

    puts("IDENTITY_INDEPENDENCE=PASS");
    puts("IDENTITY_MONOTONICITY=PASS");
    puts("IDENTITY_RESTORE=PASS");
    puts("IDENTITY_EXHAUSTION=PASS");
    puts("IDENTITY_INVALID_INPUT=PASS");
    puts("IDENTITY_VALIDATION=PASS");

    return 0;
}
