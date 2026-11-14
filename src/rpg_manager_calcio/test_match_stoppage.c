#include "match_stoppage.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_policy_constants(void)
{
    assert(RMC_STOPPAGE_POLICY_VERSION == 1);
    assert(RMC_PERIOD_REGULATION_SECONDS == 2700);
    assert(RMC_STOPPAGE_MAX_SECONDS == 900);
}

static void test_accumulation(void)
{
    uint32_t accumulated = 0;

    assert(rmc_stoppage_record(&accumulated, 30));
    assert(accumulated == 30);

    assert(rmc_stoppage_record(&accumulated, 90));
    assert(accumulated == 120);

    assert(rmc_stoppage_record(&accumulated, 780));
    assert(accumulated == 900);
}

static void test_atomic_rejection(void)
{
    uint32_t accumulated = 120;

    assert(!rmc_stoppage_record(&accumulated, 0));
    assert(accumulated == 120);

    assert(!rmc_stoppage_record(&accumulated, 781));
    assert(accumulated == 120);

    assert(!rmc_stoppage_record(&accumulated, UINT32_MAX));
    assert(accumulated == 120);

    assert(!rmc_stoppage_record(NULL, 1));

    accumulated = 901;
    assert(!rmc_stoppage_record(&accumulated, 1));
    assert(accumulated == 901);

    accumulated = 900;
    assert(!rmc_stoppage_record(&accumulated, 1));
    assert(accumulated == 900);
}

static void test_period_deadlines(void)
{
    uint32_t deadline = 0;

    assert(rmc_stoppage_deadline(0, 0, &deadline));
    assert(deadline == 2700);

    assert(rmc_stoppage_deadline(0, 120, &deadline));
    assert(deadline == 2820);

    assert(rmc_stoppage_deadline(2820, 0, &deadline));
    assert(deadline == 5520);

    assert(rmc_stoppage_deadline(2820, 180, &deadline));
    assert(deadline == 5700);

    assert(rmc_stoppage_deadline(0, 900, &deadline));
    assert(deadline == 3600);
}

static void test_deadline_rejection(void)
{
    uint32_t deadline = UINT32_C(123456789);

    assert(!rmc_stoppage_deadline(0, 901, &deadline));
    assert(deadline == UINT32_C(123456789));

    assert(!rmc_stoppage_deadline(
        UINT32_MAX, 0, &deadline
    ));
    assert(deadline == UINT32_C(123456789));

    assert(!rmc_stoppage_deadline(
        UINT32_MAX - 2699, 0, &deadline
    ));
    assert(deadline == UINT32_C(123456789));

    assert(rmc_stoppage_deadline(
        UINT32_MAX - 2700, 0, &deadline
    ));
    assert(deadline == UINT32_MAX);

    assert(!rmc_stoppage_deadline(0, 0, NULL));
}

static void test_deterministic_replay(void)
{
    const uint32_t interruptions[] = {
        10, 25, 40, 15, 30
    };

    uint32_t first = 0;
    uint32_t second = 0;

    for (size_t i = 0;
         i < sizeof(interruptions) / sizeof(interruptions[0]);
         ++i) {
        assert(rmc_stoppage_record(
            &first, interruptions[i]
        ));
        assert(rmc_stoppage_record(
            &second, interruptions[i]
        ));
        assert(first == second);
    }

    uint32_t first_deadline = 0;
    uint32_t second_deadline = 0;

    assert(rmc_stoppage_deadline(
        0, first, &first_deadline
    ));
    assert(rmc_stoppage_deadline(
        0, second, &second_deadline
    ));

    assert(first == 120);
    assert(first_deadline == 2820);
    assert(first_deadline == second_deadline);
}

int main(void)
{
    test_policy_constants();
    test_accumulation();
    test_atomic_rejection();
    test_period_deadlines();
    test_deadline_rejection();
    test_deterministic_replay();

    puts("STOPPAGE_POLICY_CONSTANTS=PASS");
    puts("STOPPAGE_ACCUMULATION=PASS");
    puts("STOPPAGE_ATOMIC_REJECTION=PASS");
    puts("STOPPAGE_PERIOD_DEADLINES=PASS");
    puts("STOPPAGE_OVERFLOW_PROTECTION=PASS");
    puts("STOPPAGE_DETERMINISTIC_REPLAY=PASS");

    return 0;
}
