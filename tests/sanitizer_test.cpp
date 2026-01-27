#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <string>

// libc++ debug hardening: out-of-bounds vector access
TEST_CASE("libcxx hardening: vector out of bounds", "[hardened libc++]") {
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    int x = v[5]; // out of bounds
    CHECK(x != 0);
}

// UBSan: signed integer overflow
TEST_CASE("ubsan: signed integer overflow", "[ubsan]") {
    int x = 2147483647; // INT_MAX
    int y = x + 1;      // undefined behavior
    CHECK(y != 0);
}

// ASan: heap use-after-free
TEST_CASE("asan: heap use after free", "[asan]") {
    int* p = new int(42);
    delete p;
    int x = *p; // use after free
    CHECK(x != 0);
}
