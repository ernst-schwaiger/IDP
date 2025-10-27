#pragma once

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

namespace acc
{

class BTListenSocket
{
public:
    BTListenSocket(void);
    ~BTListenSocket(void);

    [[ nodiscard ]] int getListenSocket(void) const
    {
        return m_listenSocket;
    }

private:

    static void makeBTDeviceVisible(void);
    
    int m_listenSocket; // Deviation Dir 4.6: type used in external socket API
    struct sockaddr_l2 m_local_addr;
};

} // namespace acc