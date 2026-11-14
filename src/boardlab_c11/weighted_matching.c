#include "weighted_matching.h"

#include <stdint.h>
#include <stddef.h>

enum {
    WM_CAP = BOARDLAB_WEIGHTED_MATCH_MAX_NODES
};

/*
 * Costs are nonnegative. An unavailable edge has
 * a cost strictly greater than any possible total
 * cost of a completely compatible assignment.
 */
#define WM_FORBIDDEN_COST INT64_C(1000000000)
#define WM_INFINITY      INT64_C(4000000000000)

bool boardlab_weighted_match(
    const uint8_t *compatibility,
    const uint32_t *scores,
    size_t left_count,
    size_t right_count,
    size_t *assignment,
    uint64_t *total_score)
{
    if (compatibility == NULL ||
        scores == NULL ||
        assignment == NULL ||
        total_score == NULL ||
        left_count == 0 ||
        left_count > right_count ||
        right_count > WM_CAP) {
        return false;
    }

    /*
     * Validate the entire score matrix before
     * calculating or publishing any assignment.
     */
    for (size_t i = 0; i < left_count; ++i) {
        for (size_t j = 0; j < right_count; ++j) {
            if (scores[i * right_count + j] >
                BOARDLAB_WEIGHTED_MATCH_MAX_SCORE) {
                return false;
            }
        }
    }

    /*
     * Rectangular Hungarian algorithm.
     *
     * Internal indices are one-based.
     *
     * u and v are dual potentials;
     * p[j] is the row assigned to column j;
     * way[j] reconstructs an augmenting path.
     *
     * Maximizing a score is equivalent to minimizing:
     *
     *   MAX_SCORE - score
     *
     * Forbidden edges receive an unreachable penalty.
     */
    int64_t u[WM_CAP + 1] = {0};
    int64_t v[WM_CAP + 1] = {0};
    size_t p[WM_CAP + 1] = {0};
    size_t way[WM_CAP + 1] = {0};

    for (size_t row = 1; row <= left_count; ++row) {
        p[0] = row;

        int64_t minv[WM_CAP + 1];
        bool used[WM_CAP + 1] = {false};

        for (size_t j = 0; j <= right_count; ++j) {
            minv[j] = WM_INFINITY;
            way[j] = 0;
        }

        size_t column = 0;

        do {
            used[column] = true;

            const size_t current_row = p[column];
            int64_t delta = WM_INFINITY;
            size_t next_column = 0;

            for (size_t j = 1; j <= right_count; ++j) {
                if (used[j]) {
                    continue;
                }

                const size_t index =
                    (current_row - 1) * right_count + (j - 1);

                const int64_t cost =
                    compatibility[index] != 0
                        ? (int64_t)
                            BOARDLAB_WEIGHTED_MATCH_MAX_SCORE -
                            (int64_t)scores[index]
                        : WM_FORBIDDEN_COST;

                const int64_t reduced =
                    cost - u[current_row] - v[j];

                if (reduced < minv[j]) {
                    minv[j] = reduced;
                    way[j] = column;
                }

                /*
                 * Strict comparison and ascending
                 * column traversal provide stable,
                 * deterministic tie resolution.
                 */
                if (minv[j] < delta) {
                    delta = minv[j];
                    next_column = j;
                }
            }

            if (next_column == 0 ||
                delta == WM_INFINITY) {
                return false;
            }

            for (size_t j = 0; j <= right_count; ++j) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }

            column = next_column;
        } while (p[column] != 0);

        /*
         * Augment the matching along the
         * alternating path just discovered.
         */
        do {
            const size_t previous = way[column];
            p[column] = p[previous];
            column = previous;
        } while (column != 0);
    }

    /*
     * Reconstruct into local storage first.
     * Unassigned rows and forbidden edges cause
     * failure without changing caller outputs.
     */
    size_t result[WM_CAP];

    for (size_t i = 0; i < left_count; ++i) {
        result[i] = SIZE_MAX;
    }

    for (size_t j = 1; j <= right_count; ++j) {
        if (p[j] != 0) {
            result[p[j] - 1] = j - 1;
        }
    }

    uint64_t sum = 0;

    for (size_t i = 0; i < left_count; ++i) {
        if (result[i] == SIZE_MAX) {
            return false;
        }

        const size_t index =
            i * right_count + result[i];

        if (compatibility[index] == 0) {
            return false;
        }

        sum += scores[index];
    }

    /*
     * Atomic publication after complete validation.
     */
    for (size_t i = 0; i < left_count; ++i) {
        assignment[i] = result[i];
    }

    *total_score = sum;
    return true;
}
