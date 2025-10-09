#pragma once

#include <ThreadWrapper.h>
#include "Node2Types.h"
namespace acc
{

class ACCThread : public ThreadWrapper<void>
{
public:
    ACCThread() : ThreadWrapper(nullptr), latestValidDistanceReading{ 0, 0xffff } {}
    virtual ~ACCThread() {}
    virtual void threadLoop();

private:

    uint32_t accFunc(uint16_t currentDistance, uint32_t currentSpeedMetersPerHour);
    DistanceReadingInfoType latestValidDistanceReading;
};    
}