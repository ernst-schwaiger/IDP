#pragma once

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