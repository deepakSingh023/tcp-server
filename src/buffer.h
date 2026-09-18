#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

#define BUFFER_INITIAL_CAPACITY 4096


typedef struct{
    char *data;
    size_t length;
    size_t capacity;
} Buffer;

int buffer_init(Buffer *buffer);

int buffer_append(Buffer *buffer, char *data , size_t length);

int buffer_consume(Buffer *buffer, size_t length);

void buffer_free(Buffer *buffer);

#endif