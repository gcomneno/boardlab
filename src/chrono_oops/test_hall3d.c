#include "hall3d.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    ChronoHall hall;
    ChronoHallRanks ranks = {0};
    chrono_hall_init(&hall);

    assert(chrono_hall_record(&hall, false, CHRONO_PLAN_BRIDGE,
                             20261125, "GIA", 1000, &ranks) == 0);
    assert(ranks.daily_rank == 1 && ranks.all_time_rank == 1);

    assert(chrono_hall_record(&hall, false, CHRONO_PLAN_BRIDGE,
                             20261125, "BOB", 1200, &ranks) == 0);
    assert(ranks.daily_rank == 1 && ranks.all_time_rank == 1);

    assert(chrono_hall_record(&hall, false, CHRONO_PLAN_BRIDGE,
                             20261125, "DOC", 1000, &ranks) == 0);
    assert(ranks.daily_rank == 3);
    assert(strcmp(hall.daily[0].entries[1].initials, "GIA") == 0);
    assert(strcmp(hall.daily[0].entries[2].initials, "DOC") == 0);

    assert(chrono_hall_record(&hall, true, CHRONO_PLAN_DUCK,
                             20261125, "ACE", 1300, &ranks) == 0);
    assert(hall.daily[3].count == 1);
    assert(hall.daily[0].count == 3);

    for (unsigned i = 0; i < 15; ++i) {
        char name[4] = {'A', 'A', (char)('A' + i), '\0'};
        assert(chrono_hall_record(&hall, false, CHRONO_PLAN_BRIDGE,
                                 20261125, name, 900 - i,
                                 &ranks) == 0);
    }
    assert(hall.daily[0].count == 10);
    assert(hall.all_time[0].count == 10);
    assert(ranks.daily_rank == 0);

    ChronoHall before = hall;
    assert(chrono_hall_record(&hall, false, CHRONO_PLAN_BRIDGE,
                             20260230, "XYZ", 3000, &ranks) == -1);
    assert(chrono_hall_record(&hall, false, CHRONO_PLAN_NONE,
                             20261125, "XYZ", 3000, &ranks) == -1);
    assert(chrono_hall_record(&hall, false, CHRONO_PLAN_BRIDGE,
                             20261125, "xYZ", 3000, &ranks) == -1);
    assert(memcmp(&before, &hall, sizeof(hall)) == 0);

    chrono_hall_set_day(&hall, 20261126);
    assert(hall.daily[0].count == 0);
    assert(hall.all_time[0].count == 10);
    assert(hall.daily[3].count == 0);
    assert(hall.all_time[3].count == 1);

    assert(chrono_hall_record(&hall, false, CHRONO_PLAN_BRIDGE,
                             20261126, "NEW", 1500, &ranks) == 0);
    assert(ranks.daily_rank == 1 && ranks.all_time_rank == 1);

    puts("HALL_OF_FAME_CORE_GATE=PASS");
    return 0;
}
