#pragma once

#include <ThreadWrapper.h>
#include "Node2Types.h"
namespace acc
{

class CommThread : public ThreadWrapper<char>
{
public:
    CommThread(bool &terminateApp, char *pRemoteMAC) : ThreadWrapper(terminateApp, pRemoteMAC) {}
    virtual ~CommThread(void) {}
    virtual void run(void);

private:
    void commLoop(char const *remoteMAC);
};    
}