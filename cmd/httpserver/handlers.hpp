#ifndef HANDLERS_HPP
#define HANDLERS_HPP

#include "Response.hpp"
#include "Router.hpp"
#include <string>

class Request;

void handleDefault(Response::Writer& w, const Request& req);
void handle400(Response::Writer& w, const Request& req);
void handle500(Response::Writer& w, const Request& req);

struct VideoHandler : public RouteHandler {
    std::string path;
    VideoHandler(const std::string& p) : path(p) {}
    void handle(Response::Writer& w, const Request& req);
};

void handleHttpbin(Response::Writer& w, const Request& req);

#endif
