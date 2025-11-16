#pragma once

#include <ThreadWrapper.h>
#include "Sensor.h"

static constexpr uint8_t TRIGGER_PIN = 18U;
static constexpr uint8_t ECHO_PIN = 24U;

namespace acc
{
class SensorThread : public ThreadWrapper<void>
{
public:
    explicit SensorThread(bool &terminateApp) : ThreadWrapper(terminateApp, nullptr), sensor(TRIGGER_PIN, ECHO_PIN) {}
    virtual ~SensorThread(void) override {}
    virtual void run(void) override;

private:
    Sensor sensor;
};    
uint16_t validRange(double distance);
bool validDviation(double distance1, double distance2);
}