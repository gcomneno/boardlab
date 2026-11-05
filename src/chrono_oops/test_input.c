#include "input.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static FILE *stream_from(const char *text)
{
    FILE *stream = tmpfile();
    assert(stream != NULL);
    assert(fputs(text, stream) >= 0);
    rewind(stream);
    return stream;
}

static void expect(
    FILE *stream,
    int allow_quit,
    ChronoInputResult expected,
    long expected_number
)
{
    long number = 1234567;
    ChronoInputResult result =
        chrono_input_read(stream, &number, allow_quit);

    assert(result == expected);

    if (expected == CHRONO_INPUT_NUMBER)
        assert(number == expected_number);
    else
        assert(number == 1234567);
}

int main(void)
{
    FILE *stream;

    stream = stream_from("3\n0\n-2\n");
    expect(stream, 1, CHRONO_INPUT_NUMBER, 3);
    expect(stream, 1, CHRONO_INPUT_NUMBER, 0);
    expect(stream, 1, CHRONO_INPUT_NUMBER, -2);
    expect(stream, 1, CHRONO_INPUT_EOF, 0);
    assert(fclose(stream) == 0);
    puts("NUMERIC_AND_EOF_GATE=PASS");

    stream = stream_from("q\nQ\nq123\nq \n5\n");
    expect(stream, 1, CHRONO_INPUT_QUIT, 0);
    expect(stream, 1, CHRONO_INPUT_QUIT, 0);
    expect(stream, 1, CHRONO_INPUT_INVALID, 0);
    expect(stream, 1, CHRONO_INPUT_INVALID, 0);
    expect(stream, 1, CHRONO_INPUT_NUMBER, 5);
    assert(fclose(stream) == 0);

    stream = stream_from("q\n2\n");
    expect(stream, 0, CHRONO_INPUT_INVALID, 0);
    expect(stream, 0, CHRONO_INPUT_NUMBER, 2);
    assert(fclose(stream) == 0);
    puts("QUIT_COMMAND_GATE=PASS");

    stream = stream_from(
        "\nabc\n1x\n"
        "999999999999999999999999999999999999999999999\n"
        "4\n"
    );
    expect(stream, 1, CHRONO_INPUT_INVALID, 0);
    expect(stream, 1, CHRONO_INPUT_INVALID, 0);
    expect(stream, 1, CHRONO_INPUT_INVALID, 0);
    expect(stream, 1, CHRONO_INPUT_INVALID, 0);
    expect(stream, 1, CHRONO_INPUT_NUMBER, 4);
    assert(fclose(stream) == 0);
    puts("INVALID_AND_OVERFLOW_GATE=PASS");

    {
        char oversized[260];
        memset(oversized, '7', 250);
        oversized[250] = '\n';
        oversized[251] = '5';
        oversized[252] = '\n';
        oversized[253] = '\0';

        stream = stream_from(oversized);
        expect(stream, 1, CHRONO_INPUT_INVALID, 0);
        expect(stream, 1, CHRONO_INPUT_NUMBER, 5);
        expect(stream, 1, CHRONO_INPUT_EOF, 0);
        assert(fclose(stream) == 0);
    }
    puts("OVERSIZED_LINE_RECOVERY_GATE=PASS");

    stream = stream_from("2");
    expect(stream, 1, CHRONO_INPUT_NUMBER, 2);
    expect(stream, 1, CHRONO_INPUT_EOF, 0);
    assert(fclose(stream) == 0);
    puts("UNTERMINATED_FINAL_LINE_GATE=PASS");

    assert(chrono_input_read(NULL, NULL, 1) == CHRONO_INPUT_INVALID);
    puts("INVALID_ARGUMENT_GATE=PASS");

    puts("SHARED_INPUT_REGRESSION_GATE=PASS");
    return 0;
}
