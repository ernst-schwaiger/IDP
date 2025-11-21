#include <stdexcept>
#include <iostream>
#include <cmath>
#include <unistd.h>

#include <Helper.h>
#include "SensorThread.h"
#include "Node1Types.h"

constexpr uint16_t SAMPLE_INTERVALL_US = 5'000;
constexpr uint16_t INVALID_DISTANCE_BELOW = 0;
constexpr uint16_t INVALID_DISTANCE_ABOVE = 400;
constexpr uint16_t INVALID_READING = 0xffff;
constexpr uint16_t VALID_RANGE = 1;

using namespace std;
using namespace acc;

uint16_t acc::validRange(double distance)
{
    if (distance < 10.0) 
    {
    return INVALID_DISTANCE_BELOW;
    } else if (distance > 400.0)
    {
    return INVALID_DISTANCE_ABOVE;
    }
    else return VALID_RANGE;
}

bool acc::validDviation(double distance1, double distance2)
{
   double diff = std::abs(distance1 - distance2);
    double maxAllowed = std::abs(distance1) * 0.1; // 10 %

    return diff < maxAllowed;
}

void acc::SensorThread::run(void)
{
    // check the valisity of the sensor readings at start up
    // TODO loop for the check
    Sensor sensor1(18, 24);
    Sensor sensor2(17, 23);
    

    usleep(SAMPLE_INTERVALL_US);
    double startupd1 = sensor1.getDistanceCm();

    usleep(SAMPLE_INTERVALL_US);
    double startupd2 = sensor2.getDistanceCm();
    // check deviation between sensor 1 and 2
    // if deviation is to high, the range is not regarded
    if(validDviation(startupd1,startupd2))
    {
        cout << "Deviation OK\n";
        if(validRange(startupd1) == 1 && validRange(startupd2) == 1)
        {
        // just for demonstration
        // TODO rework the if clause logic here
        cout << "Reading valid\n";
        } else
        {
        setCurrentDistanceReading(INVALID_READING);
        cout << "Reading invalid\n";
        }
    } else
    {
       cout << "Deviation NOK\n";
       setCurrentDistanceReading(INVALID_READING);
    }
    // TODO exit thread if Sesnor readings invalid
    while (!terminateApp())
    {
        double d1 = sensor1.getDistanceCm(); // we just assume everything is OK here...
        cout << d1 <<"cm\n";
        uint16_t nextReadingVal = static_cast<uint16_t>(floor(d1));
        // Sleep 5ms
        usleep(SAMPLE_INTERVALL_US);

        double d2 = sensor2.getDistanceCm(); // we just assume everything is OK here...
        nextReadingVal = static_cast<uint16_t>(floor(d2));
        cout << d2 <<"cm\n";
        // check deviation
        if(validDviation(startupd1,startupd2))
        {
        cout << "Deviation OK\n";
        // if deviation is ok, check plausibility of reading
            if(validRange(startupd1) == 1 && validRange(startupd2) == 1)
            {
            cout << "Reading valid\n";
            setCurrentDistanceReading(nextReadingVal);
            } else 
            {
            cout << "Reading invalid\n";
            setCurrentDistanceReading(INVALID_READING);
            }
        } else
        {
        cout << "Deviation NOK\n";
        setCurrentDistanceReading(INVALID_READING);
        }
        // sleep 5 ms
        usleep(SAMPLE_INTERVALL_US);

    }

    
}