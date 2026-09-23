#include "controller.h"



void users_controller(HttpRequest *request){


}

void print_controller(HttpRequest *request){

    if(request->body != NULL){
        printf("%s\n", request->body);
    }
}