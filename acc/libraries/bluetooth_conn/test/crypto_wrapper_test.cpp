#include <bluetooth.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE( "generation of session key works", "crypto" )
{
    acc::BTConnectionCryptoWrapper wrapper1;
    acc::BTConnectionCryptoWrapper wrapper2;

    REQUIRE( wrapper1.getSessionKey().has_value() == false );
    REQUIRE( wrapper2.getSessionKey().has_value() == false );

    wrapper1.generateSessionKey(wrapper2.getLocalRandomNumber());
    wrapper2.generateSessionKey(wrapper1.getLocalRandomNumber());

    REQUIRE( wrapper1.getSessionKey().has_value());
    REQUIRE( wrapper2.getSessionKey().has_value());

    REQUIRE(wrapper1.getSessionKey() == wrapper2.getSessionKey());
}