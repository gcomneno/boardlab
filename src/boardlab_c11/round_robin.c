#include "round_robin.h"

#include <stddef.h>

bool boardlab_round_robin_generate(
    size_t participant_count,
    BoardlabRoundRobinPair *output,
    size_t output_capacity)
{
    if (output == NULL ||
        participant_count < 2 ||
        participant_count > BOARDLAB_ROUND_ROBIN_MAX_PARTICIPANTS ||
        (participant_count % 2) != 0) {
        return false;
    }

    /*
     * The maximum supported count is 64, so the
     * required pair count cannot overflow size_t.
     */
    const size_t rounds = participant_count - 1;
    const size_t pairs_per_round = participant_count / 2;
    const size_t required_pairs = rounds * pairs_per_round;

    if (output_capacity < required_pairs) {
        return false;
    }

    /*
     * Circle method.
     *
     * Position zero remains fixed.
     * The remaining positions rotate one step right
     * after each round.
     *
     * Position order and pairing order are canonical.
     */
    size_t positions[BOARDLAB_ROUND_ROBIN_MAX_PARTICIPANTS];

    for (size_t i = 0; i < participant_count; ++i) {
        positions[i] = i;
    }

    for (size_t round = 0; round < rounds; ++round) {
        const size_t offset = round * pairs_per_round;

        for (size_t pair = 0; pair < pairs_per_round; ++pair) {
            output[offset + pair].first = positions[pair];
            output[offset + pair].second =
                positions[participant_count - 1 - pair];
        }

        /*
         * Rotate positions [1, participant_count).
         * The final position becomes position one.
         */
        const size_t last = positions[participant_count - 1];

        for (size_t i = participant_count - 1; i > 1; --i) {
            positions[i] = positions[i - 1];
        }

        positions[1] = last;
    }

    return true;
}
