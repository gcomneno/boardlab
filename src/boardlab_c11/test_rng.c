#include "rng.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_reference_vector(void)
{
    static const uint32_t expected[5] = {
        UINT32_C(0xa15c02b7),
        UINT32_C(0x7b47f409),
        UINT32_C(0xba1d3330),
        UINT32_C(0x83d2f293),
        UINT32_C(0xbfa4784b)
    };

    RmcPcg32 rng;
    rmc_pcg32_seed(&rng, UINT64_C(42), UINT64_C(54));

    for (unsigned i = 0; i < 5; ++i) {
        assert(rmc_pcg32_next(&rng) == expected[i]);
    }
}

static void test_reproducibility(void)
{
    RmcPcg32 a;
    RmcPcg32 b;

    rmc_pcg32_seed(&a, UINT64_C(123456789), UINT64_C(7));
    rmc_pcg32_seed(&b, UINT64_C(123456789), UINT64_C(7));

    for (unsigned i = 0; i < 10000; ++i) {
        assert(rmc_pcg32_next(&a) == rmc_pcg32_next(&b));
    }

    assert(a.state == b.state);
    assert(a.increment == b.increment);
}

static void test_bounded_draws(void)
{
    RmcPcg32 rng;
    uint32_t value = UINT32_MAX;

    rmc_pcg32_seed(&rng, UINT64_C(42), UINT64_C(54));

    for (uint32_t bound = 1; bound <= 100; ++bound) {
        for (unsigned i = 0; i < 1000; ++i) {
            assert(rmc_pcg32_bounded(&rng, bound, &value));
            assert(value < bound);
        }
    }
}

static void test_invalid_arguments(void)
{
    RmcPcg32 rng;
    uint32_t value = UINT32_C(999);

    rmc_pcg32_seed(&rng, UINT64_C(42), UINT64_C(54));

    const uint64_t previous_state = rng.state;

    assert(!rmc_pcg32_bounded(&rng, 0, &value));
    assert(!rmc_pcg32_bounded(&rng, 10, NULL));
    assert(!rmc_pcg32_bounded(NULL, 10, &value));

    assert(rng.state == previous_state);
    assert(value == UINT32_C(999));
}

int main(void)
{
    test_reference_vector();
    test_reproducibility();
    test_bounded_draws();
    test_invalid_arguments();

    puts("RNG_REFERENCE_VECTOR=PASS");
    puts("RNG_REPRODUCIBILITY=PASS");
    puts("RNG_BOUNDED=PASS");
    puts("RNG_INVALID_ARGUMENTS=PASS");

    return 0;
}
