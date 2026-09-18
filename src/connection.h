#ifndef CONNECTION_H
#define CONNECTION_H
#include "buffer.h"
#include <stddef.h>


typedef struct{
    int fd;
    Buffer input;
    Buffer output;
} Connection;


int connection_init(Connection *connection, int fd);

void connection_free(Connection *connection);


#endif