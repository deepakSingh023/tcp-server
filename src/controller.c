#include "controller.h"
#include "response.h"
#include "connection.h"
#include <stdio.h>
#include "common.h"



void users_controller(HttpRequest *request , Connection *conn){

    if(request->body != NULL){
        printf("%s\n", request->body);
        LOG_PRINT("This is body of user GET" , request->body);
    }

    create_response(conn,request);

}

void print_controller(HttpRequest *request ,Connection *conn ){

    if(request->body != NULL){
        LOG_PRINT("This is body of user POST" , request->body);
    }

    create_response(conn,request);
}

void root_controller(HttpRequest *request , Connection *conn){
    create_response(conn,request);
}