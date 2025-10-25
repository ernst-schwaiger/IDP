#pragma once

#include <cstdint>
#include <unistd.h>

// API used by Comm Thread
uint16_t getCurrentDistanceReading();

// API used by Sensor Thread
void setCurrentDistanceReading(uint16_t value);