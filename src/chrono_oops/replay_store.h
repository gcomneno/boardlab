#ifndef CHRONO_REPLAY_STORE_H
#define CHRONO_REPLAY_STORE_H

#include "replay_progress.h"

typedef enum {
    CHRONO_STORE_OK,
    CHRONO_STORE_NOT_FOUND,
    CHRONO_STORE_CORRUPT,
    CHRONO_STORE_IO_ERROR
} ChronoStoreResult;

ChronoStoreResult chrono_store_load(
    const char *path, ChronoReplayProgress *progress
);

ChronoStoreResult chrono_store_save(
    const char *path, const ChronoReplayProgress *progress
);

#endif
