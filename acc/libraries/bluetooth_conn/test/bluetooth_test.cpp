#include <bluetooth.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE( "foo() works correctly", "[foo_category]" )
{
    REQUIRE( foo() == 42 );
}
