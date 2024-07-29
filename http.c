#include "http.h"

err_t parse_http_request(http_request_t *destination, const char *http_request_buffer)
{
    if (destination == NULL || http_request_buffer == NULL)
    {
        return ERR_ARG;
    }

    memset(destination, 0, sizeof(http_request_t));

    if (strncmp(http_request_buffer, "GET", 3) == 0)
    {
        destination->type = GET;
    }
    else if (strncmp(http_request_buffer, "POST", 4) == 0)
    {
        destination->type = POST;
    }
    else if (strncmp(http_request_buffer, "PUT", 3) == 0 ||
             strncmp(http_request_buffer, "PATCH", 5) == 0)
    {
        destination->type = PUT;
    }
    else if (strncmp(http_request_buffer, "DELETE", 6) == 0)
    {
        destination->type = DELETE;
    }
    else
    {
        debug_printf("Unable to parse HTTP Request\n");
        return ERR_VAL; 
    }

    return ERR_OK;
}

err_t format_http_response(char *destination, size_t max_size, int error_code, const char *json_object)
{
    datetime_t current_time;
    char error_code_string[MAX_STRING_SIZE] = "";

    if (destination == NULL)
    {
        return ERR_ARG;
    }

    (void)memset((void *)destination, 0, max_size);

    // // Identify the error code string
    // switch (error_code)
    // {
    // case 200:
    //     strcat(error_code_string, "OK", MAX_STRING_SIZE);
    //     break;
    // case 204:
    //     strcat(error_code_string, "No Content", MAX_STRING_SIZE);
    //     break;
    // case 400:
    //     strcat(error_code_string, "Bad Request", MAX_STRING_SIZE);
    //     break;
    // case 401:
    //     strcat(error_code_string, "Unauthorized", MAX_STRING_SIZE);
    //     break;
    // case 404:
    //     strcat(error_code_string, "Not Found", MAX_STRING_SIZE);
    //     break;
    // case 405:
    //     strcat(error_code_string, "Method Not Allowed", MAX_STRING_SIZE);
    //     break;
    // default:
    //     strcat(error_code_string, "Unsupported", MAX_STRING_SIZE);
    //     return ERR_ARG;
    // }

    // Add HTTP headers to the response
    snprintf(
        destination, // Add to the beginning
        max_size, // Remaining space
        "HTTP/1.1 %d\r\n"
        "Access-Control-Allow-Origin: *\r\n",
        error_code
    );

    // Add current date to the HTTP response
    if (rtc_get_datetime(&current_time))
    {
        const char *month_names[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        const char *day_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

        snprintf(
            destination + strlen(destination), // Add to the end of the string
            max_size - strlen(destination), // Remaining space
            "Date: %s, %d %s %d %d:%d:%d CST\r\n",
            day_names[current_time.dotw],
            current_time.day,
            month_names[current_time.month],
            current_time.year,
            current_time.hour,
            current_time.min,
            current_time.sec
        );
    }
    else
    {
        debug_printf("Unable to add date to HTTP response\n");
    }

    // Add "Server" fields
    strncat(destination, "Server: Shades Machine\r\n", max_size - strlen(destination));

    if (json_object != NULL)
    {
        // Add "content type" and "content length" fields
        snprintf(
            destination + strlen(destination), // Add to the end of the string
            max_size - strlen(destination), // Remaining space
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n",
            strlen(json_object)
        );

        // Add ending line to HTTP Header
        strncat(destination, "\r\n", max_size - strlen(destination));

        // Add JSON response to the body of the HTTP response
        strncat(destination, json_object, max_size - strlen(destination));
    }

    debug_printf("HTTP Response:\n%s\n------------------------------\n", destination);

    return ERR_OK;
}
