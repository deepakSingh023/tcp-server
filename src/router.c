#include "router.h"
#include "controller.h"

#include <string.h>

int router_handle(HttpRequest *request)
{
    if (
        strcmp(request->method, "GET") == 0 &&
        strcmp(request->path, "/users") == 0
    ) {
        users_controller(request);
        return 0;
    }

    if (
        strcmp(request->method, "POST") == 0 &&
        strcmp(request->path, "/print") == 0
    ) {
        print_controller(request);
        return 0;
    }

    return 404;

    
}