#ifndef CHRONO_INPUT_H
#define CHRONO_INPUT_H

#include <stdio.h>

typedef enum {
    CHRONO_INPUT_INVALID = -1,
    CHRONO_INPUT_EOF = 0,
    CHRONO_INPUT_NUMBER = 1,
    CHRONO_INPUT_QUIT = 2
} ChronoInputResult;

/*
 * Reads one complete input line.
 * Invalid/oversized lines are consumed without affecting the next read.
 * On NUMBER, writes the parsed value to *choice.
 * On other results, leaves *choice unchanged.
 */
ChronoInputResult chrono_input_read(
    FILE *stream,
    long *choice,
    int allow_quit
);

#endif
