/**
 * @file
 * @brief simple 室内机房号注册服务实现。
 */

#include "simple_d2d_indoor_registry.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <utils/time_helper.h>
#include <utils/util.h>

#include "simple_log.h"
#include "simple_plc_addr.h"

static uint32_t registry_timeout_or_default(uint32_t value, uint32_t fallback)
{
    return value != 0 ? value : fallback;
}

static void registry_sleep_step_ms(uint32_t total_ms,
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

static void registry_fill_ack(d2d_message_t *reply, int result)
{
    d2d_message_init(reply, D2D_CMD_INDOOR_SYNC_ACK);
    reply->header.dev_type = D2D_DEV_SIGNAL_EXTENDER;
    reply->header.message_type = D2D_MESSAGE_REPLY_OK;
    reply->body.simple_result.result = result;
}

void simple_d2d_indoor_registry_config_init(simple_d2d_indoor_registry_config_t *cfg)
{
    if (!cfg)
    {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    cfg->plc.port = SIMPLE_D2D_INDOOR_REGISTRY_DEFAULT_PLC_PORT;
    cfg->timing.interval_ms = SIMPLE_D2D_INDOOR_REGISTRY_DEFAULT_HEARTBEAT_MS;
    cfg->timing.ttl_ms = SIMPLE_D2D_INDOOR_REGISTRY_DEFAULT_TTL_MS;
    cfg->timing.timeout_ms = 5000U;
}

int simple_d2d_indoor_registry_init(simple_d2d_indoor_registry_t *registry,
                                    const simple_d2d_indoor_registry_config_t *cfg)
{
    if (!registry || !cfg || cfg->role == 0)
    {
        return -EINVAL;
    }

    memset(registry, 0, sizeof(*registry));
    registry->cfg = *cfg;
    registry->cfg.plc.port = cfg->plc.port ? cfg->plc.port :
                             SIMPLE_D2D_INDOOR_REGISTRY_DEFAULT_PLC_PORT;
    registry->cfg.timing.interval_ms = registry_timeout_or_default(
        cfg->timing.interval_ms,
        SIMPLE_D2D_INDOOR_REGISTRY_DEFAULT_HEARTBEAT_MS);
    registry->cfg.timing.ttl_ms = registry_timeout_or_default(
        cfg->timing.ttl_ms,
        SIMPLE_D2D_INDOOR_REGISTRY_DEFAULT_TTL_MS);
    registry->cfg.timing.timeout_ms = registry_timeout_or_default(cfg->timing.timeout_ms, 5000U);

    if (pthread_mutex_init(&registry->lock, NULL) != 0)
    {
        return -errno;
    }
    registry->lock_ready = true;
    return 0;
}

void simple_d2d_indoor_registry_deinit(simple_d2d_indoor_registry_t *registry)
{
    if (!registry)
    {
        return;
    }
    if (registry->lock_ready)
    {
        pthread_mutex_destroy(&registry->lock);
    }
    memset(registry, 0, sizeof(*registry));
}

int simple_d2d_indoor_registry_build_request(
    const simple_d2d_indoor_registry_config_t *cfg,
    bool online,
    d2d_message_t *message,
    d2d_device_info_t *entry)
{
    if (!cfg || !message || !entry ||
        !IS_VALID_STRING(cfg->identity.local_device_id) ||
        !IS_VALID_STRING(cfg->identity.peer_device_id) ||
        !IS_VALID_STRING(cfg->room_id))
    {
        return -EINVAL;
    }

    memset(entry, 0, sizeof(*entry));
    entry->id = (char *)cfg->room_id;
    entry->dev_type = D2D_DEV_INDOOR;
    entry->dev_model = "indoor";
    entry->net_type.present = true;
    entry->net_type.value = 2;
    entry->state.present = true;
    entry->state.value = online ? 1 : 0;

    memset(message, 0, sizeof(*message));
    d2d_message_init(message, D2D_CMD_INDOOR_SYNC_REQ);
    message->header.dev_type = D2D_DEV_INDOOR;
    message->header.message_type = D2D_MESSAGE_REQUEST;
    snprintf(message->header.orig_addr, sizeof(message->header.orig_addr),
             "%s", cfg->identity.local_device_id);
    snprintf(message->header.dest_addr, sizeof(message->header.dest_addr),
             "%s", cfg->identity.peer_device_id);
    message->body.sync_request.id = (char *)cfg->identity.local_device_id;
    message->body.sync_request.sync_ver.present = true;
    message->body.sync_request.sync_ver.value = "simple-indoor";
    message->body.sync_request.entries.present = true;
    message->body.sync_request.entries.count = 1;
    message->body.sync_request.entries.items = entry;
    return 0;
}

static int registry_send_client_state(simple_d2d_indoor_registry_t *registry,
                                      bool online)
{
    d2d_adapter_request_t tx;
    d2d_message_t message;
    d2d_device_info_t entry;
    d2d_adapter_t *adapter;
    int ret;

    if (!registry)
    {
        return -EINVAL;
    }
    ret = simple_d2d_indoor_registry_build_request(&registry->cfg,
                                                   online,
                                                   &message,
                                                   &entry);
    if (ret != 0)
    {
        return ret;
    }

    adapter = registry->cfg.shared_service ?
              simple_d2d_service_adapter(registry->cfg.shared_service) :
              simple_d2d_service_adapter(&registry->service);
    if (!adapter)
    {
        return -ENODEV;
    }

    memset(&tx, 0, sizeof(tx));
    tx.link_kind = D2D_LINK_PLC;
    tx.remote_endpoint.addr = registry->cfg.plc.peer_addr;
    tx.remote_endpoint.port = registry->cfg.plc.port;
    tx.opaque = D2D_CMD_INDOOR_SYNC_REQ;

    ret = d2d_adapter_send_request(adapter, &message, &tx);
    if (ret == 0)
    {
        SIMPLE_LOGD("room registry send: ret=%d room=%s online=%d target=0x%08x:%u\n",
                    ret,
                    registry->cfg.room_id ? registry->cfg.room_id : "-",
                    online ? 1 : 0,
                    tx.remote_endpoint.addr,
                    tx.remote_endpoint.port);
    }
    else
    {
        SIMPLE_LOGW("room registry send failed: ret=%d room=%s "
                    "online=%d target=0x%08x:%u\n",
                    ret,
                    registry->cfg.room_id ? registry->cfg.room_id : "-",
                    online ? 1 : 0,
                    tx.remote_endpoint.addr,
                    tx.remote_endpoint.port);
    }
    return ret;
}

static void registry_adapter_event_cb(void *user_data,
                                      const d2d_adapter_event_t *event)
{
    simple_d2d_indoor_registry_t *registry =
        (simple_d2d_indoor_registry_t *)user_data;

    if (!registry || !event)
    {
        return;
    }

    switch (event->type)
    {
        case D2D_ADAPTER_EVENT_REQUEST_IN:
            if (event->data.request_in.message &&
                event->data.request_in.message->header.cmd == D2D_CMD_INDOOR_SYNC_REQ)
            {
                d2d_message_t reply;
                int ret;

                memset(&reply, 0, sizeof(reply));
                ret = simple_d2d_indoor_registry_handle_sync(
                    registry,
                    event->data.request_in.message,
                    &reply);
                (void)d2d_adapter_begin_request(
                    simple_d2d_service_adapter(&registry->service),
                    event->data.request_in.handle,
                    D2D_REQUEST_MODE_LOCAL);
                if (ret == 0)
                {
                    (void)d2d_adapter_complete_request(
                        simple_d2d_service_adapter(&registry->service),
                        event->data.request_in.handle,
                        &reply);
                }
                else
                {
                    (void)d2d_adapter_fail_request(
                        simple_d2d_service_adapter(&registry->service),
                        event->data.request_in.handle,
                        D2D_REQUEST_FAIL_REJECTED);
                }
            }
            break;
        case D2D_ADAPTER_EVENT_REQUEST_DONE:
            if (event->data.request_done.reply &&
                event->data.request_done.reply->header.cmd == D2D_CMD_INDOOR_SYNC_ACK)
            {
                SIMPLE_LOGD("room registry acked: value=%d\n",
                            event->data.request_done.reply->body.simple_result.result);
            }
            break;
        case D2D_ADAPTER_EVENT_REQUEST_FAIL:
            SIMPLE_LOGW("room registry ack failed: cmd=%d type=%d\n",
                        event->data.request_fail.request_cmd,
                        event->data.request_fail.reply_type);
            break;
        case D2D_ADAPTER_EVENT_REQUEST_TIMEOUT:
            SIMPLE_LOGW("room registry ack timeout: cmd=%d\n",
                        event->data.request_timeout.request_cmd);
            break;
        default:
            break;
    }
}

static simple_d2d_indoor_registry_room_t *registry_find_room(
    simple_d2d_indoor_registry_t *registry,
    const char *room_id)
{
    simple_d2d_indoor_registry_room_t *free_slot = NULL;
    size_t i;

    if (!registry || !IS_VALID_STRING(room_id))
    {
        return NULL;
    }
    for (i = 0; i < SIMPLE_D2D_INDOOR_REGISTRY_MAX_ROOMS; i++)
    {
        simple_d2d_indoor_registry_room_t *room = &registry->rooms[i];

        if (room->room_id[0] == '\0')
        {
            if (!free_slot)
            {
                free_slot = room;
            }
            continue;
        }
        if (strcmp(room->room_id, room_id) == 0)
        {
            return room;
        }
    }
    return free_slot;
}

static size_t registry_copy_online_locked(
    simple_d2d_indoor_registry_t *registry,
    simple_d2d_indoor_registry_room_t *out,
    size_t capacity)
{
    size_t count = 0;
    size_t i;

    for (i = 0; i < SIMPLE_D2D_INDOOR_REGISTRY_MAX_ROOMS; i++)
    {
        const simple_d2d_indoor_registry_room_t *room = &registry->rooms[i];

        if (room->room_id[0] == '\0' || !room->online)
        {
            continue;
        }
        if (out && count < capacity)
        {
            out[count] = *room;
        }
        count++;
    }
    return count;
}

static void registry_emit_update(simple_d2d_indoor_registry_t *registry)
{
    simple_d2d_indoor_registry_room_t online[SIMPLE_D2D_INDOOR_REGISTRY_MAX_ROOMS];
    size_t count;

    if (!registry || !registry->cfg.callbacks.update_cb)
    {
        return;
    }
    pthread_mutex_lock(&registry->lock);
    count = registry_copy_online_locked(registry, online,
                                        SIMPLE_D2D_INDOOR_REGISTRY_MAX_ROOMS);
    pthread_mutex_unlock(&registry->lock);
    registry->cfg.callbacks.update_cb(registry->cfg.callbacks.user_ctx, online, count);
}

int simple_d2d_indoor_registry_handle_sync(simple_d2d_indoor_registry_t *registry,
                                           const d2d_message_t *message,
                                           d2d_message_t *reply_out)
{
    const d2d_sync_request_t *sync;
    const d2d_device_info_t *entry;
    simple_d2d_indoor_registry_room_t *room;
    uint32_t plc_addr = 0;
    bool online;
    bool changed;
    int ret = 0;

    if (!registry || !message || !reply_out || !registry->lock_ready)
    {
        return -EINVAL;
    }
    registry_fill_ack(reply_out, 0);
    if (message->header.cmd != D2D_CMD_INDOOR_SYNC_REQ)
    {
        return -EINVAL;
    }

    sync = &message->body.sync_request;
    if (!IS_VALID_STRING(sync->id) || !sync->entries.present ||
        sync->entries.count != 1 || !sync->entries.items)
    {
        ret = -EINVAL;
        goto out;
    }
    entry = &sync->entries.items[0];
    if (!IS_VALID_STRING(entry->id) || !entry->state.present ||
        simple_plc_addr_from_device_id(sync->id, &plc_addr) != 0)
    {
        ret = -EINVAL;
        goto out;
    }

    online = entry->state.value != 0;
    pthread_mutex_lock(&registry->lock);
    room = registry_find_room(registry, entry->id);
    if (!room)
    {
        pthread_mutex_unlock(&registry->lock);
        ret = -ENOSPC;
        goto out;
    }
    changed = room->room_id[0] == '\0' ||
              strcmp(room->device_id, sync->id) != 0 ||
              room->plc_addr != plc_addr ||
              room->online != online;
    safe_strncpy(room->room_id, entry->id, sizeof(room->room_id));
    safe_strncpy(room->device_id, sync->id, sizeof(room->device_id));
    room->plc_addr = plc_addr;
    room->online = online;
    room->last_seen_ms = time_now();
    room->ttl_ms = registry->cfg.timing.ttl_ms;
    pthread_mutex_unlock(&registry->lock);

    if (changed)
    {
        SIMPLE_LOGI("room registry updated: room=%s device_id=%s "
                    "plc_addr=0x%08x online=%d\n",
                    entry->id,
                    sync->id,
                    plc_addr,
                    online ? 1 : 0);
    }
    else
    {
        SIMPLE_LOGD("room registry refresh: room=%s "
                    "device_id=%s plc_addr=0x%08x online=%d changed=0\n",
                    entry->id,
                    sync->id,
                    plc_addr,
                    online ? 1 : 0);
    }
    registry_fill_ack(reply_out, 1);
    if (changed)
    {
        registry_emit_update(registry);
    }
    return 0;

out:
    SIMPLE_LOGW("room registry update failed: ret=%d\n", ret);
    registry_fill_ack(reply_out, 0);
    return ret;
}

int simple_d2d_indoor_registry_expire(simple_d2d_indoor_registry_t *registry,
                                      uint64_t now_ms)
{
    bool changed = false;
    size_t i;

    if (!registry || !registry->lock_ready)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&registry->lock);
    for (i = 0; i < SIMPLE_D2D_INDOOR_REGISTRY_MAX_ROOMS; i++)
    {
        simple_d2d_indoor_registry_room_t *room = &registry->rooms[i];
        uint32_t ttl_ms;

        if (room->room_id[0] == '\0' || !room->online)
        {
            continue;
        }
        ttl_ms = room->ttl_ms ? room->ttl_ms : registry->cfg.timing.ttl_ms;
        if (now_ms >= room->last_seen_ms &&
            now_ms - room->last_seen_ms > (uint64_t)ttl_ms)
        {
            room->online = false;
            changed = true;
            SIMPLE_LOGW("room registry expired: room=%s device_id=%s ttl=%u\n",
                        room->room_id,
                        room->device_id,
                        ttl_ms);
        }
    }
    pthread_mutex_unlock(&registry->lock);

    if (changed)
    {
        registry_emit_update(registry);
    }
    return changed ? 1 : 0;
}

size_t simple_d2d_indoor_registry_snapshot(
    simple_d2d_indoor_registry_t *registry,
    simple_d2d_indoor_registry_room_t *rooms,
    size_t capacity)
{
    size_t count = 0;
    size_t i;

    if (!registry || !registry->lock_ready)
    {
        return 0;
    }
    pthread_mutex_lock(&registry->lock);
    for (i = 0; i < SIMPLE_D2D_INDOOR_REGISTRY_MAX_ROOMS; i++)
    {
        const simple_d2d_indoor_registry_room_t *room = &registry->rooms[i];

        if (room->room_id[0] == '\0')
        {
            continue;
        }
        if (rooms && count < capacity)
        {
            rooms[count] = *room;
        }
        count++;
    }
    pthread_mutex_unlock(&registry->lock);
    return count;
}

static void *registry_client_thread(void *arg)
{
    simple_d2d_indoor_registry_t *registry =
        (simple_d2d_indoor_registry_t *)arg;

    (void)registry_send_client_state(registry, true);
    while (!registry->stop_requested)
    {
        registry_sleep_step_ms(registry->cfg.timing.interval_ms,
                               (volatile bool *)&registry->stop_requested);
        if (!registry->stop_requested)
        {
            (void)registry_send_client_state(registry, true);
        }
    }
    return NULL;
}

static void *registry_server_thread(void *arg)
{
    simple_d2d_indoor_registry_t *registry =
        (simple_d2d_indoor_registry_t *)arg;

    while (!registry->stop_requested)
    {
        registry_sleep_step_ms(1000U, (volatile bool *)&registry->stop_requested);
        if (!registry->stop_requested)
        {
            (void)simple_d2d_indoor_registry_expire(registry, time_now());
        }
    }
    return NULL;
}

static int registry_start_owned_service(simple_d2d_indoor_registry_t *registry)
{
    simple_d2d_service_config_t service_cfg;
    transport_endpoint_t plc_local;

    if (registry->cfg.shared_service)
    {
        return 0;
    }

    memset(&plc_local, 0, sizeof(plc_local));
    plc_local.addr = registry->cfg.plc.local_addr;
    plc_local.port = registry->cfg.plc.port;

    memset(&service_cfg, 0, sizeof(service_cfg));
    service_cfg.mode = SIMPLE_D2D_SERVICE_MODE_ADAPTER_PLC;
    service_cfg.local_dev_type = registry->cfg.role ==
        SIMPLE_D2D_INDOOR_REGISTRY_ROLE_SERVER ?
        D2D_DEV_SIGNAL_EXTENDER : D2D_DEV_INDOOR;
    service_cfg.plc_if = registry->cfg.plc.if_name;
    service_cfg.plc_local = plc_local;
    service_cfg.timeout_ms = registry->cfg.timing.timeout_ms;
    service_cfg.event_cb = registry_adapter_event_cb;
    service_cfg.user_data = registry;
    return simple_d2d_service_start(&registry->service, &service_cfg);
}

int simple_d2d_indoor_registry_start(simple_d2d_indoor_registry_t *registry,
                                     const simple_d2d_indoor_registry_config_t *cfg)
{
    int ret;

    if (!registry || !cfg || cfg->role == 0 || registry->started)
    {
        return -EINVAL;
    }

    ret = simple_d2d_indoor_registry_init(registry, cfg);
    if (ret != 0)
    {
        return ret;
    }
    ret = registry_start_owned_service(registry);
    if (ret != 0)
    {
        simple_d2d_indoor_registry_deinit(registry);
        return ret;
    }

    registry->started = true;
    if (registry->cfg.role == SIMPLE_D2D_INDOOR_REGISTRY_ROLE_CLIENT)
    {
        ret = pthread_create(&registry->thread, NULL, registry_client_thread, registry);
    }
    else
    {
        ret = pthread_create(&registry->thread, NULL, registry_server_thread, registry);
    }
    if (ret != 0)
    {
        registry->started = false;
        simple_d2d_service_stop(&registry->service);
        simple_d2d_indoor_registry_deinit(registry);
        return -ret;
    }
    registry->thread_started = true;
    SIMPLE_LOGI("room registry started: role=%d plc=0x%08x:%u shared=%d\n",
                registry->cfg.role,
                registry->cfg.plc.local_addr,
                registry->cfg.plc.port,
                registry->cfg.shared_service ? 1 : 0);
    return 0;
}

void simple_d2d_indoor_registry_stop(simple_d2d_indoor_registry_t *registry)
{
    if (!registry)
    {
        return;
    }
    registry->stop_requested = true;
    if (registry->cfg.role == SIMPLE_D2D_INDOOR_REGISTRY_ROLE_CLIENT &&
        registry->started)
    {
        (void)registry_send_client_state(registry, false);
    }
    if (registry->thread_started)
    {
        pthread_join(registry->thread, NULL);
    }
    if (!registry->cfg.shared_service)
    {
        simple_d2d_service_stop(&registry->service);
    }
    if (registry->started)
    {
        SIMPLE_LOGI("room registry stopped\n");
    }
    simple_d2d_indoor_registry_deinit(registry);
}
