#ifndef CONTROLLER_H
#define CONTROLLER_H
#include "connection.h"
#include "http_request.h"



void users_controller(HttpRequest *request , Connection *conn);

void print_controller(HttpRequest *request, Connection *conn);

void root_controller(HttpRequest *request , Connection *conn);
#endif