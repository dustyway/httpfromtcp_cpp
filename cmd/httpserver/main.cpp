#include <iostream>
#include <sstream>
#include "Server.hpp"
#include "Request.hpp"

#define PORT 42069

static const char BODY_200[] =
    "<html>\n"
    "  <head>\n"
    "    <title>200 OK</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>Success!</h1>\n"
    "    <p>Your request was an absolute banger.</p>\n"
    "  </body>\n"
    "</html>";

static const char BODY_400[] =
    "<html>\n"
    "  <head>\n"
    "    <title>400 Bad Request</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>Bad Request</h1>\n"
    "    <p>Your request honestly kinda sucked.</p>\n"
    "  </body>\n"
    "</html>";

static const char BODY_500[] =
    "<html>\n"
    "  <head>\n"
    "    <title>500 Internal Server Error</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>Internal Server Error</h1>\n"
    "    <p>Okay, you know what? This one is on me.</p>\n"
    "  </body>\n"
    "</html>";

static void handleRequest(Response::Writer& w, const Request& req) {
    Headers h = Response::getDefaultHeaders(0);
    const char* body = BODY_200;
    size_t bodyLen = sizeof(BODY_200) - 1;
    Response::StatusCode status = Response::StatusOk;

    if (req.getTarget() == "/yourproblem") {
        body = BODY_400;
        bodyLen = sizeof(BODY_400) - 1;
        status = Response::StatusBadRequest;
    } else if (req.getTarget() == "/myproblem") {
        body = BODY_500;
        bodyLen = sizeof(BODY_500) - 1;
        status = Response::StatusInternalServerError;
    }

    w.writeStatusLine(status);
    std::ostringstream oss;
    oss << bodyLen;
    h.replace("Content-Length", oss.str());
    h.replace("Content-Type", "text/html");
    w.writeHeaders(h);
    w.writeBody(body, bodyLen);
}

int main() {
    std::string errorMsg;
    Server* s = Server::serve(PORT, handleRequest, errorMsg);
    if (s == NULL) {
        std::cerr << "Error starting server: " << errorMsg << std::endl;
        return 1;
    }

    std::cout << "Server started on port " << PORT << std::endl;

    s->run();

    s->close();
    delete s;

    std::cout << "Server gracefully stopped" << std::endl;
    return 0;
}
