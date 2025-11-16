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
    else return 1;
}

bool acc::validDviation(double distance1, double distance2)
{
   double diff = std::abs(distance1 - distance2);
    double maxAllowed = std::abs(distance1) * 0.1; // 10 %

    return diff < maxAllowed;
}

void acc::SensorThread::run(void)
{
    Sensor sensor1(18, 24);
    Sensor sensor2(17, 23);
    usleep(SAMPLE_INTERVALL_US);
    double startupd1 = sensor1.getDistanceCm();

    usleep(SAMPLE_INTERVALL_US);
    double startupd2 = sensor2.getDistanceCm();
   
    if(validDviation(startupd1,startupd2))
    {
        //cout << "Abweichung OK\n";
    } else
    {
       setCurrentDistanceReading(INVALID_READING);
    }
    if(validRange(startupd1) == 1 && validRange(startupd2) == 1)
    {
        cout << "Werte Gültig\n";
    } else
    {
        cout << "Werte Ungültig\n";
    }
    while (!terminateApp())
    {
        // Read sensor 1
        // Read sensor 2
        // Do plausibility checks, compare values, determine output

        double d1 = sensor1.getDistanceCm(); // we just assume everything is OK here...
         cout << d1 <<"cm\n";
        uint16_t nextReadingVal = static_cast<uint16_t>(floor(d1));
       
        if(validRange(d1))
        {
        // Critical section start, take care that no exception can be thrown in it!
        setCurrentDistanceReading(nextReadingVal);
        }
        else
        {
            //error übergeben
        }
        
        
        // Sleep 50ms
        usleep(SAMPLE_INTERVALL_US);

        double d2 = sensor2.getDistanceCm(); // we just assume everything is OK here...
        nextReadingVal = static_cast<uint16_t>(floor(d2));
        cout << d2 <<"cm\n";
        if(validRange(d2))
        {
        // Critical section start, take care that no exception can be thrown in it!
        setCurrentDistanceReading(nextReadingVal);
        }
        else 
        {
            setCurrentDistanceReading(INVALID_READING);
            //error übergeben
        }
        usleep(SAMPLE_INTERVALL_US);
        if(validDviation(d1,d2))
        {
            cout << "valid\n";
   
        } else
        {
            cout << "invalid\n";
        }

    }

    
}