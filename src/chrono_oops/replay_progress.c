#include "replay_progress.h"

#include <limits.h>
#include <string.h>

static unsigned allowed_achievements(unsigned mission)
{
    switch (mission) {
    case 1:
        return CHRONO_REPLAY_TIDE_REGISTER;
    case 4:
        return CHRONO_REPLAY_RECONFIGURED;
    case 5:
        return CHRONO_REPLAY_CORRECTION_RECOVERED;
    default:
        return 0;
    }
}

void chrono_replay_init(ChronoReplayProgress *progress)
{
    if (progress != NULL)
        memset(progress, 0, sizeof(*progress));
}

/*
 * Reconcile the session counter with IDs loaded from disk.
 * Prevent wraparound instead of reusing an historical identifier.
 */
uint64_t chrono_replay_next_run_id(
    const ChronoReplayProgress *progress, uint64_t *counter
)
{
    if (progress == NULL || counter == NULL)
        return 0;

    uint64_t highest = *counter;

    for (unsigned i = 0; i < CHRONO_REPLAY_MISSIONS; ++i) {
        if (progress->missions[i].last_run_id > highest)
            highest = progress->missions[i].last_run_id;
    }

    if (highest == UINT64_MAX)
        return 0;

    *counter = highest + 1;
    return *counter;
}

const ChronoReplayMission *chrono_replay_get(
    const ChronoReplayProgress *progress,
    unsigned mission
)
{
    if (progress == NULL || mission < 1 ||
        mission > CHRONO_REPLAY_MISSIONS)
        return NULL;

    return &progress->missions[mission - 1];
}

ChronoReplayResult chrono_replay_record(
    ChronoReplayProgress *progress,
    unsigned mission,
    uint64_t run_id,
    bool victory,
    unsigned turns,
    unsigned verified_achievements
)
{
    if (progress == NULL || mission < 1 ||
        mission > CHRONO_REPLAY_MISSIONS ||
        run_id == 0 || turns == 0 ||
        (verified_achievements &
         ~allowed_achievements(mission)) != 0 ||
        (!victory && verified_achievements != 0))
        return CHRONO_REPLAY_INVALID;

    ChronoReplayMission *record = &progress->missions[mission - 1];

    if (record->last_run_id == run_id)
        return CHRONO_REPLAY_DUPLICATE;

    if (victory && record->victories == UINT_MAX)
        return CHRONO_REPLAY_INVALID;

    record->last_run_id = run_id;

    if (victory) {
        ++record->victories;
        if (record->best_turn == 0 || turns < record->best_turn)
            record->best_turn = turns;
        record->achievements |= verified_achievements;
    }

    return CHRONO_REPLAY_RECORDED;
}
