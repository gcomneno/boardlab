#include "replay3d.h"
#include <stddef.h>

void chrono_3d_replay_init(Chrono3DReplay *replay)
{
    if (replay == NULL) {
        return;
    }

    chrono_replay_init(&replay->progress);
    replay->next_run_counter = 0;
}

uint64_t chrono_3d_replay_begin(
    Chrono3DReplay *replay
)
{
    if (replay == NULL) {
        return 0;
    }

    return chrono_replay_next_run_id(
        &replay->progress,
        &replay->next_run_counter
    );
}

ChronoReplayResult chrono_3d_replay_record(
    Chrono3DReplay *replay,
    unsigned mission,
    uint64_t run_id,
    bool victory,
    unsigned turns,
    unsigned achievements
)
{
    if (replay == NULL) {
        return CHRONO_REPLAY_INVALID;
    }

    return chrono_replay_record(
        &replay->progress,
        mission,
        run_id,
        victory,
        turns,
        achievements
    );
}

ChronoStoreResult chrono_3d_replay_load(
    Chrono3DReplay *replay,
    const char *path
)
{
    if (replay == NULL) {
        return CHRONO_STORE_IO_ERROR;
    }

    return chrono_store_load(path, &replay->progress);
}

ChronoStoreResult chrono_3d_replay_save(
    const Chrono3DReplay *replay,
    const char *path
)
{
    if (replay == NULL) {
        return CHRONO_STORE_IO_ERROR;
    }

    return chrono_store_save(path, &replay->progress);
}
