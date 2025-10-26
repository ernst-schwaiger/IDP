#pragma once

#include <cstdint>
#include <unistd.h>

namespace acc
{

// API used by Comm Thread
[[ nodiscard ]] uint16_t getCurrentDistanceReading();

// API used by Sensor Thread
void setCurrentDistanceReading(uint16_t value);

} // namespace acc
