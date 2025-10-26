#include <iostream>
#include <optional>
#include <QApplication>

#include "MainWindow.h"
#include "Node2Types.h"
#include "ACCThread.h"
#include "CommThread.h"

using namespace std;
using namespace acc;

typedef struct
{
    VehicleStateInfoType info;
    pthread_mutex_t lock;
} VehicleStateType;

// constants
static constexpr uint16_t DISTANCE_READING_ERROR_1 = 65534U;
static constexpr uint16_t DISTANCE_READING_ERROR_2 = 65535U;

// global var, written by the sensor thread, read by the acc thread, shall only be accessed via get/set functions
static DistanceReadingType gCurrentDistanceReading = { { 0UL, 0xffff}, PTHREAD_MUTEX_INITIALIZER };

// global vehicle state, read and written by acc thread and GUI/main thread, shall only be accessed via get/set functions
static VehicleStateType gVehicleState = { {AccState::Off, 0U, 0U }, PTHREAD_MUTEX_INITIALIZER};

// global app termination flag; no critical sections required
static bool gTerminateApplication = false;

namespace acc
{
void getCurrentDistanceReading(DistanceReadingInfoType *pReading)
{
    pthread_mutex_lock(&gCurrentDistanceReading.lock);
    pReading->distance = gCurrentDistanceReading.info.distance;
    pReading->timestamp = gCurrentDistanceReading.info.timestamp;
    pthread_mutex_unlock(&gCurrentDistanceReading.lock);
}

void setCurrentDistanceReading(DistanceReadingInfoType const *pReading)
{
    pthread_mutex_lock(&gCurrentDistanceReading.lock);
    gCurrentDistanceReading.info.distance = pReading->distance;
    gCurrentDistanceReading.info.timestamp = pReading->timestamp;
    pthread_mutex_unlock(&gCurrentDistanceReading.lock);
}

bool isValidDistance(uint16_t currentReading)
{
    return ((currentReading != DISTANCE_READING_ERROR_1) && (currentReading != DISTANCE_READING_ERROR_2));
}

void getCurrentVehicleState(VehicleStateInfoType *pVehicleState)
{
    pthread_mutex_lock(&gVehicleState.lock);
    pVehicleState->accState = gVehicleState.info.accState;
    pVehicleState->distanceMeters = gVehicleState.info.distanceMeters;
    pVehicleState->speedMetersPerHour = gVehicleState.info.speedMetersPerHour;
    pthread_mutex_unlock(&gVehicleState.lock);
}

void setCurrentVehicleState(AccState const *pACCState, uint32_t const *pSpeedMetersPerHour, uint16_t const *pDistanceMeters)
{
    pthread_mutex_lock(&gVehicleState.lock);
    if (pACCState != nullptr)
    {
        gVehicleState.info.accState = *pACCState;
    }

    if (pSpeedMetersPerHour != nullptr)
    {
        gVehicleState.info.speedMetersPerHour = *pSpeedMetersPerHour;

    }

    if (pDistanceMeters != nullptr)
    {
        gVehicleState.info.distanceMeters = *pDistanceMeters;
    }

    pthread_mutex_unlock(&gVehicleState.lock);
}

static void *accThreadFunc(void *)
{
    ACCThread t(gTerminateApplication);

    try
    {
        t.run();
    }
    catch(runtime_error const &e)
    {
        cerr << "Runtime Exception: " << e.what() << '\n';
        perror("Error message: ");
    }
    catch(...)
    {
        cerr << "Unknown exception occurred.\n";
    }

    // signal other threads to stop
    gTerminateApplication = true;
    
    return nullptr;
}

static void *commThreadFunc(void *arg)
{
    char *remoteMAC = reinterpret_cast<char *>(arg);

    CommThread t(gTerminateApplication, remoteMAC);
    
    try
    {
        t.run();
    }
    catch(runtime_error const &e)
    {
        cerr << "Runtime Exception: " << e.what() << '\n';
        perror("Error message: ");
    }
    catch(...)
    {
        cerr << "Unknown exception occurred.\n";
    }

    // signal other threads to stop
    gTerminateApplication = true;

    return nullptr;
}

} // namespace acc

int main(int argc, char *argv[])
{
    int ret = 0;
    optional<pthread_t> optCommThreadHandle;
    optional<pthread_t> optAccThreadHandle;

    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << "<MAC_ADDR_NODE1>\n";
        return -1;
    }

    cout << "Node 2 started.\n";

    try
    {
        pthread_t tmpThreadHandle;
        // Start comm thread
        if (pthread_create(&tmpThreadHandle, nullptr, commThreadFunc, argv[1]))
        {
            cerr << "Error creating acc thread\n";
            ret = 1;
        }
        else
        {
            optCommThreadHandle = tmpThreadHandle;

            // Start ACC thread
            if (pthread_create(&tmpThreadHandle, nullptr, accThreadFunc, nullptr))
            {
                cerr << "Error creating acc thread\n";
                ret = 1;
            }
            else
            {
                optAccThreadHandle = tmpThreadHandle;
                
                // Init GUI
                qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", QByteArray("0"));
                qputenv("QT_SCALE_FACTOR", QByteArray("1"));
                QApplication app(argc, argv);
                MainWindow w(gTerminateApplication);
                w.resize(760, 440);
                w.show();
                
                // QT requires the app to run in the main thread.
                ret = app.exec();                
            }
        }
    }
    catch(runtime_error const &e)
    {
        cerr << "Runtime Exception: " << e.what() << '\n';
        perror("Error message: ");
        ret = -1;
    }
    catch(...)
    {
        cerr << "Unknown exception occurred.\n";
        ret = -1;
    }

    // signal all threads to stop
    gTerminateApplication = true;
    cout << "Shutting down node2...\n";

    // collect threads
    if (optAccThreadHandle.has_value())
    {
        cout << "Joining acc thread...\n";
        pthread_join(*optAccThreadHandle, nullptr);
    }

    if (optCommThreadHandle.has_value())
    {
        cout << "Joining comm thread...\n";
        pthread_join(*optCommThreadHandle, nullptr);
    }

    return ret;
}
