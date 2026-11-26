#include "hall3d.h"

#include <string.h>

static int category(bool hard_mode, ChronoPlan plan)
{
    if (plan != CHRONO_PLAN_BRIDGE && plan != CHRONO_PLAN_DUCK)
        return -1;
    return (hard_mode ? 2 : 0) +
           (plan == CHRONO_PLAN_DUCK ? 1 : 0);
}

static bool valid_day(unsigned day)
{
    unsigned year = day / 10000u;
    unsigned month = (day / 100u) % 100u;
    unsigned d = day % 100u;
    if (year < 1970u || year > 9999u ||
        month < 1u || month > 12u)
        return false;

    static const unsigned days[] = {
        0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    unsigned limit = days[month];
    if (month == 2u &&
        year % 4u == 0u &&
        (year % 100u != 0u || year % 400u == 0u))
        limit = 29u;
    return d >= 1u && d <= limit;
}

static bool valid_initials(const char *initials)
{
    if (initials == NULL)
        return false;
    for (unsigned i = 0; i < 3; ++i) {
        if (initials[i] < 'A' || initials[i] > 'Z')
            return false;
    }
    return initials[3] == '\0';
}

static unsigned insert(ChronoHallBoard *board, ChronoHallEntry entry)
{
    unsigned position = 0;
    while (position < board->count &&
           board->entries[position].score >= entry.score)
        ++position;

    if (position >= CHRONO_HALL_CAPACITY)
        return 0;

    unsigned limit = board->count;
    if (limit < CHRONO_HALL_CAPACITY)
        ++board->count;
    else
        limit = CHRONO_HALL_CAPACITY - 1u;

    for (unsigned i = limit; i > position; --i)
        board->entries[i] = board->entries[i - 1u];
    board->entries[position] = entry;
    return position + 1u;
}

void chrono_hall_init(ChronoHall *hall)
{
    if (hall != NULL)
        *hall = (ChronoHall){0};
}

void chrono_hall_set_day(ChronoHall *hall, unsigned day)
{
    if (hall == NULL || !valid_day(day))
        return;
    if (hall->daily_day != day) {
        memset(hall->daily, 0, sizeof(hall->daily));
        hall->daily_day = day;
    }
}

int chrono_hall_record(
    ChronoHall *hall, bool hard_mode, ChronoPlan plan,
    unsigned day, const char *initials, unsigned score,
    ChronoHallRanks *ranks)
{
    int index = category(hard_mode, plan);
    if (hall == NULL || index < 0 || !valid_day(day) ||
        !valid_initials(initials) || score == 0)
        return -1;

    chrono_hall_set_day(hall, day);

    ChronoHallEntry entry = {{0}, score, day};
    memcpy(entry.initials, initials, 4u);

    ChronoHallRanks result = {
        insert(&hall->daily[index], entry),
        insert(&hall->all_time[index], entry)
    };
    if (ranks != NULL)
        *ranks = result;
    return 0;
}
