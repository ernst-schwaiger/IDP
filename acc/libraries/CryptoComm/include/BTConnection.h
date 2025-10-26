#pragma once

#include "BTListenSocket.h"
#include "CryptoWrapper.h"
#include <span>
#include <cstdint>
#include <stdexcept>

namespace acc
{
    constexpr uint32_t TYPE_LEN = 1U;
    constexpr uint32_t COUNTER_LEN = 4U;
    constexpr uint32_t MAX_PAYLOAD_LEN = 27U;
    constexpr uint32_t MAC_LEN = 32U;
    constexpr uint32_t MAX_MSG_LEN = TYPE_LEN + COUNTER_LEN + MAX_PAYLOAD_LEN + MAC_LEN;
    constexpr uint32_t RND_LEN = 32U;
    constexpr uint32_t MAX_RND_MSG_LEN = TYPE_LEN + RND_LEN;

class BTConnection
{
public:
    // Server Constructor
    explicit BTConnection(BTListenSocket const *pListenSocket);
    // Client constructor
    explicit BTConnection(char const *remoteMAC);
    ~BTConnection(void);

    [[ nodiscard ]] bool keyExchangeClient(void);
    [[ nodiscard ]] bool keyExchangeServer(void);

    [[ nodiscard ]] ssize_t sendLocalRandom(void) noexcept;

    [[ nodiscard ]] ssize_t send(std::span<const uint8_t> txData) noexcept;
    [[ nodiscard ]] ssize_t receive(std::span<uint8_t> rxData) noexcept;

    [[ nodiscard ]] ssize_t sendWithCounterAndMAC(uint8_t msgType, std::span<const uint8_t> txData, uint32_t counter);
    [[ nodiscard ]] ssize_t receiveWithCounterAndMAC(uint8_t &msgType, std::span<uint8_t> payload, uint32_t &counter, bool verifyCounter);

private:

    static void serializeUint32(uint8_t *pBuf, uint32_t val);
    [[ nodiscard ]] static uint32_t deSerializeUint32(uint8_t const *pBuf);
    void setNonBlockingAndPoll(int socketHandle, bool isClient) const;

    CryptoWrapper m_cryptoWrapper;
    int m_socket;
    struct sockaddr_l2 m_remote_addr;
};

} // namespace acc
