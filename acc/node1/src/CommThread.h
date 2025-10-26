#pragma once

#include <ThreadWrapper.h>
#include <BTListenSocket.h>
#include <unistd.h>

#include "Node1Types.h"

namespace acc
{

class CommThread : public ThreadWrapper<void>
{
public:
    explicit CommThread(bool &terminateApp) : ThreadWrapper(terminateApp, nullptr), m_listenSocket() {}
    virtual ~CommThread(void) override {}
    virtual void run(void) override;

private:
    void commLoop(acc::BTListenSocket &listenSocket);
    acc::BTListenSocket m_listenSocket;
};    
}