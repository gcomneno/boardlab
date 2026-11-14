#ifndef RMC_MATCH_CLI_H
#define RMC_MATCH_CLI_H

#include <stdio.h>

/* argv includes the program name. Streams are borrowed, never closed.
 * Returns 0 on completion/help, 1 on runtime/I/O failure, 2 on invalid
 * options/selection, 3 on interactive EOF. No stdin reads in --auto. */
int rmc_match_cli_run(int argc, const char *const argv[],
    FILE *input, FILE *output, FILE *error);

#endif
