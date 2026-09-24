#include "response.h"

#include <stdio.h>
#include <string.h>

void create_response(Connection *conn, HttpRequest *request)
{
    const char *body;

    if (request->body != NULL) {
        body = request->body;
    } else {
        body = "Hello from server";
    }

    int length = snprintf(
        NULL,
        0,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "%s",
        strlen(body),
        body
    );

    if (length < 0) {
        return;
    }

    char response[length + 1];

    snprintf(
        response,
        length + 1,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "%s",
        strlen(body),
        body
    );

    buffer_append(
        &conn->output,
        response,
        length
    );
}