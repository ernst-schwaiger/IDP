#include "BTConnection.h"

#include <stdexcept>
#include <span>
#include <array>
#include <unistd.h>

using namespace std;

namespace acc
{
BTConnection::BTConnection(BTListenSocket *pListenSocket) : 
    m_cryptoWrapper{},
    m_localCounter { 0 },
    m_remoteCounter { 0 },
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
    m_cryptoWrapper{},
    m_localCounter { 0 },
    m_remoteCounter { 0 },
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
    ssize_t ret = read(m_socket, &rxData[0], rxData.size());
    return ret;
}

ssize_t BTConnection::sendWithCounterAndMAC(uint8_t msgType, std::span<const uint8_t> txData)
{
    if (txData.size() > MAX_PAYLOAD_LEN)
    {
        return -1;
    }

    array<uint8_t, MAX_MSG_LEN> msgBuf;
    msgBuf[0] = msgType;
    serializeUint32(&msgBuf[1], m_localCounter);
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

    m_localCounter++;
    return sentBytes;
}

ssize_t BTConnection::receiveWithCounterAndMAC(uint8_t &msgType, std::span<uint8_t> payload)
{
    uint32_t overheadLength = TYPE_LEN + COUNTER_LEN + MAC_LEN;
    array<uint8_t, MAX_MSG_LEN> rxBuf;
    ssize_t rxMsgLen = receive(rxBuf);

    if (rxMsgLen < 0)
    {
        throw runtime_error ("Failed to receive data");
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
    // accept higher counters, but not lower ones, prevents replay attacks
    if (m_remoteCounter > remoteCounter)
    {
        return -1; // unexpected message counter.
    }

    m_remoteCounter = remoteCounter + 1;

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

BTConnection::~BTConnection()
{
    close(m_socket);
}

};