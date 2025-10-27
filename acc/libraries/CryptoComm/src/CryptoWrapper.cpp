#include "CryptoWrapper.h"

#include <array>
#include <stdexcept>
#include <cstring>
#include <tomcrypt.h>

using namespace std;

namespace acc
{
// FIXME: Cleanup, provide a real random bytestream here

const array<uint8_t, KEY_LEN_BYTES> CryptoWrapper::PRE_SHARED_KEY = 
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

CryptoWrapper::CryptoWrapper(void)
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

    int wprng = find_prng("sprng"); // Deviation Dir 4.6: type used in external LibTomCrypt API

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

void CryptoWrapper::generateSessionKey(std::span<uint8_t const> remoteRandomNumber)
{
    // create the common salt: append the larger random number to the smaller one
    std::array<uint8_t, RND_LEN_BYTES * 2U> salt;
    if (std::memcmp(begin(m_localRandomNumber), &remoteRandomNumber[0], m_localRandomNumber.size()) < 0)
    {
        std::copy(begin(m_localRandomNumber), end(m_localRandomNumber), begin(salt));
        std::copy(begin(remoteRandomNumber), end(remoteRandomNumber), begin(salt) + RND_LEN_BYTES);
    }
    else
    {
        std::copy(begin(remoteRandomNumber), end(remoteRandomNumber), begin(salt));
        std::copy(begin(m_localRandomNumber), end(m_localRandomNumber), begin(salt) + RND_LEN_BYTES);
    }

    array<uint8_t, KEY_LEN_BYTES> sessionKey;

    if (hkdf(find_hash("sha256"), begin(salt), salt.size(), nullptr, 0, begin(PRE_SHARED_KEY), PRE_SHARED_KEY.size(), begin(sessionKey), sessionKey.size()) != CRYPT_OK)
    {
        throw runtime_error("Could not calculate common key");
    }

    m_optSessionKey = sessionKey;
}

uint8_t CryptoWrapper::generateHMAC(std::span<uint8_t const> data, std::span<uint8_t> hmac) const
{
    if (!m_optSessionKey.has_value())
    {
        return 1U;
    }

    hmac_state hs;

    if (hmac_init(&hs, find_hash("sha256"), begin(*m_optSessionKey), m_optSessionKey->size()) != CRYPT_OK)
    {
        return 2U;
    }

    if (hmac_process(&hs, &data[0], data.size()) != CRYPT_OK)
    {
        return 3U;
    }

    unsigned long hmac_size = hmac.size(); // Deviation Dir 4.6: type used in external TomCrypt library
    if (hmac_done(&hs, &hmac[0], &hmac_size) != CRYPT_OK)
    {
        return 4U;
    }

    if (hmac_size != HMAC_LEN_BYTES)
    {
        return 5U;
    }

    return 0;
}

uint8_t CryptoWrapper::verifyHMAC(std::span<uint8_t const> data, std::span<uint8_t const> hmac) const
{
    array<uint8_t, HMAC_LEN_BYTES> genHMAC;
    if (generateHMAC(data, genHMAC) != 0)
    {
        return 7;
    }

    if (hmac.size() != HMAC_LEN_BYTES)
    {
        return 5U;
    }

    if (memcmp(&genHMAC[0], &hmac[0], genHMAC.size()) != 0)
    {
        return 6U;
    }

    return 0U;
}
}