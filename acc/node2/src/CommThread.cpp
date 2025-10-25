#include <stdexcept>
#include <iostream>

#include <BTConnection.h>
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
            commLoop(m_pthreadArg);
        }
        catch(const runtime_error &e)
        {
            cerr << "Runtime Exception: " << e.what() << '\n';
            perror("Error message: ");
        }
        catch(...)
        {
            cerr << "Unknown exception occurred.\n";
        }
    } 
}

void acc::CommThread::commLoop(char const *remoteMAC)
{
    // Set up a BT connection to node1
    BTConnection conn(remoteMAC);

    // At this point, we have a connection set up
    cout << "Connection to " << remoteMAC << " established.\n";
    
    // Create session key
    conn.keyExchangeClient();

    // counter;  a timestamp indicating the ms since the Bluetooth connection
    // was established between Node 1, Node 2
    uint32_t counter = 0;

    while (!terminateApp())
    {
        uint8_t msgType;
        // buffer for sending and receiving messages
        array<uint8_t, MAX_MSG_LEN> msgBuf = { 0 };
        // message reception (blocking)
        if (conn.receiveWithCounterAndMAC(msgType, {&msgBuf[0], msgBuf.size()}, counter, true) >= 0)
        {
            if (msgType == 0x02)
            {
                DistanceReadingInfoType reading = { getTimestampMs(), static_cast<uint16_t>((msgBuf[0] << 8) + msgBuf[1]) };
                setCurrentDistanceReading(&reading);     
            }
        }

        // no sleep required here; we block in the receive function
    }
}
