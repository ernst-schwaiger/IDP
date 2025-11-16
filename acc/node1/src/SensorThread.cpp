#include <stdexcept>
#include <iostream>
#include <cmath>
#include <unistd.h>

#include <Helper.h>
#include "SensorThread.h"
#include "Node1Types.h"

using namespace std;
using namespace acc;

bool acc::validRange(double distance)
{
    if (distance < 10.0 || distance > 400.0)
    {
    return false;
    }
    else return true;
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
    usleep(50'000U);
    double startupd1 = sensor1.getDistanceCm();

    usleep(50'000U);
    double startupd2 = sensor2.getDistanceCm();
   
    if(validDviation(startupd1,startupd2))
    {
        cout << "Abweichung OK\n";
    } else
    {
        cout << "Abweichung zu groß\n";
    }
    if(validRange(startupd1) && validRange(startupd2))
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
        usleep(50'000U);

        double d2 = sensor2.getDistanceCm(); // we just assume everything is OK here...
        nextReadingVal = static_cast<uint16_t>(floor(d2));
        if(validRange(d2))
        {
        // Critical section start, take care that no exception can be thrown in it!
        setCurrentDistanceReading(nextReadingVal);
        }
        else 
        {
            //error übergeben
        }
        usleep(50'000U);
        if(validDviation(d1,d2))
        {
        //setCurrentDistanceReading(0xffff);
        }

    }

    
}