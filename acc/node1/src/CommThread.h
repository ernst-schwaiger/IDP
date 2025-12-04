#pragma once

#include <ThreadWrapper.h>
#include <BTListenSocket.h>
#include <unistd.h>

#include "Node1Types.h"

namespace acc
{
// Communication thread on node 1/server node
// Implements Saf-REQ-5, Saf-REQ-8
class CommThread : public ThreadWrapper<void>
{
public:
    // Initialize communication thread
    explicit CommThread(bool &terminateApp) : ThreadWrapper(terminateApp, nullptr), m_listenSocket() {}
    // Cleanup communication thread
    virtual ~CommThread(void) override {}
    // Execute communication thread function
    virtual void run(void) override;

private:
    void commLoop(acc::BTListenSocket const &listenSocket) const; // communication loop (tx side)
    acc::BTListenSocket m_listenSocket; // bluetooth socket for sending
};    
}