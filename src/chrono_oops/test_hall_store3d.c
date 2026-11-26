#if defined(__APPLE__)
#define _DARWIN_C_SOURCE 1
#endif
#define _POSIX_C_SOURCE 200809L
#include "hall_store3d.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
#ifdef _WIN32
    const char *temp_root = getenv("TEMP");
    if (temp_root == NULL || *temp_root == '\0')
        temp_root = getenv("TMP");
    assert(temp_root != NULL && *temp_root != '\0');

    char directory[4096];
    int temp_length = snprintf(
        directory, sizeof(directory),
        "%s/chrono-hall-test-XXXXXX", temp_root
    );
    assert(temp_length > 0 &&
           (size_t)temp_length < sizeof(directory));
#else
    char directory[] = "/tmp/chrono-hall-test-XXXXXX";
#endif
    assert(mkdtemp(directory) != NULL);

    char path[4096];
    int path_length = snprintf(
        path, sizeof(path), "%s/hall.dat", directory
    );
    assert(path_length > 0 &&
           (size_t)path_length < sizeof(path));

    ChronoHall hall;
    ChronoHall restored;
    ChronoHallRanks ranks;
    chrono_hall_init(&hall);

    assert(chrono_hall_store_load(path, &restored) ==
           CHRONO_HALL_STORE_NOT_FOUND);

    assert(chrono_hall_record(&hall, true, CHRONO_PLAN_DUCK,
                             20261125, "GIA", 1300, &ranks) == 0);

    assert(chrono_hall_store_save(path, &hall) ==
           CHRONO_HALL_STORE_OK);
    assert(chrono_hall_store_load(path, &restored) ==
           CHRONO_HALL_STORE_OK);
    assert(restored.daily[3].count == 1);
    assert(restored.all_time[3].count == 1);
    assert(strcmp(restored.daily[3].entries[0].initials,
                  "GIA") == 0);

    chrono_hall_set_day(&restored, 20261126);
    assert(restored.daily[3].count == 0);
    assert(restored.all_time[3].count == 1);

    assert(chrono_hall_store_save(path, &restored) ==
           CHRONO_HALL_STORE_OK);
    assert(chrono_hall_store_load(path, &hall) ==
           CHRONO_HALL_STORE_OK);
    assert(hall.daily_day == 20261126);
    assert(hall.daily[3].count == 0);
    assert(hall.all_time[3].count == 1);

    FILE *stream = fopen(path, "w");
    assert(stream != NULL);
    assert(fputs("BROKEN\n", stream) >= 0);
    assert(fclose(stream) == 0);

    assert(chrono_hall_store_load(path, &restored) ==
           CHRONO_HALL_STORE_CORRUPT);
    assert(chrono_hall_store_save(path, &hall) ==
           CHRONO_HALL_STORE_CORRUPT);

    assert(unlink(path) == 0);
    assert(rmdir(directory) == 0);

    puts("HALL_PERSISTENCE_GATE=PASS");
    return 0;
}
