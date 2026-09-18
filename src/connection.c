
#include "connection.h";
#include <stddef.h> 
#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



int connection_init(Connection *connection, int fd)
{
    connection->fd = fd;

    if (buffer_init(&connection->input) == -1)
        return -1;

    if (buffer_init(&connection->output) == -1)
        return -1;

    return 0;
}


void connection_free(Connection *connection)
{
    buffer_free(&connection->input);
    buffer_free(&connection->output);
}