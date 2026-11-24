#ifndef CHRONO_REPLAY_PROGRESS_H
#define CHRONO_REPLAY_PROGRESS_H

#include <stdbool.h>
#include <stdint.h>

#define CHRONO_REPLAY_MISSIONS 5u

#define CHRONO_REPLAY_TIDE_REGISTER (1u << 0)
#define CHRONO_REPLAY_RECONFIGURED (1u << 1)
#define CHRONO_REPLAY_CORRECTION_RECOVERED (1u << 2)

typedef struct {
    unsigned victories;
    unsigned best_turn;
    unsigned achievements;
    uint64_t last_run_id;
} ChronoReplayMission;

typedef struct {
    ChronoReplayMission missions[CHRONO_REPLAY_MISSIONS];
} ChronoReplayProgress;

typedef enum {
    CHRONO_REPLAY_INVALID = 0,
    CHRONO_REPLAY_RECORDED,
    CHRONO_REPLAY_DUPLICATE
} ChronoReplayResult;

void chrono_replay_init(ChronoReplayProgress *progress);

/* Returns 0 only when allocation is impossible. */
uint64_t chrono_replay_next_run_id(
    const ChronoReplayProgress *progress, uint64_t *counter
);

ChronoReplayResult chrono_replay_record(
    ChronoReplayProgress *progress,
    unsigned mission,
    uint64_t run_id,
    bool victory,
    unsigned turns,
    unsigned verified_achievements
);

const ChronoReplayMission *chrono_replay_get(
    const ChronoReplayProgress *progress,
    unsigned mission
);

#endif
