#ifndef CONNECTION_H
#define CONNECTION_H

#include "buffer.h"
#include <stddef.h>
#include "http_parser.h"

typedef enum {
    CONNECTION_IDLE,
    CONNECTION_PROCESSING
} ConnectionState;

/* Tell the compiler Connection exists */
typedef struct Connection Connection;

typedef enum {
    EPOLL_LISTENER,
    EPOLL_EVENTFD,
    EPOLL_CONNECTION
} EpollType;

typedef struct {
    EpollType type;
    int fd;
    Connection *connection;
} EpollContext;

struct Connection {
    int fd;

    Buffer input;
    Buffer output;

    HttpParser parser;

    ConnectionState state;

    int close_requested;

    EpollContext *context;
};

int connection_init(Connection *connection, int fd);

void connection_free(Connection *connection);

int connection_write(Connection *conn);

#endif