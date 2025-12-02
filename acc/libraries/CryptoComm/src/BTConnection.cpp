#include "BTConnection.h"
#include "BTRuntimeError.h"

#include <stdexcept>
#include <span>
#include <array>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

using namespace std;

namespace acc
{

// Server side constructor, sets up the connection on the server side/node 1
BTConnection::BTConnection(BTListenSocket const *pListenSocket) : 
    m_cryptoWrapper{},
    m_remote_addr{ AF_BLUETOOTH, htobs(0x1001), { 0 }, 0, 0 }
{
    // Wait for the client/node 2 to connect (blocking)
    socklen_t socklen = sizeof(m_remote_addr);
    m_socket = ::accept(pListenSocket->getListenSocket(), reinterpret_cast<struct sockaddr *>(&m_remote_addr), &socklen);
    if (m_socket < 0)
    {
        //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, the ACC shall initiate a connection.         
        throw BTRuntimeError("failed to open bluetooth client socket");
    }
    
    // Continue transmission in non-blocking mode (server-side)
    setNonBlockingAndPoll(m_socket, false);
}

// Client side constructor, sets up the connection on client side/node 2
BTConnection::BTConnection(char const *remoteMAC) : // Deviation Dir 4.6: type passed via main() function
    m_cryptoWrapper{},
    m_remote_addr{ AF_BLUETOOTH, htobs(0x1001), { 0 }, 0, 0 }
{
    // Try to connect to server
    m_socket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
    ::str2ba( remoteMAC, &m_remote_addr.l2_bdaddr );
    if (::connect(m_socket, reinterpret_cast<struct sockaddr *>(&m_remote_addr), sizeof(m_remote_addr)) < 0)
    {
        // Connection failed, throw BTRuntimeError to re-initiate a connection setup
        close(m_socket);
        //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, the ACC shall initiate a connection.  
        throw BTRuntimeError("failed to open bluetooth server socket");
    }

    // Continue transmission in non-blocking mode (client-side)
    setNonBlockingAndPoll(m_socket, true);
}

// Generates a true random number, transmits it to the remote peer
ssize_t BTConnection::sendLocalRandom(void) noexcept
{
    array<uint8_t, MAX_RND_MSG_LEN> msgPayload; // buffer for random message
    array<uint8_t, RND_LEN> const &localRandom = m_cryptoWrapper.getLocalRandomNumber(); // fetch random number
    msgPayload[0] = 0U; // Message Type: Random message
    // append payload
    copy(begin(localRandom), end(localRandom), &msgPayload[1]);
    // transmit to remote peer
    return send(msgPayload);
}

// Initiates key exchange on client side, returns true on success
bool BTConnection::keyExchangeClient(void)
{
    bool ret = true;
    // Determine session key
    if (sendLocalRandom() <= 0)
    {
        //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, the ACC shall initiate a connection. 
        throw BTRuntimeError("Failed to send random message.");
    }

    usleep(1'000'000U); // give peer time to react
    
    // Fetch random number from peer
    array<uint8_t, MAX_RND_MSG_LEN> remoteKeyMsg;
    ssize_t remoteRandom = receive(remoteKeyMsg);

    // key exchange message response from server did not arrive yet, ->keep connection open, resend new rnd
    if ((remoteRandom == 0U) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
    {
        ret = false;
    }
    else
    {
        // incorrect payload received, reset, restart connection
        if ((remoteRandom != MAX_RND_MSG_LEN) || (remoteKeyMsg[0] != 0U))
        {
            //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, the ACC shall initiate a connection.
            throw BTRuntimeError("Received incorrect random message from server");
        }
        // Successful reception, calculate common AES session key
        //Sec-REQ-1: The ACC shall regenerate a new session key after a new Bluetooth connection has been set 
        m_cryptoWrapper.generateSessionKey({ &remoteKeyMsg[1], remoteKeyMsg.size() - 1});
    }

    return ret;
}

// Initiates key exchange on server side, returns true on success
bool BTConnection::keyExchangeServer(void)
{
    bool ret = true;

    // Receive remote random number message
    array<uint8_t, MAX_RND_MSG_LEN> remoteKeyMsg;
    ssize_t remoteRandom = receive(remoteKeyMsg);

    // nothing received yet, keep session open
    if ((remoteRandom == 0U) && ((errno == EAGAIN) || (errno == EWOULDBLOCK)))
    {
        ret = false;
    }
    else
    {
        // received incorrect random message, reset connection
        if ((remoteRandom != MAX_RND_MSG_LEN) || (remoteKeyMsg[0] != 0U))
        {
            //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, the ACC shall initiate a connection.
            throw BTRuntimeError("Received incorrect random message from server");
        }

        // send own random number
        if (sendLocalRandom() <= 0)
        {
            //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, the ACC shall initiate a connection.
            throw BTRuntimeError("Failed to send random message.");
        }

        // generate session key. If something went wrong at the peer it will close the BT session
        //Sec-REQ-1: The ACC shall regenerate a new session key after a new Bluetooth connection has been set
        m_cryptoWrapper.generateSessionKey({ &remoteKeyMsg[1], remoteKeyMsg.size() - 1});
    }

    // Give the peer time to calculate the session key
    usleep(500'000U);

    return ret;
}

// sends txData payload, returns the number of sent bytes, or a negative value on failure
ssize_t BTConnection::send(std::span<const uint8_t> txData) noexcept
{
    return write(m_socket, &txData[0], txData.size());
}

// receives payload, returns the number of received bytes, or a negative value on failure, payload is passed to rxData
ssize_t BTConnection::receive(std::span<uint8_t> rxData) noexcept
{
    ssize_t ret = read(m_socket, &rxData[0], rxData.size());
    return ret;
}

// Sends passed payload with counter value, together with matching MAC, returns number of sent bytes, or a negative value on failure
ssize_t BTConnection::sendWithCounterAndMAC(uint8_t msgType, std::span<const uint8_t> txData, uint32_t counter)
{
    // Incorrect number of bytes received
    if (txData.size() > MAX_PAYLOAD_LEN)
    {
        return -1;
    }

    // populate message bytes with msg type, counter fields, actual payload, followed by HMAC
    array<uint8_t, MAX_MSG_LEN> msgBuf;
    msgBuf[0] = msgType;
    //Sec-REQ-3: The ACC shall prevent replay attacks with a unique timestamp field. 
    serializeUint32(&msgBuf[1], counter);
    std::copy(&txData[0], &txData[txData.size()], &msgBuf[5]);
    uint32_t protectedByteByHMAC = TYPE_LEN + COUNTER_LEN + txData.size();
    //Sec-REQ-2: The ACC shall protect the integrity of the messages using an HMAC. 
    if (m_cryptoWrapper.generateHMAC({&msgBuf[0], protectedByteByHMAC}, {&msgBuf[protectedByteByHMAC], MAC_LEN}) != 0)
    {
        return -1;
    }

    // send message
    ssize_t sentBytes = send({&msgBuf[0], protectedByteByHMAC + MAC_LEN});
    if (sentBytes < 0)
    {
        // Throwing leaves the comm loop and will set up a new connection
        //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, the ACC shall initiate a connection.
        throw BTRuntimeError("Failed to send data");
    }

    return sentBytes;
}

// Receives passed payload, verifies HMAC, extracts msgType, payload and counter fields from it, returns the payload field length or a negative value on failure 
ssize_t BTConnection::receiveWithCounterAndMAC(uint8_t &msgType, std::span<uint8_t> payload, uint32_t &counter, bool verifyCounter)
{
    array<uint8_t, MAX_MSG_LEN> rxBuf;
    ssize_t rxMsgLen = receive(rxBuf);

    // error occurred during reception, bail out, set up a new connection
    if (rxMsgLen == 0)
    {
        if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
        {
            //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, the ACC shall initiate a connection.
            throw BTRuntimeError("Failed to receive data");
        }
    }

    // message too short to contain message fields and payload
    uint32_t overheadLength = TYPE_LEN + COUNTER_LEN + MAC_LEN;
    if (rxMsgLen <= overheadLength)
    {
        return -1;
    }

    ssize_t payloadLength = rxMsgLen - overheadLength;
    if (payload.size() < static_cast<size_t>(payloadLength))
    {
        return -1; // Cannot copy full payload into payload span
    }

    // extract fields
    //Sec-REQ-3: The ACC shall prevent replay attacks with a unique timestamp field. 
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

    // verify correct HMAC, avoid counterfeit messages
    //Sec-REQ-2: The ACC shall protect the integrity of the messages using an HMAC.
    if (m_cryptoWrapper.verifyHMAC({&rxBuf[0], static_cast<uint32_t>(rxMsgLen) - MAC_LEN}, 
        {&rxBuf[rxMsgLen - MAC_LEN], MAC_LEN}) != 0U)
    {
        return -1;
    }

    // everything is OK, extract payload and message type
    std::copy(&rxBuf[TYPE_LEN + COUNTER_LEN], &rxBuf[TYPE_LEN + COUNTER_LEN + payloadLength], &payload[0]);
    msgType = rxBuf[0];
    
    return payloadLength;
}

// serializes a uint32 into big endian byte array
void BTConnection::serializeUint32(uint8_t *pBuf, uint32_t val)
{
    pBuf[0] = static_cast<uint8_t>((val >> 24U) & 0xff);
    pBuf[1] = static_cast<uint8_t>((val >> 16U) & 0xff);
    pBuf[2] = static_cast<uint8_t>((val >> 8U) & 0xff);
    pBuf[3] = static_cast<uint8_t>(val & 0xff);
}

// deserializes a uint32 from a big endian byte array
uint32_t BTConnection::deSerializeUint32(uint8_t const *pBuf)
{
    return (pBuf[0] << 24U) | (pBuf[1] << 16U) | (pBuf[2] << 8U) | pBuf[3];
}

// Sets the socket API mode to non-blocking for client and server
void BTConnection::setNonBlockingAndPoll(int socketHandle, bool isClient) const
{
    int flags = fcntl(socketHandle, F_GETFL, 0); // Deviation Dir 4.6: type used in external POSIX socket API
    if (fcntl(socketHandle, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, the ACC shall initiate a connection.
        throw BTRuntimeError("Failed to configure socket as nonblocking");
    }

    // We poll for readiness events fpr receiving and sending
    struct pollfd pfd = { m_socket, POLLIN | POLLOUT, 0 };

    while(1)
    {
        // Wait for readiness events, at most one sec
        if (poll(&pfd, 1U, 1'000U) <= 0)
        {
            //Saf-REQ-8: If a failure of the communication subsystem (Bluetooth) is detected, the ACC shall initiate a connection.
            throw BTRuntimeError("failed to poll bluetooth server socket");
        }
        else
        {
            // Clients only poll for tx readiness as they send the "Random Request" message
            // Servers wait for rx and tx readiness as well as they receive the "Random Request" message
            if ((pfd.revents & POLLOUT) && (isClient || (pfd.revents & POLLIN)))
            {
                break;
            }
        }
    }    
}

// destructor, cleanup allocated resources
BTConnection::~BTConnection(void)
{
    close(m_socket);
}

}
