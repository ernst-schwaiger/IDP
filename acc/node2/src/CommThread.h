#pragma once

#include <ThreadWrapper.h>
#include "Node2Types.h"
namespace acc
{

class CommThread : public ThreadWrapper<char>
{
public:
    CommThread(char *pRemoteMAC) : ThreadWrapper(pRemoteMAC) {}
    virtual ~CommThread() {}
    virtual void threadLoop();

private:
    void commLoop(char const *remoteMAC);
};    
}