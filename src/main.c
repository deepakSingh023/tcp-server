#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>         // close()
#include <sys/socket.h>     // socket(), bind(), listen(), accept(), recv(), send()
#include <netinet/in.h>     // sockaddr_in, INADDR_ANY, htons()
#include <arpa/inet.h>      // inet_addr(), inet_pton()


int main(int argc, char *argv[]){
    

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd == -1){
        perror("socket failed");
        return 1;
    }

    struct sockaddr_in address;

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);


    if(bind(server_fd,(struct sockaddr* )&address, sizeof(address)) == -1){
        perror("bind failed");
        return 1;
    };

    if(listen(server_fd , 10 ) == -1){
        perror("listen failed");
        close(server_fd);
        return 1;
    }

    int client_fd = accept(server_fd, NULL, NULL);

    if(client_fd == -1){
        perror("accept failed");
        close(server_fd);
        return 1;
    }

    char buffer[4096];

    ssize_t receiver = recv(client_fd, buffer, sizeof(buffer) -1 , 0);

    if(receiver == -1){
        perror("recv failed");
        close(client_fd);
        close(server_fd);
        return 1;
    }

    buffer[receiver] = '\0';


    ssize_t sender = send(client_fd, buffer, receiver, 0);

    if(sender == -1){
        perror("send failed");
        close(client_fd);
        close(server_fd);
        return 1;
    }

    close(client_fd);
    close(server_fd);

    return 0;



    
}