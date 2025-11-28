#pragma once

#include <ThreadWrapper.h>
#include "Node2Types.h"
namespace acc
{

// Adaptive Cruise Control/ACC Thread on node 2/client node
// Implements
//Saf-REQ-1: The ACC shall be able to respond to a measurement error within 1 second by deactivating itself. 
//Saf-REQ-2: The ACC shall automatically detect sensor failure by checking the measured values for plausibility. 
//Saf-REQ-4: The ACC shall switch off when a sensor failure is detected. 
//Saf-REQ-11: Once activated, the ACC shall note the current speed of the vehicle and not exceed it. 
//Saf-REQ-12: The ACC shall deactivate when the vehicle speed falls below 30 km/h. 
//Saf-REQ-13: The ACC shall reduce the speed to n/2 km/h at a measured distance of n meters. 
class ACCThread : public ThreadWrapper<void>
{
public:
    // Initialize acc thread
    explicit ACCThread(bool &terminateApp) : ThreadWrapper(terminateApp, nullptr), latestValidDistanceReading{ 0, 0xffff } {}
    // Cleanup acc thread
    virtual ~ACCThread(void) override {}
    // Execute communication thread function    
    virtual void run(void) override;

private:
    // Implements acceleration/deceleration of the acc system
    [[nodiscard]] uint32_t accFunc(uint16_t currentDistance, uint32_t currentSpeedMetersPerHour, uint32_t maxAllowedSpeedMetersPerHour);
    DistanceReadingInfoType latestValidDistanceReading; // most recent valid distance reading
};    
}