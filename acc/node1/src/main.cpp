#include <iostream>
#include <stdexcept>
#include <cmath>
#include <optional>

#include "Node1Types.h"
#include "CommThread.h"
#include "SensorThread.h"

using namespace std;
using namespace acc;

// global var, written by the sensor thread, read by the communication thread
volatile uint16_t gCurrentDistanceReading = 0xffff;

// global app termination flag; no critical sections required
bool gTerminateApplication = false;


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
    try
    {
        pSensorThread->run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        perror("Error message: ");
    }
    catch(...)
    {
        cerr << "Unknown exception occurred.\n";
    }

    // signal other threads to stop
    gTerminateApplication = true;    

    return nullptr;
}

int main()
{
    int ret = 0;
    optional<pthread_t> optSensorThreadHandle;

    cout << "Node 1 started.\n";

    try
    {
        // mutex for setting up the critical section
        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

        // Start Sensor thread
        SensorThread sensorThread(gTerminateApplication, &lock);
        pthread_t sensorThreadHandle;
        if (pthread_create(&sensorThreadHandle, NULL, sensorThreadFunc, &sensorThread)) 
        {
            cerr << "Error creating sensor thread\n";
            ret = 1;
        }
        else
        {
            optSensorThreadHandle = sensorThreadHandle;
            // ...this leaves the Communication Thread :)
            acc::CommThread commThread(gTerminateApplication, &lock);
            commThread.run();
        }
    }
    catch(runtime_error const &e)
    {
        cerr << "Runtime Exception: " << e.what() << '\n';
        perror("Error message: ");
        ret = -1;
    }
    catch(...)
    {
        cerr << "Unknown exception occurred.\n";
        ret = -1;
    }

    // signal all threads to stop
    gTerminateApplication = true;

    // collect sensor thread
    if (optSensorThreadHandle.has_value())
    {
        pthread_join(*optSensorThreadHandle, NULL);
    }

    return ret;
}