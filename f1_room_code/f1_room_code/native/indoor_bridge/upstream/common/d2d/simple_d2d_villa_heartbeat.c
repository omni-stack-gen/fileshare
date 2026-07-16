/**
 * @file
 * @brief simple 二次确认机/小门口机 25000 心跳来源服务实现。
 */

#include "simple_d2d_villa_heartbeat.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <utils/time_helper.h>
#include <utils/util.h>

#include "simple_log.h"

static uint32_t villa_hb_or_default(uint32_t value, uint32_t fallback)
{
    return value != 0 ? value : fallback;
}

static void villa_hb_sleep_step_ms(uint32_t total_ms, volatile bool *stop_requested)
{
    uint32_t elapsed = 0;

    while ((!stop_requested || !*stop_requested) && elapsed < total_ms)
    {
        uint32_t step = total_ms - elapsed;

        if (step > 100U)
        {
            step = 100U;
        }
        usleep((useconds_t)step * 1000U);
        elapsed += step;
    }
}

static void villa_hb_fill_ack(d2d_message_t *reply, int result)
{
    d2d_message_init(reply, D2D_CMD_VILLA_HEARTBEAT_ACK);
    reply->header.dev_type = D2D_DEV_INDOOR;
    reply->header.message_type = D2D_MESSAGE_REPLY_OK;
    reply->body.simple_result.result = result;
}

static int villa_hb_bus_handler(void *user_ctx,
                                const d2d_message_t *request,
                                d2d_link_kind_t ingress_link,
                                const transport_endpoint_t *src,
                                d2d_message_t *reply)
{
    return simple_d2d_villa_heartbeat_handle_request(
        (simple_d2d_villa_heartbeat_t *)user_ctx,
        request,
        ingress_link,
        src,
        reply);
}

static void villa_hb_emit(simple_d2d_villa_heartbeat_t *heartbeat,
                          const simple_d2d_villa_heartbeat_status_t *status)
{
    if (heartbeat && heartbeat->cfg.callbacks.update_cb && status)
    {
        heartbeat->cfg.callbacks.update_cb(heartbeat->cfg.callbacks.user_ctx, status);
    }
}

