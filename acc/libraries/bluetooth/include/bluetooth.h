#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include <unistd.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <cstdint>

#include <stdexcept>
#include <span>

namespace acc
{

class BTListenSocket
{
public:
    BTListenSocket();
    ~BTListenSocket();

    int getListenSocket() const
    {
        return m_listenSocket;
    }

private:
    int m_listenSocket;
    struct sockaddr_l2 m_local_addr;
};

class BTConnection
{
public:
    // Server Constructor
    BTConnection(BTListenSocket *pListenSocket);
    // Client constructor
    BTConnection(char const *remoteMAC);
    ~BTConnection();

    ssize_t send(std::span<const uint8_t> txData) noexcept
    {
        return write(m_socket, &txData[0], txData.size());
    }

    ssize_t receive(std::span<uint8_t> rxData) noexcept
    {
        return read(m_socket, &rxData[0], rxData.size());
    }

private:

    int m_socket;
    struct sockaddr_l2 m_remote_addr;
};

} // namespace acc

int foo();

#endif
