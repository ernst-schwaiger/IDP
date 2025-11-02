#include <stdexcept>
#include <iostream>
#include <cmath>
#include <unistd.h>

#include <Helper.h>
#include "SensorThread.h"
#include "Node1Types.h"

using namespace std;
using namespace acc;

void acc::SensorThread::run(void)
{
    while (!terminateApp())
    {
        // Read sensor 1
        // Read sensor 2
        // Do plausibility checks, compare values, determine output
        cout << "test" << std::endl;

        Sensor sensor1(18, 24);
        Sensor sensor2(17, 23);


        double d1 = sensor1.getDistanceCm(); // we just assume everything is OK here...
        uint16_t nextReadingVal = static_cast<uint16_t>(floor(d1));
        cout << "Messung1 " << ": " << d1 << " cm\n";
        // Critical section start, take care that no exception can be thrown in it!
        setCurrentDistanceReading(nextReadingVal);
        
        // Sleep 50ms
        usleep(50'000U);

        double d2 = sensor2.getDistanceCm(); // we just assume everything is OK here...
        nextReadingVal = static_cast<uint16_t>(floor(d2));
        cout << "Messung2 " << ": " << d2 << " cm\n";
        // Critical section start, take care that no exception can be thrown in it!
        setCurrentDistanceReading(nextReadingVal);

    }
}