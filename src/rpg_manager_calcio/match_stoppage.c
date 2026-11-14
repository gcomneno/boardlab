#include "match_stoppage.h"

bool rmc_stoppage_record(
    uint32_t *accumulated_seconds,
    uint32_t duration_seconds)
{
    if (accumulated_seconds == 0 ||
        duration_seconds == 0 ||
        *accumulated_seconds > RMC_STOPPAGE_MAX_SECONDS ||
        duration_seconds >
            RMC_STOPPAGE_MAX_SECONDS - *accumulated_seconds) {
        return false;
    }

    *accumulated_seconds += duration_seconds;
    return true;
}

bool rmc_stoppage_deadline(
    uint32_t period_start_second,
    uint32_t accumulated_seconds,
    uint32_t *deadline_second)
{
    if (deadline_second == 0 ||
        accumulated_seconds > RMC_STOPPAGE_MAX_SECONDS) {
        return false;
    }

    const uint32_t period_duration =
        RMC_PERIOD_REGULATION_SECONDS + accumulated_seconds;

    if (period_start_second >
        UINT32_MAX - period_duration) {
        return false;
    }

    const uint32_t next_deadline =
        period_start_second + period_duration;

    *deadline_second = next_deadline;
    return true;
}
