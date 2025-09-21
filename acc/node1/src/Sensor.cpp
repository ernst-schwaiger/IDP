#include "Sensor.h"

#include <unistd.h>
#include <iostream>
#include <stdexcept>

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
    gpioTerminate();
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
    // No sensor available, send pi * 10
    return 31.415926;
#endif
}

} // namespace acc
