#include "tcp_server.h"

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    debug_printf("TCP Server: tcp_server_sent %u\n", len);
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE)
    {
        // We should get the data back from the client
        state->recv_len = 0;
        debug_printf("TCP Server: Waiting for buffer from client\n");
    }

    return ERR_OK;
}

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    for(int i=0; i< BUF_SIZE; i++)
    {
        state->buffer_sent[i] = rand();
    }

    state->sent_len = 0;
    debug_printf("TCP Server: Writing %ld bytes to client\n", BUF_SIZE);
    cyw43_arch_lwip_begin();
    err_t err = tcp_write(tpcb, state->buffer_sent, BUF_SIZE, TCP_WRITE_FLAG_COPY);
    cyw43_arch_lwip_end();
    if (err != ERR_OK)
    {
        debug_printf("TCP Server: Failed to write data %d\n", err);
    }
    return err;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p)
    {
        return ERR_ARG;
    }

    cyw43_arch_lwip_begin(); 
    if (p->tot_len > 0)
    {
        debug_printf("TCP Server: tcp_server_recv %d bytes err %d\n", p->tot_len, state->recv_len, err);

        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    cyw43_arch_lwip_end();

    pbuf_free(p);

    // Have we have received the whole buffer
    if (state->recv_len == BUF_SIZE)
    {
        // check it matches
        if (memcmp(state->buffer_sent, state->buffer_recv, BUF_SIZE) != 0)
        {
            debug_printf("TCP Server: Buffer mismatch\n");
            return ERR_RTE;
        }
        debug_printf("TCP Server: tcp_server_recv buffer ok\n");
        
        state->run_count++;
        state->complete = true;

        // Send another buffer
        return tcp_server_send_data(arg, state->client_pcb);
    }
    return ERR_OK;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb)
{
    debug_printf("TCP Server: tcp_server_poll_fn\nClosing connection...");
    return tcp_close_connection((TCP_SERVER_T*)arg);
}

static void tcp_server_err(void *arg, err_t err)
{
    if (err != ERR_ABRT)
    {
        debug_printf("TCP Server: tcp_client_err_fn %d\nClosing connection...", err);
        tcp_close_connection((TCP_SERVER_T*)arg);
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

    return tcp_server_send_data(arg, state->client_pcb);
}

err_t tcp_close_connection(TCP_SERVER_T *state)
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
    return err;
}

err_t tcp_server_close(TCP_SERVER_T *state)
{
    err_t err = ERR_OK;
    if (tcp_close_connection(state) == ERR_ABRT)
    {
        return ERR_ABRT;
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

    debug_printf("TCP Server: Allocate %ld bytes\n", sizeof(TCP_SERVER_T));

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