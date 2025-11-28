#pragma once

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

namespace acc
{

// abstraction of the listen socket on server side, turns on visibility on activation
// requirements implemented: Saf-REQ-8
class BTListenSocket
{
public:
    // Set up BT listen socket
    BTListenSocket(void);
    // Clean up resources of BT listen socket
    ~BTListenSocket(void);

    // Retrieve the socket handle
    [[ nodiscard ]] int getListenSocket(void) const
    {
        return m_listenSocket;
    }

private:

    // turns on visibilty so peer can contact it
    static void makeBTDeviceVisible(void);
    
    int m_listenSocket; // Deviation Dir 4.6: type used in external socket API
    struct sockaddr_l2 m_local_addr;
};

} // namespace acc