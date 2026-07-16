/**
 * @file
 * @brief simple 示例应用公共模块 simple_d2d_unlock 的实现文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_d2d_unlock.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <utils/util.h>

#include "simple_log.h"

static void simple_d2d_unlock_fill_ack(d2d_message_t *reply,
                                       int dev_type,
                                       int result)
{
    d2d_message_init(reply, D2D_CMD_UNLOCK_ACK);
    reply->header.dev_type = dev_type;
    reply->header.message_type = D2D_MESSAGE_REPLY_OK;
    reply->body.simple_result.result = result;
}

static int simple_d2d_unlock_bus_handler(void *user_ctx,
                                         const d2d_message_t *msg,
                                         d2d_link_kind_t ingress_link,
                                         const transport_endpoint_t *src,
                                         d2d_message_t *reply)
{
    simple_d2d_unlock_t *unlock = (simple_d2d_unlock_t *)user_ctx;
    simple_d2d_unlock_request_t request;
    int place;
    int action_ret = 0;

    if (!unlock || !msg || !reply || msg->header.cmd != D2D_CMD_UNLOCK_REQ)
    {
        return -EINVAL;
    }

    place = msg->body.unlock_request.place.present ?
            msg->body.unlock_request.place.value : 0;
    if (IS_VALID_STRING(unlock->cfg.identity.local_device_id) &&
        strcmp(msg->header.dest_addr, unlock->cfg.identity.local_device_id) != 0)
    {
        SIMPLE_LOGW("unlock request rejected: reason=dest_mismatch "
                    "expected=%s dest=%s id=%s orig=%s\n",
                    unlock->cfg.identity.local_device_id,
                    msg->header.dest_addr,
                    msg->body.unlock_request.id ?
                    msg->body.unlock_request.id : "-",
                    msg->header.orig_addr);
        return -EACCES;
    }

    memset(&request, 0, sizeof(request));
    request.orig_device_id = msg->header.orig_addr;
    request.dest_device_id = msg->header.dest_addr;
    request.id = msg->body.unlock_request.id;
    request.place = place;
    request.ingress_link = ingress_link;
    if (src)
    {
        request.source_endpoint = *src;
    }

    if (unlock->cfg.callbacks.server_unlock_cb)
    {
        action_ret = unlock->cfg.callbacks.server_unlock_cb(unlock->cfg.callbacks.server_unlock_user_ctx,
                                                  &request);
    }

    simple_d2d_unlock_fill_ack(reply,
                               unlock->cfg.identity.local_dev_type,
                               action_ret == 0 ? 1 : 0);
    if (action_ret == 0)
    {
        SIMPLE_LOGI("unlock request accepted: id=%s place=%d orig=%s dest=%s "
                    "source=0x%x:%u\n",
                    msg->body.unlock_request.id ? msg->body.unlock_request.id : "-",
                    place,
                    msg->header.orig_addr,
                    msg->header.dest_addr,
                    request.source_endpoint.addr,
                    request.source_endpoint.port);
    }
    else
    {
        SIMPLE_LOGW("unlock request action failed: ret=%d id=%s place=%d "
                    "orig=%s dest=%s source=0x%x:%u\n",
                    action_ret,
                    msg->body.unlock_request.id ? msg->body.unlock_request.id : "-",
                    place,
                    msg->header.orig_addr,
                    msg->header.dest_addr,
                    request.source_endpoint.addr,
                    request.source_endpoint.port);
    }
    return 0;
}

static int simple_d2d_unlock_build_request(const simple_d2d_unlock_config_t *cfg,
                                           const char *peer_id,
                                           const char *id,
                                           int place,
                                           d2d_message_t *message)
{
    const char *target_peer_id = peer_id;

    if (!IS_VALID_STRING(target_peer_id) && cfg)
    {
        target_peer_id = cfg->identity.peer_device_id;
    }

    if (!cfg || !message || !IS_VALID_STRING(id) ||
        !IS_VALID_STRING(cfg->identity.local_device_id) ||
        !IS_VALID_STRING(target_peer_id))
    {
        return -EINVAL;
    }

    memset(message, 0, sizeof(*message));
    d2d_message_init(message, D2D_CMD_UNLOCK_REQ);
    message->header.dev_type = cfg->identity.local_dev_type;
    message->header.message_type = D2D_MESSAGE_REQUEST;
    snprintf(message->header.orig_addr, sizeof(message->header.orig_addr), "%s",
             cfg->identity.local_device_id);
    snprintf(message->header.dest_addr, sizeof(message->header.dest_addr), "%s",
             target_peer_id);
    message->body.unlock_request.id = (char *)id;
    message->body.unlock_request.place.present = true;
    message->body.unlock_request.place.value = place;
    return 0;
}

#ifdef SIMPLE_D2D_UNLOCK_ENABLE_TEST_HOOKS
int simple_d2d_unlock_build_request_for_test(const simple_d2d_unlock_config_t *cfg,
                                             const char *peer_id,
                                             const char *id,
                                             int place,
                                             d2d_message_t *message)
{
    return simple_d2d_unlock_build_request(cfg, peer_id, id, place, message);
}

int simple_d2d_unlock_handle_request_for_test(simple_d2d_unlock_t *unlock,
                                              const d2d_message_t *request,
                                              d2d_message_t *reply)
{
    transport_endpoint_t src;

    memset(&src, 0, sizeof(src));
    return simple_d2d_unlock_bus_handler(unlock,
                                         request,
                                         D2D_LINK_ETH,
                                         &src,
                                         reply);
}
#endif

void simple_d2d_unlock_config_init(simple_d2d_unlock_config_t *cfg)
{
    if (!cfg)
    {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    cfg->identity.local_dev_type = D2D_DEV_INDOOR;
    cfg->eth.port = SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT;
    cfg->plc.port = SIMPLE_D2D_UNLOCK_DEFAULT_PLC_PORT;
    cfg->timeout_ms = 5000;
}

void simple_d2d_unlock_bus_app_init(simple_d2d_bus_app_t *app,
                                    simple_d2d_unlock_t *unlock,
                                    bool bridge_local)
{
    if (!app)
    {
        return;
    }

    memset(app, 0, sizeof(*app));
    app->request_cmd = D2D_CMD_UNLOCK_REQ;
    app->reply_cmd = D2D_CMD_UNLOCK_ACK;
    app->app_name = "unlock";
    app->handler = simple_d2d_unlock_bus_handler;
    app->user_ctx = unlock;
    app->bridge_local = bridge_local;
}

static uint16_t simple_d2d_unlock_eth_port(const simple_d2d_unlock_config_t *cfg)
{
    return cfg && cfg->eth.port ? cfg->eth.port : SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT;
}

int simple_d2d_unlock_start(simple_d2d_unlock_t *unlock,
                            const simple_d2d_unlock_config_t *cfg)
{
    transport_endpoint_t eth_local;
    transport_endpoint_t plc_local;
    simple_d2d_bus_config_t bus_cfg;
    simple_d2d_bus_app_t app;
    int ret;

    if (!unlock || !cfg || cfg->role == 0 || unlock->started)
    {
        return -EINVAL;
    }

    memset(unlock, 0, sizeof(*unlock));
    unlock->cfg = *cfg;
    if (unlock->cfg.timeout_ms == 0)
    {
        unlock->cfg.timeout_ms = 5000;
    }

    if (unlock->cfg.shared_service)
    {
        unlock->started = true;
        SIMPLE_LOGI("unlock service started: role=%d shared=1 eth=0x%x:%u "
                    "plc=0x%x:%u peer_eth=0x%x:%u\n",
                    cfg->role,
                    cfg->eth.local_addr,
                    cfg->eth.port ? cfg->eth.port :
                        SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT,
                    cfg->plc.local_addr,
                    cfg->plc.port ? cfg->plc.port :
                        SIMPLE_D2D_UNLOCK_DEFAULT_PLC_PORT,
                    cfg->eth.peer_addr,
                    cfg->eth.port ? cfg->eth.port :
                        SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT);
        return 0;
    }

    memset(&eth_local, 0, sizeof(eth_local));
    eth_local.addr = cfg->eth.local_addr;
    eth_local.port = simple_d2d_unlock_eth_port(cfg);
    if (cfg->role == SIMPLE_D2D_UNLOCK_ROLE_BRIDGE &&
        cfg->eth.local_addr == ADDR_ANY)
    {
        eth_local.port = 0;
    }
    memset(&plc_local, 0, sizeof(plc_local));
    plc_local.addr = cfg->plc.local_addr;
    plc_local.port = cfg->plc.port ? cfg->plc.port : SIMPLE_D2D_UNLOCK_DEFAULT_PLC_PORT;

    memset(&bus_cfg, 0, sizeof(bus_cfg));
    bus_cfg.local_dev_type = cfg->identity.local_dev_type;
    bus_cfg.eth_if = cfg->eth.if_name;
    bus_cfg.plc_if = cfg->plc.if_name;
    bus_cfg.eth_local = eth_local;
    bus_cfg.plc_local = plc_local;
    bus_cfg.eth_peer.addr = cfg->eth.peer_addr;
    bus_cfg.eth_peer.port = simple_d2d_unlock_eth_port(cfg);
    bus_cfg.timeout_ms = unlock->cfg.timeout_ms;
    bus_cfg.observer = cfg->callbacks.observer.observer;
    bus_cfg.observer_user_ctx = cfg->callbacks.observer.observer_user_ctx;
    if (cfg->role == SIMPLE_D2D_UNLOCK_ROLE_CLIENT &&
        cfg->client_link == SIMPLE_D2D_UNLOCK_CLIENT_LINK_ETH)
    {
        bus_cfg.mode = SIMPLE_D2D_SERVICE_MODE_ADAPTER_ETH;
    }
    else if (cfg->role == SIMPLE_D2D_UNLOCK_ROLE_CLIENT)
    {
        bus_cfg.mode = SIMPLE_D2D_SERVICE_MODE_ADAPTER_PLC;
    }
    else if (cfg->role == SIMPLE_D2D_UNLOCK_ROLE_SERVER)
    {
        bus_cfg.mode = SIMPLE_D2D_SERVICE_MODE_ADAPTER_ETH;
    }
    else
    {
        bus_cfg.mode = SIMPLE_D2D_SERVICE_MODE_BRIDGE;
    }

    simple_d2d_unlock_bus_app_init(&app,
                                   unlock,
                                   cfg->role != SIMPLE_D2D_UNLOCK_ROLE_BRIDGE);
    ret = simple_d2d_bus_register_app(&unlock->bus, &app);
    if (ret != 0)
    {
        goto fail;
    }
    if (cfg->extra_apps.apps && cfg->extra_apps.count > 0)
    {
        size_t i;

        for (i = 0; i < cfg->extra_apps.count; ++i)
        {
            ret = simple_d2d_bus_register_app(&unlock->bus,
                                              &cfg->extra_apps.apps[i]);
            if (ret != 0)
            {
                goto fail;
            }
        }
    }

    ret = simple_d2d_bus_start(&unlock->bus, &bus_cfg);
    if (ret != 0)
    {
        goto fail;
    }

    unlock->started = true;
    SIMPLE_LOGI("unlock service started: role=%d eth=0x%x:%u "
                "plc=0x%x:%u peer_eth=0x%x:%u\n",
                cfg->role,
                eth_local.addr,
                eth_local.port,
                plc_local.addr,
                plc_local.port,
                cfg->eth.peer_addr,
                bus_cfg.eth_peer.port);
    return 0;

fail:
    simple_d2d_unlock_stop(unlock);
    return ret;
}

void simple_d2d_unlock_stop(simple_d2d_unlock_t *unlock)
{
    if (!unlock)
    {
        return;
    }
    if (!unlock->cfg.shared_service)
    {
        simple_d2d_bus_stop(&unlock->bus);
    }
    if (unlock->started)
    {
        SIMPLE_LOGI("unlock service stopped\n");
    }
    memset(unlock, 0, sizeof(*unlock));
}

int simple_d2d_unlock_send(simple_d2d_unlock_t *unlock,
                           const char *id,
                           int place)
{
    return simple_d2d_unlock_send_to(unlock,
                                     unlock ? unlock->cfg.identity.peer_device_id : NULL,
                                     id,
                                     place);
}

int simple_d2d_unlock_send_to(simple_d2d_unlock_t *unlock,
                              const char *peer_id,
                              const char *id,
                              int place)
{
    d2d_adapter_request_t tx;
    d2d_message_t message;
    int ret;

    if (!unlock || !unlock->started)
    {
        return -EINVAL;
    }

    ret = simple_d2d_unlock_build_request(&unlock->cfg, peer_id, id, place, &message);
    if (ret != 0)
    {
        return ret;
    }

    memset(&tx, 0, sizeof(tx));
    if (unlock->cfg.client_link == SIMPLE_D2D_UNLOCK_CLIENT_LINK_ETH)
    {
        if (unlock->cfg.eth.peer_addr == 0)
        {
            return -ENOTCONN;
        }
        tx.link_kind = D2D_LINK_ETH;
        tx.remote_endpoint.addr = unlock->cfg.eth.peer_addr;
        tx.remote_endpoint.port = unlock->cfg.eth.port ?
                                  unlock->cfg.eth.port :
                                  SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT;
    }
    else
    {
        if (unlock->cfg.plc.peer_addr == 0)
        {
            return -ENOTCONN;
        }
        tx.link_kind = D2D_LINK_PLC;
        tx.remote_endpoint.addr = unlock->cfg.plc.peer_addr;
        tx.remote_endpoint.port = unlock->cfg.plc.port ?
                                  unlock->cfg.plc.port :
                                  SIMPLE_D2D_UNLOCK_DEFAULT_PLC_PORT;
    }
    tx.opaque = D2D_CMD_UNLOCK_REQ;

    if (unlock->cfg.shared_service)
    {
        if (simple_d2d_service_adapter(unlock->cfg.shared_service))
        {
            ret = d2d_adapter_send_request(
                simple_d2d_service_adapter(unlock->cfg.shared_service),
                &message,
                &tx);
        }
        else if (simple_d2d_service_forwarder(unlock->cfg.shared_service))
        {
            ret = d2d_forwarder_send_request(
                simple_d2d_service_forwarder(unlock->cfg.shared_service),
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
        ret = simple_d2d_bus_send_request(&unlock->bus, &message, &tx);
    }
    if (ret == 0)
    {
        SIMPLE_LOGI("unlock request sent: ret=%d id=%s place=%d "
                    "orig=%s dest=%s target=0x%x:%u\n",
                    ret,
                    id,
                    place,
                    message.header.orig_addr,
                    message.header.dest_addr,
                    tx.remote_endpoint.addr,
                    tx.remote_endpoint.port);
    }
    else
    {
        SIMPLE_LOGW("unlock request failed: ret=%d id=%s place=%d "
                    "orig=%s dest=%s target=0x%x:%u\n",
                    ret,
                    id,
                    place,
                    message.header.orig_addr,
                    message.header.dest_addr,
                    tx.remote_endpoint.addr,
                    tx.remote_endpoint.port);
    }
    return ret;
}
