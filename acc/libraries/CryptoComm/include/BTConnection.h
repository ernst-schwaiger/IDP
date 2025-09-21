#pragma once

#include "BTListenSocket.h"
#include "CryptoWrapper.h"
#include <span>
#include <cstdint>

namespace acc
{
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

private:

    CryptoWrapper m_cryptoWrapper;
    int m_socket;
    struct sockaddr_l2 m_remote_addr;
};

} // namespace acc
