#define _POSIX_C_SOURCE 200809L
#include "hall_runtime3d.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static unsigned current_day(void)
{
    time_t now = time(NULL);
    struct tm *date = localtime(&now);
    if (date == NULL)
        return 0;
    return (unsigned)(date->tm_year + 1900) * 10000u +
           (unsigned)(date->tm_mon + 1) * 100u +
           (unsigned)date->tm_mday;
}

static bool storage_path(char *path, size_t capacity)
{
    const char *data = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");
    int length;

    if (data != NULL && data[0] == '/') {
        length = snprintf(path, capacity,
                          "%s/chrono-oops/hall-v1.dat", data);
    } else if (home != NULL && home[0] == '/') {
        length = snprintf(path, capacity,
                          "%s/.local/share/chrono-oops/hall-v1.dat",
                          home);
    } else {
        return false;
    }

    return length > 0 && (size_t)length < capacity;
}

static bool create_parent_directories(const char *path)
{
    char copy[1024];
    size_t length = strlen(path);
    if (length == 0 || length >= sizeof(copy))
        return false;

    memcpy(copy, path, length + 1u);

    for (char *p = copy + 1; *p != '\0'; ++p) {
        if (*p != '/')
            continue;
        *p = '\0';
        if (mkdir(copy, 0700) != 0 && errno != EEXIST)
            return false;
        struct stat info;
        if (stat(copy, &info) != 0 || !S_ISDIR(info.st_mode))
            return false;
        *p = '/';
    }
    return true;
}

void chrono_hall_session_open(ChronoHallSession *session)
{
    if (session == NULL)
        return;

    *session = (ChronoHallSession){0};
    chrono_hall_init(&session->hall);
    if (!storage_path(session->path, sizeof(session->path)))
        return;

    ChronoHallStoreResult result =
        chrono_hall_store_load(session->path, &session->hall);

    session->ready = result == CHRONO_HALL_STORE_OK ||
                     result == CHRONO_HALL_STORE_NOT_FOUND;
    session->save_error = !session->ready;
}

bool chrono_hall_session_qualifies(
    ChronoHallSession *session,
    bool hard_mode, ChronoPlan plan, unsigned score)
{
    if (session == NULL || !session->ready || score == 0)
        return false;
    if (plan != CHRONO_PLAN_BRIDGE && plan != CHRONO_PLAN_DUCK)
        return false;

    unsigned day = current_day();
    if (day == 0)
        return false;

    chrono_hall_set_day(&session->hall, day);
    unsigned category = (hard_mode ? 2u : 0u) +
        (plan == CHRONO_PLAN_DUCK ? 1u : 0u);

    const ChronoHallBoard *daily = &session->hall.daily[category];
    const ChronoHallBoard *all = &session->hall.all_time[category];

    return daily->count < CHRONO_HALL_CAPACITY ||
           all->count < CHRONO_HALL_CAPACITY ||
           score > daily->entries[CHRONO_HALL_CAPACITY - 1].score ||
           score > all->entries[CHRONO_HALL_CAPACITY - 1].score;
}

bool chrono_hall_session_record(
    ChronoHallSession *session, bool hard_mode,
    ChronoPlan plan, const char *initials, unsigned score)
{
    if (!chrono_hall_session_qualifies(
            session, hard_mode, plan, score))
        return false;

    ChronoHall next = session->hall;
    ChronoHallRanks ranks = {0};
    if (chrono_hall_record(
            &next, hard_mode, plan, current_day(),
            initials, score, &ranks) != 0)
        return false;

    if (!create_parent_directories(session->path) ||
        chrono_hall_store_save(session->path, &next) !=
            CHRONO_HALL_STORE_OK) {
        session->save_error = true;
        return false;
    }

    session->hall = next;
    session->last_ranks = ranks;
    session->saved = true;
    session->save_error = false;
    return true;
}
