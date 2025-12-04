
#include <stdexcept>
#include <iostream>

#include <BTConnection.h>
#include <BTRuntimeError.h>
#include <Helper.h>
#include "CommThread.h"

using namespace std;
using namespace acc;

// Execute communication thread function
void acc::CommThread::run(void)
{
    while (!terminateApp()) // while the app is not terminating...
    {
        try
        {
            commLoop(m_listenSocket); //...execute the comm op loop
        }
        catch(BTRuntimeError const &e)
        {
            //Saf-REQ-5: The ACC shall automatically detect a failure of the communication subsystem 
            // (Bluetooth) by recognizing that a packet could not be sent, or by recognizing that no 
            // response is received within a defined period (500 ms) after a successful transmission.             
            // Connection reset happened in the loop, is handled gracefully here
            //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, 
            // the ACC shall initiate a connection.
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

// communication loop (tx side)
void acc::CommThread::commLoop(acc::BTListenSocket const &listenSocket) const
{
    uint32_t numTxFails = 0U; // keep track of the number of failed tx operations
    uint32_t numTxSuccess = 0U; // keep track of the number of successful tx operations

    // Set up bluetooth connection
    acc::BTConnection conn(&listenSocket);
    
    // Set up session key/key exchange phase
    bool keyExChangeFinished = false;
    while (!terminateApp() && !keyExChangeFinished)
    {
        keyExChangeFinished = conn.keyExchangeServer();
    }

    if (!terminateApp())
    {
        cout << "Connection to client established.\n";
    }

    // timestamp value [ms] starts after a successfuly connection was established
    uint64_t baselineMs = getTimestampMs();

    while (!terminateApp()) // we leave this one only via failed send/receive operations
    {
        // get distance reading
        uint16_t currentDistanceReading = getCurrentDistanceReading();
        uint8_t readingToTransmit[sizeof(currentDistanceReading)];
        readingToTransmit[0] = currentDistanceReading >> 8;
        readingToTransmit[1] = currentDistanceReading & 0xff;
        // send distance reading with counter and HMAC
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