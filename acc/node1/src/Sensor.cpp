#include "Sensor.h"

#include <unistd.h>
#include <iostream>
#include <stdexcept>

// comment this out if no sensort are mounted to node 1
#define PROXIMITY_SENSORS_MOUNTED

#ifdef PROXIMITY_SENSORS_MOUNTED
#include <pigpio.h>
#else
#include <cmath>
#endif

using namespace std;

namespace acc
{

Sensor::Sensor(int trigPin, int echoPin) : trigPin(trigPin), echoPin(echoPin)
{
#ifdef PROXIMITY_SENSORS_MOUNTED
    int rc = gpioInitialise();
    if (rc < 0)
    {
        char msgBuf[80];
        sprintf(msgBuf, "pigpio init failed: %d", rc);

        throw runtime_error(msgBuf);
    }

    gpioSetMode(trigPin, PI_OUTPUT);
    gpioSetMode(echoPin, PI_INPUT);
    gpioWrite(trigPin, 0);
    usleep(200000);
#endif
}

Sensor::~Sensor()
{
#ifdef PROXIMITY_SENSORS_MOUNTED    
    gpioTerminate();
#endif
}

double Sensor::getDistanceCm() 
{
#ifdef PROXIMITY_SENSORS_MOUNTED
    gpioWrite(trigPin, 1);
    gpioDelay(10);
    gpioWrite(trigPin, 0);

    while (gpioRead(echoPin) == 0) {}
    uint32_t start = gpioTick();

    while (gpioRead(echoPin) == 1) {}
    uint32_t end = gpioTick();

    uint32_t diff = end - start; 
    return (diff * 0.0343) / 2.0; 
#else
    // No sensor available, send test values
    static double i = 0.0;
    double val = fabs(314 * sin(i));
    i += 0.1;
    return val;
#endif
}

} // namespace acc
