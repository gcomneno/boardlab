#include "streams.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static const RmcWorldIdentity identity = {
    UINT64_C(123456789),
    RMC_GENERATOR_VERSION,
    RMC_RULES_VERSION
};

static void test_reproducibility(void)
{
    RmcPcg32 a;
    RmcPcg32 b;

    assert(rmc_stream_init(&a, &identity, RMC_STREAM_WORLD, 0));
    assert(rmc_stream_init(&b, &identity, RMC_STREAM_WORLD, 0));

    for (unsigned i = 0; i < 10000; ++i) {
        assert(rmc_pcg32_next(&a) == rmc_pcg32_next(&b));
    }
}

static void test_domain_separation(void)
{
    RmcPcg32 streams[6];

    for (unsigned i = 0; i < 6; ++i) {
        assert(rmc_stream_init(
            &streams[i],
            &identity,
            (RmcStreamDomain)(i + 1),
            (i == 3) ? UINT64_C(1) : 0
        ));
    }

    for (unsigned i = 0; i < 6; ++i) {
        for (unsigned j = i + 1; j < 6; ++j) {
            assert(
                streams[i].state != streams[j].state ||
                streams[i].increment != streams[j].increment
            );
        }
    }
}

static void test_initialization_order(void)
{
    RmcPcg32 world_a;
    RmcPcg32 world_b;
    RmcPcg32 calendar;

    assert(rmc_stream_init(
        &world_a, &identity, RMC_STREAM_WORLD, 0
    ));

    assert(rmc_stream_init(
        &calendar, &identity, RMC_STREAM_CALENDAR, 0
    ));

    for (unsigned i = 0; i < 1000; ++i) {
        (void)rmc_pcg32_next(&calendar);
    }

    assert(rmc_stream_init(
        &world_b, &identity, RMC_STREAM_WORLD, 0
    ));

    for (unsigned i = 0; i < 1000; ++i) {
        assert(rmc_pcg32_next(&world_a) ==
               rmc_pcg32_next(&world_b));
    }
}

static void test_match_isolation(void)
{
    RmcPcg32 first;
    RmcPcg32 second;
    RmcPcg32 replay;

    assert(rmc_stream_init(
        &first, &identity, RMC_STREAM_MATCH, UINT64_C(1)
    ));

    assert(rmc_stream_init(
        &second, &identity, RMC_STREAM_MATCH, UINT64_C(2)
    ));

    assert(rmc_stream_init(
        &replay, &identity, RMC_STREAM_MATCH, UINT64_C(1)
    ));

    assert(first.state != second.state ||
           first.increment != second.increment);

    for (unsigned i = 0; i < 10000; ++i) {
        (void)rmc_pcg32_next(&second);

        assert(rmc_pcg32_next(&first) ==
               rmc_pcg32_next(&replay));
    }
}

static void test_invalid_input(void)
{
    RmcPcg32 destination;
    RmcWorldIdentity invalid = identity;

    rmc_pcg32_seed(&destination, 42, 54);

    const uint64_t original_state = destination.state;
    const uint64_t original_increment = destination.increment;

    invalid.generator_version++;

    assert(!rmc_stream_init(
        &destination, &invalid, RMC_STREAM_WORLD, 0
    ));

    invalid = identity;
    invalid.rules_version++;

    assert(!rmc_stream_init(
        &destination, &invalid, RMC_STREAM_WORLD, 0
    ));

    assert(!rmc_stream_init(
        &destination, &identity, RMC_STREAM_MATCH, 0
    ));

    assert(!rmc_stream_init(
        &destination, &identity, (RmcStreamDomain)99, 0
    ));

    assert(!rmc_stream_init(
        NULL, &identity, RMC_STREAM_WORLD, 0
    ));

    assert(!rmc_stream_init(
        &destination, NULL, RMC_STREAM_WORLD, 0
    ));

    assert(destination.state == original_state);
    assert(destination.increment == original_increment);
}

int main(void)
{
    test_reproducibility();
    test_domain_separation();
    test_initialization_order();
    test_match_isolation();
    test_invalid_input();

    puts("STREAM_REPRODUCIBILITY=PASS");
    puts("STREAM_DOMAIN_SEPARATION=PASS");
    puts("STREAM_ORDER_INDEPENDENCE=PASS");
    puts("STREAM_MATCH_ISOLATION=PASS");
    puts("STREAM_INVALID_INPUT=PASS");

    return 0;
}
