#pragma once
#include <pigpio.h>

// comment this out if no sensort are mounted to node 1
#define PROXIMITY_SENSORS_MOUNTED

namespace acc
{

class Sensor
{
public:
    Sensor(int trigPin, int echoPin);
    ~Sensor();
    double getDistanceCm();

private:
    int trigPin;
    int echoPin;
};

} // namespace acc