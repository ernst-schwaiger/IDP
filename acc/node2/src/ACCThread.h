#pragma once

#include <ThreadWrapper.h>
#include "Node2Types.h"
namespace acc
{

class ACCThread : public ThreadWrapper<void>
{
public:
    explicit ACCThread(bool &terminateApp) : ThreadWrapper(terminateApp, nullptr), latestValidDistanceReading{ 0, 0xffff } {}
    virtual ~ACCThread(void) override {}
    virtual void run(void) override;

private:

    [[nodiscard]] uint32_t accFunc(uint16_t currentDistance, uint32_t currentSpeedMetersPerHour, uint32_t maxAllowedSpeedMetersPerHour);
    DistanceReadingInfoType latestValidDistanceReading;
};    
}