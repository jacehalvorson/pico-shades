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

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    char json_response[JSON_RESPONSE_SIZE];
    char http_response[BUF_SIZE];
    datetime_t current_time;
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

    if (strncmp((char *)state->buffer_recv, "GET", 3) == 0)
    {
        debug_printf("TCP Server: Received GET Request\n");
        debug_printf("%s\n", state->buffer_recv);
        debug_printf("--------------------------------\n\n");

        // Respond with state of shades (open or closed)
        snprintf(json_response, JSON_RESPONSE_SIZE, "{\"state\": \"%s\"}\n", are_shades_closed() ? "closed" : "open");

        snprintf(
            http_response, // Add to the end of the string
            BUF_SIZE, // Remaining space
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Content-Type: application/json\r\n"
        );

        // Add current date to the HTTP response
        if (rtc_get_datetime(&current_time))
        {
            const char *month_names[] = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
            const char *day_names[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

            snprintf(
                http_response + strlen(http_response), // Add to the end of the string
                BUF_SIZE - strlen(http_response), // Remaining space
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
            debug_printf("Unable to set date in HTTP response\n");
            strncat(
                http_response + strlen(http_response), // Add to the end of the string
                "Date: Sun, 1 Jan 1970 00:00:00 CST\r\n",
                BUF_SIZE - strlen(http_response) // Remaining space
            );
        }

        snprintf(
            http_response + strlen(http_response), // Add to the end of the string
            BUF_SIZE - strlen(http_response), // Remaining space
            "Server: Shades Machine\r\n"
            "Content-Length: %d\r\n"
            "\r\n",
            strlen(json_response)
        );

        // Add JSON response to the body of the HTTP response
        strncat(http_response, json_response, BUF_SIZE - strlen(http_response));
        debug_printf("HTTP Response:\n%s\n------------------------------\n", http_response);

        // Copy data to the buffer to be sent
        const size_t buffer_length = MIN(strlen(http_response), BUF_SIZE);
        for (int i = 0; i < buffer_length; i++)
        {
            state->buffer_sent[i] = (uint8_t)http_response[i];
        }

        // Send the response
        tcp_server_send_data(state, tpcb, buffer_length);
    }
    else if (strncmp((char *)state->buffer_recv, "POST", 4) == 0)
    {
        debug_printf("TCP Server: Received POST Request\n");
        debug_printf("%s\n", state->buffer_recv);
        debug_printf("--------------------------------\n\n");
    }
    else
    {
        debug_printf("TCP Server: Received Unknown Request\n");
        debug_printf("%s\n", state->buffer_recv);
        debug_printf("--------------------------------\n\n");
    }

    state->run_count++;
    state->recv_len = 0;
    pbuf_free(p);
    return ERR_OK;
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
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_SEC * 2);
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