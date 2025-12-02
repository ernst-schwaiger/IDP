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
    uint16_t doMeasure(Sensor &sensor1, Sensor &sensor2, bool &isReading1Valid, bool &isReading2Valid, bool &isDeviationValid);

    static bool validRange(double distance);
    static bool validDeviation(double distance1, double distance2);
    static uint16_t toUint16(double distance);


    Sensor sensor;
};

} // namespace acc