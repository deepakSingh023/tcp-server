#include <stddef.h> 
#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int buffer_init(Buffer *buffer){

    if(!(buffer->data = malloc(4096))){
        perror("mallocl failed");
        return -1;
    }

    buffer->length = 0;

    buffer->capacity = 4096;

    return 0;
}


int buffer_append(Buffer *buffer, char *data , size_t length){

    while(buffer->length + length > buffer->capacity){

        size_t new_capacity = buffer->capacity * 2;

        char *new_data = realloc(buffer->data, new_capacity);

        if( new_data == NULL){
            perror("returned null");
            return -1;
        }

        buffer->data = new_data;

        buffer->capacity = new_capacity;
    }


    memcpy(buffer->data+buffer->length, data, length);

    buffer->length += length;

    return 0;

}



int buffer_consume(Buffer *buffer, size_t length){

    if(length > buffer->length){
        return -1;
    }
    memmove(buffer->data, buffer->data + length, buffer->length-length);

    buffer -> length -= length;

    return 0;

}

void buffer_free(Buffer *buffer){

    free(buffer->data);

    buffer-> data = NULL;

}