#include <stdexcept>
#include <iostream>
#include <cmath>
#include <unistd.h>

#include <Helper.h>
#include "SensorThread.h"
#include "Node1Types.h"

constexpr uint16_t SAMPLE_INTERVALL_US = 5'000;
constexpr uint16_t MEASURE_MINIMUM = 5;
constexpr uint16_t INVALID_DISTANCE_BELOW = 0;
constexpr uint16_t INVALID_DISTANCE_ABOVE = 400;
constexpr uint16_t INVALID_READING = 0xffff;
constexpr uint16_t VALID_RANGE = 1;

using namespace std;
using namespace acc;

bool acc::SensorThread::validRange(double distance)
{
    return (distance >= 10.0) && (distance <= 400.0);
}

bool acc::SensorThread::validDeviation(double distance1, double distance2)
{
    double diff = std::abs(distance1 - distance2);
    double maxAllowed = std::abs(distance1) * 0.2; // 20% deviation is OK for us, we pick the lower distance reading anyway

    return diff < maxAllowed;
}

uint16_t acc::SensorThread::toUint16(double distance)
{
    uint16_t ret = static_cast<uint16_t>(floor(distance));

    if (ret > INVALID_DISTANCE_ABOVE) // we assume distances > 400 as 400
    {
        ret = INVALID_DISTANCE_ABOVE;
    }

    if (ret < MEASURE_MINIMUM) // anything between MEASURE_MINIMUM and 0 is considered 0
    {
        ret = INVALID_DISTANCE_BELOW;
    }

    return ret;
}

uint16_t acc::SensorThread::doMeasure(Sensor &sensor1, Sensor &sensor2, bool &isReading1Valid, bool &isReading2Valid, bool &isDeviationValid)
{
    usleep(SAMPLE_INTERVALL_US);
    double d1 = sensor1.getDistanceCm();
    usleep(SAMPLE_INTERVALL_US);
    double d2 = sensor2.getDistanceCm();

    isReading1Valid = validRange(d1);
    isReading2Valid = validRange(d2);
    isDeviationValid = isReading1Valid && isReading2Valid && validDeviation(d1, d2);
    
    return isDeviationValid ? toUint16(min(d1, d2)) : INVALID_READING;
}

void acc::SensorThread::run(void)
{
    Sensor sensor1(18, 24);
    Sensor sensor2(17, 23);

    // check the validity of the sensor readings at start up
    // Initial measurement, in a loop(?)
    // If logging is not required, we can remove all these booleans
    bool isReading1Valid = false;
    bool isReading2Valid = false;
    bool isDeviationValid = false;
    uint16_t currentReading = doMeasure(sensor1, sensor2, isReading1Valid, isReading2Valid, isDeviationValid);

    // If we detect an invalid measurement, we bail out
    if (currentReading == INVALID_READING)
    {
        // Log status of initial measurement, if required
        return;
    }

    setCurrentDistanceReading(currentReading);

    // Measure in a loop
    while (!terminateApp())
    {
        uint16_t currentReading = doMeasure(sensor1, sensor2, isReading1Valid, isReading2Valid, isDeviationValid);
        setCurrentDistanceReading(currentReading);
        // Log status of initial measurement, if required
    }    
}