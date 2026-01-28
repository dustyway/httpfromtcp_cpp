#include <iostream>
#include "Router.hpp"
#include "Server.hpp"
#include "handlers.hpp"

#define PORT 42069

int main() {
    Router router;
    router.get("/yourproblem", handle400);
    router.get("/myproblem", handle500);
    VideoHandler videoHandler("assets/vim.mp4");
    router.get("/video", videoHandler);
    router.prefix("/httpbin/", handleHttpbin);
    router.setDefault(handleDefault);

    std::string errorMsg;
    Server* s = Server::serve(PORT, router, errorMsg);
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
