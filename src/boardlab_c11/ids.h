#ifndef BOARDLAB_C11_IDS_H
#define BOARDLAB_C11_IDS_H

#include <stdbool.h>
#include <stdint.h>

#define BOARDLAB_INVALID_ID UINT64_C(0)

typedef struct {
    uint64_t next;
} BoardlabIdAllocator;

/*
 * Initialize an allocator at the first valid identifier.
 */
void boardlab_ids_init(BoardlabIdAllocator *allocator);

/*
 * Allocate a stable, nonzero identifier.
 *
 * Returns false for null pointers, exhausted or invalid
 * allocator state.
 *
 * Failure never changes the allocator or output.
 */
bool boardlab_ids_next(
    BoardlabIdAllocator *allocator,
    uint64_t *id
);

/*
 * Restore a persisted next-identifier value.
 *
 * The caller must validate the value against the
 * identifiers already present in its domain.
 */
bool boardlab_ids_restore(
    BoardlabIdAllocator *allocator,
    uint64_t next
);

#endif
