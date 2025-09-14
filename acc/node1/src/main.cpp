#include <iostream>
#include <array>
#include <bluetooth.h>
#include <span>

using namespace std;

static char const * MAC_RASPI_5 = "D8:3A:DD:F5:BA:9F";
//static char const * MAC_RASPI_4 = "DC:A6:32:BB:79:EF";

array<uint8_t, 4096> rxBuf;

int main()
{
    cout << "Hello, I am node1.\n";

    acc::BTConnection conn(MAC_RASPI_5, 4242, true); 

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

    return 0;
}