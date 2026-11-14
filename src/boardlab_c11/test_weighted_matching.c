#include "weighted_matching.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum {
    MAX_NODES = BOARDLAB_WEIGHTED_MATCH_MAX_NODES,
    MAX_CELLS = MAX_NODES * MAX_NODES
};

/*
 * Independent exhaustive oracle.
 * Intended only for small matrices.
 */
static void exhaustive_search(
    const uint8_t *edges,
    const uint32_t *scores,
    size_t rows,
    size_t cols,
    size_t row,
    uint64_t used,
    uint64_t accumulated,
    bool *found,
    uint64_t *best)
{
    if (row == rows) {
        if (!*found || accumulated > *best) {
            *best = accumulated;
        }
        *found = true;
        return;
    }

    for (size_t col = 0; col < cols; ++col) {
        const uint64_t bit = UINT64_C(1) << col;
        const size_t index = row * cols + col;

        if ((used & bit) != 0 || edges[index] == 0) {
            continue;
        }

        exhaustive_search(
            edges, scores, rows, cols, row + 1,
            used | bit, accumulated + scores[index],
            found, best
        );
    }
}

static void verify_against_oracle(
    const uint8_t *edges,
    const uint32_t *scores,
    size_t rows,
    size_t cols)
{
    bool feasible = false;
    uint64_t optimum = 0;

    exhaustive_search(
        edges, scores, rows, cols, 0, 0, 0,
        &feasible, &optimum
    );

    size_t assignment[MAX_NODES];
    size_t replay[MAX_NODES];
    uint64_t total = UINT64_C(987654321);
    uint64_t replay_total = UINT64_C(987654321);

    for (size_t i = 0; i < MAX_NODES; ++i) {
        assignment[i] = SIZE_MAX;
        replay[i] = SIZE_MAX;
    }

    const bool success = boardlab_weighted_match(
        edges, scores, rows, cols, assignment, &total
    );

    const bool replay_success = boardlab_weighted_match(
        edges, scores, rows, cols, replay, &replay_total
    );

    assert(success == feasible);
    assert(replay_success == feasible);

    if (!feasible) {
        assert(total == UINT64_C(987654321));
        assert(replay_total == UINT64_C(987654321));
        for (size_t i = 0; i < rows; ++i) {
            assert(assignment[i] == SIZE_MAX);
            assert(replay[i] == SIZE_MAX);
        }
        return;
    }

    assert(total == optimum);
    assert(replay_total == optimum);

    bool used[MAX_NODES] = {false};
    uint64_t calculated = 0;

    for (size_t i = 0; i < rows; ++i) {
        const size_t col = assignment[i];

        assert(col < cols);
        assert(!used[col]);
        assert(edges[i * cols + col] != 0);

        used[col] = true;
        calculated += scores[i * cols + col];

        assert(replay[i] == col);
    }

    assert(calculated == total);
}

static void test_exhaustive_campaign(void)
{
    uint8_t edges[5 * 6];
    uint32_t scores[5 * 6];

    for (size_t rows = 1; rows <= 5; ++rows) {
        for (size_t cols = rows; cols <= 6; ++cols) {
            for (uint32_t scenario = 0;
                 scenario < 100; ++scenario) {
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        const size_t k = i * cols + j;
                        const uint32_t value =
                            (uint32_t)(i * 37 + j * 19) +
                            scenario * UINT32_C(23);

                        edges[k] =
                            (uint8_t)((value % 7) < 5);
                        scores[k] =
                            (value * 13 + (uint32_t)(i * j))
                            % 101;
                    }
                }

                verify_against_oracle(
                    edges, scores, rows, cols
                );
            }
        }
    }

    puts("WEIGHTED_MATCH_EXHAUSTIVE_ORACLE=PASS");
    puts("WEIGHTED_MATCH_RECTANGULAR=PASS");
    puts("WEIGHTED_MATCH_FORBIDDEN_EDGES=PASS");
    puts("WEIGHTED_MATCH_DETERMINISM=PASS");
}

static void test_ties_and_limits(void)
{
    uint8_t edges[MAX_CELLS];
    uint32_t scores[MAX_CELLS];
    size_t assignment[MAX_NODES];
    uint64_t total = 0;

    for (size_t i = 0; i < MAX_CELLS; ++i) {
        edges[i] = 1;
        scores[i] = BOARDLAB_WEIGHTED_MATCH_MAX_SCORE;
    }

    assert(boardlab_weighted_match(
        edges, scores, MAX_NODES, MAX_NODES,
        assignment, &total
    ));

    assert(total ==
        (uint64_t)MAX_NODES *
        BOARDLAB_WEIGHTED_MATCH_MAX_SCORE);

    bool used[MAX_NODES] = {false};

    for (size_t i = 0; i < MAX_NODES; ++i) {
        assert(assignment[i] < MAX_NODES);
        assert(!used[assignment[i]]);
        used[assignment[i]] = true;
    }

    puts("WEIGHTED_MATCH_64_NODES=PASS");
    puts("WEIGHTED_MATCH_MAXIMUM_SCORE=PASS");
    puts("WEIGHTED_MATCH_TIES=PASS");
}

static void test_invalid_arguments(void)
{
    uint8_t edges[4] = {1, 1, 1, 1};
    uint32_t scores[4] = {1, 2, 3, 4};
    size_t assignment[2] = {777, 888};
    uint64_t total = 999;

    assert(!boardlab_weighted_match(
        NULL, scores, 2, 2, assignment, &total));
    assert(!boardlab_weighted_match(
        edges, NULL, 2, 2, assignment, &total));
    assert(!boardlab_weighted_match(
        edges, scores, 0, 2, assignment, &total));
    assert(!boardlab_weighted_match(
        edges, scores, 2, 1, assignment, &total));
    assert(!boardlab_weighted_match(
        edges, scores, 1, 65, assignment, &total));
    assert(!boardlab_weighted_match(
        edges, scores, 2, 2, NULL, &total));
    assert(!boardlab_weighted_match(
        edges, scores, 2, 2, assignment, NULL));

    scores[0] = BOARDLAB_WEIGHTED_MATCH_MAX_SCORE + 1;

    assert(!boardlab_weighted_match(
        edges, scores, 2, 2, assignment, &total));

    assert(assignment[0] == 777);
    assert(assignment[1] == 888);
    assert(total == 999);

    puts("WEIGHTED_MATCH_INVALID_INPUT=PASS");
    puts("WEIGHTED_MATCH_FAILURE_ATOMICITY=PASS");
}

static void test_impossible_assignment(void)
{
    const uint8_t edges[9] = {
        1, 0, 0,
        1, 0, 0,
        0, 0, 1
    };
    const uint32_t scores[9] = {
        10, 0, 0,
        20, 0, 0,
        0, 0, 30
    };

    verify_against_oracle(edges, scores, 3, 3);

    puts("WEIGHTED_MATCH_IMPOSSIBLE=PASS");
}

int main(void)
{
    test_exhaustive_campaign();
    test_ties_and_limits();
    test_invalid_arguments();
    test_impossible_assignment();

    puts("WEIGHTED_MATCH_QUALIFICATION=PASS");
    return 0;
}
