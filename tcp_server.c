#include "tcp_server.h"

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    debug_printf("TCP Server: tcp_server_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE)
    {
        debug_printf("TCP Server: Waiting for buffer from client\n");
    }

    return ERR_OK;
}

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb, size_t len)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (state == NULL || len > BUF_SIZE)
    {
        return ERR_ARG;
    }
    state->sent_len = 0;
    debug_printf("TCP Server: Writing %ld bytes to client\n", len);
    cyw43_arch_lwip_begin();
    err_t err = tcp_write(tpcb, state->buffer_sent, len, TCP_WRITE_FLAG_COPY);
    cyw43_arch_lwip_end();
    if (err != ERR_OK)
    {
        debug_printf("TCP Server: Failed to write data %d\n", err);
    }
    return err;
}

err_t handle_post_parameters(http_request_t http_request)
{
    datetime_t alarm_time = {-1, -1, -1, -1, -1, -1, -1};
    char alarm_time_buffer[4];
    int parameter_index = 0;
    char *endptr;

    if (http_request.num_parameters < 1)
    {
        debug_printf("No parameters\n");
        return ERR_VAL;
    }

    if (strncmp(http_request.parameters[parameter_index], "open", 4) == 0)
    {
        open_shades();
        return ERR_OK;
    }
    if (strncmp(http_request.parameters[parameter_index], "close", 5) == 0)
    {
        close_shades();
        return ERR_OK;
    }
    
    if (strncmp(http_request.parameters[parameter_index], "mode", 4) == 0)
    {
        parameter_index++;
        if (http_request.num_parameters < 2)
        {
            debug_printf("No mode specified\n");
            return ERR_VAL;
        }
        else if (strncmp(http_request.parameters[parameter_index], "normal", 6) == 0)
        {
            set_mode(NORMAL);
        }
        else if (strncmp(http_request.parameters[parameter_index], "important", 9) == 0)
        {
            set_mode(IMPORTANT);
        }
        else
        {
            debug_printf("Unknown mode\n");
            return ERR_VAL;
        }
    }
    else if (strncmp(http_request.parameters[parameter_index], "alarm", 5) == 0)
    {
        parameter_index++;
        if (http_request.num_parameters < 2)
        {
            debug_printf("No alarm time specified\n");
            return ERR_VAL;
        }
        if (strlen(http_request.parameters[parameter_index]) != 6)
        {
            debug_printf("Time is the wrong length\n");
            return ERR_VAL;
        }

        // Parse the hour (e.g., ""22" from "223000")
        strncpy(alarm_time_buffer, http_request.parameters[parameter_index], 2);
        alarm_time_buffer[2] = '\0';
        debug_printf("Hour %s\n", alarm_time_buffer);
        errno = 0;
        alarm_time.hour = strtol(alarm_time_buffer, &endptr, 10);
        if ( errno ||
             endptr == alarm_time_buffer ||
             alarm_time.hour < 0 ||
             alarm_time.hour > 23 )
        {
            debug_printf("Error parsing hour\n");
            return ERR_VAL;
        }

        // Parse the minute (e.g., "30" from "223000")
        strncpy(alarm_time_buffer, &http_request.parameters[parameter_index][2], 2);
        alarm_time_buffer[2] = '\0';
        debug_printf("Minute %s\n", alarm_time_buffer);
        errno = 0;
        alarm_time.min = strtol(alarm_time_buffer, &endptr, 10);
        if ( errno ||
             endptr == alarm_time_buffer ||
             alarm_time.min < 0 ||
             alarm_time.min > 59 )
        {
            debug_printf("Error parsing minute\n");
            return ERR_VAL;
        }

        // Parse the second (e.g., "00" from "223000")
        strncpy(alarm_time_buffer, &http_request.parameters[parameter_index][4], 2);
        alarm_time_buffer[2] = '\0';
        debug_printf("Second %s\n", alarm_time_buffer);
        errno = 0;
        alarm_time.sec = strtol(alarm_time_buffer, &endptr, 10);
        if ( errno ||
             endptr == alarm_time_buffer ||
             alarm_time.sec < 0 ||
             alarm_time.sec > 59 )
        {
            debug_printf("Error parsing second\n");
            return ERR_VAL;
        }

        // Parse type of alarm (open or close)
        parameter_index++;
        if (http_request.num_parameters < 3)
        {
            debug_printf("No alarm type specified\n");
            return ERR_VAL;
        }
        else if (strncmp(http_request.parameters[parameter_index], "open", 4) == 0)
        {
            debug_printf("Setting alarm for %02d:%02d:%02d\n", alarm_time.hour, alarm_time.min, alarm_time.sec);
            rtc_set_alarm(&alarm_time, (rtc_callback_t)&open_shades);
        }
        else if (strncmp(http_request.parameters[parameter_index], "close", 5) == 0)
        {
            debug_printf("Setting alarm for %02d:%02d:%02d\n", alarm_time.hour, alarm_time.min, alarm_time.sec);
            rtc_set_alarm(&alarm_time, (rtc_callback_t)&close_shades);
        }
        else
        {
            debug_printf("Unknown alarm type %s\n", http_request.parameters[parameter_index]);
            return ERR_VAL;
        }
    }
    else
    {
        debug_printf("Unknown parameter %s\n", http_request.parameters[parameter_index]);
        return ERR_VAL;
    }
    
    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    http_request_t http_request;
    char json_response[JSON_RESPONSE_SIZE];
    char http_response[BUF_SIZE];
    TCP_SERVER_T *state;

    state = (TCP_SERVER_T*)arg;
    if (p == NULL || state == NULL)
    {
        return ERR_ARG;
    }

    const u16_t buffer_left = BUF_SIZE - state->recv_len;
    const u16_t transfer_length = p->tot_len > buffer_left ? buffer_left : p->tot_len;

    cyw43_arch_lwip_begin(); 
    if (transfer_length > 0)
    {
        debug_printf("TCP Server: tcp_server_recv %d bytes err %d\n", p->tot_len, err);

        // Receive the buffer
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             transfer_length, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    cyw43_arch_lwip_end();
    pbuf_free(p);

    state->run_count++;
    state->recv_len = 0;

    debug_printf("\nTCP Server: Received Request:\n%s\n", state->buffer_recv);
    debug_printf("--------------------------------\n\n");
    
    err = parse_http_request(&http_request, (const char*)state->buffer_recv);
    if (err)
    {
        debug_printf("Error %d\n", err);
        return err;
    }

    switch (http_request.type)
    {
    case GET:
        // Construct JSON with state of shades (open or closed) and mode and send to client
        snprintf(json_response,
                 JSON_RESPONSE_SIZE,
                 "{\"state\": \"%s\"}\n"
                 "{\"mode\": \"%s\"}\n",
                 are_shades_closed() ? "closed" : "open",
                 shades_mode == NORMAL ? "normal" : "important");

        // Format the HTTP response
        err = format_http_response(http_response, BUF_SIZE, HTTP_OK, (const char *)json_response);
        break;
    case POST:
        // Handle the POST parameters (e.g., open or close)
        err = handle_post_parameters(http_request);
        if (err)
        {
            // Respond with 400 Bad request
            debug_printf("Error %d\n", err);
            err = format_http_response(http_response, BUF_SIZE, HTTP_BAD_REQUEST, NULL);
        }
        else
        {
            // Respond with 200 OK
            err = format_http_response(http_response, BUF_SIZE, HTTP_OK, NULL);
        }

        break;
    case PUT:
        // Format the HTTP response
        err = format_http_response(http_response, BUF_SIZE, HTTP_METHOD_NOT_ALLOWED, NULL);
        break;
    case DELETE:
        // Format the HTTP response
        err = format_http_response(http_response, BUF_SIZE, HTTP_METHOD_NOT_ALLOWED, NULL);
        break;
    default:
        err = format_http_response(http_response, BUF_SIZE, HTTP_BAD_REQUEST, NULL);
    }
    if (err)
    {
        // Unrecognized method, respond with 400 Bad request
        debug_printf("Error %d\n", err);
        return err;
    }

    // Copy data to the buffer to be sent
    const size_t buffer_length = MIN(strlen(http_response), BUF_SIZE);
    for (int i = 0; i < buffer_length; i++)
    {
        state->buffer_sent[i] = (uint8_t)http_response[i];
    }

    // Send the response
    err = tcp_server_send_data(state, tpcb, buffer_length);
    if (err)
    {
        debug_printf("Error %d\n", err);
    }

    return err;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb)
{
    debug_printf("TCP Server: tcp_server_poll_fn\n");
    return ERR_OK;
}

static void tcp_server_err(void *arg, err_t err)
{
    if (err != ERR_ABRT)
    {
        debug_printf("TCP Server: tcp_client_err_fn %d\n", err);
        tcp_server_close((TCP_SERVER_T *)arg);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL)
    {
        debug_printf("TCP Server: Failure in accept\n");
        return ERR_VAL;
    }
    debug_printf("TCP Server: Client connected\n");

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, UINT8_MAX);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

err_t tcp_server_close(TCP_SERVER_T *state)
{
    err_t err = ERR_OK;
    if (state->client_pcb != NULL)
    {
        tcp_arg(state->client_pcb, NULL);
        tcp_poll(state->client_pcb, NULL, 0);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK)
        {
            debug_printf("TCP Server: close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb)
    {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }

    free(state);
    return err;
}

TCP_SERVER_T *tcp_server_open()
{
    TCP_SERVER_T *state;
    int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    if (link_status != CYW43_LINK_JOIN && link_status != CYW43_LINK_UP && link_status != CYW43_LINK_NOIP)
    {
        debug_printf("TCP Server: Not connected %d\n", link_status);
        return NULL;
    }

    state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state)
    {
        debug_printf("TCP Server: Failed to allocate %ld bytes", sizeof(TCP_SERVER_T));
        return NULL;
    }

    debug_printf("Starting server at %s on port %u\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb)
    {
        debug_printf("TCP Server: Failed to create pcb\n");
        return NULL;
    }

    err_t err = tcp_bind(pcb, NULL, TCP_PORT);
    if (err)
    {
        debug_printf("TCP Server: Failed to bind to port %u\n", TCP_PORT);
        return NULL;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb)
    {
        debug_printf("TCP Server: Failed to listen\n");
        if (pcb)
        {
            tcp_close(pcb);
        }
        return NULL;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return state;
}