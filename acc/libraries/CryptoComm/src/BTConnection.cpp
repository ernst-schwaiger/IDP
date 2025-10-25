#include "BTConnection.h"

#include <stdexcept>
#include <span>
#include <array>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

namespace acc
{
BTConnection::BTConnection(BTListenSocket *pListenSocket) : 
    m_cryptoWrapper{},
    m_remote_addr{ AF_BLUETOOTH, htobs(0x1001), { 0 }, 0, 0 }
{
    setNonBlocking(pListenSocket->getListenSocket());
    socklen_t socklen = sizeof(m_remote_addr);
    m_socket = ::accept(pListenSocket->getListenSocket(), reinterpret_cast<struct sockaddr *>(&m_remote_addr), &socklen);

    if (m_socket < 0)
    {
        throw std::runtime_error("failed to open bluetooth client socket");
    }
}

BTConnection::BTConnection(char const *remoteMAC) : 
    m_cryptoWrapper{},
    m_remote_addr{ AF_BLUETOOTH, htobs(0x1001), { 0 }, 0, 0 }
{
    m_socket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    setNonBlocking(m_socket);
    ::str2ba( remoteMAC, &m_remote_addr.l2_bdaddr );
    if (::connect(m_socket, reinterpret_cast<struct sockaddr *>(&m_remote_addr), sizeof(m_remote_addr)) < 0)
    {
        throw std::runtime_error("failed to open bluetooth server socket");
    }            
}

ssize_t BTConnection::sendLocalRandom(void) noexcept
{
    array<uint8_t, 33> msgPayload;
    array<uint8_t, 32> const &localRandom = m_cryptoWrapper.getLocalRandomNumber();
    msgPayload[0] = 0;
    copy(begin(localRandom), end(localRandom), &msgPayload[1]);
    return send(msgPayload);
}

bool BTConnection::keyExchangeClient(void)
{
    bool ret = true;
    // Determine session key
    array<uint8_t, 33> remoteKeyMsg;
    sendLocalRandom();
    usleep(1'000'000U);
    ssize_t remoteRandom = receive(remoteKeyMsg);

    // key exchange message response from server did not arrive yet
    if ((remoteRandom < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
    {
        ret = false;
    }
    else
    {
        // incorrect payload received, reset, restart connection
        if ((remoteRandom != 33U) || (remoteKeyMsg[0] != 0U))
        {
            throw runtime_error("Received incorrect random message from server");
        }
        // Successful reception
        m_cryptoWrapper.generateSessionKey({ &remoteKeyMsg[1], remoteKeyMsg.size() - 1});
    }

    return ret;
}

bool BTConnection::keyExchangeServer(void)
{
    bool ret = true;

    // Determine session key
    array<uint8_t, 33> remoteKeyMsg;
    ssize_t remoteRandom = receive(remoteKeyMsg);

    if ((remoteRandom < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
    {
        ret = false;
    }
    else
    {
        if ((remoteRandom != 33) || (remoteKeyMsg[0] != 0))
        {
            throw runtime_error("Received incorrect random message from server");
        }
        sendLocalRandom();
        m_cryptoWrapper.generateSessionKey({ &remoteKeyMsg[1], remoteKeyMsg.size() - 1});
    }

    usleep(500'000U);

    return ret;
}

ssize_t BTConnection::send(std::span<const uint8_t> txData) noexcept
{
    return write(m_socket, &txData[0], txData.size());
}

ssize_t BTConnection::receive(std::span<uint8_t> rxData) noexcept
{
    ssize_t ret = read(m_socket, &rxData[0], rxData.size());
    return ret;
}

ssize_t BTConnection::sendWithCounterAndMAC(uint8_t msgType, std::span<const uint8_t> txData, uint32_t counter)
{
    if (txData.size() > MAX_PAYLOAD_LEN)
    {
        return -1;
    }

    array<uint8_t, MAX_MSG_LEN> msgBuf;
    msgBuf[0] = msgType;
    serializeUint32(&msgBuf[1], counter);
    std::copy(&txData[0], &txData[txData.size()], &msgBuf[5]);
    uint32_t protectedByteByHMAC = TYPE_LEN + COUNTER_LEN + txData.size();
    if (m_cryptoWrapper.generateHMAC({&msgBuf[0], protectedByteByHMAC}, {&msgBuf[protectedByteByHMAC], MAC_LEN}) != 0)
    {
        return -1;
    }

    ssize_t sentBytes = send({&msgBuf[0], protectedByteByHMAC + MAC_LEN});
    if (sentBytes < 0)
    {
        // Throwing leaves the comm loop and will set up a new connection
        throw runtime_error ("Failed to send data");
    }

    return sentBytes;
}

ssize_t BTConnection::receiveWithCounterAndMAC(uint8_t &msgType, std::span<uint8_t> payload, uint32_t &counter, bool verifyCounter)
{
    uint32_t overheadLength = TYPE_LEN + COUNTER_LEN + MAC_LEN;
    array<uint8_t, MAX_MSG_LEN> rxBuf;
    ssize_t rxMsgLen = receive(rxBuf);

    if (rxMsgLen < 0)
    {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
        {
            throw runtime_error ("Failed to receive data");
        }
    }

    if (rxMsgLen <= overheadLength)
    {
        return -1;
    }

    ssize_t payloadLength = rxMsgLen - overheadLength;

    if (payload.size() < static_cast<size_t>(payloadLength))
    {
        return -1; // Cannot copy full payload into payload span
    }

    uint32_t remoteCounter = deSerializeUint32(&rxBuf[1]);

    if (verifyCounter)
    {
        // if the counter value must be verified (at Node 2), only messages 
        // with identical or higher counter are accepted
        if (counter >= remoteCounter)
        {
            return -1;
        }
    }
    else
    {
        // if the counter must only be read (Node 1), we return it as an out param
        counter = remoteCounter;
    }

    if (m_cryptoWrapper.verifyHMAC({&rxBuf[0], static_cast<uint32_t>(rxMsgLen) - MAC_LEN}, 
        {&rxBuf[rxMsgLen - MAC_LEN], MAC_LEN}) != 0)
    {
        return -1;
    }

    // everything is OK, extract payload and message type
    std::copy(&rxBuf[TYPE_LEN + COUNTER_LEN], &rxBuf[TYPE_LEN + COUNTER_LEN + payloadLength], &payload[0]);
    msgType = rxBuf[0];
    
    return payloadLength;
}

void BTConnection::serializeUint32(uint8_t *pBuf, uint32_t val) const
{
    pBuf[0] = static_cast<uint8_t>((val >> 24) & 0xff);
    pBuf[1] = static_cast<uint8_t>((val >> 16) & 0xff);
    pBuf[2] = static_cast<uint8_t>((val >> 8) & 0xff);
    pBuf[3] = static_cast<uint8_t>(val & 0xff);
}

uint32_t BTConnection::deSerializeUint32(uint8_t const *pBuf) const
{
    return (pBuf[0] << 24) | (pBuf[1] << 16) | (pBuf[2] << 8) | pBuf[3];
}

void BTConnection::setNonBlocking(int socketHandle) const
{
    int flags = fcntl(socketHandle, F_GETFL, 0); 
    fcntl(socketHandle, F_SETFL, flags | O_NONBLOCK);
}

BTConnection::~BTConnection(void)
{
    close(m_socket);
}

};