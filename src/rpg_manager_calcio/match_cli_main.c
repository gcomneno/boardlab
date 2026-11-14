#include "match_cli.h"

int main(int argc, char *argv[])
{
    return rmc_match_cli_run(argc, (const char *const *)argv,
        stdin, stdout, stderr);
}
