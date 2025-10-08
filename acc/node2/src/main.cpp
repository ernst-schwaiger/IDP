#include <iostream>
#include <QApplication>
#include <CryptoComm.h>

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
constexpr uint16_t DISTANCE_READING_ERROR_1 = 65534U;
constexpr uint16_t DISTANCE_READING_ERROR_2 = 65535U;

// global var, written by the sensor thread, read by the acc thread, shall only be accessed via get/set functions
DistanceReadingType gCurrentDistanceReading = { { 0UL, 0xffff}, PTHREAD_MUTEX_INITIALIZER };

// global vehicle state, read and written by acc thread and GUI/main thread, shall only be accessed via get/set functions
VehicleStateType gVehicleState = { {AccState::Off, 0, 0 }, PTHREAD_MUTEX_INITIALIZER};

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

static void *accThreadFunc(void *arg)
{
    ACCThread *t = reinterpret_cast<ACCThread *>(arg);
    t->run();
    return nullptr; // wont be reached
}

static void *commThreadFunc(void *arg)
{
    CommThread *t = reinterpret_cast<CommThread *>(arg);
    t->run();
    return nullptr; // wont be reached
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << "<MAC_ADDR_NODE1>\n";
        return -1;
    }

    cout << "Node 2 started.\n";

    // Start Comm thread
    CommThread commThread(argv[1]);
    pthread_t commThreadHandle;
    if (pthread_create(&commThreadHandle, NULL, commThreadFunc, &commThread))
    {
        cerr << "Error creating acc thread\n";
        return 1;
    }    

    // Start ACC thread
    ACCThread accThread;
    pthread_t accThreadHandle;
    if (pthread_create(&accThreadHandle, NULL, accThreadFunc, &accThread))
    {
        cerr << "Error creating acc thread\n";
        return 1;
    }

    // Init GUI, start GUI thread
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", QByteArray("0"));
    qputenv("QT_SCALE_FACTOR", QByteArray("1"));
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(760, 440);
    w.show();
    
    // QT requires the app to run in the main thread.
    return app.exec();
}
