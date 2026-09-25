#define _GNU_SOURCE
#include "thread_pool.h"
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
#include <sys/eventfd.h>
#include <stdint.h>
#include "completion_queue.h"
#define MAX_EVENTS 64
#include "connection.h"

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
    CompletionQueue completion_queue;

    if (completion_queue_init(&completion_queue) == -1) {
        fprintf(stderr, "completion_queue_init failed\n");
        return 1;
    }

    int event_fd = eventfd(0, EFD_NONBLOCK);

    if (event_fd == -1) {
        perror("eventfd");
        return 1;
    }

    ThreadPool pool;

    pool.event_fd = event_fd;
    pool.completion_queue = &completion_queue;

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

    EpollContext listener_context;

    listener_context.type = EPOLL_LISTENER;
    listener_context.fd = server_fd;
    listener_context.connection = NULL;
    
    event.data.ptr = &listener_context;

    event.events = EPOLLIN;

    // NULL means this is the listening socket.
    event.data.ptr = &listener_context;


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

    EpollContext event_context;

    event_context.connection = NULL;
    event_context.fd= event_fd;
    event_context.type=EPOLL_EVENTFD;


    struct epoll_event events[MAX_EVENTS];

    struct epoll_event eventfd_event;

    memset(&eventfd_event, 0, sizeof(eventfd_event));
    
    eventfd_event.events = EPOLLIN;
    eventfd_event.data.ptr = &event_context;
    
    if (epoll_ctl(
        epoll_fd,
        EPOLL_CTL_ADD,
        event_fd,
        &eventfd_event
    ) == -1) {
    
        perror("epoll_ctl eventfd");
    
        close(event_fd);
        close(epoll_fd);
        close(server_fd);
    
        return 1;
    }

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
        
            EpollContext *context = events[i].data.ptr;
        
            switch (context->type) {
        
                case EPOLL_LISTENER: {
        
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
        
                        Connection *conn = malloc(sizeof(Connection));
        
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
        
                        EpollContext *client_context =
                            malloc(sizeof(EpollContext));
        
                        if (client_context == NULL) {
        
                            perror("malloc failed");
                            connection_free(conn);
                            free(conn);
                            close(client_fd);
                            continue;
                        }
        
                        client_context->connection = conn;
                        client_context->fd = client_fd;
                        client_context->type = EPOLL_CONNECTION;
                        conn->context = client_context;
        
                        struct epoll_event client_event;
        
                        memset(
                            &client_event,
                            0,
                            sizeof(client_event)
                        );
        
                        client_event.events =
                            EPOLLIN | EPOLLET;
        
                        client_event.data.ptr =
                            client_context;
        
                        if (epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_ADD,
                            conn->fd,
                            &client_event
                        ) == -1) {
        
                            perror("epoll_ctl client");
        
                            free(client_context);
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
        
                    break;
                }
        
        
                case EPOLL_EVENTFD: {
                
                    uint64_t value;
                
                    if (read(event_fd, &value, sizeof(value)) == -1) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            perror("read eventfd");
                        }
                    }
                
                    Connection *conn;
                
                    while ((conn = completion_dequeue(&completion_queue)) != NULL) {
                
                        conn->state = CONNECTION_IDLE;
                
                        struct epoll_event event;
                        memset(&event, 0, sizeof(event));
                
                        event.events = EPOLLIN | EPOLLOUT | EPOLLET;
                        event.data.ptr = conn->context;
                
                        epoll_ctl(
                            epoll_fd,
                            EPOLL_CTL_MOD,
                            conn->fd,
                            &event
                        );
                    }
                
                    break;
                }
        
        
                case EPOLL_CONNECTION: {
        
                    Connection *conn = context->connection;

                    if(events[i].events & EPOLLIN){
    
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
            
                                    free(context);
            
                                    break;
                                }
            
                                int result = http_parser_execute(
                                    &conn->parser,
                                    &conn->input
                                );
                                            
                                if (result == 1 &&
                                    conn->state == CONNECTION_IDLE) {
                                
                                    conn->state = CONNECTION_PROCESSING;
                                
                                    Task task;
                                    task.conn = conn;
                                
                                    enqueue(&pool.queue, task);
                                
                                    struct epoll_event event;
                                    memset(&event, 0, sizeof(event));
                                
                                    event.events = EPOLLET;
                                    event.data.ptr = conn->context;
                                
                                    if (epoll_ctl(
                                        epoll_fd,
                                        EPOLL_CTL_MOD,
                                        conn->fd,
                                        &event
                                    ) == -1) {
                                        perror("epoll_ctl processing");
                                    }
                                
                                    printf("request enqued\n");
                                
                                    break;
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
            
                                    free(context);
            
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
            
                                free(context);
            
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
            
                            free(context);
            
                            break;
                        }
    
    
    
    
    
                    }


                                    
                    if (events[i].events & EPOLLOUT) {
                    
                        int result = connection_write(conn);
                    
                        if (result == -1) {
                    
                            epoll_ctl(
                                epoll_fd,
                                EPOLL_CTL_DEL,
                                conn->fd,
                                NULL
                            );
                    
                            close(conn->fd);
                            connection_free(conn);
                            free(context);
                    
                            break;
                        }
                    
                        if (result == 1) {
                    
                            struct epoll_event event;
                            memset(&event, 0, sizeof(event));
                    
                            event.events = EPOLLIN | EPOLLET;
                            event.data.ptr = conn->context;
                    
                            if (epoll_ctl(
                                epoll_fd,
                                EPOLL_CTL_MOD,
                                conn->fd,
                                &event
                            ) == -1) {
                                perror("epoll_ctl after send");
                            }
                        }
                    }
            
    
        
                    break;
                }
            }
        }
    }


    close(epoll_fd);
    close(server_fd);

    return 0;
}