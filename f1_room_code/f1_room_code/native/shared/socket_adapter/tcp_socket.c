#include "transport.h"
#include "transport_internal_log.h"
#include "transport_stat.h"
#include "list.h"

#include <utils/bmem.h>

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <sys/eventfd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define MAX_TCP_SOCKETS 32
#define TCP_FRAME_HEADER_SIZE 100u
#define TCP_MAX_FRAME_SIZE (1024u * 1024u)
#define TCP_LISTEN_BACKLOG 8
#define TCP_IO_TIMEOUT_MS 3000
#define TCP_CONNECT_RETRY_COUNT 5
#define TCP_CONNECT_RETRY_DELAY_MS 100

typedef struct tcp_rx_node
{
    struct list_head list;
    transport_endpoint_t endpoint;
    uint32_t data_len;
    int reply_fd;
    uint8_t data[0];
} tcp_rx_node_t;

typedef struct tcp_reply_conn
{
    struct list_head list;
    transport_endpoint_t endpoint;
    int conn_fd;
} tcp_reply_conn_t;

typedef struct tcp_sock_ctx
{
    int in_use;
    int event_fd;
    int listen_fd;
    int stop_fd;
    char interface[IFNAMSIZ];
    transport_endpoint_t local_endpoint;
    pthread_t worker;
    pthread_mutex_t lock;
    struct list_head rx_queue;
    struct list_head reply_conns;
    size_t current_mem;
    size_t peak_mem;
    uint32_t current_rx_pkts;
    uint32_t peak_rx_pkts;
    uint64_t open_time_ms;
    transport_internal_stat_t rx_stat;
    transport_internal_stat_t tx_stat;
} tcp_sock_ctx_t;

static tcp_sock_ctx_t g_tcp_table[MAX_TCP_SOCKETS];
static pthread_mutex_t g_tcp_lock = PTHREAD_MUTEX_INITIALIZER;

static const char *tcp_endpoint_to_string(const transport_endpoint_t *ep, char *buf, size_t len)
{
    struct in_addr addr;
    char ip[INET_ADDRSTRLEN];

    if (buf == NULL || len == 0)
    {
        return "-";
    }

    if (ep == NULL)
    {
        snprintf(buf, len, "-");
        return buf;
    }

    addr.s_addr = htonl(ep->addr);
    if (inet_ntop(AF_INET, &addr, ip, sizeof(ip)) == NULL)
    {
        snprintf(ip, sizeof(ip), "0x%08x", ep->addr);
    }
    snprintf(buf, len, "%s:%u", ip, ep->port);
    return buf;
}

static uint64_t tcp_now_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
}

static uint32_t tcp_read_u32_le(const uint8_t *src)
{
    return ((uint32_t)src[0]) |
           ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}

static int tcp_set_io_timeout(int fd, int timeout_ms)
{
    struct timeval tv;

    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
    {
        return -1;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0)
    {
        return -1;
    }

    return 0;
}

static void tcp_sleep_ms(int sleep_ms)
{
    struct timespec ts;

    if (sleep_ms <= 0)
    {
        return;
    }

    ts.tv_sec = sleep_ms / 1000;
    ts.tv_nsec = (long)(sleep_ms % 1000) * 1000000L;
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR)
    {
    }
}

static int tcp_connect_errno_retryable(int err)
{
    return err == ECONNREFUSED ||
           err == ETIMEDOUT ||
           err == ENETUNREACH ||
           err == EHOSTUNREACH ||
           err == EADDRNOTAVAIL;
}

static int tcp_bind_to_device(int fd, const char *interface)
{
    size_t len;

    if (interface == NULL || interface[0] == '\0')
    {
        return 0;
    }

    len = strlen(interface);
    if (len >= IFNAMSIZ)
    {
        return -EINVAL;
    }

#ifdef SO_BINDTODEVICE
    if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, interface, len + 1) != 0)
    {
        return -errno;
    }
    return 0;
#else
    (void)fd;
    return -ENOTSUP;
#endif
}

