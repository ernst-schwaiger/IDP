#include <iostream>
#include <span>

#include <signal.h>
#include <time.h>

#include <CryptoComm.h>

using namespace std;
using namespace acc;

// Type definitions
enum class ACC_Status
{
    OFF,
    ON,
    ERROR
};

// constants
constexpr uint8_t ERROR_COUNT_MAX = 4;

// global var, written by the sensor thread, read by the communication thread
volatile uint16_t gCurrentDistanceReading = 0xffff;

static bool isValidDistance(uint16_t currentReading)
{
    // FIXME: Implement comparison of aging counters
    return true;
}

static ACC_Status getACCStatusFromGUI()
{
    // FIXME: Implement this
    return ACC_Status::ON;
}

static uint8_t getCurrentSpeedFromGUI()
{
    // FIXME: Implement this
    return 42U;
}

static uint8_t accFunc(uint16_t currentDistance, uint8_t currentSpeed)
{
    // FIXME: Implement
    return currentSpeed;
}

void *accThread(void *arg)
{
    pthread_mutex_t *pLock = reinterpret_cast<pthread_mutex_t *>(arg);
    uint16_t lastSuccessfulReading = 0xffff;
    uint8_t errorCount = 0;

    while (1)
    {
        // get the current speed from the GUI
        uint8_t currentSpeed = getCurrentSpeedFromGUI();
        // get the current status of the ACC from the GUI
        ACC_Status acc_status = getACCStatusFromGUI();

        uint16_t lastSuccessfulReadDistance = lastSuccessfulReading;

        // Critical section start, take care that no exception can be thrown in it!
        pthread_mutex_lock(pLock);
        uint16_t currentReading = gCurrentDistanceReading;
        pthread_mutex_unlock(pLock);
        // Critical section end

        if (isValidDistance(currentReading))
        {
            lastSuccessfulReading = currentReading;
            lastSuccessfulReadDistance = lastSuccessfulReading;
            if (acc_status == ACC_Status::ERROR)
            {
                acc_status = ACC_Status::OFF;
            }
        }
        else
        {
            errorCount = std::min(ERROR_COUNT_MAX, ++errorCount);
        }

        if (errorCount < ERROR_COUNT_MAX)
        {
            if (acc_status == ACC_Status::ON)
            {
                currentSpeed = accFunc(lastSuccessfulReadDistance, currentSpeed);
            }
        }
        else
        {
            acc_status = ACC_Status::ERROR;
        }

        // FIXME: Update GUI with acc_status, currentSpeed, lastSuccessfulReadDistance

        // Sleep 5ms
        usleep(5'000);        
    }

    return nullptr;
}

static void commLoop(char *remoteMAC, pthread_mutex_t *pLock)
{
    // Set up a BT connection to node1
    BTConnection conn(remoteMAC);

    // buffer for sending and receiving messages
    array<uint8_t, MAX_MSG_LEN> msgBuf = { 0 };

    // counter for preventing replay attacks
    uint32_t counter = 0;
    
    // Create session key
    conn.keyExchangeClient();

    while (1)
    {
        // just send a dummy byte
        msgBuf[0] = 0;
        if (conn.sendWithCounterAndMAC(0x01, {&msgBuf[0], 1}, counter) >= 0)
        {
            // If we fail to receive, we convey an error status
            // FIXME: Define error codes for reading.
            uint16_t receivedReading = 0xffff;

            // wait for the response
            uint8_t msgType;
            if (conn.receiveWithCounterAndMAC(msgType, {&msgBuf[0], msgBuf.size()}, counter, true) < 0)
            {
                if (msgType == 0x02)
                {
                    receivedReading = (msgBuf[0] << 8) + msgBuf[1];
                }
            }

            // Critical section start, take care that no exception can be thrown in it!
            pthread_mutex_lock(pLock);
            gCurrentDistanceReading = receivedReading;
            pthread_mutex_unlock(pLock);
            // Critical section end
        }

        counter++;

        // sleep for 5ms
        usleep(5'000);
    }
}

int main(int argc, char *argv[])
{
    cout << "Node 2 started.\n";

    // mutex for setting up the critical section
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    // Start ACC thread
    pthread_t thread;
    if (pthread_create(&thread, NULL, accThread, &lock)) 
    {
        cerr << "Error creating thread\n";
        return 1;
    }    

    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << "<MAC_ADDR_NODE1>\n";
        return -1;
    }
// hier funktion einfügen
    while (1)
    {
        try
        {
            commLoop(argv[0], &lock);        
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

    // Will never be reached
    return 0;
}
