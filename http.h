#ifndef HTTP_H
#define HTTP_H

#include "utils.h"

#define MAX_STRING_SIZE 128
#define MAX_PARAMETERS 16

typedef enum http_request_type
{
    GET,
    POST,
    PUT,
    PATCH = PUT,
    DELETE,
    NUM_HTTP_REQUEST_TYPES
} http_request_type;

typedef struct http_parameter_t
{
    char field[MAX_STRING_SIZE];
    char value[MAX_STRING_SIZE];
} http_parameter_t;

typedef struct http_request_t
{
    http_request_type type;
    // http_parameter_t parameters[MAX_PARAMETERS];
} http_request_t;

err_t parse_http_request(http_request_t *destination, const char *http_request_buffer);
err_t format_http_response(char *destination, size_t max_size, int error_code, const char *json_object);

#endif
