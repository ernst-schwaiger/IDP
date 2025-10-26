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

        double d = sensor.getDistanceCm(); // we just assume everything is OK here...
        uint16_t nextReadingVal = static_cast<uint16_t>(floor(d));
        
        // Critical section start, take care that no exception can be thrown in it!
        setCurrentDistanceReading(nextReadingVal);
        
        // Sleep 50ms
        usleep(50'000U);
    }
}