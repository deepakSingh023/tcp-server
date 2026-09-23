#include "http_request.h"

#include <string.h>
#include <stdlib.h>


static int parse_headers(
    HttpRequest *request,
    const char *data,
    size_t *i,
    size_t length
){
    while (1) {

        /*
         * Empty line means:
         *
         * \r\n
         *
         * Headers are finished.
         */
        if (
            *i + 1 < length &&
            data[*i] == '\r' &&
            data[*i + 1] == '\n'
        ) {
            *i += 2;
            return 0;
        }


        /*
         * Make sure we haven't exceeded
         * our maximum number of headers.
         */
        if (request->headers.count >= MAX_HEADERS) {
            return -1;
        }


        /*
         * Remember where the header name starts.
         */
        size_t name_start = *i;


        /*
         * Find ':'.
         */
        while (
            *i < length &&
            data[*i] != ':'
        ) {
            (*i)++;
        }


        /*
         * ':' wasn't found.
         */
        if (*i >= length) {
            return -1;
        }


        /*
         * Calculate header-name length.
         */
        size_t name_length = *i - name_start;


        /*
         * Make sure it fits inside
         * char name[64].
         */
        if (
            name_length >=
            sizeof(request->headers.items[0].name)
        ) {
            return -1;
        }


        /*
         * Copy header name.
         *
         * destination:
         * request->headers.items[count].name
         *
         * source:
         * data + name_start
         *
         * number of bytes:
         * name_length
         */
        memcpy(
            request->headers.items[request->headers.count].name,
            data + name_start,
            name_length
        );


        /*
         * Add string terminator.
         */
        request->headers.items[request->headers.count]
            .name[name_length] = '\0';


        /*
         * Skip ':'.
         */
        (*i)++;


        /*
         * Skip spaces after ':'.
         */
        while (
            *i < length &&
            data[*i] == ' '
        ) {
            (*i)++;
        }


        /*
         * Remember where the value starts.
         */
        size_t value_start = *i;


        /*
         * Find CRLF.
         */
        while (
            *i + 1 < length &&
            !(data[*i] == '\r' &&
              data[*i + 1] == '\n')
        ) {
            (*i)++;
        }


        /*
         * CRLF wasn't found.
         */
        if (*i + 1 >= length) {
            return -1;
        }


        /*
         * Calculate value length.
         */
        size_t value_length = *i - value_start;


        /*
         * Make sure value fits inside
         * char value[256].
         */
        if (
            value_length >=
            sizeof(request->headers.items[0].value)
        ) {
            return -1;
        }


        /*
         * Copy header value.
         */
        memcpy(
            request->headers.items[request->headers.count].value,
            data + value_start,
            value_length
        );


        /*
         * Add string terminator.
         */
        request->headers.items[request->headers.count]
            .value[value_length] = '\0';


        /*
         * Skip CRLF.
         */
        *i += 2;


        /*
         * We successfully parsed one header.
         */
        request->headers.count++;
    }
}


int http_request_parse(
    HttpRequest *request,
    const char *data,
    size_t length
){
    /*
     * Initial state.
     */
    request->headers.count = 0;
    request->body = NULL;
    request->body_length = 0;


    size_t i = 0;


    /*
     * -------------------------
     * Parse method
     * -------------------------
     */

    while (
        i < length &&
        data[i] != ' '
    ) {
        i++;
    }


    if (i == length) {
        return -1;
    }


    if (i >= sizeof(request->method)) {
        return -1;
    }


    memcpy(
        request->method,
        data,
        i
    );

    request->method[i] = '\0';


    /*
     * Skip space after method.
     */
    i++;


    if (i == length) {
        return -1;
    }


    /*
     * -------------------------
     * Parse path
     * -------------------------
     */

    size_t path_start = i;


    while (
        i < length &&
        data[i] != ' '
    ) {
        i++;
    }


    if (i == length) {
        return -1;
    }


    size_t path_length = i - path_start;


    if (
        path_length >=
        sizeof(request->path)
    ) {
        return -1;
    }


    memcpy(
        request->path,
        data + path_start,
        path_length
    );


    request->path[path_length] = '\0';


    /*
     * Skip space after path.
     *
     * We are now at:
     *
     * GET /users HTTP/1.1
     *           ^
     */
    i++;


    /*
     * -------------------------
     * Skip HTTP version
     * -------------------------
     */

    while (
        i + 1 < length &&
        !(data[i] == '\r' &&
          data[i + 1] == '\n')
    ) {
        i++;
    }


    if (i + 1 >= length) {
        return -1;
    }


    /*
     * Skip request-line CRLF.
     */
    i += 2;


    /*
     * -------------------------
     * Parse headers
     * -------------------------
     */

    if (
        parse_headers(
            request,
            data,
            &i,
            length
        ) != 0
    ) {
        return -1;
    }


    /*
     * -------------------------
     * Parse body
     * -------------------------
     *
     * i now points to the first
     * byte after the blank line.
     */

    request->body_length = length - i;


    if (request->body_length > 0) {

        request->body = malloc(
            request->body_length + 1
        );

        if (request->body == NULL) {
            return -1;
        }


        memcpy(
            request->body,
            data + i,
            request->body_length
        );


        request->body[request->body_length] = '\0';
    }


    return 0;
}


void http_request_free(HttpRequest *request)
{
    free(request->body);

    request->body = NULL;
    request->body_length = 0;
}