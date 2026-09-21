
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <fcntl.h>

typedef struct {
    char method[16];
    char path[256];

    // later
    HttpHeaders headers;

    char *body;
    size_t body_length;
} HttpRequest;


int http_request_parse(
    HttpRequest *request,
    const char *data,
    size_t length
);