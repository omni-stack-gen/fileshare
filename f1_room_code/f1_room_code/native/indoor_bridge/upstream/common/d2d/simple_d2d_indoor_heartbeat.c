/**
 * @file
 * @brief simple 室内机 20010 心跳 presence 来源实现。
 */

#include "simple_d2d_indoor_heartbeat.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <utils/time_helper.h>
#include <utils/util.h>

#include "simple_log.h"
#include "simple_plc_addr.h"

static uint32_t indoor_hb_or_default(uint32_t value, uint32_t fallback)
{
    return value != 0 ? value : fallback;
}

static void indoor_hb_sleep_step_ms(uint32_t total_ms,
                                    volatile bool *stop_requested)
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

static void indoor_hb_fill_ack(d2d_message_t *reply, int result)
{
    d2d_message_init(reply, D2D_CMD_INDOOR_HEARTBEAT_ACK);
    reply->header.dev_type = D2D_DEV_SIGNAL_EXTENDER;
    reply->header.message_type = D2D_MESSAGE_REPLY_OK;
    reply->body.ack_sync.result = result;
    reply->body.ack_sync.dev_type.present = true;
    reply->body.ack_sync.dev_type.value = D2D_DEV_SIGNAL_EXTENDER;
    reply->body.ack_sync.dev_model.present = true;
    reply->body.ack_sync.dev_model.value = "simple_converter";
    reply->body.ack_sync.sync.present = true;
    reply->body.ack_sync.sync.value = 0;
}

static int indoor_hb_bus_handler(void *user_ctx,
                                 const d2d_message_t *request,
                                 d2d_link_kind_t ingress_link,
                                 const transport_endpoint_t *src,
                                 d2d_message_t *reply)
{
    return simple_d2d_indoor_heartbeat_handle_request(
        (simple_d2d_indoor_heartbeat_t *)user_ctx,
        request,
        ingress_link,
        src,
        reply);
}

static void indoor_hb_emit(simple_d2d_indoor_heartbeat_t *heartbeat,
                           const simple_d2d_indoor_heartbeat_status_t *status)
{
    if (heartbeat && heartbeat->cfg.callbacks.update_cb && status)
    {
        heartbeat->cfg.callbacks.update_cb(heartbeat->cfg.callbacks.user_ctx, status);
    }
}

