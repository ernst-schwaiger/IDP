#include <iostream>
#include <stdexcept>
#include <cmath>
#include <time.h>
#include <unistd.h>

#include "Sensor.h"

#include <CryptoComm.h>

using namespace std;
using namespace acc;

// global var, written by the sensor thread, read by the communication thread
volatile uint16_t gCurrentDistanceReading = 0xffff;

static unsigned long getTimestampMs()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    long ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    printf("Current time in ms: %ld\n", ms);
    return 0;
}

static uint32_t getTimestampMsSinceBaseline(long baselineMs)
{
    long currentTimeMs = getTimestampMs();
    return static_cast<uint32_t>(currentTimeMs - baselineMs);
}

static uint16_t getCurrentDistanceReading(pthread_mutex_t *pLock)
{
    pthread_mutex_lock(pLock);
    uint16_t currentReading = gCurrentDistanceReading;
    pthread_mutex_unlock(pLock);
    return currentReading;
}

static void setCurrentDistanceReading(pthread_mutex_t *pLock, uint16_t value)
{
    pthread_mutex_lock(pLock);
    gCurrentDistanceReading = value;
    pthread_mutex_unlock(pLock);
}

static void *sensorThread(void *arg)
{
    pthread_mutex_t *pLock = reinterpret_cast<pthread_mutex_t *>(arg);

    // FIXME: Does this thread throw any exceptions?
    // If so, they must be handled here

    // Initialize sensor
    Sensor sensor(18, 24);
    
    // Sensor loop
    while (1)
    {
        // Read sensor 1
        // Read sensor 2
        // Do plausibility checks, compare values, determine output

        double d = sensor.getDistanceCm(); // we just assume everything is OK here...
        uint16_t nextReadingVal = static_cast<uint16_t>(floor(d));
        
        // Critical section start, take care that no exception can be thrown in it!
        setCurrentDistanceReading(pLock, nextReadingVal);
        
        // Sleep 2.5ms
        usleep(2'500);
    }

    return nullptr; // Won't ever be reached
}


static void commLoop(acc::BTListenSocket &listenSocket, pthread_mutex_t *pLock)
{
    uint32_t numTxFails = 0;
    uint32_t numTxSuccess = 0;
    acc::BTConnection conn(&listenSocket);
    
    // Set up session key
    conn.keyExchangeServer();
    cout << "Connection to client established.\n";

    long baselineMs = getTimestampMs();

    while (1) // we leave this one only via failed send/receive operations
    {
        uint16_t currentDistanceReading = getCurrentDistanceReading(pLock);
        uint8_t readingToTransmit[sizeof(currentDistanceReading)];
        readingToTransmit[0] = currentDistanceReading >> 8;
        readingToTransmit[1] = currentDistanceReading & 0xff;

        if (conn.sendWithCounterAndMAC(0x02, {readingToTransmit, sizeof(readingToTransmit)}, getTimestampMsSinceBaseline(baselineMs)) < 0)
        {
            numTxFails++;
        }
        else
        {
            numTxSuccess++;
        }        

        // Sleep 25ms, oversampling sensor readings by a factor of two
        usleep(25'000);
    }

    cout << "Tx Fails: " << numTxFails << ", Tx Success: " << numTxSuccess << "\n";
}

int main()
{
    cout << "Node 1 started.\n";

    // mutex for setting up the critical section
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    // Start Sensor thread
    pthread_t thread;
    if (pthread_create(&thread, NULL, sensorThread, &lock)) 
    {
        cerr << "Error creating thread\n";
        return 1;
    }

    // ...this leaves the Communication Thread :)
    try
    {
        // Listen socket must be only created once, out of the loop
        acc::BTListenSocket listenSocket = acc::BTListenSocket();

        while (1)
        {
            try
            {
                // Run the loop (currently only in one thread)
                commLoop(listenSocket, &lock);
            }
            catch(runtime_error const &e)
            {
                cerr << "Runtime Exception: " << e.what() << '\n';
                perror("Error message: ");
            }

            // send or receive error happened, we set up a new connection here
        }
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