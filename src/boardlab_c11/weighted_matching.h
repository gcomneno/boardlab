#ifndef BOARDLAB_C11_WEIGHTED_MATCHING_H
#define BOARDLAB_C11_WEIGHTED_MATCHING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    BOARDLAB_WEIGHTED_MATCH_MAX_NODES = 64,
    BOARDLAB_WEIGHTED_MATCH_MAX_SCORE = 1000000
};

/*
 * Deterministic maximum-weight bipartite assignment.
 *
 * Each left node must be assigned to exactly one
 * distinct right node.
 *
 * Matrix layout is row-major:
 *
 *   index = left * right_count + right
 *
 * compatibility[index] != 0 permits an edge.
 * scores[index] supplies its nonnegative weight.
 *
 * Forbidden edges must never be selected, regardless
 * of their score.
 *
 * Supported dimensions:
 * - 1 <= left_count <= right_count.
 * - right_count <= BOARDLAB_WEIGHTED_MATCH_MAX_NODES.
 *
 * Every score must be in:
 *   [0, BOARDLAB_WEIGHTED_MATCH_MAX_SCORE].
 *
 * On success:
 * - assignment[left] contains the selected right index.
 * - total_score contains the maximum attainable sum.
 *
 * Ties must be resolved deterministically.
 *
 * Returns false for invalid arguments, unsupported
 * dimensions, invalid scores or impossible matching.
 *
 * On failure, assignment and total_score remain
 * unchanged.
 *
 * This algorithm has no game-specific knowledge,
 * random draws or dynamic allocation requirements.
 */
bool boardlab_weighted_match(
    const uint8_t *compatibility,
    const uint32_t *scores,
    size_t left_count,
    size_t right_count,
    size_t *assignment,
    uint64_t *total_score
);

#endif
