#include <iostream>
#include <stdexcept>
#include <time.h>
#include <unistd.h>
#include "Sensor.h"

#include <CryptoComm.h>

using namespace std;
using namespace acc;

// global var, written by the sensor thread, read by the communication thread
volatile uint16_t gCurrentDistanceReading;

void *sensorThread(void *arg)
{
    pthread_mutex_t *pLock = reinterpret_cast<pthread_mutex_t *>(arg);

    // FIXME: Does this thread throw any exceptions?
    // If so, they must be handled here

    // Initialize sensor
    Sensor sensor(18, 24);
    uint8_t readingCounter = 0;
    
    // Sensor loop
    while (1)
    {
        // Read sensor 1
        // Read sensor 2
        // Do plausibility checks, compare values, determine output

        double d = sensor.getDistanceCm(); // we just assume everything is OK here...
        uint16_t nextReadingVal = static_cast<uint16_t>(d);
        // most significant 12 bit: Reading value, least significant 4 bit: readingCounter;
        nextReadingVal <<= 4;
        nextReadingVal |= (readingCounter & 0x0f);
        
        // Critical section start, take care that no exception can be thrown in it!
        pthread_mutex_lock(pLock);
        gCurrentDistanceReading = nextReadingVal;
        pthread_mutex_unlock(pLock);
        // Critical section end
        
        readingCounter++;
    }
}


static void commLoop(acc::BTListenSocket &listenSocket, pthread_mutex_t *pLock)
{
    uint32_t numTxFails = 0;
    uint32_t numTxSuccess = 0;
    uint32_t numRxFails = 0;
    uint32_t numRxSuccess = 0;

    acc::BTConnection conn(&listenSocket);
    array<uint8_t, MAX_MSG_LEN> rxBuf = { 0x00 };
    uint8_t msgType = 0x00;
    
    // Set up session key
    conn.keyExchangeServer();

    while (1) // we leave this one only via failed send/receive operations
    {
        ssize_t bytes_received = conn.receiveWithCounterAndMAC(msgType, rxBuf);
        if (bytes_received < 0)
        {
            numRxFails++;
        }
        else
        {
            numRxSuccess++;

            pthread_mutex_lock(pLock);
            uint16_t readingToTransmit = gCurrentDistanceReading;
            pthread_mutex_unlock(pLock);

            // FIXME: Get rid of the reinterpret_cast
            if (conn.sendWithCounterAndMAC(0x02, {reinterpret_cast<uint8_t const *>(&readingToTransmit), sizeof(readingToTransmit)}) < 0)
            {
                numTxFails++;
            }
            else
            {
                numTxSuccess++;
            }
        }
    }

    cout << "Tx Fails: " << numTxFails << ", Rx Fails: " << numRxFails << "\n";
    cout << "Tx Success: " << numTxSuccess << ", Rx Success: " << numRxSuccess << "\n";    
}

int main()
{
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
    }
    catch(...)
    {
        cerr << "Unknown exception occurred.\n";
        return -1;
    }    

    // won't ever be reached
    return 0;
}