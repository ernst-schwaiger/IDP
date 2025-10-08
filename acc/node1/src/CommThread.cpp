
#include <stdexcept>
#include <iostream>

#include <BTConnection.h>
#include <Helper.h>
#include "CommThread.h"

using namespace std;
using namespace acc;

void acc::CommThread::threadLoop()
{
    try
    {
        // Run the loop (currently only in one thread)
        commLoop(m_listenSocket, m_pthreadArg);
    }
    catch(runtime_error const &e)
    {
        cerr << "Runtime Exception: " << e.what() << '\n';
        perror("Error message: ");
    }
}

void acc::CommThread::commLoop(acc::BTListenSocket &listenSocket, pthread_mutex_t *pLock)
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