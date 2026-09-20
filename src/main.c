
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "connection.h"
#include <arpa/inet.h>// inet_addr(), inet_pton()
#include <stddef.h> 
#include "thread_pool.h"
#include <fcntl.h>
#define MAX_EVENTS 64
#define _GNU_SOURCE
#include <string.h>
int make_nonBlocking(int fd){

    int flag = fcntl(fd , F_GETFL , 0);

    if(flag == -1){
        return -1;
    }

    if(fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1){
        return -1;
    }

    return 0;

}

int helper_funtion(Buffer *buffer){
    char *found = memmem(
        buffer->data,
        buffer->length,
        "\r\n\r\n",
        4
    ); 

    if(found == NULL){
        return -1;
    }

    size_t header_end = (char *)found - buffer->data;
    size_t body_start = header_end + 4;

    char *content_length = memmem(
        buffer->data,
        header_end,
        "Content-Length:",
        15
    );
    if (content_length == NULL) {
        return -1;
    }

    size_t cont = (char *)content_length - buffer->data;

    size_t length_start = cont + 15;

    size_t content_len = 0;

    for (size_t i = length_start; i < header_end; i++) {
    
        if (buffer->data[i] < '0' ||
            buffer->data[i] > '9') {
            break;
        }
    
        content_len =
            content_len * 10 +
            (buffer->data[i] - '0');
    }

    return 0;

}


int main(){

    ThreadPool pool;

    thread_pool_init(&pool);

    //create the tcp listenign socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd == -1){
        perror("socket failed");
        return 1;
    }
    
    //create an adress 
    struct sockaddr_in address;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);


    //bind the socket
    if(bind(server_fd,(struct sockaddr* )&address, sizeof(address)) == -1){
        perror("bind failed");
        return 1;
    };


    //listen
    if(listen(server_fd , 10 ) == -1){
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    if(make_nonBlocking(server_fd) == -1){
        perror("make_nonBlocking failed");
        close(server_fd);
        return 1;
    }

    int epoll_fd = epoll_create1(0);

    if(epoll_fd == -1){
        perror("epoll_create1 failed");
        close(server_fd);
        return 1;
    }


    //epoll setup listening on the fd 3 which is the socket
    struct epoll_event event;
    event.events = EPOLLIN ;
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

    while(1){

        //the event count never becomes  because we havent given it a timeout it will sleep while there is nothing also it give -1 when there is an error 
        int event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if(event_count == -1){
            //the EINTR error is the signal that is sent when it is interuppted by a signal and we continue because we want to continue
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            break;
        }

        for(int i = 0 ; i < event_count; i++){

            Connection *conn = events[i].data.ptr;

            if(conn == NULL){
                int client_fd = accept(server_fd, NULL, NULL);

                if (client_fd == -1) {

                    if (errno == EAGAIN ||
                        errno == EWOULDBLOCK) {
                        continue;
                    }

                    perror("accept");
                    continue;
                }

                if(make_nonBlocking(client_fd) == -1){
                    perror("make_nonBlocking failed");
                    close(client_fd);
                    continue;
                }

                conn = malloc(sizeof(Connection));

                if( conn == NULL){
                    perror("malloc failed in main");
                    return -1;
                }

                if(connection_init(conn,client_fd) == -1){
                    free(conn);
                    close(client_fd);
                    perror("failed to initiate");
                    return -1;

                }
                

                struct epoll_event client_event;

                client_event.events = EPOLLIN| EPOLLET;
                client_event.data.ptr = conn;

                if (epoll_ctl(
                    epoll_fd,
                    EPOLL_CTL_ADD,
                    conn->fd,
                    &client_event
                ) == -1) {
                    connection_free(conn);
                    free(conn);
                    close(client_fd);
                    perror("epoll_ctl client");
                    continue;
                }

                printf("Client connected: fd=%d\n", client_fd);

            
            }else{


                char Buffer[4096];

                while(1){

                    ssize_t receiver_data = recv(conn->fd, Buffer, sizeof(Buffer) - 1, 0);


                    if(receiver_data > 0){

                        if(buffer_append(&conn->input,Buffer,receiver_data) == -1){
                            perror("not appended");
                            return -1;
                        }
                        continue;
                    }

                    if (receiver_data == 0) {
                        // client closed connection
                    
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->fd, NULL);
                    
                        close(conn->fd);
                        connection_free(conn);
                        free(conn);
                    
                        break;
                    }

                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        // nothing currently left
                        break;
                    }

                    perror("recv");
                    
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->fd, NULL);
                    
                    close(conn->fd);
                    connection_free(conn);
                    free(conn);
                    
                    break;

                    break;



                }


            }
        }
    }

    close(epoll_fd);
    close(server_fd);

    return 0;
}