#define _GNU_SOURCE

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

#include "connection.h"
#include "thread_pool.h"

#define MAX_EVENTS 64


int make_nonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);

    if (flag == -1) {
        return -1;
    }

    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1) {
        return -1;
    }

    return 0;
}


int main()
{
    ThreadPool pool;

    thread_pool_init(&pool);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1) {
        perror("socket failed");
        return 1;
    }


    struct sockaddr_in address;

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);


    if (bind(
        server_fd,
        (struct sockaddr *)&address,
        sizeof(address)
    ) == -1) {

        perror("bind failed");
        close(server_fd);
        return 1;
    }


    if (listen(server_fd, 10) == -1) {

        perror("listen failed");
        close(server_fd);
        return 1;
    }


    if (make_nonBlocking(server_fd) == -1) {

        perror("make_nonBlocking failed");
        close(server_fd);
        return 1;
    }



    int epoll_fd = epoll_create1(0);

    if (epoll_fd == -1) {

        perror("epoll_create1 failed");
        close(server_fd);
        return 1;
    }

    struct epoll_event event;

    memset(&event, 0, sizeof(event));

    event.events = EPOLLIN;

    // NULL means this is the listening socket.
    event.data.ptr = NULL;


    if (epoll_ctl(
        epoll_fd,
        EPOLL_CTL_ADD,
        server_fd,
        &event
    ) == -1) {

        perror("epoll_ctl listener");

        close(epoll_fd);
        close(server_fd);

        return 1;
    }


    struct epoll_event events[MAX_EVENTS];

    while (1) {

        int event_count = epoll_wait(
            epoll_fd,
            events,
            MAX_EVENTS,
            -1
        );


        if (event_count == -1) {

            if (errno == EINTR) {
                continue;
            }

            perror("epoll_wait");
            break;
        }


        for (int i = 0; i < event_count; i++) {

            Connection *conn = events[i].data.ptr;

            if (conn == NULL) {

                int client_fd;

                while (1) {

                    client_fd = accept(
                        server_fd,
                        NULL,
                        NULL
                    );


                    if (client_fd == -1) {

                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK) {

                            break;
                        }

                        if (errno == EINTR) {
                            continue;
                        }

                        perror("accept");
                        break;
                    }

                    if (make_nonBlocking(client_fd) == -1) {

                        perror("make_nonBlocking failed");

                        close(client_fd);
                        continue;
                    }

                    conn = malloc(sizeof(Connection));

                    if (conn == NULL) {

                        perror("malloc failed");

                        close(client_fd);
                        continue;
                    }

                    if (connection_init(conn, client_fd) == -1) {

                        perror("connection_init failed");

                        free(conn);
                        close(client_fd);

                        continue;
                    }

                    struct epoll_event client_event;

                    memset(
                        &client_event,
                        0,
                        sizeof(client_event)
                    );


                    client_event.events =
                        EPOLLIN | EPOLLET;

                    client_event.data.ptr = conn;


                    if (epoll_ctl(
                        epoll_fd,
                        EPOLL_CTL_ADD,
                        conn->fd,
                        &client_event
                    ) == -1) {

                        perror("epoll_ctl client");

                        connection_free(conn);
                        free(conn);
                        close(client_fd);

                        continue;
                    }
                    printf(
                        "Client connected: fd=%d\n",
                        client_fd
                    );
                }


                continue;
            }

            while (1) {

                char buffer[4096];
                ssize_t received = recv(
                    conn->fd,
                    buffer,
                    sizeof(buffer),
                    0
                );

                if (received > 0) {

                    if (buffer_append(
                        &conn->input,
                        buffer,
                        received
                    ) == -1) {

                        perror("buffer_append failed");

                        epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_DEL,
                            conn->fd,
                            NULL
                        );

                        close(conn->fd);

                        connection_free(conn);
                        free(conn);

                        break;
                    }

                    int result = http_parser_execute(
                        &conn->parser,
                        &conn->input
                    );


                    if (result == 1) {

                        Task task;

                        task.conn = conn;

                        enqueue(
                            &pool.queue,
                            task
                        );


                        printf(
                            "Request ready for worker: fd=%d\n",
                            conn->fd
                        );

                    }

                    if (result == -1) {

                        fprintf(
                            stderr,
                            "HTTP parser error on fd=%d\n",
                            conn->fd
                        );
                        epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_DEL,
                            conn->fd,
                            NULL
                        );
                        close(conn->fd);
                        connection_free(conn);
                        free(conn);

                        break;
                    }
                    continue;
                }


                if (received == 0) {

                    printf(
                        "Client disconnected: fd=%d\n",
                        conn->fd
                    );


                    epoll_ctl(
                        epoll_fd,
                        EPOLL_CTL_DEL,
                        conn->fd,
                        NULL
                    );

                    close(conn->fd);

                    connection_free(conn);
                    free(conn);

                    break;
                }

                if (errno == EAGAIN ||
                    errno == EWOULDBLOCK) {
                    break;
                }

                perror("recv");


                epoll_ctl(
                    epoll_fd,
                    EPOLL_CTL_DEL,
                    conn->fd,
                    NULL
                );


                close(conn->fd);

                connection_free(conn);
                free(conn);

                break;
            }
        }
    }


    close(epoll_fd);
    close(server_fd);

    return 0;
}