#include "replay_progress.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    ChronoReplayProgress progress;
    chrono_replay_init(&progress);

    assert(chrono_replay_get(&progress, 0) == NULL);
    assert(chrono_replay_get(&progress, 6) == NULL);
    assert(chrono_replay_get(&progress, 1)->victories == 0);

    assert(chrono_replay_record(
        &progress, 1, 1, true, 8,
        CHRONO_REPLAY_TIDE_REGISTER
    ) == CHRONO_REPLAY_RECORDED);

    assert(chrono_replay_record(
        &progress, 1, 1, true, 6, 0
    ) == CHRONO_REPLAY_DUPLICATE);

    assert(chrono_replay_get(&progress, 1)->victories == 1);
    assert(chrono_replay_get(&progress, 1)->best_turn == 8);

    assert(chrono_replay_record(
        &progress, 1, 2, true, 6, 0
    ) == CHRONO_REPLAY_RECORDED);

    assert(chrono_replay_get(&progress, 1)->victories == 2);
    assert(chrono_replay_get(&progress, 1)->best_turn == 6);
    assert(chrono_replay_get(&progress, 1)->achievements ==
           CHRONO_REPLAY_TIDE_REGISTER);

    assert(chrono_replay_record(
        &progress, 2, 3, false, 8, 0
    ) == CHRONO_REPLAY_RECORDED);

    assert(chrono_replay_get(&progress, 2)->victories == 0);
    assert(chrono_replay_get(&progress, 2)->best_turn == 0);

    assert(chrono_replay_record(
        &progress, 4, 4, true, 8,
        CHRONO_REPLAY_RECONFIGURED
    ) == CHRONO_REPLAY_RECORDED);

    assert(chrono_replay_record(
        &progress, 5, 5, true, 8,
        CHRONO_REPLAY_CORRECTION_RECOVERED
    ) == CHRONO_REPLAY_RECORDED);

    assert(chrono_replay_record(
        &progress, 3, 6, true, 6,
        CHRONO_REPLAY_TIDE_REGISTER
    ) == CHRONO_REPLAY_INVALID);

    assert(chrono_replay_record(
        &progress, 1, 7, false, 8,
        CHRONO_REPLAY_TIDE_REGISTER
    ) == CHRONO_REPLAY_INVALID);

    assert(chrono_replay_record(
        &progress, 1, 0, true, 6, 0
    ) == CHRONO_REPLAY_INVALID);

    assert(chrono_replay_record(
        &progress, 1, 8, true, 0, 0
    ) == CHRONO_REPLAY_INVALID);

    /* A loaded mission record must advance the new session counter. */
    uint64_t new_session_counter = 0;
    uint64_t run_id =
        chrono_replay_next_run_id(&progress, &new_session_counter);
    assert(run_id == 6);
    assert(new_session_counter == 6);

    assert(chrono_replay_record(
        &progress, 5, run_id, true, 6, 0
    ) == CHRONO_REPLAY_RECORDED);

    assert(chrono_replay_get(&progress, 5)->victories == 2);
    assert(chrono_replay_get(&progress, 5)->best_turn == 6);

    assert(chrono_replay_next_run_id(NULL, &new_session_counter) == 0);
    assert(chrono_replay_next_run_id(&progress, NULL) == 0);

    new_session_counter = UINT64_MAX;
    assert(chrono_replay_next_run_id(
        &progress, &new_session_counter
    ) == 0);
    assert(new_session_counter == UINT64_MAX);

    puts("REPLAY_RUN_ID_CONTINUITY_GATE=PASS");
    puts("REPLAY_PROGRESS_TEST_GATE=PASS");
    return 0;
}
