#include <iostream>
#include <stdexcept>
#include <cmath>
#include <optional>

#include "Node1Types.h"
#include "CommThread.h"
#include "SensorThread.h"

using namespace std;
using namespace acc;

typedef struct
{
    uint16_t distance;
    pthread_mutex_t lock;
} DistanceReadingType;

static constexpr uint16_t DISTANCE_READING_ERROR_2 = 65535U;

// global var, written by the sensor thread, read by the communication thread
static DistanceReadingType gCurrentDistanceReading = { DISTANCE_READING_ERROR_2, PTHREAD_MUTEX_INITIALIZER };

// global app termination flag; no critical sections required
static bool gTerminateApplication = false;

namespace acc
{

uint16_t getCurrentDistanceReading()
{
    pthread_mutex_lock(&gCurrentDistanceReading.lock);
    uint16_t currentReading = gCurrentDistanceReading.distance;
    pthread_mutex_unlock(&gCurrentDistanceReading.lock);
    return currentReading;
}

void setCurrentDistanceReading(uint16_t value)
{
    pthread_mutex_lock(&gCurrentDistanceReading.lock);
    gCurrentDistanceReading.distance = value;
    pthread_mutex_unlock(&gCurrentDistanceReading.lock);
}

static void *sensorThreadFunc(void *)
{
    try
    {
        SensorThread t(gTerminateApplication);
        t.run();
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

} // namespace acc

int main()
{
    int ret = 0;
    optional<pthread_t> optSensorThreadHandle;

    cout << "Node 1 started.\n";

    try
    {
        // Start Sensor thread
        pthread_t sensorThreadHandle;
        if (pthread_create(&sensorThreadHandle, nullptr, sensorThreadFunc, nullptr)) 
        {
            cerr << "Error creating sensor thread\n";
            ret = 1;
        }
        else
        {
            optSensorThreadHandle = sensorThreadHandle;
            // ...this leaves the Communication Thread :)
            acc::CommThread commThread(gTerminateApplication);
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
    cout << "Shutting down node1...\n";

    // collect sensor thread
    if (optSensorThreadHandle.has_value())
    {
        cout << "Joining sensor thread...\n";
        pthread_join(*optSensorThreadHandle, nullptr);
    }

    return ret;
}