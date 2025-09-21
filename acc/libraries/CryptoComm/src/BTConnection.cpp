#include "BTConnection.h"

#include <stdexcept>
#include <span>
#include <array>
#include <unistd.h>

using namespace std;

namespace acc
{
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

ssize_t BTConnection::sendLocalRandom() noexcept
{
    array<uint8_t, 33> msgPayload;
    array<uint8_t, 32> const &localRandom = m_cryptoWrapper.getLocalRandomNumber();
    msgPayload[0] = 0;
    copy(begin(localRandom), end(localRandom), &msgPayload[1]);
    return send(msgPayload);
}

void BTConnection::keyExchangeClient()
{
    // Determine session key
    array<uint8_t, 33> remoteKeyMsg;
    sendLocalRandom();
    ssize_t remoteRandom = receive(remoteKeyMsg);
    if ((remoteRandom != 33) || (remoteKeyMsg[0] != 0))
    {
        throw runtime_error("Received incorrect random message from server");
    }

    m_cryptoWrapper.generateSessionKey({ &remoteKeyMsg[1], remoteKeyMsg.size() - 1});

    // FIXME: Send an ack that the remote rnd number arrived sccessfully
}

void BTConnection::keyExchangeServer()
{
    // Determine session key
    array<uint8_t, 33> remoteKeyMsg;
    ssize_t remoteRandom = receive(remoteKeyMsg);
    if ((remoteRandom != 33) || (remoteKeyMsg[0] != 0))
    {
        throw runtime_error("Received incorrect random message from server");
    }
    sendLocalRandom();
    m_cryptoWrapper.generateSessionKey({ &remoteKeyMsg[1], remoteKeyMsg.size() - 1});
    
    // FIXME: Receive an ack to ensure that the local rnd number arrived at the remote node
}

ssize_t BTConnection::send(std::span<const uint8_t> txData) noexcept
{
    return write(m_socket, &txData[0], txData.size());
}

ssize_t BTConnection::receive(std::span<uint8_t> rxData) noexcept
{
    return read(m_socket, &rxData[0], rxData.size());
}

BTConnection::~BTConnection()
{
    close(m_socket);
}

};