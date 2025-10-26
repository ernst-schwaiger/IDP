#pragma once

#include <cstdint>

namespace acc
{

class Sensor
{
public:
    Sensor(uint8_t trigPin, uint8_t echoPin);
    ~Sensor(void);
    [[ nodiscard ]] double getDistanceCm(void);

private:
    uint8_t trigPin;
    uint8_t echoPin;
};

} // namespace acc