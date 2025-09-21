#pragma once
#include <pigpio.h>

class Sensor
{
public:
    Sensor(int trigPin, int echoPin);
    ~Sensor() = default;
    double getDistanceCm();

private:
    int trigPin;
    int echoPin;
};