static tcp_sock_ctx_t *tcp_find_ctx_by_fd(int fd)
{
    size_t i;

    if (fd < 0)
    {
        return NULL;
    }

    for (i = 0; i < MAX_TCP_SOCKETS; ++i)
    {
        if (g_tcp_table[i].in_use != 0 && g_tcp_table[i].event_fd == fd)
        {
            return &g_tcp_table[i];
        }
    }

    return NULL;
}

static void tcp_notify_event(tcp_sock_ctx_t *ctx)
{
    uint64_t value = 1;

    if (write(ctx->event_fd, &value, sizeof(value)) < 0)
    {
        TRANSPORT_LOGW("tcp eventfd notify failed, err=%d\n", errno);
    }
}

static int tcp_read_exact(int fd, void *buf, size_t len)
{
    uint8_t *cursor = (uint8_t *)buf;
    size_t total = 0;

    while (total < len)
    {
        ssize_t ret = recv(fd, cursor + total, len - total, 0);
        if (ret == 0)
        {
            return -1;
        }
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        total += (size_t)ret;
    }

    return 0;
}

static int tcp_write_exact(int fd, const void *buf, size_t len)
{
    const uint8_t *cursor = (const uint8_t *)buf;
    size_t total = 0;

    while (total < len)
    {
        ssize_t ret = send(fd, cursor + total, len - total, 0);
        if (ret < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return -1;
        }
        total += (size_t)ret;
    }

    return 0;
}

static int tcp_read_frame(int fd, uint8_t **frame_out, uint32_t *frame_len_out)
{
    uint8_t header[TCP_FRAME_HEADER_SIZE];
    uint8_t *frame = NULL;
    uint32_t body_len;
    uint32_t total_len;

    if (frame_out == NULL || frame_len_out == NULL)
    {
        return -1;
    }

    if (tcp_read_exact(fd, header, sizeof(header)) != 0)
    {
        return -1;
    }

    body_len = tcp_read_u32_le(header + 96);
    total_len = TCP_FRAME_HEADER_SIZE + body_len;
    if (total_len > TCP_MAX_FRAME_SIZE)
    {
        TRANSPORT_LOGW("tcp frame too large, total_len=%u\n", total_len);
        return -1;
    }

    frame = (uint8_t *)bmalloc(total_len);
    if (frame == NULL)
    {
        return -1;
    }

    memcpy(frame, header, sizeof(header));
    if (body_len > 0 && tcp_read_exact(fd, frame + TCP_FRAME_HEADER_SIZE, body_len) != 0)
    {
        bfree(frame);
        return -1;
    }

    *frame_out = frame;
    *frame_len_out = total_len;
    return 0;
}

static int tcp_queue_frame(tcp_sock_ctx_t *ctx,
                           const transport_endpoint_t *endpoint,
                           const uint8_t *frame,
                           uint32_t frame_len,
                           int reply_fd)
{
    tcp_rx_node_t *node;
    uint64_t now_ms = tcp_now_ms();

    node = (tcp_rx_node_t *)bmalloc(sizeof(*node) + frame_len);
    if (node == NULL)
    {
        return -1;
    }

    memset(node, 0, sizeof(*node));
    node->endpoint = *endpoint;
    node->data_len = frame_len;
    node->reply_fd = reply_fd;
    memcpy(node->data, frame, frame_len);

    pthread_mutex_lock(&ctx->lock);
    list_add_tail(&node->list, &ctx->rx_queue);
    ctx->current_mem += sizeof(*node) + frame_len;
    if (ctx->current_mem > ctx->peak_mem)
    {
        ctx->peak_mem = ctx->current_mem;
    }
    ctx->current_rx_pkts++;
    if (ctx->current_rx_pkts > ctx->peak_rx_pkts)
    {
        ctx->peak_rx_pkts = ctx->current_rx_pkts;
    }
    transport_update_internal_stat(&ctx->rx_stat, frame_len, now_ms);
    pthread_mutex_unlock(&ctx->lock);

    tcp_notify_event(ctx);
    return 0;
}

