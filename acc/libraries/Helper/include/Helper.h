#pragma once

#include <cstdint>

namespace acc
{
uint64_t getTimestampMs();
uint32_t getTimestampMsSinceBaseline(uint64_t baselineMs);
}