#include "replay3d.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    Chrono3DReplay replay = {0};
    chrono_3d_replay_init(&replay);

    assert(chrono_3d_replay_begin(NULL) == 0);
    assert(chrono_3d_replay_record(
        NULL, 1, 1, true, 3, 0
    ) == CHRONO_REPLAY_INVALID);

    uint64_t run = chrono_3d_replay_begin(&replay);
    assert(run != 0);

    assert(chrono_3d_replay_record(
        &replay, 1, run, true, 3, 0
    ) == CHRONO_REPLAY_RECORDED);

    assert(chrono_3d_replay_record(
        &replay, 1, run, true, 3, 0
    ) == CHRONO_REPLAY_DUPLICATE);

    const ChronoReplayMission *mission =
        chrono_replay_get(&replay.progress, 1);

    assert(mission != NULL);
    assert(mission->victories == 1);
    assert(mission->best_turn == 3);

    uint64_t next = chrono_3d_replay_begin(&replay);
    assert(next > run);

    puts("REPLAY3D_TEST=PASS");
    return 0;
}
