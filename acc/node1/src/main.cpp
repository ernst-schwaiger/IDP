#include <iostream>
#include <array>
#include <bluetooth.h>
#include <span>
#include <stdexcept>

using namespace std;

static char const * MAC_RASPI_5 = "D8:3A:DD:F5:BA:9F";
//static char const * MAC_RASPI_4 = "DC:A6:32:BB:79:EF";

array<uint8_t, 4096> rxBuf;

int main()
{
    try
    {
        cout << "Hello, I am node1.\n";

        acc::BTConnection conn(MAC_RASPI_5, true); 

        while(1)
        {
            if (conn.receive(rxBuf) < 0)
            {
                cerr << "Failed to read data from buffer.";
            }
            else
            {
                cout << &rxBuf[0] << "\n";
            }
        }
    }
    catch(const runtime_error &e)
    {
        cerr << e.what() << '\n';
    }
    catch(...)
    {
        cerr << "Unknown exception occurred.\n";
    }

    return 0;
}