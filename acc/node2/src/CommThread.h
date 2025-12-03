#pragma once

#include <ThreadWrapper.h>
#include "Node2Types.h"
namespace acc
{

// Communication thread on node 2/client node
// Implements Saf-REQ-5, Saf-REQ-8
class CommThread : public ThreadWrapper<char> // Deviation Dir 4.6: type passed via main() function
{
public:
    // Initialize communication thread
    CommThread(bool &terminateApp, char *pRemoteMAC) : ThreadWrapper(terminateApp, pRemoteMAC) {}
    // Cleanup communication thread    
    virtual ~CommThread(void) override {}
    // Execute communication thread function    
    virtual void run(void) override;

private:
    void commLoop(char const *remoteMAC) const; // communication loop (rx side)
};    
}