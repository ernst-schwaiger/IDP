#include <stdexcept>
#include <cstring>

#include <gio/gio.h>
#include <glib.h>
#include <unistd.h>
#include <sys/socket.h>

#include <tomcrypt.h>
#include "bluetooth.h"

using namespace std;

namespace acc
{

// FIXME: Cleanup, provide a real random bytestream here

const array<uint8_t, 32> BTConnectionCryptoWrapper::SHARED_KEY = 
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

BTConnectionCryptoWrapper::BTConnectionCryptoWrapper()
{
    crypt_mp_init("LibTomMath");
    if (register_hash(&sha256_desc) != CRYPT_OK)
    {
        throw runtime_error("Could not get libtom sha256.");
    }

    if (register_prng(&sprng_desc) != CRYPT_OK)
    {
        throw runtime_error("Could not get libtom secure random generator.");
    }

    if (register_cipher(&aes_desc) == -1)
    {
        throw runtime_error("Could not register rijndael/aes cipher");
    }

    int wprng = find_prng("sprng");

    if (wprng < 0)
    {
        throw runtime_error("Could not get RNG");
    }

    prng_state prngState;

    if (rng_make_prng(m_localRandomNumber.size() * 8, wprng, &prngState, nullptr) != CRYPT_OK)
    {
        throw runtime_error("Could not get sufficient entropy bits.");
    }

    if (rng_get_bytes(m_localRandomNumber.data(), m_localRandomNumber.size(), nullptr) < m_localRandomNumber.size())
    {
        throw runtime_error("Could not get sufficient random data.");
    }    
}

void BTConnectionCryptoWrapper::generateSessionKey(std::span<uint8_t const> remoteRandomNumber)
{
    // create the common salt: append the larger random number to the smaller one
    std::array<uint8_t, 64> salt;
    if (std::memcmp(begin(m_localRandomNumber), &remoteRandomNumber[0], m_localRandomNumber.size()) < 0)
    {
        std::copy(begin(m_localRandomNumber), end(m_localRandomNumber), begin(salt));
        std::copy(begin(remoteRandomNumber), end(remoteRandomNumber), begin(salt) + 32);
    }
    else
    {
        std::copy(begin(remoteRandomNumber), end(remoteRandomNumber), begin(salt));
        std::copy(begin(m_localRandomNumber), end(m_localRandomNumber), begin(salt)+ 32);
    }

    array<uint8_t, 32> sessionKey;

    if (hkdf(find_hash("sha256"), begin(salt), salt.size(), nullptr, 0, begin(SHARED_KEY), SHARED_KEY.size(), begin(sessionKey), sessionKey.size()) != CRYPT_OK)
    {
        throw runtime_error("Could not calculate common key");
    }

    m_optSessionKey = sessionKey;
}

void BTConnectionCryptoWrapper::generateHMAC(std::span<uint8_t const> data, std::span<uint8_t> hmac) const
{
    if (!m_optSessionKey.has_value())
    {
        throw runtime_error("No session key for calculating hmac is available.");
    }

    hmac_state hs;


    if (hmac_init(&hs, find_hash("sha256"), begin(*m_optSessionKey), m_optSessionKey->size()) != CRYPT_OK)
    {
        throw runtime_error("Failed to initialize hmac.");
    }

    if (hmac_process(&hs, &data[0], data.size()) != CRYPT_OK)
    {
        throw runtime_error("Failed to calculate hmac.");
    }

    unsigned long hmac_size = hmac.size();
    if (hmac_done(&hs, &hmac[0], &hmac_size) != CRYPT_OK)
    {
        throw runtime_error("Failed to calculate hmac.");
    }

    if (hmac_size != 256 / 8)
    {
        throw runtime_error("Failed to calculate hmac.");
    }
}

BTConnectionCryptoWrapper::~BTConnectionCryptoWrapper()
{
}

BTListenSocket::BTListenSocket() : 
    m_local_addr{ AF_BLUETOOTH, htobs(0x1001), { 0 }, 0, 0 }
{
    makeBTDeviceVisible();
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

// This is the same as "bluetoothctrl discoverable yes"
void BTListenSocket::makeBTDeviceVisible()
{
    GError *error = nullptr;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if (error != nullptr)
    {
        throw std::runtime_error(error->message);
    }

    GDBusProxy *adapter_proxy = g_dbus_proxy_new_sync(
        connection,
        G_DBUS_PROXY_FLAGS_NONE,
        NULL,
        "org.bluez",
        "/org/bluez/hci0",
        "org.bluez.Adapter1",
        NULL,
    &error);

    if (error != nullptr)
    {
        throw std::runtime_error(error->message);
    }

    GVariant *value = g_variant_new_boolean(TRUE);

    g_dbus_proxy_set_cached_property(adapter_proxy, "Discoverable", value);

    g_dbus_connection_call_sync(
        connection,
        "org.bluez",                        // Bus name
        "/org/bluez/hci0",                 // Object path
        "org.freedesktop.DBus.Properties", // Interface
        "Set",                              // Method
        g_variant_new("(ssv)", 
            "org.bluez.Adapter1",           // Interface name
            "Discoverable",                 // Property name
            g_variant_new_boolean(TRUE)),   // Value
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        &error);    

    if (error != nullptr)
    {
        throw std::runtime_error(error->message);
    }
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

} // namespace acc


int foo()
{
    return 42;
}