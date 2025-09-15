#include <iostream>
#include <bluetooth.h>
#include <span>
using namespace std;

//static char const * MAC_RASPI_5 = "D8:3A:DD:F5:BA:9F";
static char const * MAC_RASPI_4 = "DC:A6:32:BB:79:EF";

static char const * MSG = "Hello, I am node2.";

int main()
{
    cout << "Hello, I am node2.\n";
    acc::BTConnection conn(MAC_RASPI_4, false); 

    while(1)
    {
        std::span<const uint8_t> mySpan(reinterpret_cast<uint8_t const *>(MSG), 128);

        if (conn.send(mySpan) < 0)
        {
            cerr << "Failed to send data from buffer.";
        }

        sleep(1);
    }

    return 0;
}