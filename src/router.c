#include "router.h"
#include "controller.h"
#include "connection.h"
#include <string.h>

int router_handle(HttpRequest *request , Connection *conn)
{
    if (
        strcmp(request->method, "GET") == 0 &&
        strcmp(request->path, "/users") == 0
    ) {
        users_controller(request,conn);
        return 0;
    }

    if (
        strcmp(request->method, "POST") == 0 &&
        strcmp(request->path, "/print") == 0
    ) {
        print_controller(request,conn);
        return 0;
    }


    if (
        strcmp(request->method, "GET") == 0 &&
        strcmp(request->path, "/") == 0
    ) {
        root_controller(request,conn);
        return 0;
    }

    return 404;

    
}