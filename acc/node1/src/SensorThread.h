#pragma once

#include <ThreadWrapper.h>
#include "Sensor.h"

namespace acc
{
class SensorThread : public ThreadWrapper<pthread_mutex_t>
{
public:
    SensorThread(pthread_mutex_t *pLock) : ThreadWrapper(pLock), sensor(18, 24) {}
    virtual ~SensorThread() {}
    virtual void threadLoop();

private:
    Sensor sensor;
};    
}