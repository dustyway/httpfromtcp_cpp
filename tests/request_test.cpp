#include <catch2/catch_test_macros.hpp>
#include "Request.hpp"

TEST_CASE("Good GET request to root path", "[request]") {
    std::string data = "GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    Request* r = Request::parse(data, &errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->method == "GET");
    CHECK(r->requestTarget == "/");
    CHECK(r->httpVersion == "1.1");

    delete r;
}


TEST_CASE("Good GET request with path", "[request]") {
    std::string data = "GET /coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    Request* r = Request::parse(data, &errorMsg);

    REQUIRE(r != NULL);
    CHECK(r->method == "GET");
    CHECK(r->requestTarget == "/coffee");
    CHECK(r->httpVersion == "1.1");

    delete r;
}

TEST_CASE("Invalid request line - missing method", "[request]") {
    std::string data = "/coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    Request* r = Request::parse(data, &errorMsg);

    REQUIRE(r == NULL);
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

