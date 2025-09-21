#pragma once

#include <span>
#include <array>
#include <optional>
#include <cstdint>

namespace acc
{
class CryptoWrapper
{
public:
    CryptoWrapper();
    ~CryptoWrapper();

    void generateSessionKey(std::span<uint8_t const> remoteRandomNumber);

    uint8_t generateHMAC(std::span<uint8_t const> data, std::span<uint8_t> hmac) const;
    uint8_t verifyHMAC(std::span<uint8_t const> data, std::span<uint8_t const> hmac) const;

    std::array<uint8_t, 32> const &getLocalRandomNumber() const { return m_localRandomNumber; }
    std::optional<std::array<uint8_t, 32>> const &getSessionKey() const { return m_optSessionKey; }

private:
    static const std::array<uint8_t,32> PRE_SHARED_KEY;
    std::array<uint8_t, 32> m_localRandomNumber;
    std::optional<std::array<uint8_t, 32>> m_optSessionKey;
};

}