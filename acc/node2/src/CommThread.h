#pragma once

#include <ThreadWrapper.h>
#include "Node2Types.h"
namespace acc
{

class CommThread : public ThreadWrapper<char>
{
public:
    CommThread(bool &terminateApp, char *pRemoteMAC) : ThreadWrapper(terminateApp, pRemoteMAC) {}
    virtual ~CommThread(void) override {}
    virtual void run(void) override;

private:
    void commLoop(char const *remoteMAC);
};    
}