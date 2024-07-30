#ifndef HTTP_H
#define HTTP_H

#include "hardware/rtc.h"
#include "utils.h"

#define MAX_STRING_SIZE 64
#define MAX_PARAMETERS 4

// HTTP Error codes
#define HTTP_OK 200
#define HTTP_NO_CONTENT 204
#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_NOT_FOUND 404
#define HTTP_METHOD_NOT_ALLOWED 405

typedef enum http_request_type
{
    GET,
    POST,
    PUT,
    PATCH = PUT,
    DELETE,
    NUM_HTTP_REQUEST_TYPES
} http_request_type;

typedef struct http_request_t
{
    http_request_type type;
    unsigned int num_parameters;
    char parameters[MAX_PARAMETERS][MAX_STRING_SIZE+1];
} http_request_t;

err_t parse_http_request(http_request_t *destination, const char *http_request_buffer);
err_t format_http_response(char *destination, size_t max_size, int error_code, const char *json_object);

#endif
