#include <iostream>
#include <array>
#include <bluetooth.h>
#include <span>
#include <stdexcept>
#include <time.h>

using namespace std;

//static char const * MAC_RASPI_5 = "D8:3A:DD:F5:BA:9F";
//static char const * MAC_RASPI_4 = "DC:A6:32:BB:79:EF";

array<uint8_t, 4096> rxBuf;

int main()
{
    try
    {
        acc::BTListenSocket listenSocket = acc::BTListenSocket();

        while (1)
        {
            rxBuf[0] = 0x00;
            cout << "Hello, I am node1.\n";
            acc::BTConnection conn(&listenSocket); 

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

    return 0;
}