#if defined(__APPLE__)
#define _DARWIN_C_SOURCE 1
#endif
#define _POSIX_C_SOURCE 200809L
#include "replay_store.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    char directory[] = "/tmp/chrono-replay-test-XXXXXX";
    assert(mkdtemp(directory) != NULL);

    char path[256];
    int n = snprintf(path, sizeof(path),
                     "%s/progress.v1", directory);
    assert(n > 0 && (size_t)n < sizeof(path));

    ChronoReplayProgress original;
    ChronoReplayProgress loaded;
    chrono_replay_init(&original);
    chrono_replay_init(&loaded);

    assert(chrono_store_load(path, &loaded) ==
           CHRONO_STORE_NOT_FOUND);

    assert(chrono_replay_record(
        &original, 1, 1, true, 8,
        CHRONO_REPLAY_TIDE_REGISTER
    ) == CHRONO_REPLAY_RECORDED);
    assert(chrono_replay_record(
        &original, 4, 2, true, 8,
        CHRONO_REPLAY_RECONFIGURED
    ) == CHRONO_REPLAY_RECORDED);
    assert(chrono_replay_record(
        &original, 5, 3, true, 8,
        CHRONO_REPLAY_CORRECTION_RECOVERED
    ) == CHRONO_REPLAY_RECORDED);

    assert(chrono_store_save(path, &original) ==
           CHRONO_STORE_OK);
    assert(chrono_store_load(path, &loaded) ==
           CHRONO_STORE_OK);
    assert(memcmp(&original, &loaded, sizeof(original)) == 0);
    puts("STORE_ROUNDTRIP_GATE=PASS");

    assert(chrono_replay_record(
        &loaded, 1, 4, true, 6, 0
    ) == CHRONO_REPLAY_RECORDED);
    assert(chrono_store_save(path, &loaded) ==
           CHRONO_STORE_OK);

    ChronoReplayProgress verified;
    chrono_replay_init(&verified);
    assert(chrono_store_load(path, &verified) ==
           CHRONO_STORE_OK);
    assert(verified.missions[0].victories == 2);
    assert(verified.missions[0].best_turn == 6);
    assert(verified.missions[0].achievements ==
           CHRONO_REPLAY_TIDE_REGISTER);
    puts("STORE_UPDATE_GATE=PASS");

    /* Simulate process restart: counter starts at zero after load. */
    uint64_t restarted_counter = 0;
    uint64_t restarted_id =
        chrono_replay_next_run_id(&verified, &restarted_counter);
    assert(restarted_id == 5);

    assert(chrono_replay_record(
        &verified, 1, restarted_id, true, 5, 0
    ) == CHRONO_REPLAY_RECORDED);
    assert(chrono_store_save(path, &verified) == CHRONO_STORE_OK);

    ChronoReplayProgress after_restart;
    chrono_replay_init(&after_restart);
    assert(chrono_store_load(path, &after_restart) == CHRONO_STORE_OK);
    assert(after_restart.missions[0].victories == 3);
    assert(after_restart.missions[0].best_turn == 5);
    assert(after_restart.missions[0].last_run_id == restarted_id);
    assert(after_restart.missions[0].achievements ==
           CHRONO_REPLAY_TIDE_REGISTER);

    verified = after_restart;
    puts("STORE_RESTART_CONTINUITY_GATE=PASS");

    FILE *stream = fopen(path, "w");
    assert(stream != NULL);
    assert(fputs("CORRUPTED\n", stream) >= 0);
    assert(fclose(stream) == 0);

    ChronoReplayProgress untouched = verified;
    assert(chrono_store_load(path, &untouched) ==
           CHRONO_STORE_CORRUPT);
    assert(memcmp(&untouched, &verified, sizeof(verified)) == 0);
    assert(chrono_store_save(path, &verified) ==
           CHRONO_STORE_CORRUPT);

    stream = fopen(path, "r");
    assert(stream != NULL);
    char line[64];
    assert(fgets(line, sizeof(line), stream) != NULL);
    assert(strcmp(line, "CORRUPTED\n") == 0);
    assert(fclose(stream) == 0);
    puts("STORE_CORRUPTION_PROTECTION_GATE=PASS");

    assert(unlink(path) == 0);
    assert(rmdir(directory) == 0);
    puts("REPLAY_STORE_TEST_GATE=PASS");
    return 0;
}
