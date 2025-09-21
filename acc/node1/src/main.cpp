#include <iostream>
#include <stdexcept>
#include <time.h>
#include <unistd.h>
#include "sensor.h"

#include <CryptoComm.h>

using namespace std;

array<uint8_t, 4096> rxBuf;

int main()
{
    int rc = gpioInitialise();
    if (rc < 0) {
        std::cerr << "pigpio init failed: " << rc << "\n";
        return 1;
    }
    while (1)
    {
        Sensor sensor(18, 24);

        double d = sensor.getDistanceCm();
        cout << "Messung " << ": " << d << " cm\n";
        usleep(500000);
    }
    try
    {
        acc::BTListenSocket listenSocket = acc::BTListenSocket();

        while (1)
        {
            rxBuf[0] = 0x00;
            cout << "Hello, I am node1.\n";
            acc::BTConnection conn(&listenSocket);
            conn.keyExchangeServer(); 

            

            while(strcmp("bye", reinterpret_cast<char *>(&rxBuf[0])))
            {
                ssize_t bytes_received = conn.receive(rxBuf);
                if (bytes_received < 0)
                {
                    cerr << "Failed to read data from buffer.";
                }
                else
                {
                    if (bytes_received > 0)
                    {
                        cout << &rxBuf[0] << "\n";
                    }
                }
                usleep(5000);
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

    gpioTerminate();
    return 0;
}