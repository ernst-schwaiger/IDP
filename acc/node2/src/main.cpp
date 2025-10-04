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

typedef struct 
{
    uint64_t timestamp;
    uint16_t distance;
} DistanceReadingType;

// constants
constexpr uint8_t ERROR_COUNT_MAX = 4U;
constexpr uint16_t VEHICLE_SPEED_MAX = 200U;
constexpr uint16_t DISTANCE_READING_ERROR_1 = 65534U;
constexpr uint16_t DISTANCE_READING_ERROR_2 = 65535U;

// global var, written by the sensor thread, shall only be accessed via get/set functions
volatile DistanceReadingType gCurrentDistanceReading = { 0U, 0xffff };

static uint64_t getTimestampMs()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return ms;
}

void getCurrentDistanceReading(pthread_mutex_t *pLock, DistanceReadingType *pReading)
{
    pthread_mutex_lock(pLock);
    pReading->distance = gCurrentDistanceReading.distance;
    pReading->timestamp = gCurrentDistanceReading.timestamp;
    pthread_mutex_unlock(pLock);
}

void setCurrentDistanceReading(pthread_mutex_t *pLock, uint16_t distance, uint64_t timestamp)
{
    pthread_mutex_lock(pLock);
    gCurrentDistanceReading.distance = distance;
    gCurrentDistanceReading.timestamp = timestamp;
    pthread_mutex_unlock(pLock);
}

static bool isValidDistance(uint16_t currentReading)
{
    return (currentReading < DISTANCE_READING_ERROR_1);
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
    // FIXME: Also take the set speed of the ACC into account here!
    // "Halber Tacho"
    uint16_t targetSpeed = currentDistance / 2;
    int speedDifference = targetSpeed - currentSpeed;
    // We assume that in each iteration, we cover 10% of the difference current speed / target speed.
    int appliedSpeedDelta = speedDifference / 10;
    currentSpeed = static_cast<uint16_t>(currentSpeed + appliedSpeedDelta);
    return currentSpeed;
}

static void *accThread(void *arg)
{
    pthread_mutex_t *pLock = reinterpret_cast<pthread_mutex_t *>(arg);
    DistanceReadingType latestValidDistanceReading = { 0, 0xffff };

    while (1)
    {
        // get the current speed from the GUI
        uint8_t currentSpeed = getCurrentSpeedFromGUI();
        // get the current status of the ACC from the GUI
        ACC_Status acc_status = getACCStatusFromGUI();

        DistanceReadingType reading;
        getCurrentDistanceReading(pLock, &reading);

        if (isValidDistance(reading.distance))
        {
            latestValidDistanceReading.distance = reading.distance;
            latestValidDistanceReading.timestamp = reading.timestamp;

            if (acc_status == ACC_Status::ERROR)
            {
                acc_status = ACC_Status::OFF;
            }
        }

        // Is our latest reading too old?
        if ((getTimestampMs() - reading.timestamp) > 500)
        {
            acc_status = ACC_Status::ERROR;
        }
        else
        {
            if (acc_status == ACC_Status::ON)
            {
                currentSpeed = accFunc(latestValidDistanceReading.distance, currentSpeed);
            }
            
            // Comment in for debugging.
            // cout << "Distance: " << latestValidDistanceReading.distance 
            //      << ", Speed: " << static_cast<uint16_t>(currentSpeed)
            //      << ", TS: " << (latestValidDistanceReading.timestamp % 1000) << "\n";   
        }

        // FIXME: Update GUI with acc_status, currentSpeed, lastSuccessfulReadDistance

        // Sleep 50ms
        usleep(50'000);       
    }

    return nullptr;
}

static void *guiThread(void *arg)
{
    // MUTEX for accessing the current distance reading via getCurrentDistanceReading(). 
    pthread_mutex_t *pLock = reinterpret_cast<pthread_mutex_t *>(arg);

    // FIXME: Add one-time GUI init code here

    while (1)
    {
        // FIXME: Add GUI event loop here.
        usleep(1'000'000);
    }

    // won't ever be reached.
    return nullptr;
}

static void commLoop(char *remoteMAC, pthread_mutex_t *pLock)
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

    while (1)
    {
        uint8_t msgType;
        // buffer for sending and receiving messages
        array<uint8_t, MAX_MSG_LEN> msgBuf = { 0 };
        // message reception (blocking)
        if (conn.receiveWithCounterAndMAC(msgType, {&msgBuf[0], msgBuf.size()}, counter, true) >= 0)
        {
            if (msgType == 0x02)
            {
                uint16_t receivedReading = (msgBuf[0] << 8) + msgBuf[1];
                // Update current distance reading in a critical section, together with the measured timestamp
                uint64_t timestampMs = getTimestampMs();
                setCurrentDistanceReading(pLock, receivedReading, timestampMs);     
            }
        }

        // no sleep required here; we block in the receive function
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << "<MAC_ADDR_NODE1>\n";
        return -1;
    }

    cout << "Node 2 started.\n";

    // mutex for setting up the critical section
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    // Start ACC thread
    pthread_t accThreadHandle;
    if (pthread_create(&accThreadHandle, NULL, accThread, &lock)) 
    {
        cerr << "Error creating acc thread\n";
        return 1;
    }    

    // Start GUI thread
    pthread_t guiThreadHandle;
    if (pthread_create(&guiThreadHandle, NULL, guiThread, &lock)) 
    {
        cerr << "Error creating acc thread\n";
        return 1;
    }    

    while (1)
    {
        try
        {
            commLoop(argv[1], &lock);        
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
