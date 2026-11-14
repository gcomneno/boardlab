#include "matching.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void verify_assignment(
    const uint8_t *edges,
    size_t left_count,
    size_t right_count,
    const size_t *assignment)
{
    bool used[256] = {false};

    for (size_t left = 0; left < left_count; ++left) {
        const size_t right = assignment[left];

        assert(right < right_count);
        assert(edges[left * right_count + right] != 0);
        assert(!used[right]);

        used[right] = true;
    }
}

static void test_complete_matching(void)
{
    const uint8_t edges[3][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 1}
    };

    size_t assignment[3] = {99, 99, 99};

    assert(boardlab_bipartite_match(
        &edges[0][0], 3, 4, assignment
    ));

    verify_assignment(&edges[0][0], 3, 4, assignment);

    assert(assignment[0] == 0);
    assert(assignment[1] == 1);
    assert(assignment[2] == 2);
}

static void test_augmenting_path(void)
{
    /*
     * Left 0 initially takes right 0.
     * Left 1 can only take right 0.
     * The solver must reassign left 0 to right 1.
     */
    const uint8_t edges[2][2] = {
        {1, 1},
        {1, 0}
    };

    size_t assignment[2] = {99, 99};

    assert(boardlab_bipartite_match(
        &edges[0][0], 2, 2, assignment
    ));

    verify_assignment(&edges[0][0], 2, 2, assignment);

    assert(assignment[0] == 1);
    assert(assignment[1] == 0);
}

static void test_impossible_matching(void)
{
    const uint8_t edges[3][3] = {
        {1, 0, 0},
        {1, 0, 0},
        {0, 1, 1}
    };

    size_t assignment[3] = {77, 88, 99};

    assert(!boardlab_bipartite_match(
        &edges[0][0], 3, 3, assignment
    ));

    assert(assignment[0] == 77);
    assert(assignment[1] == 88);
    assert(assignment[2] == 99);
}

static void test_determinism(void)
{
    const uint8_t edges[3][4] = {
        {1, 1, 1, 1},
        {1, 1, 1, 1},
        {1, 1, 1, 1}
    };

    size_t first[3];
    size_t second[3];

    assert(boardlab_bipartite_match(
        &edges[0][0], 3, 4, first
    ));

    assert(boardlab_bipartite_match(
        &edges[0][0], 3, 4, second
    ));

    for (size_t i = 0; i < 3; ++i) {
        assert(first[i] == second[i]);
    }

    verify_assignment(&edges[0][0], 3, 4, first);
}

static void test_invalid_arguments(void)
{
    const uint8_t edges[2][2] = {
        {1, 0},
        {0, 1}
    };

    size_t assignment[2] = {55, 66};

    assert(!boardlab_bipartite_match(
        NULL, 2, 2, assignment
    ));

    assert(!boardlab_bipartite_match(
        &edges[0][0], 2, 2, NULL
    ));

    assert(!boardlab_bipartite_match(
        &edges[0][0], 0, 2, assignment
    ));

    assert(!boardlab_bipartite_match(
        &edges[0][0], 3, 2, assignment
    ));

    assert(!boardlab_bipartite_match(
        &edges[0][0], 2, 257, assignment
    ));

    assert(!boardlab_bipartite_match(
        &edges[0][0], 2, SIZE_MAX, assignment
    ));

    assert(assignment[0] == 55);
    assert(assignment[1] == 66);
}

static void test_maximum_dimensions(void)
{
    uint8_t edges[256][256] = {{0}};
    size_t assignment[256];

    for (size_t i = 0; i < 256; ++i) {
        edges[i][i] = 1;
    }

    assert(boardlab_bipartite_match(
        &edges[0][0], 256, 256, assignment
    ));

    verify_assignment(
        &edges[0][0], 256, 256, assignment
    );
}

int main(void)
{
    test_complete_matching();
    test_augmenting_path();
    test_impossible_matching();
    test_determinism();
    test_invalid_arguments();
    test_maximum_dimensions();

    puts("MATCHING_COMPLETE=PASS");
    puts("MATCHING_AUGMENTING_PATH=PASS");
    puts("MATCHING_IMPOSSIBLE=PASS");
    puts("MATCHING_DETERMINISM=PASS");
    puts("MATCHING_ATOMICITY=PASS");
    puts("MATCHING_BOUNDARIES=PASS");

    return 0;
}
