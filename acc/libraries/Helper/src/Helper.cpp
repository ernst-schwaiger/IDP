#include <time.h>
#include <assert.h>

#include "Helper.h"

namespace acc
{

uint64_t getTimestampMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return ms;
}

uint32_t getTimestampMsSinceBaseline(uint64_t baselineMs)
{
    uint64_t currentTimeMs = getTimestampMs();
    assert(currentTimeMs >= baselineMs);
    return static_cast<uint32_t>(currentTimeMs - baselineMs);
}

}

