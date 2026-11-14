#include "ids.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_monotonicity(void)
{
    BoardlabIdAllocator allocator;
    uint64_t id = 0;

    boardlab_ids_init(&allocator);

    for (uint64_t expected = 1; expected <= 10000; ++expected) {
        assert(boardlab_ids_next(&allocator, &id));
        assert(id == expected);
    }

    assert(allocator.next == UINT64_C(10001));
}

static void test_restore(void)
{
    BoardlabIdAllocator allocator;
    uint64_t id = 0;

    boardlab_ids_init(&allocator);

    assert(boardlab_ids_restore(&allocator, UINT64_C(500)));
    assert(boardlab_ids_next(&allocator, &id));
    assert(id == UINT64_C(500));
    assert(allocator.next == UINT64_C(501));

    assert(!boardlab_ids_restore(&allocator, 0));
    assert(allocator.next == UINT64_C(501));
}

static void test_exhaustion(void)
{
    BoardlabIdAllocator allocator;
    uint64_t id = 0;

    assert(boardlab_ids_restore(
        &allocator, UINT64_MAX - UINT64_C(1)
    ));

    assert(boardlab_ids_next(&allocator, &id));
    assert(id == UINT64_MAX - UINT64_C(1));
    assert(allocator.next == UINT64_MAX);

    id = UINT64_C(123);
    assert(!boardlab_ids_next(&allocator, &id));
    assert(id == UINT64_C(123));
    assert(allocator.next == UINT64_MAX);
}

static void test_invalid_arguments(void)
{
    BoardlabIdAllocator allocator;
    uint64_t id = UINT64_C(777);

    boardlab_ids_init(&allocator);

    assert(!boardlab_ids_next(NULL, &id));
    assert(!boardlab_ids_next(&allocator, NULL));
    assert(!boardlab_ids_restore(NULL, 10));

    assert(id == UINT64_C(777));
    assert(allocator.next == UINT64_C(1));

    allocator.next = 0;

    assert(!boardlab_ids_next(&allocator, &id));
    assert(allocator.next == 0);
    assert(id == UINT64_C(777));
}

int main(void)
{
    test_monotonicity();
    test_restore();
    test_exhaustion();
    test_invalid_arguments();

    puts("ID_MONOTONICITY=PASS");
    puts("ID_RESTORE=PASS");
    puts("ID_EXHAUSTION=PASS");
    puts("ID_FAILURE_ATOMICITY=PASS");
    puts("ID_INVALID_INPUT=PASS");

    return 0;
}
