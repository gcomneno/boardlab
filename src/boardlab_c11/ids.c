#include "ids.h"

#include <stddef.h>

void boardlab_ids_init(BoardlabIdAllocator *allocator)
{
    if (allocator != NULL) {
        allocator->next = UINT64_C(1);
    }
}

bool boardlab_ids_next(
    BoardlabIdAllocator *allocator,
    uint64_t *id)
{
    if (allocator == NULL || id == NULL) {
        return false;
    }

    if (allocator->next == BOARDLAB_INVALID_ID ||
        allocator->next == UINT64_MAX) {
        return false;
    }

    const uint64_t assigned = allocator->next;

    allocator->next = assigned + UINT64_C(1);
    *id = assigned;

    return true;
}

bool boardlab_ids_restore(
    BoardlabIdAllocator *allocator,
    uint64_t next)
{
    if (allocator == NULL ||
        next == BOARDLAB_INVALID_ID) {
        return false;
    }

    allocator->next = next;
    return true;
}
