#include "round_robin.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

enum {
    MAX_PARTICIPANTS = BOARDLAB_ROUND_ROBIN_MAX_PARTICIPANTS,
    MAX_PAIRS = MAX_PARTICIPANTS * (MAX_PARTICIPANTS - 1) / 2
};

static void verify_schedule(size_t count)
{
    BoardlabRoundRobinPair schedule[MAX_PAIRS];
    BoardlabRoundRobinPair replay[MAX_PAIRS];

    bool pair_seen[MAX_PARTICIPANTS][MAX_PARTICIPANTS] = {{false}};

    const size_t rounds = count - 1;
    const size_t per_round = count / 2;
    const size_t total = rounds * per_round;

    assert(boardlab_round_robin_generate(count, schedule, total));
    assert(boardlab_round_robin_generate(count, replay, total));

    for (size_t round = 0; round < rounds; ++round) {
        bool participant_seen[MAX_PARTICIPANTS] = {false};

        for (size_t slot = 0; slot < per_round; ++slot) {
            const size_t index = round * per_round + slot;
            const size_t a = schedule[index].first;
            const size_t b = schedule[index].second;

            assert(a < count);
            assert(b < count);
            assert(a != b);

            assert(!participant_seen[a]);
            assert(!participant_seen[b]);

            participant_seen[a] = true;
            participant_seen[b] = true;

            const size_t low = a < b ? a : b;
            const size_t high = a < b ? b : a;

            assert(!pair_seen[low][high]);
            pair_seen[low][high] = true;

            assert(replay[index].first == a);
            assert(replay[index].second == b);
        }

        for (size_t p = 0; p < count; ++p) {
            assert(participant_seen[p]);
        }
    }

    for (size_t a = 0; a < count; ++a) {
        for (size_t b = a + 1; b < count; ++b) {
            assert(pair_seen[a][b]);
        }
    }
}

static void test_m001_dimensions(void)
{
    BoardlabRoundRobinPair schedule[28];

    assert(boardlab_round_robin_generate(8, schedule, 28));

    verify_schedule(8);

    puts("ROUND_ROBIN_8_TEAMS=PASS");
    puts("ROUND_ROBIN_7_ROUNDS=PASS");
    puts("ROUND_ROBIN_28_PAIRS=PASS");
}

static void test_supported_dimensions(void)
{
    for (size_t count = 2; count <= MAX_PARTICIPANTS;
         count += 2) {
        verify_schedule(count);
    }

    puts("ROUND_ROBIN_ALL_EVEN_DIMENSIONS=PASS");
    puts("ROUND_ROBIN_64_PARTICIPANTS=PASS");
    puts("ROUND_ROBIN_DETERMINISM=PASS");
}

static void test_failure_atomicity(void)
{
    BoardlabRoundRobinPair output[28];

    for (size_t i = 0; i < 28; ++i) {
        output[i].first = 777;
        output[i].second = 888;
    }

    assert(!boardlab_round_robin_generate(0, output, 28));
    assert(!boardlab_round_robin_generate(1, output, 28));
    assert(!boardlab_round_robin_generate(3, output, 28));
    assert(!boardlab_round_robin_generate(65, output, 28));
    assert(!boardlab_round_robin_generate(66, output, 28));
    assert(!boardlab_round_robin_generate(8, output, 27));
    assert(!boardlab_round_robin_generate(8, NULL, 28));

    for (size_t i = 0; i < 28; ++i) {
        assert(output[i].first == 777);
        assert(output[i].second == 888);
    }

    puts("ROUND_ROBIN_INVALID_ARGUMENTS=PASS");
    puts("ROUND_ROBIN_FAILURE_ATOMICITY=PASS");
}

int main(void)
{
    test_m001_dimensions();
    test_supported_dimensions();
    test_failure_atomicity();

    puts("ROUND_ROBIN_QUALIFICATION=PASS");
    return 0;
}
