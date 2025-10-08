#include <time.h>
#include "Helper.h"

namespace acc
{

uint64_t getTimestampMs()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return ms;
}

}

