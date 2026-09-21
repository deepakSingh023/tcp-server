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
)
{

    if (parser->state == HTTP_COMPLETE) {
        return 1;
    }


    if (parser->state == HTTP_ERROR) {
        return -1;
    }

    if (parser->state == HTTP_PARSING_HEADERS) {


        size_t search_start = parser->scan_position;

        if (search_start >= 3) {
            search_start -= 3;
        } else {
            search_start = 0;
        }

        if (search_start > buffer->length) {
            search_start = buffer->length;
        }


        char *found = memmem(
            buffer->data + search_start,
            buffer->length - search_start,
            "\r\n\r\n",
            4
        );

        if (found == NULL) {

            parser->scan_position = buffer->length;

            return 0;
        }

        parser->header_end =
            (size_t)(found - buffer->data);

        parser->body_start =
            parser->header_end + 4;

        char *content_length_header = memmem(
            buffer->data,
            parser->header_end,
            "Content-Length:",
            15
        );


        if (content_length_header == NULL) {

            parser->content_length = 0;

            parser->state = HTTP_COMPLETE;

            return 1;
        }


        size_t length_start =
            (size_t)(content_length_header - buffer->data) + 15;


        // Skip spaces

        while (
            length_start < parser->header_end &&
            buffer->data[length_start] == ' '
        ) {
            length_start++;
        }


        size_t content_length = 0;

        size_t i = length_start;


        // Read digits

        while (
            i < parser->header_end &&
            buffer->data[i] >= '0' &&
            buffer->data[i] <= '9'
        ) {

            content_length =
                content_length * 10 +
                (buffer->data[i] - '0');

            i++;
        }


        parser->content_length = content_length;

        if (parser->content_length == 0) {

            parser->state = HTTP_COMPLETE;

            return 1;
        }


        parser->state = HTTP_PARSING_BODY;
    }

    if (parser->state == HTTP_PARSING_BODY) {

        size_t body_received =
            buffer->length - parser->body_start;


        if (body_received < parser->content_length) {
            return 0;
        }
        parser->state = HTTP_COMPLETE;

        return 1;
    }


    return 0;
}