static void tcp_close_reply_conns(tcp_sock_ctx_t *ctx)
{
    tcp_reply_conn_t *item;
    tcp_reply_conn_t *next;

    list_for_each_entry_safe(item, next, &ctx->reply_conns, list)
    {
        list_del(&item->list);
        close(item->conn_fd);
        bfree(item);
    }
}

static void tcp_close_rx_queue(tcp_sock_ctx_t *ctx)
{
    tcp_rx_node_t *item;
    tcp_rx_node_t *next;

    list_for_each_entry_safe(item, next, &ctx->rx_queue, list)
    {
        list_del(&item->list);
        if (item->reply_fd >= 0)
        {
            close(item->reply_fd);
        }
        bfree(item);
    }
    ctx->current_mem = 0;
    ctx->current_rx_pkts = 0;
}

static void *tcp_worker_main(void *arg)
{
    tcp_sock_ctx_t *ctx = (tcp_sock_ctx_t *)arg;

    for (;;)
    {
        fd_set read_fds;
        int max_fd;
        int ready;

        FD_ZERO(&read_fds);
        FD_SET(ctx->listen_fd, &read_fds);
        FD_SET(ctx->stop_fd, &read_fds);
        max_fd = ctx->listen_fd > ctx->stop_fd ? ctx->listen_fd : ctx->stop_fd;

        ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ready < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            TRANSPORT_LOGE("tcp worker select failed, err=%d\n", errno);
            return NULL;
        }

        if (FD_ISSET(ctx->stop_fd, &read_fds))
        {
            uint64_t value;
            (void)read(ctx->stop_fd, &value, sizeof(value));
            return NULL;
        }

        if (FD_ISSET(ctx->listen_fd, &read_fds))
        {
            struct sockaddr_in peer_addr;
            socklen_t peer_len = sizeof(peer_addr);
            transport_endpoint_t endpoint;
            uint8_t *frame = NULL;
            uint32_t frame_len = 0;
            int conn_fd;

            conn_fd = accept(ctx->listen_fd, (struct sockaddr *)&peer_addr, &peer_len);
            if (conn_fd < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                TRANSPORT_LOGW("tcp accept failed, err=%d\n", errno);
                continue;
            }

            if (tcp_set_io_timeout(conn_fd, TCP_IO_TIMEOUT_MS) != 0)
            {
                close(conn_fd);
                continue;
            }

            endpoint.addr = ntohl(peer_addr.sin_addr.s_addr);
            endpoint.port = ntohs(peer_addr.sin_port);
            if (tcp_read_frame(conn_fd, &frame, &frame_len) != 0)
            {
                char ep_buf[64];

                TRANSPORT_LOGW("tcp accept read frame failed, peer=%s err=%d\n",
                               tcp_endpoint_to_string(&endpoint, ep_buf, sizeof(ep_buf)),
                               errno);
                close(conn_fd);
                continue;
            }

            if (tcp_queue_frame(ctx, &endpoint, frame, frame_len, conn_fd) != 0)
            {
                char ep_buf[64];

                TRANSPORT_LOGW("tcp accept queue frame failed, peer=%s len=%u err=%d\n",
                               tcp_endpoint_to_string(&endpoint, ep_buf, sizeof(ep_buf)),
                               frame_len,
                               errno);
                bfree(frame);
                close(conn_fd);
                continue;
            }

            bfree(frame);
        }
    }
}

