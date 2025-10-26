#pragma once

#include <cstdint>

namespace acc
{
[[ nodiscard ]] uint64_t getTimestampMs(void);
[[ nodiscard ]] uint32_t getTimestampMsSinceBaseline(uint64_t baselineMs);
}