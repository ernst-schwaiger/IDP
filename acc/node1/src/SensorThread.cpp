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

static constexpr uint8_t TRIGGER_PIN_1 = 18U;
static constexpr uint8_t ECHO_PIN_1 = 24U;
static constexpr uint8_t TRIGGER_PIN_2 = 17U;
static constexpr uint8_t ECHO_PIN_2 = 23U;

using namespace std;
using namespace acc;

//Saf-REQ-2: This function chekcks if the measured range is within certain boundarys. In the implemetation values below 10
//and above 400 meters
bool acc::SensorThread::validRange(uint16_t distance)
{
    return (distance >= 10) && (distance <= 400);
}

//Saf-REQ-2: This funtion checks if the deviation of the measuremnt between the sensors is greater than 20%
bool acc::SensorThread::validDeviation(uint16_t distance1, uint16_t distance2)
{
    uint16_t diff = std::abs(distance1 - distance2);
    uint16_t maxAllowed = std::abs(distance1) / 5U; // 20% deviation is OK for us, we pick the lower distance reading anyway
    return diff < maxAllowed;
}

//Saf-REQ-2: This function 
/*uint16_t acc::SensorThread::clamp(uint16_t distance)
{

    if (distance > INVALID_DISTANCE_ABOVE) // we assume distances > 400 as 400
    {
        distance = INVALID_DISTANCE_ABOVE;
    }

    if (distance < MEASURE_MINIMUM) // anything between MEASURE_MINIMUM and 0 is considered 0
    {
        distance = INVALID_DISTANCE_BELOW;
    }

    return distance;
}*/

uint16_t acc::SensorThread::doMeasure(Sensor &sensor1, Sensor &sensor2)
{
    usleep(SAMPLE_INTERVALL_US);
    uint16_t d1 = sensor1.getDistanceCm();
    usleep(SAMPLE_INTERVALL_US);
    uint16_t d2 = sensor2.getDistanceCm();

    bool isReading1Valid = validRange(d1);
    bool isReading2Valid = validRange(d2);
    bool isDeviationValid = isReading1Valid && isReading2Valid && validDeviation(d1, d2);
    
    return isDeviationValid ? min(d1, d2) : INVALID_READING;
}

void acc::SensorThread::run(void)
{
    Sensor sensor1(TRIGGER_PIN_1, ECHO_PIN_1);
    Sensor sensor2(TRIGGER_PIN_2, ECHO_PIN_2);


    // check the validity of the sensor readings at start up
    // Initial measurement, in a loop(?)
    // If logging is not required, we can remove all these booleans
    uint16_t currentReading = doMeasure(sensor1, sensor2);

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
        currentReading = doMeasure(sensor1, sensor2);
        setCurrentDistanceReading(currentReading);
        // Log status of initial measurement, if required
    }    
}