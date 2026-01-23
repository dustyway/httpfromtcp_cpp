#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include "Request.hpp"

// Tests matching Go implementation (httpfromtcp2)

TEST_CASE("Good GET Request line", "[request]") {
    std::string data = "GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    REQUIRE(r.done());
    REQUIRE_FALSE(r.error());
    CHECK(r.requestLine.method == "GET");
    CHECK(r.requestLine.requestTarget == "/");
    CHECK(r.requestLine.httpVersion == "1.1");
}

TEST_CASE("Good GET Request line with path", "[request]") {
    std::string data = "GET /coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    REQUIRE(r.done());
    REQUIRE_FALSE(r.error());
    CHECK(r.requestLine.method == "GET");
    CHECK(r.requestLine.requestTarget == "/coffee");
    CHECK(r.requestLine.httpVersion == "1.1");
}

TEST_CASE("Invalid number of parts in request line", "[request]") {
    std::string data = "/coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

// Chunked parsing tests

TEST_CASE("Chunked parsing - data arrives in pieces", "[request]") {
    Request r;
    std::string errorMsg;

    // First chunk - incomplete
    int n1 = r.parse("GET /cof", &errorMsg);
    REQUIRE(n1 == 0);
    REQUIRE_FALSE(r.done());

    // Second chunk - still incomplete
    int n2 = r.parse("GET /coffee HTTP/1.1\r", &errorMsg);
    REQUIRE(n2 == 0);
    REQUIRE_FALSE(r.done());

    // Third chunk - complete
    int n3 = r.parse("GET /coffee HTTP/1.1\r\nHost: localhost\r\n\r\n", &errorMsg);
    REQUIRE(n3 > 0);
    REQUIRE(r.done());
    REQUIRE_FALSE(r.error());
    CHECK(r.requestLine.method == "GET");
    CHECK(r.requestLine.requestTarget == "/coffee");
}

TEST_CASE("Parsing after error returns error", "[request]") {
    Request r;
    std::string errorMsg;

    // Cause an error
    int n1 = r.parse("INVALID\r\n", &errorMsg);
    REQUIRE(n1 == -1);
    REQUIRE(r.error());

    // Subsequent parse should also error
    errorMsg.clear();
    int n2 = r.parse("GET / HTTP/1.1\r\n", &errorMsg);
    REQUIRE(n2 == -1);
    CHECK(errorMsg == Request::ERROR_REQUEST_IN_ERROR_STATE);
}

// Additional HTTP method tests

TEST_CASE("Good POST Request with path", "[request]") {
    std::string data = "POST /submit HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    REQUIRE(r.done());
    CHECK(r.requestLine.method == "POST");
    CHECK(r.requestLine.requestTarget == "/submit");
    CHECK(r.requestLine.httpVersion == "1.1");
}

TEST_CASE("Good PUT request", "[request]") {
    std::string data = "PUT /resource HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    CHECK(r.requestLine.method == "PUT");
    CHECK(r.requestLine.requestTarget == "/resource");
    CHECK(r.requestLine.httpVersion == "1.1");
}

TEST_CASE("Good DELETE request", "[request]") {
    std::string data = "DELETE /resource/123 HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    CHECK(r.requestLine.method == "DELETE");
    CHECK(r.requestLine.requestTarget == "/resource/123");
    CHECK(r.requestLine.httpVersion == "1.1");
}

TEST_CASE("HEAD request", "[request]") {
    std::string data = "HEAD / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    CHECK(r.requestLine.method == "HEAD");
}

TEST_CASE("OPTIONS request with asterisk", "[request]") {
    std::string data = "OPTIONS * HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    CHECK(r.requestLine.method == "OPTIONS");
    CHECK(r.requestLine.requestTarget == "*");
}

TEST_CASE("PATCH request", "[request]") {
    std::string data = "PATCH /resource HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    CHECK(r.requestLine.method == "PATCH");
}

// Request target edge cases

TEST_CASE("Request with query string", "[request]") {
    std::string data = "GET /search?q=hello&page=1 HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    CHECK(r.requestLine.method == "GET");
    CHECK(r.requestLine.requestTarget == "/search?q=hello&page=1");
    CHECK(r.requestLine.httpVersion == "1.1");
}

TEST_CASE("Request with fragment", "[request]") {
    std::string data = "GET /page#section HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    CHECK(r.requestLine.requestTarget == "/page#section");
}

TEST_CASE("Request with deep nested path", "[request]") {
    std::string data = "GET /api/v1/users/123/posts/456/comments HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    CHECK(r.requestLine.requestTarget == "/api/v1/users/123/posts/456/comments");
}

TEST_CASE("Request with URL-encoded characters in path", "[request]") {
    std::string data = "GET /path%20with%20spaces HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n > 0);
    CHECK(r.requestLine.requestTarget == "/path%20with%20spaces");
}

// Invalid request line structure tests

TEST_CASE("Invalid - empty request", "[request]") {
    std::string data = "";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == 0);
    REQUIRE_FALSE(r.done());
}

TEST_CASE("Invalid - no CRLF terminator", "[request]") {
    std::string data = "GET / HTTP/1.1";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == 0);
    REQUIRE_FALSE(r.done());
}

TEST_CASE("Invalid - LF only terminator", "[request]") {
    // When request line uses LF instead of CRLF, parser finds next CRLF
    // and treats everything before it as the request line, which is malformed
    std::string data = "GET / HTTP/1.1\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid - too many spaces in request line", "[request]") {
    std::string data = "GET / extra HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid - only one part in request line", "[request]") {
    std::string data = "GET\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

// HTTP version validation tests

TEST_CASE("Invalid version in Request line - no slash", "[request]") {
    std::string data = "GET / HTTP1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - wrong prefix", "[request]") {
    std::string data = "GET / HTTPS/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid method (lowercase) Request line", "[request]") {
    std::string data = "GET / http/1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - HTTP/1.0", "[request]") {
    std::string data = "GET / HTTP/1.0\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - HTTP/2.0", "[request]") {
    std::string data = "GET / HTTP/2.0\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - empty after slash", "[request]") {
    std::string data = "GET / HTTP/\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - extra dot", "[request]") {
    std::string data = "GET / HTTP/1.1.1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}

TEST_CASE("Invalid version in Request line - multiple slashes", "[request]") {
    std::string data = "GET / HTTP/1/1\r\nHost: localhost\r\n\r\n";
    std::string errorMsg;

    Request r;
    int n = r.parse(data, &errorMsg);

    REQUIRE(n == -1);
    REQUIRE(r.error());
    CHECK(errorMsg == Request::ERROR_MALFORMED_REQUEST_LINE);
}
