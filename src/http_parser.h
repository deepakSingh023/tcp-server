#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h>
#include "buffer.h"

typedef enum {
    HTTP_PARSING_HEADERS,
    HTTP_PARSING_BODY,
    HTTP_COMPLETE,
    HTTP_ERROR
} HttpParserState;

typedef struct {
    HttpParserState state;

    size_t scan_position;
    size_t header_end;

    size_t body_start;
    size_t content_length;
} HttpParser;

void http_parser_init(HttpParser *parser);

int http_parser_execute(
    HttpParser *parser,
    Buffer *buffer
);

#endif