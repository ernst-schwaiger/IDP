#include "Sensor.h"

#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <format>

constexpr uint32_t TIMEOUT_US = 30000; // 30 ms Timeout 

#include <pigpio.h>

using namespace std;

namespace acc
{

Sensor::Sensor(uint8_t trigPin, uint8_t echoPin) : trigPin(trigPin), echoPin(echoPin)
{
    int rc = gpioInitialise(); // Deviation Dir 4.6: type used in external gpio API
    if (rc < 0)
    {
        throw runtime_error(std::format("pigpio init failed: {}", rc));
    }

    gpioSetMode(trigPin, PI_OUTPUT);
    gpioSetMode(echoPin, PI_INPUT);
    gpioWrite(trigPin, 0U);
    usleep(200'000U);
}

Sensor::~Sensor(void)
{
    gpioTerminate();
}

double Sensor::getDistanceCm(void) 
{
    gpioWrite(trigPin, 1U);
    gpioDelay(10U);
    gpioWrite(trigPin, 0U);

    // wait for scho high
    uint32_t t0 = gpioTick();
    while (gpioRead(echoPin) == 0) {
        if (gpioTick() - t0 > TIMEOUT_US) {
            return -1.0;  // Timeout no Echo start
        }
    }
    uint32_t start = gpioTick();

    // wait for echo low
    t0 = gpioTick();
    while (gpioRead(echoPin) == 1) {
        if (gpioTick() - t0 > TIMEOUT_US) {
            return -2.0;  // Timeout no Echo end
        }
    }
    uint32_t end = gpioTick();

    uint32_t diff = end - start;
    return (diff * 0.0343) / 2.0; 
}

} // namespace acc
