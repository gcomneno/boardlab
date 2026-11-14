#ifndef BOARDLAB_C11_MATCHING_H
#define BOARDLAB_C11_MATCHING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BOARDLAB_MATCH_UNASSIGNED SIZE_MAX

/*
 * Deterministic one-to-one bipartite assignment.
 *
 * compatibility is a row-major matrix:
 *
 *   compatibility[left * right_count + right]
 *
 * A nonzero entry declares a legal edge.
 *
 * On success, assignment[left] contains the selected
 * right-side index.
 *
 * The implementation searches in ascending index order.
 *
 * Returns false for:
 * - Invalid arguments.
 * - Impossible complete assignment.
 * - Unsupported dimensions.
 *
 * The assignment buffer remains unchanged on failure.
 *
 * No game-specific rules or random generators are used.
 */
bool boardlab_bipartite_match(
    const uint8_t *compatibility,
    size_t left_count,
    size_t right_count,
    size_t *assignment
);

#endif
