#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <cstdint>

#include <span>
#include <array>
#include <optional>

namespace acc
{

class BTListenSocket
{
public:
    BTListenSocket();
    ~BTListenSocket();

    int getListenSocket() const
    {
        return m_listenSocket;
    }

private:

    void makeBTDeviceVisible();
    
    int m_listenSocket;
    struct sockaddr_l2 m_local_addr;
};

class BTConnectionCryptoWrapper
{
public:
    BTConnectionCryptoWrapper();
    ~BTConnectionCryptoWrapper();

    void generateSessionKey(std::span<uint8_t const> remoteRandomNumber);
    std::array<uint8_t, 32> const &getLocalRandomNumber() const { return m_localRandomNumber; }
    std::optional<std::array<uint8_t, 32>> const &getSessionKey() const { return m_optSessionKey; }

private:
    static const std::array<uint8_t,32> SHARED_KEY;
    std::array<uint8_t, 32> m_localRandomNumber;
    std::optional<std::array<uint8_t, 32>> m_optSessionKey;
};

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

    BTConnectionCryptoWrapper m_cryptoWrapper;
    int m_socket;
    struct sockaddr_l2 m_remote_addr;
};

} // namespace acc

int foo();

#endif
