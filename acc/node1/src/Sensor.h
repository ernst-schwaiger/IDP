#pragma once

#include <cstdint>

namespace acc
{

class Sensor
{
public:
    Sensor(uint8_t trigPin, uint8_t echoPin);
    ~Sensor(void);
    [[ nodiscard ]] uint16_t getDistanceCm(void);

private:
    uint8_t trigPin;
    uint8_t echoPin;
};

} // namespace acc