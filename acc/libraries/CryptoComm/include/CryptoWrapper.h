#pragma once

#include <span>
#include <array>
#include <optional>
#include <cstdint>

namespace acc
{
// Length of crypto fields according to chapter "Communication" in design document
static constexpr uint32_t KEY_LEN_BYTES = 32U;
static constexpr uint32_t RND_LEN_BYTES = 32U;
static constexpr uint32_t HMAC_LEN_BYTES = 32U;

// Wraps the underlying cryptographic library/LibTom
// Requirements implemented: Sec-REQ-1, Sec-REQ-2
class CryptoWrapper
{
public:
    // Initialize crypto library, registers required crypto functions, calculates local random number
    CryptoWrapper(void);
    // Cleanup crypto library
    ~CryptoWrapper(void) {};

    // create a new session key using own and remote random number
    void generateSessionKey(std::span<uint8_t const> remoteRandomNumber);

    // generate an HMAC using the passed data and the AES session key, returns 0 on success, other value on failure
    [[ nodiscard ]] uint8_t generateHMAC(std::span<uint8_t const> data, std::span<uint8_t> hmac) const;
    // generate an HMAC using the passed data and the AES session key, returns 0 on success, other value on failure
    [[ nodiscard ]] uint8_t verifyHMAC(std::span<uint8_t const> data, std::span<uint8_t const> hmac) const;

    // Returns the locally generated random number
    [[ nodiscard ]] std::array<uint8_t, RND_LEN_BYTES> const &getLocalRandomNumber() const { return m_localRandomNumber; }
    // Returns the session key if it has been generated, otherwise an empty optional
    [[ nodiscard ]] std::optional<std::array<uint8_t, KEY_LEN_BYTES>> const &getSessionKey() const { return m_optSessionKey; }

private:
    // our top-secret pre-shared key
    static const std::array<uint8_t,KEY_LEN_BYTES> PRE_SHARED_KEY;
    // locally generated random number
    std::array<uint8_t, RND_LEN_BYTES> m_localRandomNumber;
    // shared session key, will only be valid after generateSessionKey() was called successfully
    std::optional<std::array<uint8_t, KEY_LEN_BYTES>> m_optSessionKey;
};

}