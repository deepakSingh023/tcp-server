#define _GNU_SOURCE

#include "http_parser.h"

#include <string.h>
#include <ctype.h>

void http_parser_init(HttpParser *parser)
{
    parser->state = HTTP_PARSING_HEADERS;

    parser->scan_position = 0;
    parser->header_end = 0;
    parser->body_start = 0;
    parser->content_length = 0;
}

int http_parser_execute(
    HttpParser *parser,
    Buffer *buffer
){
    if(parser->state = HTTP_COMPLETE){
        return 1;
    }

    if (parser->state == HTTP_ERROR) {
        return -1;
    }

    if(parser->state = HTTP_PARSING_HEADERS){

        char *found = memme(buffer->data+parser->scan_position,buffer->length - parser->content_length, "\r\n\r\n",4);
        if(found == NULL){
            parser->scan_position= buffer->length;
        }
    }
}

