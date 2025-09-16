#include <iostream>
#include <bluetooth.h>
#include <span>

#include <time.h>

using namespace std;

//static char const * MAC_RASPI_5 = "D8:3A:DD:F5:BA:9F";
static char const * MAC_RASPI_4 = "DC:A6:32:BB:79:EF";
static char const *BYE = "bye";

int main()
{
    try
    {
        char buf[40];
        cout << "Hello, I am node2.\n";
        acc::BTConnection conn(MAC_RASPI_4); 

        for(uint32_t i = 0; i < 100; i++)
        {
            sprintf(buf, "Msg: %d", i);
            std::span<const uint8_t> mySpan(reinterpret_cast<uint8_t const *>(buf), strlen(buf) + 1);

            if (conn.send(mySpan) < 0)
            {
                cerr << "Failed to send data from buffer.";
            }

            usleep(5000);
        }

        std::span<const uint8_t> byeSpan(reinterpret_cast<uint8_t const *>(BYE
        ), strlen(BYE) + 1);
        conn.send(byeSpan);
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