#ifndef CHRONO_REPLAY3D_H
#define CHRONO_REPLAY3D_H

#include "replay_progress.h"
#include "replay_store.h"

/* Renderer-independent REPLAY boundary for the 3D frontend. */
typedef struct {
    ChronoReplayProgress progress;
    uint64_t next_run_counter;
} Chrono3DReplay;

void chrono_3d_replay_init(Chrono3DReplay *replay);

uint64_t chrono_3d_replay_begin(
    Chrono3DReplay *replay
);

ChronoReplayResult chrono_3d_replay_record(
    Chrono3DReplay *replay,
    unsigned mission,
    uint64_t run_id,
    bool victory,
    unsigned turns,
    unsigned achievements
);

ChronoStoreResult chrono_3d_replay_load(
    Chrono3DReplay *replay,
    const char *path
);

ChronoStoreResult chrono_3d_replay_save(
    const Chrono3DReplay *replay,
    const char *path
);

#endif
