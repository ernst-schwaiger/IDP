#include <unistd.h>
#include <Helper.h>
#include "ACCThread.h"

void acc::ACCThread::threadLoop()
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
    if ((acc::getTimestampMs() - reading.timestamp) > 500)
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

uint8_t acc::ACCThread::accFunc(uint16_t currentDistance, uint8_t currentSpeed)
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