#include "input.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

ChronoInputResult chrono_input_read(
    FILE *stream,
    long *choice,
    int allow_quit
)
{
    char buffer[80];
    char *end;
    size_t length;
    long value;

    if (stream == NULL || choice == NULL)
        return CHRONO_INPUT_INVALID;

    if (fgets(buffer, sizeof(buffer), stream) == NULL)
        return CHRONO_INPUT_EOF;

    length = strlen(buffer);

    if (length == 0)
        return CHRONO_INPUT_INVALID;

    if (buffer[length - 1] != '\n' && !feof(stream)) {
        int ch;

        do {
            ch = fgetc(stream);
        } while (ch != '\n' && ch != EOF);

        return CHRONO_INPUT_INVALID;
    }

    if (buffer[length - 1] == '\n')
        buffer[length - 1] = '\0';

    if (allow_quit &&
        ((buffer[0] == 'q' || buffer[0] == 'Q') &&
         buffer[1] == '\0'))
        return CHRONO_INPUT_QUIT;

    errno = 0;
    value = strtol(buffer, &end, 10);

    if (errno != 0 || end == buffer || *end != '\0')
        return CHRONO_INPUT_INVALID;

    *choice = value;
    return CHRONO_INPUT_NUMBER;
}
