#ifndef BOARDLAB_C11_ROUND_ROBIN_H
#define BOARDLAB_C11_ROUND_ROBIN_H

#include <stdbool.h>
#include <stddef.h>

enum {
    BOARDLAB_ROUND_ROBIN_MAX_PARTICIPANTS = 64
};

/*
 * A pairing of participant indices.
 *
 * Indices are in [0, participant_count).
 * The ordering does not imply home/away semantics.
 */
typedef struct {
    size_t first;
    size_t second;
} BoardlabRoundRobinPair;

/*
 * Generate a deterministic single round-robin schedule
 * using the circle method.
 *
 * Supported participant counts:
 * - Even.
 * - At least 2.
 * - At most BOARDLAB_ROUND_ROBIN_MAX_PARTICIPANTS.
 *
 * Output layout:
 * - participant_count - 1 consecutive rounds.
 * - participant_count / 2 pairs per round.
 *
 * Required capacity:
 *   participant_count * (participant_count - 1) / 2
 *
 * Each participant appears exactly once per round.
 * Each unordered pair appears exactly once overall.
 *
 * Returns false for invalid arguments or insufficient
 * output capacity.
 *
 * On failure, output remains unchanged.
 *
 * No random draws, game rules or home/away assignments.
 */
bool boardlab_round_robin_generate(
    size_t participant_count,
    BoardlabRoundRobinPair *output,
    size_t output_capacity
);

#endif
