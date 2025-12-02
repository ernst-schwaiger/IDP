#pragma once

#include "BTListenSocket.h"
#include "CryptoWrapper.h"
#include <span>
#include <cstdint>
#include <stdexcept>

namespace acc
{

// Properties of exchanged payload according to chapter "Communication" in design document
constexpr uint32_t TYPE_LEN = 1U;           // Length of Type field in payload
constexpr uint32_t COUNTER_LEN = 4U;        // Length of counter field in payload
constexpr uint32_t MAX_PAYLOAD_LEN = 27U;   // Length of the payload in total
constexpr uint32_t MAC_LEN = 32U;           // Lenght of the MAC field protecting the payload
constexpr uint32_t MAX_MSG_LEN = TYPE_LEN + COUNTER_LEN + MAX_PAYLOAD_LEN + MAC_LEN; // Length of payload plus MAC
constexpr uint32_t RND_LEN = 32U;           // Random field length used in key exchange
constexpr uint32_t MAX_RND_MSG_LEN = TYPE_LEN + RND_LEN;    // Length of Random message/key exchange message

// BTConnection sets up a bluetooth connection, provides API for key exchange and transmission of messages protected by a MAC
// Requirements implemented: Sec-REQ-1, Sec-REQ-2, Sec-REQ-3, Saf-REQ-8
class BTConnection
{
public:
    // Server side constructor, sets up the connection on the server side/node 1
    explicit BTConnection(BTListenSocket const *pListenSocket);
    // Client side constructor, sets up the connection on client side/node 2
    explicit BTConnection(char const *remoteMAC); // Deviation Dir 4.6: type passed via main() function
    // Clean up of allocated resources of bluetooth connection
    ~BTConnection(void);

    // Initiates key exchange on client side, returns true on success
    [[ nodiscard ]] bool keyExchangeClient(void);
    // Waits for key exchange on server side, returns true on success
    [[ nodiscard ]] bool keyExchangeServer(void);
    // Generates a true random number, transmits it to the remote peer
    [[ nodiscard ]] ssize_t sendLocalRandom(void) noexcept;
    // Sends passed payload with counter value, together with matching HMAC, returns number of sent bytes, or a negative value on failure
    [[ nodiscard ]] ssize_t sendWithCounterAndMAC(uint8_t msgType, std::span<const uint8_t> txData, uint32_t counter);
    // Receives passed payload, verifies HMAC, extracts msgType, payload and counter fields from it, returns the payload field length or a negative value on failure 
    [[ nodiscard ]] ssize_t receiveWithCounterAndMAC(uint8_t &msgType, std::span<uint8_t> payload, uint32_t &counter, bool verifyCounter);

private:

    // sends txData payload, returns the number of sent bytes, or a negative value on failure
    [[ nodiscard ]] ssize_t send(std::span<const uint8_t> txData) noexcept;
    // receives payload, returns the number of received bytes, or a negative value on failure, payload is passed to rxData
    [[ nodiscard ]] ssize_t receive(std::span<uint8_t> rxData) noexcept;

    // serializes a uint32 into big endian byte array
    static void serializeUint32(uint8_t *pBuf, uint32_t val);
    // deserializes a uint32 from a big endian byte array
    [[ nodiscard ]] static uint32_t deSerializeUint32(uint8_t const *pBuf);
    // Sets the socket API mode to non-blocking for client and server
    void setNonBlockingAndPoll(int socketHandle, bool isClient) const;

    // CryptoWrapper, holds the session key, provides required crypto API
    CryptoWrapper m_cryptoWrapper;
    int m_socket; // POSIX socket handle. Deviation Dir 4.6: type used in external socket API
    struct sockaddr_l2 m_remote_addr; // remote BT MAC address
};

} // namespace acc
