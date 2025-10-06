#pragma once

#include <cstdint>

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
    uint8_t speedKmH;
} VehicleStateInfoType;

// Constants
constexpr uint8_t VEHICLE_SPEED_MAX = 200U;
void getCurrentVehicleState(VehicleStateInfoType *pVehicleState);
void setCurrentVehicleState(AccState const *pACCState, uint8_t const *pSpeedKmH, uint16_t const *pDistanceMeters);
bool isValidDistance(uint16_t currentReading);
