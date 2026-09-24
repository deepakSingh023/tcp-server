#include "controller.h"
#include "response.h"
#include "connection.h"

void users_controller(HttpRequest *request , Connection *conn){

    create_response(conn,request);

}

void print_controller(HttpRequest *request ,Connection *conn ){

    if(request->body != NULL){
        printf("%s\n", request->body);
    }

    create_response(conn,request);
}