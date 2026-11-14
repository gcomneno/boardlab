#include "match_probability.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t lookup(int32_t difference)
{
    uint32_t value = UINT32_MAX;

    assert(rmc_match_logistic_lookup(
        difference, &value
    ));

    return value;
}

static uint32_t contextual(
    int32_t attack,
    int32_t opposition,
    int32_t context)
{
    uint32_t value = UINT32_MAX;

    assert(rmc_match_probability(
        attack, opposition, context, &value
    ));

    return value;
}

static void test_canonical_vectors(void)
{
    static const struct {
        int32_t difference;
        uint32_t probability;
    } vectors[] = {
        {-100, 19},
        {-32, 1192},
        {-16, 2689},
        {-1, 4844},
        {0, 5000},
        {1, 5156},
        {16, 7311},
        {32, 8808},
        {100, 9981}
    };

    assert(RMC_LOGISTIC_TABLE_VERSION == 1);
    assert(RMC_LOGISTIC_TABLE_SIZE == 201);
    assert(RMC_PROBABILITY_SCALE == 10000);

    for (size_t i = 0;
         i < sizeof(vectors) / sizeof(vectors[0]);
         ++i) {
        assert(lookup(vectors[i].difference) ==
               vectors[i].probability);
    }
}

static void test_all_differences(void)
{
    uint32_t previous = 0;

    for (int32_t difference = -100;
         difference <= 100;
         ++difference) {
        const uint32_t value = lookup(difference);

        assert(value > 0);
        assert(value < RMC_PROBABILITY_SCALE);
        assert(value >= previous);
        assert(value + lookup(-difference) ==
               RMC_PROBABILITY_SCALE);

        /*
         * A and O reproduce any valid difference
         * without using a contextual modifier.
         */
        const int32_t attack =
            difference > 0 ? difference : 0;
        const int32_t opposition =
            difference < 0 ? -difference : 0;

        assert(contextual(attack, opposition, 0) ==
               value);

        previous = value;
    }
}

static void test_context_and_clamping(void)
{
    assert(contextual(50, 50, 0) == 5000);

    assert(contextual(50, 50, 20) == lookup(20));
    assert(contextual(50, 50, -20) == lookup(-20));

    assert(contextual(100, 0, 20) == lookup(100));
    assert(contextual(0, 100, -20) == lookup(-100));

    assert(contextual(100, 100, 20) == lookup(20));
    assert(contextual(0, 0, -20) == lookup(-20));
}

static void test_invalid_lookup(void)
{
    uint32_t output = UINT32_C(4321);

    assert(!rmc_match_logistic_lookup(-101, &output));
    assert(output == 4321);

    assert(!rmc_match_logistic_lookup(101, &output));
    assert(output == 4321);

    assert(!rmc_match_logistic_lookup(0, NULL));
    assert(output == 4321);

    assert(!rmc_match_logistic_lookup(
        INT32_MIN, &output
    ));
    assert(output == 4321);

    assert(!rmc_match_logistic_lookup(
        INT32_MAX, &output
    ));
    assert(output == 4321);
}

static void test_invalid_context(void)
{
    static const struct {
        int32_t attack;
        int32_t opposition;
        int32_t context;
    } invalid[] = {
        {-1, 50, 0},
        {101, 50, 0},
        {50, -1, 0},
        {50, 101, 0},
        {50, 50, -21},
        {50, 50, 21},
        {INT32_MIN, 50, 0},
        {INT32_MAX, 50, 0},
        {50, INT32_MIN, 0},
        {50, INT32_MAX, 0},
        {50, 50, INT32_MIN},
        {50, 50, INT32_MAX}
    };

    for (size_t i = 0;
         i < sizeof(invalid) / sizeof(invalid[0]);
         ++i) {
        uint32_t output = UINT32_C(4321);

        assert(!rmc_match_probability(
            invalid[i].attack,
            invalid[i].opposition,
            invalid[i].context,
            &output
        ));

        assert(output == 4321);
    }

    assert(!rmc_match_probability(
        50, 50, 0, NULL
    ));
}

int main(void)
{
    test_canonical_vectors();
    test_all_differences();
    test_context_and_clamping();
    test_invalid_lookup();
    test_invalid_context();

    puts("PROBABILITY_CANONICAL_VECTORS=PASS");
    puts("PROBABILITY_ALL_201_DIFFERENCES=PASS");
    puts("PROBABILITY_CONTEXT_AND_CLAMPING=PASS");
    puts("PROBABILITY_INVALID_LOOKUP=PASS");
    puts("PROBABILITY_INVALID_CONTEXT=PASS");

    return 0;
}
