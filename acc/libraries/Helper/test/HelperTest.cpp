#include <Helper.h>
#include <unistd.h>
#include <catch2/catch_test_macros.hpp>

using namespace std;

static constexpr uint16_t ELAPSED_MS = 10;

TEST_CASE( "timestamp creation works", "helper" )
{
    uint64_t baseline  = acc::getTimestampMs();
    usleep(ELAPSED_MS * 1000);
    uint32_t elapsedMS = acc::getTimestampMsSinceBaseline(baseline);
    REQUIRE(elapsedMS >= ELAPSED_MS); // we should get the elapsed ms here
}
