
#include "connection.h"
#include <stddef.h> 
#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>


int connection_init(Connection *connection, int fd)
{
    connection->fd = fd;

    if (buffer_init(&connection->input) == -1)
        return -1;

    if (buffer_init(&connection->output) == -1)
        return -1;

    http_parser_init(&connection->parser);
    connection->state = CONNECTION_IDLE;
    connection->context = NULL;

    return 0;
}


void connection_free(Connection *connection)
{
    buffer_free(&connection->input);
    buffer_free(&connection->output);
}

int connection_write(Connection *conn)
{
    while (conn->output.length > 0) {

        ssize_t sent = send(
            conn->fd,
            conn->output.data,
            conn->output.length,
            0
        );

        if (sent > 0) {

            memmove(
                conn->output.data,
                conn->output.data + sent,
                conn->output.length - sent
            );

            conn->output.length -= sent;

            continue;
        }

        if (sent == -1 &&
            (errno == EAGAIN || errno == EWOULDBLOCK)) {

            return 0;
        }

        return -1;
    }

    return 1;
}