static int tcp_open(const char *interface, transport_endpoint_t *ep)
{
    tcp_sock_ctx_t *ctx = NULL;
    size_t i;
    int listen_fd = -1;
    int event_fd = -1;
    int stop_fd = -1;
    int opt = 1;
    int ret;
    struct sockaddr_in addr;

    if (ep == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&g_tcp_lock);
    for (i = 0; i < MAX_TCP_SOCKETS; ++i)
    {
        if (g_tcp_table[i].in_use == 0)
        {
            ctx = &g_tcp_table[i];
            memset(ctx, 0, sizeof(*ctx));
            ctx->in_use = 1;
            INIT_LIST_HEAD(&ctx->rx_queue);
            INIT_LIST_HEAD(&ctx->reply_conns);
            break;
        }
    }
    pthread_mutex_unlock(&g_tcp_lock);

    if (ctx == NULL)
    {
        return -1;
    }
    if (interface != NULL && interface[0] != '\0')
    {
        if (strlen(interface) >= sizeof(ctx->interface))
        {
            TRANSPORT_LOGE("tcp open interface too long, if=%s\n", interface);
            goto fail;
        }
        snprintf(ctx->interface, sizeof(ctx->interface), "%s", interface);
    }

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    stop_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (listen_fd < 0 || event_fd < 0 || stop_fd < 0)
    {
        TRANSPORT_LOGE("tcp open fd create failed, listen_fd=%d event_fd=%d stop_fd=%d err=%d\n",
                       listen_fd,
                       event_fd,
                       stop_fd,
                       errno);
        goto fail;
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
    {
        TRANSPORT_LOGE("tcp open setsockopt SO_REUSEADDR failed, err=%d\n", errno);
        goto fail;
    }

    ret = tcp_bind_to_device(listen_fd, ctx->interface);
    if (ret != 0)
    {
        TRANSPORT_LOGE("tcp open bind device failed, if=%s ret=%d err=%d\n",
                       ctx->interface[0] ? ctx->interface : "-",
                       ret,
                       errno);
        goto fail;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ep->port);
    addr.sin_addr.s_addr = ep->addr == ADDR_ANY ? htonl(INADDR_ANY) : htonl(ep->addr);
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        TRANSPORT_LOGE("tcp open bind failed, addr=0x%x port=%u err=%d\n",
                       ep->addr,
                       ep->port,
                       errno);
        goto fail;
    }

    if (listen(listen_fd, TCP_LISTEN_BACKLOG) != 0)
    {
        TRANSPORT_LOGE("tcp open listen failed, addr=0x%x port=%u err=%d\n",
                       ep->addr,
                       ep->port,
                       errno);
        goto fail;
    }

    {
        struct sockaddr_in local_addr;
        socklen_t local_len = sizeof(local_addr);

        if (getsockname(listen_fd, (struct sockaddr *)&local_addr, &local_len) == 0)
        {
            ep->addr = ntohl(local_addr.sin_addr.s_addr);
            ep->port = ntohs(local_addr.sin_port);
        }
    }

    ctx->listen_fd = listen_fd;
    ctx->event_fd = event_fd;
    ctx->stop_fd = stop_fd;
    ctx->local_endpoint = *ep;
    ctx->open_time_ms = tcp_now_ms();
    pthread_mutex_init(&ctx->lock, NULL);

    {
        char ep_buf[64];

        TRANSPORT_LOGI("[STD_TCP Open success] listen_fd=%d event_fd=%d local=%s if=%s\n",
                       listen_fd,
                       event_fd,
                       tcp_endpoint_to_string(&ctx->local_endpoint,
                                              ep_buf,
                                              sizeof(ep_buf)),
                       ctx->interface[0] ? ctx->interface : "-");
    }

    if (pthread_create(&ctx->worker, NULL, tcp_worker_main, ctx) != 0)
    {
        TRANSPORT_LOGE("tcp open worker create failed, addr=0x%x port=%u err=%d\n",
                       ep->addr,
                       ep->port,
                       errno);
        goto fail;
    }

    return ctx->event_fd;

fail:
    if (listen_fd >= 0)
    {
        close(listen_fd);
    }
    if (event_fd >= 0)
    {
        close(event_fd);
    }
    if (stop_fd >= 0)
    {
        close(stop_fd);
    }
    if (ctx != NULL)
    {
        memset(ctx, 0, sizeof(*ctx));
    }
    return -1;
}

static int tcp_send_on_reply_conn(tcp_sock_ctx_t *ctx,
                                  tcp_reply_conn_t *reply_conn,
                                  const void *data,
                                  uint32_t len)
{
    uint64_t now_ms = tcp_now_ms();

    if (tcp_write_exact(reply_conn->conn_fd, data, len) != 0)
    {
        char ep_buf[64];

        TRANSPORT_LOGW("tcp reply write failed, remote=%s len=%u err=%d\n",
                       tcp_endpoint_to_string(&reply_conn->endpoint, ep_buf, sizeof(ep_buf)),
                       len,
                       errno);
        close(reply_conn->conn_fd);
        return -1;
    }

    shutdown(reply_conn->conn_fd, SHUT_RDWR);
    close(reply_conn->conn_fd);
    transport_update_internal_stat(&ctx->tx_stat, len, now_ms);
    return 0;
}

