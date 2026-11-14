#include "matching.h"

#include <stddef.h>
#include <stdint.h>

#define BOARDLAB_MATCH_MAX 256u

typedef struct {
    const uint8_t *edges;
    size_t right_count;
    size_t owner[BOARDLAB_MATCH_MAX];
    bool visited[BOARDLAB_MATCH_MAX];
} MatchingContext;

static bool augment(MatchingContext *ctx, size_t left)
{
    for (size_t right = 0; right < ctx->right_count; ++right) {
        const size_t index = left * ctx->right_count + right;

        if (ctx->edges[index] == 0 || ctx->visited[right]) {
            continue;
        }

        ctx->visited[right] = true;

        if (ctx->owner[right] == BOARDLAB_MATCH_UNASSIGNED ||
            augment(ctx, ctx->owner[right])) {
            ctx->owner[right] = left;
            return true;
        }
    }

    return false;
}

bool boardlab_bipartite_match(
    const uint8_t *compatibility,
    size_t left_count,
    size_t right_count,
    size_t *assignment)
{
    if (compatibility == NULL || assignment == NULL ||
        left_count == 0 || right_count == 0 ||
        left_count > right_count ||
        left_count > BOARDLAB_MATCH_MAX ||
        right_count > BOARDLAB_MATCH_MAX ||
        left_count > SIZE_MAX / right_count) {
        return false;
    }

    MatchingContext ctx = {
        .edges = compatibility,
        .right_count = right_count
    };

    for (size_t right = 0; right < right_count; ++right) {
        ctx.owner[right] = BOARDLAB_MATCH_UNASSIGNED;
    }

    for (size_t left = 0; left < left_count; ++left) {
        for (size_t right = 0; right < right_count; ++right) {
            ctx.visited[right] = false;
        }

        if (!augment(&ctx, left)) {
            return false;
        }
    }

    /*
     * Publish the complete solution only after all
     * left-side elements have been assigned.
     */
    size_t result[BOARDLAB_MATCH_MAX];

    for (size_t left = 0; left < left_count; ++left) {
        result[left] = BOARDLAB_MATCH_UNASSIGNED;
    }

    for (size_t right = 0; right < right_count; ++right) {
        if (ctx.owner[right] != BOARDLAB_MATCH_UNASSIGNED) {
            result[ctx.owner[right]] = right;
        }
    }

    for (size_t left = 0; left < left_count; ++left) {
        if (result[left] == BOARDLAB_MATCH_UNASSIGNED) {
            return false;
        }
    }

    for (size_t left = 0; left < left_count; ++left) {
        assignment[left] = result[left];
    }

    return true;
}
