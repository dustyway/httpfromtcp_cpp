#include <iostream>
#include "Server.hpp"
#include "Request.hpp"

#define PORT 42069

static bool handleRequest(std::string& responseBody, const Request& req, HandlerError& error) {
    if (req.getTarget() == "/yourproblem") {
        error.statusCode = Response::StatusBadRequest;
        error.message = "Your problem is not my problem\n";
        return true;
    } else if (req.getTarget() == "/myproblem") {
        error.statusCode = Response::StatusInternalServerError;
        error.message = "Woopsie, my bad\n";
        return true;
    }
    responseBody = "All good, frfr\n";
    return false;
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
