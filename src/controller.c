#include "controller.h"
#include "response.h"
#include "connection.h"
#include <stdio.h>
void users_controller(HttpRequest *request , Connection *conn){

    if(request->body != NULL){
        printf("%s\n", request->body);
    }

    create_response(conn,request);

}

void print_controller(HttpRequest *request ,Connection *conn ){

    if(request->body != NULL){
        printf("%s\n", request->body);
    }

    create_response(conn,request);
}

void root_controller(HttpRequest *request , Connection *conn){

    printf("root api");

    create_response(conn,request);

}