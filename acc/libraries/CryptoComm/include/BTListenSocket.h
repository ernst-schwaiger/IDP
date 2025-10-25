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

    int getListenSocket(void) const
    {
        return m_listenSocket;
    }

private:

    void makeBTDeviceVisible(void);
    
    int m_listenSocket;
    struct sockaddr_l2 m_local_addr;
};

} // namespace acc