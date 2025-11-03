#include <algorithm>
#include <unistd.h>
#include <Helper.h>
#include "ACCThread.h"

using namespace acc;

namespace
{
    constexpr uint32_t ACC_MIN_SPEED_METERS_PER_HOUR = 30000U; // NEW: Mindestgeschwindigkeit für ACC
}

void acc::ACCThread::run(void)
{
    while (!terminateApp())
    {
        // Get global vehicle state
        VehicleStateInfoType currentVehicleState = { AccState::Off, 0U, 0U, 0U };
        getCurrentVehicleState(&currentVehicleState);
        bool updateAccState = false;

        // NEW: ACC automatisch ausschalten, wenn Speed < 30 km/h
        if ((currentVehicleState.accState == AccState::On) &&
            (currentVehicleState.speedMetersPerHour < ACC_MIN_SPEED_METERS_PER_HOUR))
        {
            currentVehicleState.accState = AccState::Off;
            updateAccState = true;
        }

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
        if ((acc::getTimestampMs() - reading.timestamp) > 500U)
        {
            currentVehicleState.accState = AccState::Fault;
            updateAccState = true;
        }
        else
        {
            if (currentVehicleState.accState == AccState::On)
            {
                currentVehicleState.speedMetersPerHour = accFunc(latestValidDistanceReading.distance, currentVehicleState.speedMetersPerHour, currentVehicleState.accSetSpeedMetersPerHour);
            }
            
            // Comment in for debugging.
            // cout << "Distance: " << latestValidDistanceReading.distance 
            //      << ", Speed: " << static_cast<uint16_t>(currentSpeed)
            //      << ", TS: " << (latestValidDistanceReading.timestamp % 1000) << "\n";   
        }

        // Calculate new Global Vehicle State
        uint16_t const *pDistanceMeters = &reading.distance;
        AccState const *pAccState = nullptr;
        if (updateAccState)
        {
            pAccState = &currentVehicleState.accState;
        }

        uint32_t const *pSpeedMetersPerHour = nullptr;
        if (currentVehicleState.accState == AccState::On)
        {
            pSpeedMetersPerHour = &currentVehicleState.speedMetersPerHour;
        }

        // Write back new Global Vehicle State (accSetSpeed wird von GUI gesetzt)
        setCurrentVehicleState(pAccState, pSpeedMetersPerHour, pDistanceMeters);

        // Sleep 50ms
        usleep(50'000U);
    }
}

uint32_t acc::ACCThread::accFunc(uint16_t currentDistance, uint32_t currentSpeedMetersPerHour, uint32_t maxAllowedSpeedMetersPerHour)
{
    // FIXME: Also take the set speed of the ACC into account here!
    // "Halber Tacho"
    uint32_t targetSpeedMetersPerHour = std::min<uint32_t>(currentDistance / 2U, VEHICLE_SPEED_MAX) * 1000U;

    // Don't allow the vehice to increase speed beyond the speed when acc has been turned on
    if (maxAllowedSpeedMetersPerHour > 0U)
    {
        targetSpeedMetersPerHour = std::min<uint32_t>(targetSpeedMetersPerHour, maxAllowedSpeedMetersPerHour);
    }

    int64_t speedDifference = static_cast<int64_t>(targetSpeedMetersPerHour) - static_cast<int64_t>(currentSpeedMetersPerHour);
    // We assume that in each iteration, we cover 10% of the difference current speed / target speed.
    double appliedSpeedDelta = (speedDifference / 10.0);
    currentSpeedMetersPerHour = static_cast<uint32_t>(currentSpeedMetersPerHour + appliedSpeedDelta);
    return currentSpeedMetersPerHour;
}