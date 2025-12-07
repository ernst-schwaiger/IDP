#include <stdexcept>
#include <iostream>
#include <unistd.h>

#include <BTConnection.h>
#include <BTRuntimeError.h>
#include <Helper.h>

#include "CommThread.h"

using namespace std;
using namespace acc;

void acc::CommThread::run(void)
{
    uint32_t connectErrorCounter = 0U;
    while (!terminateApp()) // while the app is not terminating...
    {
        try
        {
            commLoop(m_pthreadArg); //...execute the comm op loop
        }
        catch(BTRuntimeError const &e)
        {
            //Saf-REQ-5: The ACC shall automatically detect a failure of the communication subsystem 
            // (Bluetooth) by recognizing that a packet could not be sent, or by recognizing that no 
            // response is received within a defined period (500 ms) after a successful transmission.             
            // Connection reset happened in the loop, is handled gracefully here
            //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, 
            // the ACC shall initiate a connection.

            // Handle connection refused, reset, unavailable gracefully.
            // We try to connect from start in that case
            if ((ECONNREFUSED == e.errNumber()) || (ECONNRESET == e.errNumber()) || (EAGAIN == e.errNumber()))
            {
                if ((++connectErrorCounter % 500U) == 0U) // do not clutter stdout
                {
                    cerr << "Failed to connect to server. Try to reconnect...\n";
                }
            }
            else
            {
                cerr << "BTRuntimeError (" << e.errNumber() << ") " << e.what() << ": " << strerror(e.errNumber()) << "\n";
            }
        }
    } 
}

// communication loop (rx side)
void acc::CommThread::commLoop(char const *remoteMAC) const // Deviation Dir 4.6: type passed via main() function
{
    // Set up a BT connection to node1
    BTConnection conn(remoteMAC);

    // At this point, we have a connection set up
    cout << "Connection to " << remoteMAC << " established.\n";
    
    // Set up session key
    bool keyExChangeFinished = false;
    while (!terminateApp() && !keyExChangeFinished)
    {
        keyExChangeFinished = conn.keyExchangeClient();
    }

    // counter;  a timestamp indicating the ms since the Bluetooth connection
    // was established between Node 1, Node 2
    uint32_t counter = 0U;
    uint32_t noValidMsgRxCounter = 0U;

    while (!terminateApp())
    {
        uint8_t msgType;
        // buffer for sending and receiving messages
        array<uint8_t, MAX_MSG_LEN> msgBuf = { 0U };
        // message reception (blocking)
        if (conn.receiveWithCounterAndMAC(msgType, {&msgBuf[0], msgBuf.size()}, counter, true) >= 0)
        {
            if (msgType == 0x02)
            {
                DistanceReadingInfoType reading = { getTimestampMs(), static_cast<uint16_t>((msgBuf[0] << 8) + msgBuf[1]) };
                setCurrentDistanceReading(&reading);     
            }
            noValidMsgRxCounter = 0U;
        }
        else
        {
            noValidMsgRxCounter++;
        }

        if (noValidMsgRxCounter > 75U)
        {
            //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, 
            // the ACC shall initiate a connection.            
            throw BTRuntimeError("Connection closed by server");
        }

        // Sleep 1ms, minimize latency of receiving reading message
        usleep(1'000U);        
    }
}
