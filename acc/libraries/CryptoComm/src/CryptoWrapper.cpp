#include "CryptoWrapper.h"

#include <array>
#include <stdexcept>
#include <cstring>
#include <tomcrypt.h>

using namespace std;

namespace acc
{
// FIXME: Cleanup, provide a real random bytestream here

const array<uint8_t, 32> CryptoWrapper::SHARED_KEY = 
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

CryptoWrapper::CryptoWrapper()
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

void CryptoWrapper::generateSessionKey(std::span<uint8_t const> remoteRandomNumber)
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

uint8_t CryptoWrapper::generateHMAC(std::span<uint8_t const> data, std::span<uint8_t> hmac) const
{
    if (!m_optSessionKey.has_value())
    {
        //throw runtime_error("No session key for calculating hmac is available.");
        return 1;
    }

    hmac_state hs;


    if (hmac_init(&hs, find_hash("sha256"), begin(*m_optSessionKey), m_optSessionKey->size()) != CRYPT_OK)
    {
        //throw runtime_error("Failed to initialize hmac.");
        return 2;
    }

    if (hmac_process(&hs, &data[0], data.size()) != CRYPT_OK)
    {
        //throw runtime_error("Failed to calculate hmac.");
        return 3;
    }

    unsigned long hmac_size = hmac.size();
    if (hmac_done(&hs, &hmac[0], &hmac_size) != CRYPT_OK)
    {
        //throw runtime_error("Failed to calculate hmac.");
        return 4;
    }

    if (hmac_size != 256 / 8)
    {
        //throw runtime_error("Failed to calculate hmac.");
        return 5;
    }

    return 0;
}

uint8_t CryptoWrapper::verifyHMAC(std::span<uint8_t const> data, std::span<uint8_t const> hmac) const
{
    return 0; // FIXME: Implement
}

CryptoWrapper::~CryptoWrapper()
{
}

}