void simple_d2d_villa_heartbeat_config_init(
    simple_d2d_villa_heartbeat_config_t *cfg)
{
    if (!cfg)
    {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    cfg->eth.port = SIMPLE_D2D_VILLA_HEARTBEAT_DEFAULT_ETH_PORT;
    cfg->timing.interval_ms = SIMPLE_D2D_VILLA_HEARTBEAT_DEFAULT_INTERVAL_MS;
    cfg->timing.ttl_ms = SIMPLE_D2D_VILLA_HEARTBEAT_DEFAULT_TTL_MS;
    cfg->timing.timeout_ms = 5000U;
}

int simple_d2d_villa_heartbeat_build_request(
    const simple_d2d_villa_heartbeat_config_t *cfg,
    d2d_message_t *message)
{
    const char *model;

    if (!cfg || !message || !IS_VALID_STRING(cfg->identity.local_device_id) ||
        !IS_VALID_STRING(cfg->identity.peer_device_id))
    {
        return -EINVAL;
    }

    model = IS_VALID_STRING(cfg->text.dev_model) ?
            cfg->text.dev_model : "simple_secondary_confirm";

    memset(message, 0, sizeof(*message));
    d2d_message_init(message, D2D_CMD_VILLA_HEARTBEAT_REQ);
    message->header.dev_type = D2D_DEV_VILLA_DOOR;
    message->header.message_type = D2D_MESSAGE_REQUEST;
    snprintf(message->header.orig_addr, sizeof(message->header.orig_addr),
             "%s", cfg->identity.local_device_id);
    snprintf(message->header.dest_addr, sizeof(message->header.dest_addr),
             "%s", cfg->identity.peer_device_id);

    message->body.heartbeat.dev_type = D2D_DEV_VILLA_DOOR;
    message->body.heartbeat.dev_model = (char *)model;
    message->body.heartbeat.id = (char *)cfg->identity.local_device_id;
    if (IS_VALID_STRING(cfg->text.sys_ver))
    {
        message->body.heartbeat.sys_ver.present = true;
        message->body.heartbeat.sys_ver.value = (char *)cfg->text.sys_ver;
    }
    if (IS_VALID_STRING(cfg->text.app_ver))
    {
        message->body.heartbeat.app_ver.present = true;
        message->body.heartbeat.app_ver.value = (char *)cfg->text.app_ver;
    }
    return 0;
}

int simple_d2d_villa_heartbeat_init(
    simple_d2d_villa_heartbeat_t *heartbeat,
    const simple_d2d_villa_heartbeat_config_t *cfg)
{
    if (!heartbeat || !cfg || cfg->role == 0 ||
        !IS_VALID_STRING(cfg->identity.local_device_id))
    {
        return -EINVAL;
    }
    if (cfg->role == SIMPLE_D2D_VILLA_HEARTBEAT_ROLE_CLIENT &&
        (!IS_VALID_STRING(cfg->identity.peer_device_id) || cfg->eth.peer_addr == 0))
    {
        return -EINVAL;
    }

    memset(heartbeat, 0, sizeof(*heartbeat));
    heartbeat->cfg = *cfg;
    heartbeat->cfg.eth.port = cfg->eth.port ? cfg->eth.port :
                              SIMPLE_D2D_VILLA_HEARTBEAT_DEFAULT_ETH_PORT;
    heartbeat->cfg.timing.interval_ms = villa_hb_or_default(
        cfg->timing.interval_ms,
        SIMPLE_D2D_VILLA_HEARTBEAT_DEFAULT_INTERVAL_MS);
    heartbeat->cfg.timing.ttl_ms = villa_hb_or_default(
        cfg->timing.ttl_ms,
        SIMPLE_D2D_VILLA_HEARTBEAT_DEFAULT_TTL_MS);
    heartbeat->cfg.timing.timeout_ms = villa_hb_or_default(cfg->timing.timeout_ms, 5000U);

    if (pthread_mutex_init(&heartbeat->lock, NULL) != 0)
    {
        return -errno;
    }
    heartbeat->lock_ready = true;
    return 0;
}

void simple_d2d_villa_heartbeat_deinit(simple_d2d_villa_heartbeat_t *heartbeat)
{
    if (!heartbeat)
    {
        return;
    }
    if (heartbeat->started || heartbeat->thread_started)
    {
        simple_d2d_villa_heartbeat_stop(heartbeat);
        return;
    }
    if (heartbeat->lock_ready)
    {
        pthread_mutex_destroy(&heartbeat->lock);
    }
    memset(heartbeat, 0, sizeof(*heartbeat));
}

int simple_d2d_villa_heartbeat_handle_request(
    simple_d2d_villa_heartbeat_t *heartbeat,
    const d2d_message_t *request,
    d2d_link_kind_t ingress_link,
    const transport_endpoint_t *src,
    d2d_message_t *reply)
{
    simple_d2d_villa_heartbeat_status_t status;
    const d2d_heartbeat_t *body;

    (void)ingress_link;
    if (!heartbeat || !request || !reply ||
        request->header.cmd != D2D_CMD_VILLA_HEARTBEAT_REQ)
    {
        return -EINVAL;
    }

    villa_hb_fill_ack(reply, 0);
    body = &request->body.heartbeat;
    if (body->dev_type != D2D_DEV_VILLA_DOOR ||
        !IS_VALID_STRING(body->id))
    {
        SIMPLE_LOGW("villa heartbeat rejected: reason=invalid_body dev_type=%d id=%s\n",
                    body->dev_type,
                    body->id ? body->id : "-");
        return 0;
    }
    if (IS_VALID_STRING(heartbeat->cfg.identity.peer_device_id) &&
        strcmp(body->id, heartbeat->cfg.identity.peer_device_id) != 0)
    {
        SIMPLE_LOGW("villa heartbeat rejected: reason=device_mismatch expected=%s id=%s\n",
                    heartbeat->cfg.identity.peer_device_id,
                    body->id);
        return 0;
    }
    if (heartbeat->cfg.eth.peer_addr != 0 && (!src || src->addr != heartbeat->cfg.eth.peer_addr))
    {
        SIMPLE_LOGW("villa heartbeat rejected: reason=addr_mismatch expected=0x%08x "
                    "src=0x%08x\n",
                    heartbeat->cfg.eth.peer_addr,
                    src ? src->addr : 0U);
        return 0;
    }

    memset(&status, 0, sizeof(status));
    status.dev_type = body->dev_type;
    status.endpoint = src ? *src : (transport_endpoint_t){0};
    status.observed_ms = time_now();
    status.ttl_ms = heartbeat->cfg.timing.ttl_ms;
    status.online = true;
    safe_strncpy(status.device_id, body->id, sizeof(status.device_id));
    if (body->mac.present && body->mac.value)
    {
        safe_strncpy(status.mac, body->mac.value, sizeof(status.mac));
    }
    safe_strncpy(status.dev_model, body->dev_model, sizeof(status.dev_model));
    if (body->sys_ver.present && body->sys_ver.value)
    {
        safe_strncpy(status.sys_ver, body->sys_ver.value, sizeof(status.sys_ver));
    }
    if (body->app_ver.present && body->app_ver.value)
    {
        safe_strncpy(status.app_ver, body->app_ver.value, sizeof(status.app_ver));
    }

    pthread_mutex_lock(&heartbeat->lock);
    heartbeat->status = status;
    pthread_mutex_unlock(&heartbeat->lock);

    villa_hb_fill_ack(reply, 1);
    SIMPLE_LOGI("villa heartbeat accepted: device_id=%s src=0x%08x:%u ttl=%u\n",
                status.device_id,
                status.endpoint.addr,
                status.endpoint.port,
                status.ttl_ms);
    villa_hb_emit(heartbeat, &status);
    return 0;
}

static int villa_hb_send_client_heartbeat(simple_d2d_villa_heartbeat_t *heartbeat)
{
    d2d_adapter_request_t tx;
    d2d_message_t message;
    int ret;

    if (!heartbeat)
    {
        return -EINVAL;
    }
    ret = simple_d2d_villa_heartbeat_build_request(&heartbeat->cfg, &message);
    if (ret != 0)
    {
        return ret;
    }

    memset(&tx, 0, sizeof(tx));
    tx.link_kind = D2D_LINK_ETH;
    tx.remote_endpoint.addr = heartbeat->cfg.eth.peer_addr;
    tx.remote_endpoint.port = heartbeat->cfg.eth.port;
    tx.opaque = D2D_CMD_VILLA_HEARTBEAT_REQ;

    if (heartbeat->cfg.shared_service)
    {
        if (simple_d2d_service_adapter(heartbeat->cfg.shared_service))
        {
            ret = d2d_adapter_send_request(
                simple_d2d_service_adapter(heartbeat->cfg.shared_service),
                &message,
                &tx);
        }
        else if (simple_d2d_service_forwarder(heartbeat->cfg.shared_service))
        {
            ret = d2d_forwarder_send_request(
                simple_d2d_service_forwarder(heartbeat->cfg.shared_service),
                &message,
                &tx);
        }
        else
        {
            ret = -EINVAL;
        }
    }
    else
    {
        ret = simple_d2d_bus_send_request(&heartbeat->bus, &message, &tx);
    }
    if (ret == 0)
    {
        SIMPLE_LOGI("villa heartbeat sent: device_id=%s target=0x%08x:%u\n",
                    heartbeat->cfg.identity.local_device_id,
                    tx.remote_endpoint.addr,
                    tx.remote_endpoint.port);
    }
    else
    {
        SIMPLE_LOGW("villa heartbeat send failed: ret=%d device_id=%s "
                    "target=0x%08x:%u\n",
                    ret,
                    heartbeat->cfg.identity.local_device_id ?
                        heartbeat->cfg.identity.local_device_id : "-",
                    tx.remote_endpoint.addr,
                    tx.remote_endpoint.port);
    }
    return ret;
}

static void *villa_hb_client_thread(void *arg)
{
    simple_d2d_villa_heartbeat_t *heartbeat =
        (simple_d2d_villa_heartbeat_t *)arg;

    (void)villa_hb_send_client_heartbeat(heartbeat);
    while (!heartbeat->stop_requested)
    {
        villa_hb_sleep_step_ms(heartbeat->cfg.timing.interval_ms,
                               (volatile bool *)&heartbeat->stop_requested);
        if (!heartbeat->stop_requested)
        {
            (void)villa_hb_send_client_heartbeat(heartbeat);
        }
    }
    return NULL;
}

static void villa_hb_expire(simple_d2d_villa_heartbeat_t *heartbeat)
{
    simple_d2d_villa_heartbeat_status_t status;
    bool expired = false;
    uint64_t now_ms = time_now();

    pthread_mutex_lock(&heartbeat->lock);
    if (heartbeat->status.online &&
        heartbeat->status.observed_ms != 0 &&
        heartbeat->status.ttl_ms != 0 &&
        now_ms >= heartbeat->status.observed_ms + heartbeat->status.ttl_ms)
    {
        heartbeat->status.online = false;
        status = heartbeat->status;
        expired = true;
    }
    pthread_mutex_unlock(&heartbeat->lock);

    if (expired)
    {
        SIMPLE_LOGW("villa heartbeat expired: device_id=%s ttl=%u\n",
                    status.device_id,
                    status.ttl_ms);
        villa_hb_emit(heartbeat, &status);
    }
}

static void *villa_hb_server_thread(void *arg)
{
    simple_d2d_villa_heartbeat_t *heartbeat =
        (simple_d2d_villa_heartbeat_t *)arg;

    while (!heartbeat->stop_requested)
    {
        villa_hb_sleep_step_ms(1000U, (volatile bool *)&heartbeat->stop_requested);
        if (!heartbeat->stop_requested)
        {
            villa_hb_expire(heartbeat);
        }
    }
    return NULL;
}

int simple_d2d_villa_heartbeat_start(
    simple_d2d_villa_heartbeat_t *heartbeat,
    const simple_d2d_villa_heartbeat_config_t *cfg)
{
    simple_d2d_bus_config_t bus_cfg;
    simple_d2d_bus_app_t app;
    transport_endpoint_t eth_local;
    int ret;

    if (!heartbeat || heartbeat->started)
    {
        return -EINVAL;
    }

    ret = simple_d2d_villa_heartbeat_init(heartbeat, cfg);
    if (ret != 0)
    {
        return ret;
    }

    if (!heartbeat->cfg.shared_service)
    {
        memset(&eth_local, 0, sizeof(eth_local));
        eth_local.addr = heartbeat->cfg.eth.local_addr;
        eth_local.port = heartbeat->cfg.eth.port;

        simple_d2d_bus_config_init(&bus_cfg);
        bus_cfg.mode = SIMPLE_D2D_SERVICE_MODE_ADAPTER_ETH;
        bus_cfg.local_dev_type =
            heartbeat->cfg.role == SIMPLE_D2D_VILLA_HEARTBEAT_ROLE_CLIENT ?
            D2D_DEV_VILLA_DOOR : D2D_DEV_INDOOR;
        bus_cfg.eth_if = heartbeat->cfg.eth.if_name;
        bus_cfg.eth_local = eth_local;
        bus_cfg.eth_peer.addr = heartbeat->cfg.eth.peer_addr;
        bus_cfg.eth_peer.port = heartbeat->cfg.eth.port;
        bus_cfg.timeout_ms = heartbeat->cfg.timing.timeout_ms;
        bus_cfg.observer = heartbeat->cfg.callbacks.observer.observer;
        bus_cfg.observer_user_ctx =
            heartbeat->cfg.callbacks.observer.observer_user_ctx;

        if (heartbeat->cfg.role == SIMPLE_D2D_VILLA_HEARTBEAT_ROLE_SERVER)
        {
            memset(&app, 0, sizeof(app));
            app.request_cmd = D2D_CMD_VILLA_HEARTBEAT_REQ;
            app.reply_cmd = D2D_CMD_VILLA_HEARTBEAT_ACK;
            app.app_name = "villa_heartbeat";
            app.handler = villa_hb_bus_handler;
            app.user_ctx = heartbeat;
            app.bridge_local = true;
            ret = simple_d2d_bus_register_app(&heartbeat->bus, &app);
            if (ret != 0)
            {
                simple_d2d_villa_heartbeat_stop(heartbeat);
                return ret;
            }
        }

        ret = simple_d2d_bus_start(&heartbeat->bus, &bus_cfg);
        if (ret != 0)
        {
            simple_d2d_villa_heartbeat_stop(heartbeat);
            return ret;
        }
    }

    heartbeat->started = true;
    ret = pthread_create(&heartbeat->thread,
                         NULL,
                         heartbeat->cfg.role == SIMPLE_D2D_VILLA_HEARTBEAT_ROLE_CLIENT ?
                             villa_hb_client_thread : villa_hb_server_thread,
                         heartbeat);
    if (ret != 0)
    {
        simple_d2d_villa_heartbeat_stop(heartbeat);
        return -ret;
    }
    heartbeat->thread_started = true;
    SIMPLE_LOGI("villa heartbeat started: role=%d local=0x%08x:%u peer=0x%08x:%u\n",
                heartbeat->cfg.role,
                heartbeat->cfg.eth.local_addr,
                heartbeat->cfg.eth.port,
                heartbeat->cfg.eth.peer_addr,
                heartbeat->cfg.eth.port);
    return 0;
}

void simple_d2d_villa_heartbeat_stop(simple_d2d_villa_heartbeat_t *heartbeat)
{
    if (!heartbeat)
    {
        return;
    }
    heartbeat->stop_requested = true;
    if (heartbeat->thread_started)
    {
        pthread_join(heartbeat->thread, NULL);
    }
    if (!heartbeat->cfg.shared_service)
    {
        simple_d2d_bus_stop(&heartbeat->bus);
    }
    if (heartbeat->started)
    {
        SIMPLE_LOGI("villa heartbeat stopped\n");
    }
    if (heartbeat->lock_ready)
    {
        pthread_mutex_destroy(&heartbeat->lock);
    }
    memset(heartbeat, 0, sizeof(*heartbeat));
}

size_t simple_d2d_villa_heartbeat_snapshot(
    simple_d2d_villa_heartbeat_t *heartbeat,
    simple_d2d_villa_heartbeat_status_t *out,
    size_t capacity)
{
    size_t count = 0;

    if (!heartbeat || !out || capacity == 0)
    {
        return 0;
    }
    pthread_mutex_lock(&heartbeat->lock);
    if (heartbeat->status.online)
    {
        out[0] = heartbeat->status;
        count = 1;
    }
    pthread_mutex_unlock(&heartbeat->lock);
    return count;
}

simple_d2d_service_t *simple_d2d_villa_heartbeat_bus_service(
    simple_d2d_villa_heartbeat_t *heartbeat)
{
    if (!heartbeat || heartbeat->cfg.shared_service)
    {
        return heartbeat ? heartbeat->cfg.shared_service : NULL;
    }
    return simple_d2d_bus_service(&heartbeat->bus);
}
