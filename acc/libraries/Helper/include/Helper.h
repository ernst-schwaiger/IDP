#pragma once

#include <cstdint>

namespace acc
{
uint64_t getTimestampMs(void);
uint32_t getTimestampMsSinceBaseline(uint64_t baselineMs);
}