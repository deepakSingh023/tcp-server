#ifndef CONNECTION_H
#define CONNECTION_H
#include "buffer.h"
#include <stddef.h>
#include "http_parser.h"


typedef struct{
    int fd;
    Buffer input;
    Buffer output;
    HttpParser parser;
} Connection;


int connection_init(Connection *connection, int fd);

void connection_free(Connection *connection);


#endif