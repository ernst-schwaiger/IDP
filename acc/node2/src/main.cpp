#include <iostream>
#include <span>
#include <cstdint>
#include <string.h>

#include <signal.h>
#include <time.h>

#include <QApplication>
#include <QScreen>

#include <CryptoComm.h>
#include "MainWindow.h"
#include "Node2Types.h"

using namespace std;
using namespace acc;

typedef struct 
{
    uint64_t timestamp;
    uint16_t distance;
} DistanceReadingInfoType;

typedef struct 
{
    DistanceReadingInfoType info;
    pthread_mutex_t lock;
} DistanceReadingType;

typedef struct
{
    VehicleStateInfoType info;
    pthread_mutex_t lock;
} VehicleStateType;

// constants
constexpr uint16_t DISTANCE_READING_ERROR_1 = 65534U;
constexpr uint16_t DISTANCE_READING_ERROR_2 = 65535U;

// global var, written by the sensor thread, read by the acc thread, shall only be accessed via get/set functions
DistanceReadingType gCurrentDistanceReading = { { 0UL, 0xffff}, PTHREAD_MUTEX_INITIALIZER };

// global vehicle state, read and written by acc thread and GUI/main thread, shall only be accessed via get/set functions
VehicleStateType gVehicleState = { {AccState::Off, 0, 0 }, PTHREAD_MUTEX_INITIALIZER};

static void getCurrentDistanceReading(DistanceReadingInfoType *pReading)
{
    pthread_mutex_lock(&gCurrentDistanceReading.lock);
    pReading->distance = gCurrentDistanceReading.info.distance;
    pReading->timestamp = gCurrentDistanceReading.info.timestamp;
    pthread_mutex_unlock(&gCurrentDistanceReading.lock);
}

static void setCurrentDistanceReading(DistanceReadingInfoType const *pReading)
{
    pthread_mutex_lock(&gCurrentDistanceReading.lock);
    gCurrentDistanceReading.info.distance = pReading->distance;
    gCurrentDistanceReading.info.timestamp = pReading->timestamp;
    pthread_mutex_unlock(&gCurrentDistanceReading.lock);
}

bool isValidDistance(uint16_t currentReading)
{
    return (currentReading < DISTANCE_READING_ERROR_1);
}

void getCurrentVehicleState(VehicleStateInfoType *pVehicleState)
{
    pthread_mutex_lock(&gVehicleState.lock);
    pVehicleState->accState = gVehicleState.info.accState;
    pVehicleState->distanceMeters = gVehicleState.info.distanceMeters;
    pVehicleState->speedKmH = gVehicleState.info.speedKmH;
    pthread_mutex_unlock(&gVehicleState.lock);
}

void setCurrentVehicleState(AccState const *pACCState, uint8_t const *pSpeedKmH, uint16_t const *pDistanceMeters)
{
    pthread_mutex_lock(&gVehicleState.lock);
    if (pACCState != nullptr)
    {
        gVehicleState.info.accState = *pACCState;
    }

    if (pSpeedKmH != nullptr)
    {
        gVehicleState.info.speedKmH = *pSpeedKmH;

    }

    if (pDistanceMeters != nullptr)
    {
        gVehicleState.info.distanceMeters = *pDistanceMeters;
    }

    pthread_mutex_unlock(&gVehicleState.lock);
}

static uint64_t getTimestampMs()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    uint64_t ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return ms;
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

static void *accThread(void *)
{
    // GUI and MUTEX for accessing the current distance reading via getCurrentDistanceReading(). 
    DistanceReadingInfoType latestValidDistanceReading = { 0, 0xffff };

    while (1)
    {
        // Get global vehicle state
        VehicleStateInfoType currentVehicleState = { AccState::Off, 0U, 0U };
        getCurrentVehicleState(&currentVehicleState);
        bool updateAccState = false;

        // Get most recently received distance reading
        DistanceReadingInfoType reading;
        getCurrentDistanceReading(&reading);

        if (isValidDistance(reading.distance))
        {
            latestValidDistanceReading.distance = reading.distance;
            latestValidDistanceReading.timestamp = reading.timestamp;

            if (currentVehicleState.accState == AccState::Fault)
            {
                currentVehicleState.accState = AccState::Off;
                updateAccState = true;
            }
        }

        // Is our latest reading too old (older than 500ms)?
        if ((getTimestampMs() - reading.timestamp) > 500)
        {
            currentVehicleState.accState = AccState::Fault;
            updateAccState = true;
        }
        else
        {
            if (currentVehicleState.accState == AccState::On)
            {
                currentVehicleState.speedKmH = accFunc(latestValidDistanceReading.distance, currentVehicleState.speedKmH);
            }
            
            // Comment in for debugging.
            // cout << "Distance: " << latestValidDistanceReading.distance 
            //      << ", Speed: " << static_cast<uint16_t>(currentSpeed)
            //      << ", TS: " << (latestValidDistanceReading.timestamp % 1000) << "\n";   
        }

        // Calculate new Global Vehicle State
        uint16_t *pDistanceMeters = &reading.distance;
        AccState *pAccState = nullptr;
        if (updateAccState)
        {
            pAccState = &currentVehicleState.accState;
        }

        uint8_t *pSpeedKmH = nullptr;
        if (currentVehicleState.accState == AccState::On)
        {
            pSpeedKmH = &currentVehicleState.speedKmH;
        }

        // Write back new Global Vehicle State
        setCurrentVehicleState(pAccState, pSpeedKmH, pDistanceMeters);

        // Sleep 50ms
        usleep(50'000);       
    }

    return nullptr;
}

static void commLoop(char const *remoteMAC)
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
                DistanceReadingInfoType reading = { getTimestampMs(), static_cast<uint16_t>((msgBuf[0] << 8) + msgBuf[1]) };
                setCurrentDistanceReading(&reading);     
            }
        }

        // no sleep required here; we block in the receive function
    }
}

static void *commThread(void *arg)
{
    char const *pRemoteMAC = reinterpret_cast<char const *>(arg);

    while (1)
    {
        try
        {
            commLoop(pRemoteMAC);
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

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << "<MAC_ADDR_NODE1>\n";
        return -1;
    }

    cout << "Node 2 started.\n";

    // Init GUI, start GUI thread
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", QByteArray("0"));
    qputenv("QT_SCALE_FACTOR", QByteArray("1"));
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(760, 440);
    w.show();

    // Start Comm thread
    pthread_t commThreadHandle;
    if (pthread_create(&commThreadHandle, NULL, commThread, argv[1]))
    {
        cerr << "Error creating acc thread\n";
        return 1;
    }    

    // Start ACC thread
    pthread_t accThreadHandle;
    if (pthread_create(&accThreadHandle, NULL, accThread, nullptr))
    {
        cerr << "Error creating acc thread\n";
        return 1;
    }
    
    // QT requires the app to run in the main thread.
    return app.exec();
}
