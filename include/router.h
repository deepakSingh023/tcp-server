#ifndef ROUTER_H
#define ROUTER_H
#include "connection.h"
#include "http_request.h"

int router_handle(HttpRequest *request , Connection *conn);

#endif