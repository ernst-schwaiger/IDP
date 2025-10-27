#include "Sensor.h"

#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <format>

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

Sensor::Sensor(uint8_t trigPin, uint8_t echoPin) : trigPin(trigPin), echoPin(echoPin)
{
#ifdef PROXIMITY_SENSORS_MOUNTED
    int rc = gpioInitialise(); // Deviation Dir 4.6: type used in external gpio API
    if (rc < 0)
    {
        throw runtime_error(std::format("pigpio init failed: {}", rc));
    }

    gpioSetMode(trigPin, PI_OUTPUT);
    gpioSetMode(echoPin, PI_INPUT);
    gpioWrite(trigPin, 0U);
    usleep(200'000U);
#endif
}

Sensor::~Sensor(void)
{
#ifdef PROXIMITY_SENSORS_MOUNTED    
    gpioTerminate();
#endif
}

double Sensor::getDistanceCm(void) 
{
#ifdef PROXIMITY_SENSORS_MOUNTED
    gpioWrite(trigPin, 1U);
    gpioDelay(10U);
    gpioWrite(trigPin, 0U);

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
