
#include <stdexcept>
#include <iostream>

#include <BTConnection.h>
#include <BTRuntimeError.h>
#include <Helper.h>
#include "CommThread.h"

using namespace std;
using namespace acc;

void acc::CommThread::run(void)
{
    while (!terminateApp())
    {
        try
        {
            commLoop(m_listenSocket);
        }
        catch(BTRuntimeError const &e)
        {
            // Connection reset is handled gracefully
            if (ECONNRESET == e.errNumber())
            {
                cout << "Connection to client lost. Setting up new connection.\n";
            }
            else
            {
                cerr << "BTRuntimeError (" << e.errNumber() << ") " << e.what() << ": " << strerror(e.errNumber()) << "\n";
            }
        }
    }
}

void acc::CommThread::commLoop(acc::BTListenSocket &listenSocket)
{
    uint32_t numTxFails = 0U;
    uint32_t numTxSuccess = 0U;
    acc::BTConnection conn(&listenSocket);
    
    // Set up session key
    bool keyExChangeFinished = false;
    while (!terminateApp() && !keyExChangeFinished)
    {
        keyExChangeFinished = conn.keyExchangeServer();
    }

    if (!terminateApp())
    {
        cout << "Connection to client established.\n";
    }

    uint64_t baselineMs = getTimestampMs();

    while (!terminateApp()) // we leave this one only via failed send/receive operations
    {
        uint16_t currentDistanceReading = getCurrentDistanceReading();
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
        usleep(25'000U);
    }

    cout << "Tx Fails: " << numTxFails << ", Tx Success: " << numTxSuccess << "\n";    
}