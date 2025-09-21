#include "sensor.h"
#include <unistd.h>
#include <iostream>

Sensor::Sensor(int trigPin, int echoPin) : trigPin(trigPin), echoPin(echoPin)
{
    gpioSetMode(trigPin, PI_OUTPUT);
    gpioSetMode(echoPin, PI_INPUT);
    gpioWrite(trigPin, 0);
    usleep(200000);
}

double Sensor::getDistanceCm() {
    gpioWrite(trigPin, 1);
    gpioDelay(10);
    gpioWrite(trigPin, 0);

    while (gpioRead(echoPin) == 0) {}
    uint32_t start = gpioTick();

    while (gpioRead(echoPin) == 1) {}
    uint32_t end = gpioTick();

    uint32_t diff = end - start; 
    return (diff * 0.0343) / 2.0; 
}