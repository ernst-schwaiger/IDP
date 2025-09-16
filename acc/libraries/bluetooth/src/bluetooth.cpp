#include "bluetooth.h"

namespace acc
{

BTListenSocket::BTListenSocket() : 
    m_local_addr{ AF_BLUETOOTH, htobs(0x1001), { 0 }, 0, 0 }
{
    m_listenSocket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    if (m_listenSocket < 0)
    {
        throw std::runtime_error("failed to create listen socket");
    }

    if (::bind(m_listenSocket, reinterpret_cast<struct sockaddr *>(&m_local_addr), sizeof(m_local_addr)) < 0)
    {
        throw std::runtime_error("failed to bind to listen socket");
    }

    // put socket into listening mode
    ::listen(m_listenSocket, 1);
}

BTListenSocket::~BTListenSocket()
{
    close(m_listenSocket);
}

BTConnection::BTConnection(BTListenSocket *pListenSocket) : 
    m_remote_addr{ AF_BLUETOOTH, htobs(0x1001), { 0 }, 0, 0 }
{
    socklen_t socklen = sizeof(m_remote_addr);
    m_socket = ::accept(pListenSocket->getListenSocket(), reinterpret_cast<struct sockaddr *>(&m_remote_addr), &socklen);

    if (m_socket < 0)
    {
        throw std::runtime_error("failed to open bluetooth client socket");
    }
}

BTConnection::BTConnection(char const *remoteMAC) : 
    m_remote_addr{ AF_BLUETOOTH, htobs(0x1001), { 0 }, 0, 0 }
{
    m_socket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    ::str2ba( remoteMAC, &m_remote_addr.l2_bdaddr );
    if (::connect(m_socket, reinterpret_cast<struct sockaddr *>(&m_remote_addr), sizeof(m_remote_addr)) < 0)
    {
        throw std::runtime_error("failed to open bluetooth server socket");
    }            
}

BTConnection::~BTConnection()
{
    close(m_socket);
}



} // namespace acc


int foo()
{
    return 42;
}