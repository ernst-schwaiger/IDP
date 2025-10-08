#pragma once

#include <cstdint>
#include <unistd.h>

// API used by Comm Thread
uint16_t getCurrentDistanceReading(pthread_mutex_t *pLock);

// API used by Sensor Thread
void setCurrentDistanceReading(pthread_mutex_t *pLock, uint16_t value);