void simple_d2d_indoor_heartbeat_config_init(
    simple_d2d_indoor_heartbeat_config_t *cfg)
{
    if (!cfg)
    {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    cfg->plc.port = SIMPLE_D2D_INDOOR_HEARTBEAT_DEFAULT_PLC_PORT;
    cfg->timing.interval_ms = SIMPLE_D2D_INDOOR_HEARTBEAT_DEFAULT_INTERVAL_MS;
    cfg->timing.ttl_ms = SIMPLE_D2D_INDOOR_HEARTBEAT_DEFAULT_TTL_MS;
    cfg->timing.timeout_ms = 5000U;
}

int simple_d2d_indoor_heartbeat_build_request(
    const simple_d2d_indoor_heartbeat_config_t *cfg,
    d2d_message_t *message)
{
    const char *model;
    const char *sync_ver;

    if (!cfg || !message ||
        !IS_VALID_STRING(cfg->identity.local_device_id) ||
        !IS_VALID_STRING(cfg->identity.peer_device_id) ||
        !IS_VALID_STRING(cfg->room_id))
    {
        return -EINVAL;
    }

    model = IS_VALID_STRING(cfg->text.dev_model) ?
            cfg->text.dev_model : "simple_indoor";
    sync_ver = IS_VALID_STRING(cfg->text.sync_ver) ?
               cfg->text.sync_ver : "simple-indoor";

    memset(message, 0, sizeof(*message));
    d2d_message_init(message, D2D_CMD_INDOOR_HEARTBEAT_REQ);
    message->header.dev_type = D2D_DEV_INDOOR;
    message->header.message_type = D2D_MESSAGE_REQUEST;
    snprintf(message->header.orig_addr, sizeof(message->header.orig_addr),
             "%s", cfg->identity.local_device_id);
    snprintf(message->header.dest_addr, sizeof(message->header.dest_addr),
             "%s", cfg->identity.peer_device_id);

    message->body.heartbeat.dev_type = D2D_DEV_INDOOR;
    message->body.heartbeat.dev_model = (char *)model;
    message->body.heartbeat.id = (char *)cfg->room_id;
    message->body.heartbeat.net_type.present = true;
    message->body.heartbeat.net_type.value = 2;
    message->body.heartbeat.sync_ver.present = true;
    message->body.heartbeat.sync_ver.value = (char *)sync_ver;
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

int simple_d2d_indoor_heartbeat_init(
    simple_d2d_indoor_heartbeat_t *heartbeat,
    const simple_d2d_indoor_heartbeat_config_t *cfg)
{
    if (!heartbeat || !cfg || cfg->role == 0 ||
        !IS_VALID_STRING(cfg->identity.local_device_id))
    {
        return -EINVAL;
    }
    if (cfg->role == SIMPLE_D2D_INDOOR_HEARTBEAT_ROLE_CLIENT &&
        (!IS_VALID_STRING(cfg->identity.peer_device_id) ||
         !IS_VALID_STRING(cfg->room_id) ||
         cfg->plc.peer_addr == 0))
    {
        return -EINVAL;
    }

    memset(heartbeat, 0, sizeof(*heartbeat));
    heartbeat->cfg = *cfg;
    heartbeat->cfg.plc.port = cfg->plc.port ? cfg->plc.port :
                              SIMPLE_D2D_INDOOR_HEARTBEAT_DEFAULT_PLC_PORT;
    heartbeat->cfg.timing.interval_ms = indoor_hb_or_default(
        cfg->timing.interval_ms,
        SIMPLE_D2D_INDOOR_HEARTBEAT_DEFAULT_INTERVAL_MS);
    heartbeat->cfg.timing.ttl_ms = indoor_hb_or_default(
        cfg->timing.ttl_ms,
        SIMPLE_D2D_INDOOR_HEARTBEAT_DEFAULT_TTL_MS);
    heartbeat->cfg.timing.timeout_ms = indoor_hb_or_default(cfg->timing.timeout_ms, 5000U);

    if (pthread_mutex_init(&heartbeat->lock, NULL) != 0)
    {
        return -errno;
    }
    heartbeat->lock_ready = true;
    return 0;
}

void simple_d2d_indoor_heartbeat_deinit(
    simple_d2d_indoor_heartbeat_t *heartbeat)
{
    if (!heartbeat)
    {
        return;
    }
    if (heartbeat->started || heartbeat->thread_started)
    {
        simple_d2d_indoor_heartbeat_stop(heartbeat);
        return;
    }
    if (heartbeat->lock_ready)
    {
        pthread_mutex_destroy(&heartbeat->lock);
    }
    memset(heartbeat, 0, sizeof(*heartbeat));
}

int simple_d2d_indoor_heartbeat_handle_request(
    simple_d2d_indoor_heartbeat_t *heartbeat,
    const d2d_message_t *request,
    d2d_link_kind_t ingress_link,
    const transport_endpoint_t *src,
    d2d_message_t *reply)
{
    simple_d2d_indoor_heartbeat_status_t status;
    const d2d_heartbeat_t *body;
    uint32_t plc_addr = 0;

    if (!heartbeat || !request || !reply ||
        ingress_link != D2D_LINK_PLC ||
        request->header.cmd != D2D_CMD_INDOOR_HEARTBEAT_REQ)
    {
        return -EINVAL;
    }

    indoor_hb_fill_ack(reply, 0);
    body = &request->body.heartbeat;
    if (body->dev_type != D2D_DEV_INDOOR ||
        !IS_VALID_STRING(body->id) ||
        !IS_VALID_STRING(body->dev_model) ||
        !body->sync_ver.present ||
        !IS_VALID_STRING(body->sync_ver.value) ||
        !IS_VALID_STRING(request->header.orig_addr) ||
        simple_plc_addr_from_device_id(request->header.orig_addr, &plc_addr) != 0)
    {
        SIMPLE_LOGW("indoor heartbeat rejected: reason=invalid_body dev_type=%d "
                    "id=%s orig=%s\n",
                    body->dev_type,
                    body->id ? body->id : "-",
                    request->header.orig_addr);
        return 0;
    }

    memset(&status, 0, sizeof(status));
    status.dev_type = body->dev_type;
    status.endpoint = src ? *src : (transport_endpoint_t){0};
    status.plc_addr = plc_addr;
    status.observed_ms = time_now();
    status.ttl_ms = heartbeat->cfg.timing.ttl_ms;
    status.online = true;
    safe_strncpy(status.room_id, body->id, sizeof(status.room_id));
    safe_strncpy(status.device_id, request->header.orig_addr, sizeof(status.device_id));
    safe_strncpy(status.dev_model, body->dev_model, sizeof(status.dev_model));
    if (body->sys_ver.present && body->sys_ver.value)
    {
        safe_strncpy(status.sys_ver, body->sys_ver.value, sizeof(status.sys_ver));
    }
    if (body->app_ver.present && body->app_ver.value)
    {
        safe_strncpy(status.app_ver, body->app_ver.value, sizeof(status.app_ver));
    }
    if (body->sync_ver.present && body->sync_ver.value)
    {
        safe_strncpy(status.sync_ver, body->sync_ver.value, sizeof(status.sync_ver));
    }

    pthread_mutex_lock(&heartbeat->lock);
    heartbeat->rooms[0] = status;
    pthread_mutex_unlock(&heartbeat->lock);

    indoor_hb_fill_ack(reply, 1);
    SIMPLE_LOGI("indoor heartbeat accepted: room=%s device_id=%s "
                "plc_addr=0x%08x ttl=%u\n",
                status.room_id,
                status.device_id,
                status.plc_addr,
                status.ttl_ms);
    indoor_hb_emit(heartbeat, &status);
    return 0;
}

static int indoor_hb_send_client_heartbeat(
    simple_d2d_indoor_heartbeat_t *heartbeat)
{
    d2d_adapter_request_t tx;
    d2d_message_t message;
    d2d_adapter_t *adapter;
    int ret;

    if (!heartbeat)
    {
        return -EINVAL;
    }
    ret = simple_d2d_indoor_heartbeat_build_request(&heartbeat->cfg, &message);
    if (ret != 0)
    {
        return ret;
    }

    adapter = heartbeat->cfg.shared_service ?
              simple_d2d_service_adapter(heartbeat->cfg.shared_service) :
              simple_d2d_bus_service(&heartbeat->bus) ?
                  simple_d2d_service_adapter(simple_d2d_bus_service(&heartbeat->bus)) :
                  NULL;
    if (!adapter)
    {
        return -ENODEV;
    }

    memset(&tx, 0, sizeof(tx));
    tx.link_kind = D2D_LINK_PLC;
    tx.remote_endpoint.addr = heartbeat->cfg.plc.peer_addr;
    tx.remote_endpoint.port = heartbeat->cfg.plc.port;
    tx.opaque = D2D_CMD_INDOOR_HEARTBEAT_REQ;

    ret = d2d_adapter_send_request(adapter, &message, &tx);
    if (ret == 0)
    {
        SIMPLE_LOGI("indoor heartbeat sent: room=%s device_id=%s "
                    "target=0x%08x:%u\n",
                    heartbeat->cfg.room_id,
                    heartbeat->cfg.identity.local_device_id,
                    tx.remote_endpoint.addr,
                    tx.remote_endpoint.port);
    }
    else
    {
        SIMPLE_LOGW("indoor heartbeat send failed: ret=%d room=%s "
                    "device_id=%s target=0x%08x:%u\n",
                    ret,
                    heartbeat->cfg.room_id ? heartbeat->cfg.room_id : "-",
                    heartbeat->cfg.identity.local_device_id ?
                        heartbeat->cfg.identity.local_device_id : "-",
                    tx.remote_endpoint.addr,
                    tx.remote_endpoint.port);
    }
    return ret;
}

static void *indoor_hb_client_thread(void *arg)
{
    simple_d2d_indoor_heartbeat_t *heartbeat =
        (simple_d2d_indoor_heartbeat_t *)arg;

    (void)indoor_hb_send_client_heartbeat(heartbeat);
    while (!heartbeat->stop_requested)
    {
        indoor_hb_sleep_step_ms(heartbeat->cfg.timing.interval_ms,
                                (volatile bool *)&heartbeat->stop_requested);
        if (!heartbeat->stop_requested)
        {
            (void)indoor_hb_send_client_heartbeat(heartbeat);
        }
    }
    return NULL;
}

int simple_d2d_indoor_heartbeat_expire(
    simple_d2d_indoor_heartbeat_t *heartbeat,
    uint64_t now_ms)
{
    simple_d2d_indoor_heartbeat_status_t status;
    bool expired = false;

    if (!heartbeat || !heartbeat->lock_ready)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&heartbeat->lock);
    if (heartbeat->rooms[0].online &&
        heartbeat->rooms[0].observed_ms != 0 &&
        heartbeat->rooms[0].ttl_ms != 0 &&
        now_ms >= heartbeat->rooms[0].observed_ms + heartbeat->rooms[0].ttl_ms)
    {
        heartbeat->rooms[0].online = false;
        status = heartbeat->rooms[0];
        expired = true;
    }
    pthread_mutex_unlock(&heartbeat->lock);

    if (expired)
    {
        SIMPLE_LOGW("indoor heartbeat expired: room=%s device_id=%s ttl=%u\n",
                    status.room_id,
                    status.device_id,
                    status.ttl_ms);
        indoor_hb_emit(heartbeat, &status);
    }
    return expired ? 1 : 0;
}

static void *indoor_hb_server_thread(void *arg)
{
    simple_d2d_indoor_heartbeat_t *heartbeat =
        (simple_d2d_indoor_heartbeat_t *)arg;

    while (!heartbeat->stop_requested)
    {
        indoor_hb_sleep_step_ms(1000U,
                                (volatile bool *)&heartbeat->stop_requested);
        if (!heartbeat->stop_requested)
        {
            (void)simple_d2d_indoor_heartbeat_expire(heartbeat,
                                                     time_now());
        }
    }
    return NULL;
}

static int indoor_hb_start_owned_bus(simple_d2d_indoor_heartbeat_t *heartbeat)
{
    simple_d2d_bus_config_t bus_cfg;
    simple_d2d_bus_app_t app;
    transport_endpoint_t plc_local;
    int ret;

    if (heartbeat->cfg.shared_service)
    {
        return 0;
    }

    memset(&plc_local, 0, sizeof(plc_local));
    plc_local.addr = heartbeat->cfg.plc.local_addr;
    plc_local.port = heartbeat->cfg.plc.port;

    simple_d2d_bus_config_init(&bus_cfg);
    bus_cfg.mode = SIMPLE_D2D_SERVICE_MODE_ADAPTER_PLC;
    bus_cfg.local_dev_type =
        heartbeat->cfg.role == SIMPLE_D2D_INDOOR_HEARTBEAT_ROLE_SERVER ?
        D2D_DEV_SIGNAL_EXTENDER : D2D_DEV_INDOOR;
    bus_cfg.plc_if = heartbeat->cfg.plc.if_name;
    bus_cfg.plc_local = plc_local;
    bus_cfg.timeout_ms = heartbeat->cfg.timing.timeout_ms;
    bus_cfg.observer = heartbeat->cfg.callbacks.observer.observer;
    bus_cfg.observer_user_ctx =
        heartbeat->cfg.callbacks.observer.observer_user_ctx;

    if (heartbeat->cfg.role == SIMPLE_D2D_INDOOR_HEARTBEAT_ROLE_SERVER)
    {
        simple_d2d_indoor_heartbeat_bus_app_init(&app, heartbeat, true);
        ret = simple_d2d_bus_register_app(&heartbeat->bus, &app);
        if (ret != 0)
        {
            return ret;
        }
    }
    return simple_d2d_bus_start(&heartbeat->bus, &bus_cfg);
}

int simple_d2d_indoor_heartbeat_start(
    simple_d2d_indoor_heartbeat_t *heartbeat,
    const simple_d2d_indoor_heartbeat_config_t *cfg)
{
    int ret;

    if (!heartbeat || heartbeat->started)
    {
        return -EINVAL;
    }

    ret = simple_d2d_indoor_heartbeat_init(heartbeat, cfg);
    if (ret != 0)
    {
        return ret;
    }

    ret = indoor_hb_start_owned_bus(heartbeat);
    if (ret != 0)
    {
        simple_d2d_indoor_heartbeat_stop(heartbeat);
        return ret;
    }

    heartbeat->started = true;
    ret = pthread_create(&heartbeat->thread,
                         NULL,
                         heartbeat->cfg.role ==
                             SIMPLE_D2D_INDOOR_HEARTBEAT_ROLE_CLIENT ?
                             indoor_hb_client_thread : indoor_hb_server_thread,
                         heartbeat);
    if (ret != 0)
    {
        simple_d2d_indoor_heartbeat_stop(heartbeat);
        return -ret;
    }
    heartbeat->thread_started = true;
    SIMPLE_LOGI("indoor heartbeat started: role=%d plc=0x%08x:%u "
                "peer=0x%08x:%u shared=%d\n",
                heartbeat->cfg.role,
                heartbeat->cfg.plc.local_addr,
                heartbeat->cfg.plc.port,
                heartbeat->cfg.plc.peer_addr,
                heartbeat->cfg.plc.port,
                heartbeat->cfg.shared_service ? 1 : 0);
    return 0;
}

int simple_d2d_indoor_heartbeat_start_expire_thread(
    simple_d2d_indoor_heartbeat_t *heartbeat)
{
    int ret;

    if (!heartbeat || !heartbeat->lock_ready || heartbeat->thread_started)
    {
        return -EINVAL;
    }
    heartbeat->started = true;
    ret = pthread_create(&heartbeat->thread,
                         NULL,
                         indoor_hb_server_thread,
                         heartbeat);
    if (ret != 0)
    {
        heartbeat->started = false;
        return -ret;
    }
    heartbeat->thread_started = true;
    return 0;
}

void simple_d2d_indoor_heartbeat_stop(
    simple_d2d_indoor_heartbeat_t *heartbeat)
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
    simple_d2d_bus_stop(&heartbeat->bus);
    if (heartbeat->started)
    {
        SIMPLE_LOGI("indoor heartbeat stopped\n");
    }
    if (heartbeat->lock_ready)
    {
        pthread_mutex_destroy(&heartbeat->lock);
    }
    memset(heartbeat, 0, sizeof(*heartbeat));
}

