#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int chrono_m001_run(void);
int chrono_m002_run(void);
int chrono_m003_run(void);
int chrono_m004_run(void);

static int read_selection(void)
{
    char buffer[80];
    char *end;
    long selection;

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 0;
    }

    errno = 0;
    selection = strtol(buffer, &end, 10);

    if (errno != 0 || end == buffer ||
        (*end != '\n' && *end != '\0') ||
        selection < 0 || selection > 4) {
        return -1;
    }

    return (int)selection;
}

int main(void)
{
    for (;;) {
        puts("\n====================================");
        puts("             ChronoOops!");
        puts("        Un pasticcio nel tempo");
        puts("====================================");
        puts(" 1. Missione 001 — La capsula perduta");
        puts(" 2. Missione 002 — Il prestito di domani");
        puts(" 3. Missione 003 — Il messaggio che non doveva arrivare");
        puts(" 4. Missione 004 — Due eventi, una sola causa");
        puts(" 0. Esci");
        printf("\nScelta: ");
        fflush(stdout);

        int selection = read_selection();

        if (selection == 0) {
            puts("Arrivederci da ChronoOops!");
            return 0;
        }

        if (selection == -1) {
            puts("Scelta non valida. Inserisci un numero da 0 a 4.");
            continue;
        }

        int result;
        if (selection == 1) {
            result = chrono_m001_run();
        } else if (selection == 2) {
            result = chrono_m002_run();
        } else if (selection == 3) {
            result = chrono_m003_run();
        } else {
            result = chrono_m004_run();
        }

        if (result != 0) {
            return result;
        }

        puts("\nRitorno al menu principale.");
    }
}
