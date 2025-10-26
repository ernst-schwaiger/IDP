#pragma once

#include <span>
#include <array>
#include <optional>
#include <cstdint>

namespace acc
{
    static constexpr uint32_t KEY_LEN_BYTES = 32U;
    static constexpr uint32_t RND_LEN_BYTES = 32U;
    static constexpr uint32_t HMAC_LEN_BYTES = 32U;

class CryptoWrapper
{
public:
    CryptoWrapper(void);
    ~CryptoWrapper(void) {};

    void generateSessionKey(std::span<uint8_t const> remoteRandomNumber);

    [[ nodiscard ]] uint8_t generateHMAC(std::span<uint8_t const> data, std::span<uint8_t> hmac) const;
    [[ nodiscard ]] uint8_t verifyHMAC(std::span<uint8_t const> data, std::span<uint8_t const> hmac) const;

    [[ nodiscard ]] std::array<uint8_t, RND_LEN_BYTES> const &getLocalRandomNumber() const { return m_localRandomNumber; }
    [[ nodiscard ]] std::optional<std::array<uint8_t, KEY_LEN_BYTES>> const &getSessionKey() const { return m_optSessionKey; }

private:
    static const std::array<uint8_t,KEY_LEN_BYTES> PRE_SHARED_KEY;
    std::array<uint8_t, RND_LEN_BYTES> m_localRandomNumber;
    std::optional<std::array<uint8_t, KEY_LEN_BYTES>> m_optSessionKey;
};

}