size_t simple_d2d_indoor_heartbeat_snapshot(
    simple_d2d_indoor_heartbeat_t *heartbeat,
    simple_d2d_indoor_heartbeat_status_t *out,
    size_t capacity)
{
    size_t count = 0;

    if (!heartbeat || !out || capacity == 0 || !heartbeat->lock_ready)
    {
        return 0;
    }
    pthread_mutex_lock(&heartbeat->lock);
    if (heartbeat->rooms[0].room_id[0] != '\0')
    {
        out[0] = heartbeat->rooms[0];
        count = 1;
    }
    pthread_mutex_unlock(&heartbeat->lock);
    return count;
}

void simple_d2d_indoor_heartbeat_bus_app_init(
    simple_d2d_bus_app_t *app,
    simple_d2d_indoor_heartbeat_t *heartbeat,
    bool bridge_local)
{
    if (!app)
    {
        return;
    }

    memset(app, 0, sizeof(*app));
    app->request_cmd = D2D_CMD_INDOOR_HEARTBEAT_REQ;
    app->reply_cmd = D2D_CMD_INDOOR_HEARTBEAT_ACK;
    app->app_name = "indoor_heartbeat";
    app->handler = indoor_hb_bus_handler;
    app->user_ctx = heartbeat;
    app->bridge_local = bridge_local;
}
