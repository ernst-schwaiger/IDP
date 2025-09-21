#include <iostream>
#include <stdexcept>
#include <time.h>
#include <unistd.h>
#include "Sensor.h"

#include <CryptoComm.h>

using namespace std;
using namespace acc;

int main()
{
    try
    {
        Sensor sensor(18, 24);
        acc::BTListenSocket listenSocket = acc::BTListenSocket();

        while (1)
        {
            array<uint8_t, MAX_MSG_LEN> rxBuf;
            rxBuf[0] = 0x00;
            uint8_t msgType = 0x00;

            cout << "Hello, I am node1.\n";
            acc::BTConnection conn(&listenSocket);
            conn.keyExchangeServer(); 

            while(msgType != 0x03)
            {

                ssize_t bytes_received = conn.receiveWithCounterAndMAC(msgType, rxBuf);
                if (bytes_received < 0)
                {
                    cerr << "Failed to read data from buffer.";
                }
                else
                {
                    double d = sensor.getDistanceCm(); // FIXME, put in a thread, add timeout
                    // FIXME: Get rid of the reinterpret_cast
                    if (conn.sendWithCounterAndMAC(0x02, {reinterpret_cast<uint8_t const *>(&d), sizeof(d)}) < 0)
                    {
                        cerr << "Failed to send distance reading.";
                    }
                }
            }
        }
    }
    catch(const runtime_error &e)
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

    return 0;
}