static tcp_reply_conn_t *tcp_find_reply_conn(tcp_sock_ctx_t *ctx, const transport_endpoint_t *ep)
{
    tcp_reply_conn_t *item;

    list_for_each_entry(item, &ctx->reply_conns, list)
    {
        if (item->endpoint.addr == ep->addr && item->endpoint.port == ep->port)
        {
            return item;
        }
    }

    return NULL;
}

static int tcp_open_client_conn(tcp_sock_ctx_t *ctx,
                                const transport_endpoint_t *ep,
                                int *err_out)
{
    struct sockaddr_in addr;
    int conn_fd;

    conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn_fd < 0)
    {
        if (err_out != NULL)
        {
            *err_out = errno;
        }
        return -1;
    }

    if (tcp_set_io_timeout(conn_fd, TCP_IO_TIMEOUT_MS) != 0)
    {
        if (err_out != NULL)
        {
            *err_out = errno;
        }
        close(conn_fd);
        return -1;
    }

    {
        int ret = tcp_bind_to_device(conn_fd, ctx->interface);
        if (ret != 0)
        {
            if (err_out != NULL)
            {
                *err_out = -ret;
            }
            close(conn_fd);
            return -1;
        }
    }

    if (ctx->local_endpoint.addr != ADDR_ANY)
    {
        struct sockaddr_in local_addr;

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(0);
        local_addr.sin_addr.s_addr = htonl(ctx->local_endpoint.addr);
        if (bind(conn_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) != 0)
        {
            if (err_out != NULL)
            {
                *err_out = errno;
            }
            close(conn_fd);
            return -1;
        }
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(ep->port);
    addr.sin_addr.s_addr = htonl(ep->addr);
    if (connect(conn_fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        if (err_out != NULL)
        {
            *err_out = errno;
        }
        close(conn_fd);
        return -1;
    }

    if (err_out != NULL)
    {
        *err_out = 0;
    }
    return conn_fd;
}

static int tcp_send_via_client(tcp_sock_ctx_t *ctx,
                               transport_endpoint_t *ep,
                               const void *data,
                               uint32_t len)
{
    uint8_t *reply = NULL;
    uint32_t reply_len = 0;
    int conn_fd = -1;
    int connect_err = 0;
    int connect_attempt;
    int ret = -1;
    uint64_t now_ms = tcp_now_ms();

    for (connect_attempt = 1;
         connect_attempt <= TCP_CONNECT_RETRY_COUNT;
         ++connect_attempt)
    {
        conn_fd = tcp_open_client_conn(ctx, ep, &connect_err);
        if (conn_fd >= 0)
        {
            break;
        }
        if (!tcp_connect_errno_retryable(connect_err) ||
            connect_attempt == TCP_CONNECT_RETRY_COUNT)
        {
            break;
        }
        tcp_sleep_ms(TCP_CONNECT_RETRY_DELAY_MS);
    }

    if (conn_fd < 0)
    {
        char local_buf[64];
        char remote_buf[64];

        TRANSPORT_LOGW("tcp client connect failed, local=%s remote=%s err=%d attempts=%d\n",
                       tcp_endpoint_to_string(&ctx->local_endpoint,
                                              local_buf,
                                              sizeof(local_buf)),
                       tcp_endpoint_to_string(ep, remote_buf, sizeof(remote_buf)),
                       connect_err,
                       connect_attempt);
        return -1;
    }

    if (tcp_write_exact(conn_fd, data, len) != 0)
    {
        char local_buf[64];
        char remote_buf[64];

        TRANSPORT_LOGW("tcp client write failed, local=%s remote=%s len=%u err=%d\n",
                       tcp_endpoint_to_string(&ctx->local_endpoint,
                                              local_buf,
                                              sizeof(local_buf)),
                       tcp_endpoint_to_string(ep, remote_buf, sizeof(remote_buf)),
                       len,
                       errno);
        close(conn_fd);
        return -1;
    }

    transport_update_internal_stat(&ctx->tx_stat, len, now_ms);

    if (tcp_read_frame(conn_fd, &reply, &reply_len) != 0)
    {
        char local_buf[64];
        char remote_buf[64];

        TRANSPORT_LOGW("tcp client read reply failed, local=%s remote=%s timeout_ms=%d err=%d\n",
                       tcp_endpoint_to_string(&ctx->local_endpoint,
                                              local_buf,
                                              sizeof(local_buf)),
                       tcp_endpoint_to_string(ep, remote_buf, sizeof(remote_buf)),
                       TCP_IO_TIMEOUT_MS,
                       errno);
        close(conn_fd);
        return -1;
    }

    ret = tcp_queue_frame(ctx, ep, reply, reply_len, -1);
    if (ret != 0)
    {
        char remote_buf[64];

        TRANSPORT_LOGW("tcp client queue reply failed, remote=%s len=%u err=%d\n",
                       tcp_endpoint_to_string(ep, remote_buf, sizeof(remote_buf)),
                       reply_len,
                       errno);
    }
    bfree(reply);
    close(conn_fd);
    return ret;
}

static int tcp_send(int fd, transport_endpoint_t *ep, const void *data, uint32_t len)
{
    tcp_sock_ctx_t *ctx = tcp_find_ctx_by_fd(fd);
    tcp_reply_conn_t *reply_conn = NULL;
    int ret;

    if (ctx == NULL || ep == NULL || data == NULL || len == 0)
    {
        return -1;
    }

    pthread_mutex_lock(&ctx->lock);
    reply_conn = tcp_find_reply_conn(ctx, ep);
    if (reply_conn != NULL)
    {
        list_del(&reply_conn->list);
    }
    pthread_mutex_unlock(&ctx->lock);

    if (reply_conn != NULL)
    {
        ret = tcp_send_on_reply_conn(ctx, reply_conn, data, len);
        bfree(reply_conn);
        return ret;
    }

    return tcp_send_via_client(ctx, ep, data, len);
}

static int tcp_recv(int fd, transport_endpoint_t *ep, void *buf, uint32_t buf_len, int timeout_ms)
{
    tcp_sock_ctx_t *ctx = tcp_find_ctx_by_fd(fd);
    tcp_rx_node_t *node = NULL;
    tcp_reply_conn_t *reply_conn = NULL;
    uint64_t value;
    int has_data = 0;

    if (ctx == NULL || ep == NULL || buf == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&ctx->lock);
    has_data = list_empty(&ctx->rx_queue) ? 0 : 1;
    pthread_mutex_unlock(&ctx->lock);

    if (has_data == 0 && timeout_ms >= 0)
    {
        fd_set read_fds;
        struct timeval tv;
        int ready;

        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        ready = select(fd + 1, &read_fds, NULL, NULL, &tv);
        if (ready < 0)
        {
            return errno == EINTR ? -3 : -1;
        }
        if (ready == 0)
        {
            return timeout_ms == 0 ? -3 : -2;
        }
    }

    pthread_mutex_lock(&ctx->lock);
    if (list_empty(&ctx->rx_queue))
    {
        pthread_mutex_unlock(&ctx->lock);
        return -3;
    }

    node = list_first_entry(&ctx->rx_queue, tcp_rx_node_t, list);
    list_del(&node->list);
    ctx->current_mem -= sizeof(*node) + node->data_len;
    if (ctx->current_rx_pkts > 0)
    {
        ctx->current_rx_pkts--;
    }
    pthread_mutex_unlock(&ctx->lock);

    (void)read(fd, &value, sizeof(value));

    if (node->data_len > buf_len)
    {
        if (node->reply_fd >= 0)
        {
            close(node->reply_fd);
        }
        bfree(node);
        return -1;
    }

    *ep = node->endpoint;
    memcpy(buf, node->data, node->data_len);

    if (node->reply_fd >= 0)
    {
        reply_conn = (tcp_reply_conn_t *)bmalloc(sizeof(*reply_conn));
        if (reply_conn != NULL)
        {
            memset(reply_conn, 0, sizeof(*reply_conn));
            reply_conn->endpoint = node->endpoint;
            reply_conn->conn_fd = node->reply_fd;
            pthread_mutex_lock(&ctx->lock);
            list_add_tail(&reply_conn->list, &ctx->reply_conns);
            pthread_mutex_unlock(&ctx->lock);
        }
        else
        {
            close(node->reply_fd);
        }
    }

    value = node->data_len;
    bfree(node);
    return (int)value;
}

static void tcp_close(int fd)
{
    tcp_sock_ctx_t *ctx = tcp_find_ctx_by_fd(fd);
    uint64_t value = 1;

    if (ctx == NULL)
    {
        return;
    }

    if (ctx->stop_fd >= 0)
    {
        (void)write(ctx->stop_fd, &value, sizeof(value));
    }
    if (ctx->worker != 0)
    {
        pthread_join(ctx->worker, NULL);
    }

    pthread_mutex_lock(&g_tcp_lock);
    ctx->in_use = 0;
    pthread_mutex_unlock(&g_tcp_lock);

    pthread_mutex_lock(&ctx->lock);
    tcp_close_reply_conns(ctx);
    tcp_close_rx_queue(ctx);
    pthread_mutex_unlock(&ctx->lock);

    pthread_mutex_destroy(&ctx->lock);
    close(ctx->listen_fd);
    close(ctx->event_fd);
    close(ctx->stop_fd);
    memset(ctx, 0, sizeof(*ctx));
}

static int tcp_get_info(int fd, transport_info_t *info)
{
    tcp_sock_ctx_t *ctx = tcp_find_ctx_by_fd(fd);
    uint64_t now_ms;

    if (ctx == NULL || info == NULL)
    {
        return -1;
    }

    memset(info, 0, sizeof(*info));
    now_ms = tcp_now_ms();
    info->current_mem = ctx->current_mem;
    info->peak_mem = ctx->peak_mem;
    info->current_rx_pkts = ctx->current_rx_pkts;
    info->peak_rx_pkts = ctx->peak_rx_pkts;
    info->total_run_time_ms = now_ms - ctx->open_time_ms;
    transport_fill_stat_info(&info->rx, &ctx->rx_stat, info->total_run_time_ms, TCP_MAX_FRAME_SIZE);
    transport_fill_stat_info(&info->tx, &ctx->tx_stat, info->total_run_time_ms, TCP_MAX_FRAME_SIZE);
    return 0;
}

static int tcp_setopt(int fd, transport_opt_t optname, const void *optval, uint32_t optlen)
{
    tcp_sock_ctx_t *ctx = tcp_find_ctx_by_fd(fd);
    int ret = -1;

    if (ctx == NULL || optval == NULL)
    {
        return -1;
    }

    switch (optname)
    {
        case TRANSPORT_OPT_RCVBUF:
            if (optlen == sizeof(int))
            {
                ret = setsockopt(ctx->listen_fd, SOL_SOCKET, SO_RCVBUF, optval, sizeof(int));
            }
            break;
        case TRANSPORT_OPT_SNDBUF:
            if (optlen == sizeof(int))
            {
                ret = setsockopt(ctx->listen_fd, SOL_SOCKET, SO_SNDBUF, optval, sizeof(int));
            }
            break;
        case TRANSPORT_OPT_REUSEADDR:
            if (optlen == sizeof(int))
            {
                ret = setsockopt(ctx->listen_fd, SOL_SOCKET, SO_REUSEADDR, optval, sizeof(int));
            }
            break;
        default:
            ret = -1;
            break;
    }

    return ret == 0 ? 0 : -1;
}

const transport_ops_t g_tcp_transport_ops = {
    .name = "STD_TCP",
    .open = tcp_open,
    .send = tcp_send,
    .recv = tcp_recv,
    .close = tcp_close,
    .get_info = tcp_get_info,
    .setopt = tcp_setopt,
};
