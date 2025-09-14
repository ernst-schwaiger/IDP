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
class BTConnection
{
public:
    BTConnection(char const *remoteMAC, uint16_t port, bool listen) : 
        m_listenSocket { -1 },
        m_socket { -1 },
        m_local_addr{ AF_BLUETOOTH, htobs(port), { 0 }, 0, 0 },
        m_remote_addr{ AF_BLUETOOTH, htobs(port), { 0 }, 0, 0 }
    {
        str2ba( remoteMAC, &m_local_addr.l2_bdaddr );

        if (listen)
        {
            int m_listenSocket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

            if (m_listenSocket < 0)
            {
                throw std::runtime_error("failed to open bluetooth listen socket");
            }
            ::bind(m_listenSocket, reinterpret_cast<struct sockaddr *>(&m_local_addr), sizeof(m_local_addr));
            socklen_t socklen = sizeof(m_remote_addr);
            m_socket = ::accept(m_listenSocket, reinterpret_cast<struct sockaddr *>(&m_remote_addr), &socklen);

            if (m_socket < 0)
            {
                throw std::runtime_error("failed to open bluetooth client socket");
            }
        }
        else
        {
            m_socket = ::connect(m_listenSocket, reinterpret_cast<struct sockaddr *>(&m_remote_addr), sizeof(m_remote_addr));
            if (m_socket < 0)
            {
                throw std::runtime_error("failed to open bluetooth server socket");
            }            
        }
    }

    ~BTConnection()
    {
        close(m_listenSocket);
        close(m_socket);
    }

    ssize_t send(std::span<const uint8_t> txData)
    {
        return write(m_socket, &txData[0], txData.size());
    }

    ssize_t receive(std::span<uint8_t> rxData)
    {
        return read(m_socket, &rxData[0], rxData.size());
    }

private:

    int m_listenSocket;
    int m_socket;
    struct sockaddr_l2 m_local_addr;
    struct sockaddr_l2 m_remote_addr;
};

} // namespace acc

int foo();

#endif