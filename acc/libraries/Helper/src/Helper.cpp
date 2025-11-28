#include <time.h>
#include <assert.h>

#include "Helper.h"

namespace acc
{

// gets current timestamp in units of milliseconds
uint64_t getTimestampMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t ms = ts.tv_sec * 1'000U + ts.tv_nsec / 1'000'000U;
    return ms;
}

// gets elapsed milliseconds since a time baseline
uint32_t getTimestampMsSinceBaseline(uint64_t baselineMs)
{
    uint64_t currentTimeMs = getTimestampMs();
    assert(currentTimeMs >= baselineMs); // someone turned the clock back -> terminate!
    return static_cast<uint32_t>(currentTimeMs - baselineMs);
}

}

