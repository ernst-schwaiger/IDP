#pragma once

#include <ThreadWrapper.h>
#include <BTListenSocket.h>
#include <unistd.h>

#include "Node1Types.h"

namespace acc
{

class CommThread : public ThreadWrapper<pthread_mutex_t>
{
public:
    CommThread(bool &terminateApp, pthread_mutex_t *pLock) : ThreadWrapper(terminateApp, pLock), m_listenSocket() {}
    virtual ~CommThread(void) {}
    virtual void run(void);

private:
    void commLoop(acc::BTListenSocket &listenSocket, pthread_mutex_t *pLock);
    acc::BTListenSocket m_listenSocket;
};    
}