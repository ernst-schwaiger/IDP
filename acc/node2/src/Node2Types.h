#pragma once

#include <cstdint>
#include <pthread.h>

namespace acc
{

// Type definitions
enum class AccState
{ 
    Off, 
    On, 
    Fault 
};

typedef struct
{
    AccState accState;
    uint16_t distanceMeters;
    uint32_t speedMetersPerHour;
} VehicleStateInfoType;

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

// Constants
constexpr uint8_t VEHICLE_SPEED_MAX = 200U;

// APIs for MainWindow
void getCurrentVehicleState(VehicleStateInfoType *pVehicleState);
void setCurrentVehicleState(AccState const *pACCState, uint32_t const *pSpeedMetersPerHour, uint16_t const *pDistanceMeters);
[[ nodiscard ]] bool isValidDistance(uint16_t currentReading);

// APIs for ACCThread
void getCurrentDistanceReading(DistanceReadingInfoType *pReading);
// APIs for CommThread

void setCurrentDistanceReading(DistanceReadingInfoType const *pReading);

} // namespace acc
