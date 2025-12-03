#pragma once

#include <ThreadWrapper.h>
#include "Sensor.h"

namespace acc
{
class SensorThread : public ThreadWrapper<void>
{
public:
    explicit SensorThread(bool &terminateApp) : ThreadWrapper(terminateApp, nullptr) {}
    virtual ~SensorThread(void) override {}
    virtual void run(void) override;

private:
    [[ nodiscard ]] static uint16_t doMeasure(Sensor &sensor1, Sensor &sensor2);

    [[ nodiscard ]] static bool validRange(uint16_t distance);
    [[ nodiscard ]] static bool validDeviation(uint16_t distance1, uint16_t distance2);
    [[ nodiscard ]] static uint16_t clamp(uint16_t distance);
};

} // namespace acc
