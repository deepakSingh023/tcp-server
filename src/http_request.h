#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
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

#define MAX_HEADERS 32


typedef struct {
    HttpHeader items[MAX_HEADERS];
    size_t count;
} HttpHeaders;


typedef struct {
    char name[64];
    char value[256];
} HttpHeader;

typedef struct {
    char method[16];
    char path[256];
    HttpHeaders headers;
    char *body;
    size_t body_length;
} HttpRequest;


int http_request_parse(
    HttpRequest *request,
    const char *data,
    size_t length
);

void http_request_free(HttpRequest *request);

#endif