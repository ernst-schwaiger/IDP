#include <CryptoWrapper.h>
#include <catch2/catch_test_macros.hpp>

#include <array>

using namespace std;

TEST_CASE( "generation of session key works", "crypto" )
{
    acc::CryptoWrapper wrapper1;
    acc::CryptoWrapper wrapper2;

    REQUIRE( wrapper1.getSessionKey().has_value() == false );
    REQUIRE( wrapper2.getSessionKey().has_value() == false );

    wrapper1.generateSessionKey(wrapper2.getLocalRandomNumber());
    wrapper2.generateSessionKey(wrapper1.getLocalRandomNumber());

    REQUIRE( wrapper1.getSessionKey().has_value());
    REQUIRE( wrapper2.getSessionKey().has_value());

    REQUIRE(wrapper1.getSessionKey() == wrapper2.getSessionKey());
}

TEST_CASE( "verification of HMAC works", "crypto" )
{
    static char const *MSG = "this is a test message";
    static char const *MSG2 = "this is a counterfeit test message";
    array<uint8_t, 32> HMAC;

    acc::CryptoWrapper wrapper1;
    acc::CryptoWrapper wrapper2;

    wrapper1.generateSessionKey(wrapper2.getLocalRandomNumber());
    wrapper2.generateSessionKey(wrapper1.getLocalRandomNumber());

    REQUIRE(wrapper1.generateHMAC({ reinterpret_cast<uint8_t const *>(MSG), strlen(MSG) }, HMAC) == 0);
    REQUIRE(wrapper2.verifyHMAC({ reinterpret_cast<uint8_t const *>(MSG), strlen(MSG) }, HMAC) == 0);
    REQUIRE(wrapper2.verifyHMAC({ reinterpret_cast<uint8_t const *>(MSG2), strlen(MSG) }, HMAC) > 0);
}
