#include "http.h"

err_t parse_http_request(http_request_t *destination, const char *http_request)
{
    err_t err = ERR_OK;
    char *current_byte_pointer;

    if (destination == NULL || http_request == NULL)
    {
        return ERR_ARG;
    }

    // Initialization
    memset(destination, 0, sizeof(http_request_t));

    // Identify the type of HTTP request
    if (strncmp(http_request, "GET", 3) == 0)
    {
        // GET request, no support for parameters
        destination->type = GET;
        destination->num_parameters = 0;
    }
    else if (strncmp(http_request, "POST", 4) == 0)
    {
        destination->type = POST;

        // Start with 0 parameters
        destination->num_parameters = 0;

        // Skip to the first first one
        strtok((char *)http_request, " ");

        // Dummy value
        current_byte_pointer = http_request;

        // Keep identifying parameters until the string ends, whitespace is found, or we run out of space
        while (current_byte_pointer != NULL &&
               destination->num_parameters < MAX_PARAMETERS)
        {
            // Check for whitespace
            if (*current_byte_pointer == ' ' ||
                *current_byte_pointer == '\r' ||
                *current_byte_pointer == '\n' ||
                *current_byte_pointer == '\t')
            {
                break;
            }
            
            // Find next parameter or end of string or whitespace
            current_byte_pointer = strtok(NULL, "/ \r\n");
            debug_printf("Parameter: %s\n", current_byte_pointer);

            // Add parameter to the struct
            strncpy(destination->parameters[destination->num_parameters], current_byte_pointer, MAX_STRING_SIZE);
            destination->num_parameters++;
        }
    }
    else if (strncmp(http_request, "PUT", 3) == 0 ||
             strncmp(http_request, "PATCH", 5) == 0)
    {
        destination->type = PUT;
        destination->num_parameters = 0;
    }
    else if (strncmp(http_request, "DELETE", 6) == 0)
    {
        destination->type = DELETE;
        destination->num_parameters = 0;
    }
    else
    {
        debug_printf("Unable to parse HTTP Request\n");
        err = ERR_VAL; 
    }

    return err;
}

err_t format_http_response(char *destination, size_t max_size, int error_code, const char *json_object)
{
    datetime_t current_time;
    char error_code_string[MAX_STRING_SIZE+1] = " ";

    if (destination == NULL)
    {
        return ERR_ARG;
    }

    (void)memset((void *)destination, 0, max_size);

    // Add HTTP headers to the response
    snprintf(
        destination, // Add to the beginning
        max_size, // Remaining space
        "HTTP/1.1 %d Bad Request\r\n"
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

    // Add "content type" and "content length" fields
    snprintf(
        destination + strlen(destination), // Add to the end of the string
        max_size - strlen(destination), // Remaining space
        "%sContent-Length: %d\r\n",
        json_object ? "Content-Type: application/json\r\n" : "",
        strlen(json_object)
    );

    // Add ending line to HTTP Header
    strncat(destination, "\r\n", max_size - strlen(destination));

    if (json_object != NULL)
    {
        // Add JSON response to the body of the HTTP response
        strncat(destination, json_object, max_size - strlen(destination));
    }

    debug_printf("HTTP Response:\n%s\n------------------------------\n", destination);

    return ERR_OK;
}
