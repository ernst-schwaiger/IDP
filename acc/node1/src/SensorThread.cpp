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

uint16_t acc::validRange(uint16_t distance)
{
    if (distance < 5) 
    {
    return INVALID_DISTANCE_BELOW;
    } else if (distance > 400)
    {
    return INVALID_DISTANCE_ABOVE;
    }
    else return VALID_RANGE;
}

bool acc::validDviation(uint16_t distance1, uint16_t distance2)
{
   double diff = std::abs(distance1 - distance2);
    double maxAllowed = std::abs(distance1) * 0.2; // 20 %

    return diff < maxAllowed;
}

void acc::SensorThread::run(void)
{
    // check the valisity of the sensor readings at start up
    // TODO loop for the check
    Sensor sensor1(18, 24);
    Sensor sensor2(17, 23);
    

    usleep(SAMPLE_INTERVALL_US);
    uint16_t startupd1 = sensor1.getDistanceCm();

    usleep(SAMPLE_INTERVALL_US);
    uint16_t startupd2 = sensor2.getDistanceCm();
    // check deviation between sensor 1 and 2
    // if deviation is to high, the range is not regarded
    
    if(validRange(startupd1) == VALID_RANGE && validRange(startupd2) == VALID_RANGE)
    {
        if(!validDviation(startupd1,startupd2))
        {
            cout << "invalid deviation\n";
            setCurrentDistanceReading(INVALID_READING);
            return;
        }
    } else if(validRange(startupd1) == VALID_RANGE)
    {
        cout << startupd2;
        setCurrentDistanceReading(validRange(startupd2));
        return;
    } else if(validRange(startupd2) == VALID_RANGE)
    {
        cout << startupd1;
        setCurrentDistanceReading(validRange(startupd1));
        return;
    } else 
    {
        cout << startupd1 << "---" << startupd2;
        setCurrentDistanceReading(INVALID_READING);
        return;
    }
    
    
    
    /*if(validDviation(startupd1,startupd2))
    {
        cout << "Deviation OK\n";
        if(validRange(startupd1) != 1 || validRange(startupd2) != 1)
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
    }*/
    // TODO exit thread if Sesnor readings invalid
    while (!terminateApp())
    {
        uint16_t d1 = sensor1.getDistanceCm(); // we just assume everything is OK here...
        uint16_t nextReadingVal = static_cast<uint16_t>(floor(d1));
        // Sleep 5ms
        cout << nextReadingVal << "cm d1\n";
        usleep(SAMPLE_INTERVALL_US);

        uint16_t d2 = sensor2.getDistanceCm(); // we just assume everything is OK here...
        nextReadingVal = static_cast<uint16_t>(floor(d2));
        cout << nextReadingVal << "cm d2\n";
        // check deviation and range
        if(validRange(d1) == VALID_RANGE && validRange(d2) == VALID_RANGE)
        {
            if(!validDviation(d1,d2))
            {
                cout << "invalid deviation\n";
                setCurrentDistanceReading(INVALID_READING);
            } else
            {
                cout << "valid reading\n";
                setCurrentDistanceReading(nextReadingVal);
            }
        } else if(validRange(d1) == VALID_RANGE)
        {
            cout << "d2 invalid distance " << d2 << "cm\n";
            setCurrentDistanceReading(validRange(startupd2));
        } else if(validRange(d2) == VALID_RANGE)
        {
            cout << "d1 invalid distance " << d1 << "cm\n";
            setCurrentDistanceReading(validRange(startupd1));
        } else
        {
            cout << "invalid d1 d2\n";
            setCurrentDistanceReading(INVALID_READING);
        }
        usleep(SAMPLE_INTERVALL_US);

    }

    
}