#pragma once

#include <ThreadWrapper.h>
#include "Sensor.h"

static constexpr uint8_t TRIGGER_PIN = 18U;
static constexpr uint8_t ECHO_PIN = 24U;

namespace acc
{
class SensorThread : public ThreadWrapper<pthread_mutex_t>
{
public:
    SensorThread(bool &terminateApp, pthread_mutex_t *pLock) : ThreadWrapper(terminateApp, pLock), sensor(TRIGGER_PIN, ECHO_PIN) {}
    virtual ~SensorThread(void) {}
    virtual void run(void);

private:
    Sensor sensor;
};    
}