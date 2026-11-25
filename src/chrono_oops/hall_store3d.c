#define _POSIX_C_SOURCE 200809L
#include "hall_store3d.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HALL_HEADER "CHRONO_HALL_V1\n"

static bool valid_date(unsigned date)
{
    ChronoHall probe = {0};
    chrono_hall_set_day(&probe, date);
    return probe.daily_day == date;
}

static bool valid_board(const ChronoHallBoard *board,
                        unsigned daily_day, bool daily)
{
    if (board->count > CHRONO_HALL_CAPACITY)
        return false;

    for (unsigned i = 0; i < board->count; ++i) {
        const ChronoHallEntry *entry = &board->entries[i];
        if (entry->score == 0 || !valid_date(entry->day))
            return false;
        if (daily && entry->day != daily_day)
            return false;
        for (unsigned j = 0; j < 3; ++j) {
            if (entry->initials[j] < 'A' ||
                entry->initials[j] > 'Z')
                return false;
        }
        if (entry->initials[3] != '\0')
            return false;
        if (i && board->entries[i - 1].score < entry->score)
            return false;
    }
    return true;
}

static bool valid_hall(const ChronoHall *hall)
{
    if (hall->daily_day != 0 && !valid_date(hall->daily_day))
        return false;

    for (unsigned c = 0; c < CHRONO_HALL_CATEGORIES; ++c) {
        if (!valid_board(&hall->daily[c], hall->daily_day, true) ||
            !valid_board(&hall->all_time[c], 0, false))
            return false;
        if (hall->daily_day == 0 && hall->daily[c].count != 0)
            return false;
    }
    return true;
}

static bool parse_unsigned(const char *line, unsigned *value)
{
    int consumed = 0;
    return sscanf(line, "%u %n", value, &consumed) == 1 &&
           consumed > 0 && line[consumed] == '\0' &&
           line[0] >= '0' && line[0] <= '9';
}

static bool read_board(FILE *stream, ChronoHallBoard *board)
{
    char line[128];
    unsigned count = 0;

    if (fgets(line, sizeof(line), stream) == NULL ||
        !parse_unsigned(line, &count) ||
        count > CHRONO_HALL_CAPACITY)
        return false;

    board->count = count;
    for (unsigned i = 0; i < count; ++i) {
        char name[5] = {0};
        unsigned score = 0;
        unsigned date = 0;
        int consumed = 0;

        if (fgets(line, sizeof(line), stream) == NULL ||
            sscanf(line, "%4s %u %u %n",
                   name, &score, &date, &consumed) != 3 ||
            consumed == 0 || line[consumed] != '\0' ||
            strlen(name) != 3)
            return false;

        memcpy(board->entries[i].initials, name, 4);
        board->entries[i].score = score;
        board->entries[i].day = date;
    }
    return true;
}

ChronoHallStoreResult chrono_hall_store_load(
    const char *path, ChronoHall *hall)
{
    if (path == NULL || *path == '\0' || hall == NULL)
        return CHRONO_HALL_STORE_IO_ERROR;

    FILE *stream = fopen(path, "r");
    if (stream == NULL)
        return errno == ENOENT ? CHRONO_HALL_STORE_NOT_FOUND :
                                 CHRONO_HALL_STORE_IO_ERROR;

    ChronoHall candidate = {0};
    ChronoHallStoreResult result = CHRONO_HALL_STORE_CORRUPT;
    char line[128];

    if (fgets(line, sizeof(line), stream) == NULL ||
        strcmp(line, HALL_HEADER) != 0)
        goto done;

    if (fgets(line, sizeof(line), stream) == NULL ||
        !parse_unsigned(line, &candidate.daily_day))
        goto done;

    for (unsigned c = 0; c < CHRONO_HALL_CATEGORIES; ++c) {
        if (!read_board(stream, &candidate.daily[c]))
            goto done;
        if (!read_board(stream, &candidate.all_time[c]))
            goto done;
    }

    if (fgetc(stream) != EOF || ferror(stream) ||
        !valid_hall(&candidate))
        goto done;

    *hall = candidate;
    result = CHRONO_HALL_STORE_OK;

done:
    if (ferror(stream) || fclose(stream) != 0)
        result = CHRONO_HALL_STORE_IO_ERROR;
    return result;
}

static bool write_board(FILE *stream, const ChronoHallBoard *board)
{
    if (fprintf(stream, "%u\n", board->count) < 0)
        return false;

    for (unsigned i = 0; i < board->count; ++i) {
        const ChronoHallEntry *entry = &board->entries[i];
        if (fprintf(stream, "%s %u %u\n",
                    entry->initials, entry->score, entry->day) < 0)
            return false;
    }
    return true;
}

ChronoHallStoreResult chrono_hall_store_save(
    const char *path, const ChronoHall *hall)
{
    if (path == NULL || *path == '\0' ||
        hall == NULL || !valid_hall(hall))
        return CHRONO_HALL_STORE_CORRUPT;

    ChronoHall existing;
    ChronoHallStoreResult check =
        chrono_hall_store_load(path, &existing);
    if (check != CHRONO_HALL_STORE_OK &&
        check != CHRONO_HALL_STORE_NOT_FOUND)
        return check;

    size_t length = strlen(path);
    if (length > (size_t)-1 - 12)
        return CHRONO_HALL_STORE_IO_ERROR;

    char *temporary = malloc(length + 12);
    if (temporary == NULL)
        return CHRONO_HALL_STORE_IO_ERROR;

    snprintf(temporary, length + 12, "%s.tmp.XXXXXX", path);
    int fd = mkstemp(temporary);
    if (fd < 0) {
        free(temporary);
        return CHRONO_HALL_STORE_IO_ERROR;
    }

    FILE *stream = fdopen(fd, "w");
    if (stream == NULL) {
        close(fd);
        unlink(temporary);
        free(temporary);
        return CHRONO_HALL_STORE_IO_ERROR;
    }

    bool ok = fputs(HALL_HEADER, stream) >= 0 &&
              fprintf(stream, "%u\n", hall->daily_day) >= 0;

    for (unsigned c = 0; ok && c < CHRONO_HALL_CATEGORIES; ++c) {
        ok = write_board(stream, &hall->daily[c]) &&
             write_board(stream, &hall->all_time[c]);
    }

    if (ok)
        ok = fflush(stream) == 0;
    if (ok)
        ok = fsync(fileno(stream)) == 0;
    if (fclose(stream) != 0)
        ok = false;
    if (ok)
        ok = rename(temporary, path) == 0;
    if (!ok)
        unlink(temporary);

    free(temporary);
    return ok ? CHRONO_HALL_STORE_OK :
                CHRONO_HALL_STORE_IO_ERROR;
}
