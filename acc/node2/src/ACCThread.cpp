#include <algorithm>
#include <unistd.h>
#include <Helper.h>
#include "ACCThread.h"

using namespace acc;

// Requirements traceability (MISRA Dir 7.3) - ACCThread
//
// Safety related requirements in this file: 
//Saf-REQ-1: The ACC shall be able to respond to a measurement error within 1 second by deactivating itself. 
//Saf-REQ-2: The ACC shall automatically detect sensor failure by checking the measured values for plausibility. 
//Saf-REQ-4: The ACC shall switch off when a sensor failure is detected. 
//Saf-REQ-11: Once activated, the ACC shall note the current speed of the vehicle and not exceed it. 
//Saf-REQ-12: The ACC shall deactivate when the vehicle speed falls below 30 km/h. 
//Saf-REQ-13: The ACC shall reduce the speed to n/2 km/h at a measured distance of n meters. 


namespace
{
    // Saf-REQ-12: 30 km/h in m/h
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

        // Saf-REQ-12:
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

        // Saf-REQ-2
        if (isValidDistance(reading.distance))
        {
            latestValidDistanceReading.distance = reading.distance;
            latestValidDistanceReading.timestamp = reading.timestamp;

            // Saf-REQ-4
            if (currentVehicleState.accState == AccState::Fault)
            {
                currentVehicleState.accState = AccState::Off;
                updateAccState = true;
            }
        }

        // Saf-REQ-1, Saf-REQ-2, Saf-REQ-4
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
                // Saf-REQ-11, Saf-REQ-13
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
        // (accSetSpeed wird von der GUI beim Aktivieren gesetzt - Saf-REQ-11)
        setCurrentVehicleState(pAccState, pSpeedMetersPerHour, pDistanceMeters);

        // Sleep 50ms (Saf-REQ-1)
        usleep(50'000U);
    }
}

// Saf-REQ-11, Saf-REQ-13
uint32_t acc::ACCThread::accFunc(uint16_t currentDistance, uint32_t currentSpeedMetersPerHour, uint32_t maxAllowedSpeedMetersPerHour)
{
    // FIXME: Also take the set speed of the ACC into account here!
    // "Halber Tacho" (Saf-REQ-13)
    uint32_t targetSpeedMetersPerHour = std::min<uint32_t>(currentDistance / 2U, VEHICLE_SPEED_MAX) * 1000U;

    // Don't allow the vehice to increase speed beyond the speed when acc has been turned on
    // Saf-REQ-11
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