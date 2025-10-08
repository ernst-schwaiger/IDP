#include <iostream>
#include <stdexcept>
#include <cmath>

#include "Node1Types.h"
#include "CommThread.h"
#include "SensorThread.h"

using namespace std;
using namespace acc;

// global var, written by the sensor thread, read by the communication thread
volatile uint16_t gCurrentDistanceReading = 0xffff;

uint16_t getCurrentDistanceReading(pthread_mutex_t *pLock)
{
    pthread_mutex_lock(pLock);
    uint16_t currentReading = gCurrentDistanceReading;
    pthread_mutex_unlock(pLock);
    return currentReading;
}

void setCurrentDistanceReading(pthread_mutex_t *pLock, uint16_t value)
{
    pthread_mutex_lock(pLock);
    gCurrentDistanceReading = value;
    pthread_mutex_unlock(pLock);
}

static void *sensorThreadFunc(void *arg)
{
    SensorThread *pSensorThread = reinterpret_cast<SensorThread *>(arg);
    pSensorThread->run();
    return nullptr; // Won't ever be reached
}

int main()
{
    cout << "Node 1 started.\n";

    try
    {
        // mutex for setting up the critical section
        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

        // Start Sensor thread
        SensorThread sensorThread(&lock);
        pthread_t thread;
        if (pthread_create(&thread, NULL, sensorThreadFunc, &sensorThread)) 
        {
            cerr << "Error creating thread\n";
            return 1;
        }

        // ...this leaves the Communication Thread :)
        acc::CommThread commThread(&lock);
        commThread.run();
    }
    catch(runtime_error const &e)
    {
        cerr << "Runtime Exception: " << e.what() << '\n';
        perror("Error message: ");
        return -1;
    }
    catch(...)
    {
        cerr << "Unknown exception occurred.\n";
        return -1;
    }    

    // won't ever be reached
    return 0;
}