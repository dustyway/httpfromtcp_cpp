#include <iostream>
#include "Server.hpp"

#define PORT 42069

int main() {
    std::string errorMsg;
    Server* s = Server::serve(PORT, &errorMsg);
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
