#pragma once

#include "BTListenSocket.h"
#include "CryptoWrapper.h"
#include <span>
#include <cstdint>

namespace acc
{
    constexpr uint32_t TYPE_LEN = 1U;
    constexpr uint32_t COUNTER_LEN = 4U;
    constexpr uint32_t MAX_PAYLOAD_LEN = 27U;
    constexpr uint32_t MAC_LEN = 32U;
    constexpr uint32_t MAX_MSG_LEN = TYPE_LEN + COUNTER_LEN + MAX_PAYLOAD_LEN + MAC_LEN;

class BTConnection
{
public:
    // Server Constructor
    BTConnection(BTListenSocket *pListenSocket);
    // Client constructor
    BTConnection(char const *remoteMAC);
    ~BTConnection();

    void keyExchangeClient();
    void keyExchangeServer();

    ssize_t sendLocalRandom() noexcept;

    ssize_t send(std::span<const uint8_t> txData) noexcept;
    ssize_t receive(std::span<uint8_t> rxData) noexcept;

    ssize_t sendWithCounterAndMAC(uint8_t msgType, std::span<const uint8_t> txData) noexcept;
    ssize_t receiveWithCounterAndMAC(uint8_t &msgType, std::span<uint8_t> payload) noexcept;

private:

    void serializeUint32(uint8_t *pBuf, uint32_t val) const;
    uint32_t deSerializeUint32(uint8_t const *pBuf) const;

    CryptoWrapper m_cryptoWrapper;
    uint32_t m_localCounter;
    uint32_t m_remoteCounter;
    int m_socket;
    struct sockaddr_l2 m_remote_addr;
};

} // namespace acc
