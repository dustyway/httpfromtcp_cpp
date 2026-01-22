#include <catch2/catch_test_macros.hpp>

TEST_CASE("RequestLineParse", "[request]") {
    std::string expected = "TheTestagen";
    std::string actual = "TheTestagen";
    REQUIRE(actual == expected);
}