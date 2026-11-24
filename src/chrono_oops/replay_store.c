#define _POSIX_C_SOURCE 200809L
#include "replay_store.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#endif

#define STORE_HEADER "CHRONO_REPLAY_V1\n"

static unsigned allowed_bits(unsigned mission)
{
    switch (mission) {
    case 1: return CHRONO_REPLAY_TIDE_REGISTER;
    case 4: return CHRONO_REPLAY_RECONFIGURED;
    case 5: return CHRONO_REPLAY_CORRECTION_RECOVERED;
    default: return 0;
    }
}

static bool valid_record(unsigned mission, ChronoReplayMission r)
{
    if ((r.achievements & ~allowed_bits(mission)) != 0)
        return false;

    if (r.victories == 0)
        return r.best_turn == 0 && r.achievements == 0;

    return r.best_turn != 0 && r.last_run_id != 0;
}

/* Strict decimal parser: exactly five non-negative numbers. */
static bool parse_record(const char *line, unsigned mission,
                         ChronoReplayMission *record)
{
    uintmax_t fields[5];
    const char *cursor = line;

    for (unsigned i = 0; i < 5; ++i) {
        while (*cursor == ' ' || *cursor == '\t')
            ++cursor;

        if (*cursor < '0' || *cursor > '9')
            return false;

        errno = 0;
        char *end = NULL;
        fields[i] = strtoumax(cursor, &end, 10);
        if (errno == ERANGE || end == cursor)
            return false;
        if (*end != ' ' && *end != '\t' &&
            *end != '\n' && *end != '\0')
            return false;
        cursor = end;
    }

    while (*cursor == ' ' || *cursor == '\t')
        ++cursor;
    if (*cursor != '\n' && *cursor != '\0')
        return false;

    if (fields[0] != mission ||
        fields[1] > UINT_MAX ||
        fields[2] > UINT_MAX ||
        fields[3] > UINT_MAX ||
        fields[4] > UINT64_MAX)
        return false;

    record->victories = (unsigned)fields[1];
    record->best_turn = (unsigned)fields[2];
    record->achievements = (unsigned)fields[3];
    record->last_run_id = (uint64_t)fields[4];

    return valid_record(mission, *record);
}

ChronoStoreResult chrono_store_load(
    const char *path, ChronoReplayProgress *progress
)
{
    if (path == NULL || *path == '\0' || progress == NULL)
        return CHRONO_STORE_IO_ERROR;

    FILE *stream = fopen(path, "r");
    if (stream == NULL)
        return errno == ENOENT ? CHRONO_STORE_NOT_FOUND :
                                 CHRONO_STORE_IO_ERROR;

    ChronoStoreResult result = CHRONO_STORE_CORRUPT;
    ChronoReplayProgress candidate = {0};
    char line[256];

    if (fgets(line, sizeof(line), stream) == NULL)
        goto done;
    if (strcmp(line, STORE_HEADER) != 0)
        goto done;

    for (unsigned mission = 1;
         mission <= CHRONO_REPLAY_MISSIONS; ++mission) {
        if (fgets(line, sizeof(line), stream) == NULL)
            goto done;
        if (!parse_record(line, mission,
                          &candidate.missions[mission - 1]))
            goto done;
    }

    if (fgetc(stream) != EOF)
        goto done;
    if (ferror(stream))
        goto done;

    *progress = candidate;
    result = CHRONO_STORE_OK;

done:
    if (ferror(stream))
        result = CHRONO_STORE_IO_ERROR;
    if (fclose(stream) != 0)
        result = CHRONO_STORE_IO_ERROR;
    return result;
}

ChronoStoreResult chrono_store_save(
    const char *path, const ChronoReplayProgress *progress
)
{
    if (path == NULL || *path == '\0' || progress == NULL)
        return CHRONO_STORE_IO_ERROR;

    for (unsigned mission = 1;
         mission <= CHRONO_REPLAY_MISSIONS; ++mission) {
        if (!valid_record(mission,
                          progress->missions[mission - 1]))
            return CHRONO_STORE_CORRUPT;
    }

    /* Never overwrite an existing unreadable or corrupt save. */
    ChronoReplayProgress existing;
    ChronoStoreResult check = chrono_store_load(path, &existing);
    if (check != CHRONO_STORE_OK &&
        check != CHRONO_STORE_NOT_FOUND)
        return check;

    size_t length = strlen(path);
    if (length > SIZE_MAX - 12)
        return CHRONO_STORE_IO_ERROR;

    char *temporary = malloc(length + 12);
    if (temporary == NULL)
        return CHRONO_STORE_IO_ERROR;

    snprintf(temporary, length + 12, "%s.tmp.XXXXXX", path);
    int fd = mkstemp(temporary);
    if (fd < 0) {
        free(temporary);
        return CHRONO_STORE_IO_ERROR;
    }

    FILE *stream = fdopen(fd, "w");
    if (stream == NULL) {
        close(fd);
        unlink(temporary);
        free(temporary);
        return CHRONO_STORE_IO_ERROR;
    }

    bool ok = fputs(STORE_HEADER, stream) >= 0;
    for (unsigned mission = 1;
         ok && mission <= CHRONO_REPLAY_MISSIONS; ++mission) {
        ChronoReplayMission r = progress->missions[mission - 1];
        ok = fprintf(stream, "%u %u %u %u %" PRIu64 "\n",
                     mission, r.victories, r.best_turn,
                     r.achievements, r.last_run_id) >= 0;
    }

    if (ok)
        ok = fflush(stream) == 0;
    if (ok) {
#ifdef _WIN32
        ok = _commit(_fileno(stream)) == 0;
#else
        ok = fsync(fileno(stream)) == 0;
#endif
    }
    if (fclose(stream) != 0)
        ok = false;
    if (ok) {
#ifdef _WIN32
        ok = MoveFileExA(temporary, path,
                         MOVEFILE_REPLACE_EXISTING |
                         MOVEFILE_WRITE_THROUGH) != 0;
#else
        ok = rename(temporary, path) == 0;
#endif
    }

    if (!ok)
        unlink(temporary);
    free(temporary);

    return ok ? CHRONO_STORE_OK : CHRONO_STORE_IO_ERROR;
}
