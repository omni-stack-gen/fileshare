/**
 * @file
 * @brief simple D2D 0x03 bus 实现。
 */

#include "simple_d2d_bus.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static const char *simple_d2d_bus_reason_unknown = "unknown_cmd";

static void simple_d2d_bus_emit(simple_d2d_bus_t *bus,
                                const simple_d2d_bus_event_t *event)
{
    if (bus && bus->cfg.observer && event)
    {
        bus->cfg.observer(bus->cfg.observer_user_ctx, event);
    }
}

static const simple_d2d_bus_app_t *simple_d2d_bus_find_app(
    const simple_d2d_bus_t *bus,
    d2d_cmd_t cmd)
{
    size_t i;

    if (!bus)
    {
        return NULL;
    }
    for (i = 0; i < bus->app_count; ++i)
    {
        if (bus->apps[i].request_cmd == cmd)
        {
            return &bus->apps[i];
        }
    }
    return NULL;
}

static const simple_d2d_bus_app_t *simple_d2d_bus_fill_app_fields(
    const simple_d2d_bus_t *bus,
    simple_d2d_bus_event_t *event,
    d2d_cmd_t request_cmd)
{
    const simple_d2d_bus_app_t *app;

    if (!event)
    {
        return NULL;
    }
    if (request_cmd != D2D_CMD_NONE)
    {
        event->cmd = request_cmd;
    }
    app = simple_d2d_bus_find_app(bus, event->cmd);
    if (app)
    {
        event->reply_cmd = app->reply_cmd;
        event->app_name = app->app_name;
    }
    return app;
}

static void simple_d2d_bus_fill_event_from_message(
    simple_d2d_bus_event_t *event,
    simple_d2d_bus_event_type_t type,
    const d2d_message_t *message,
    d2d_link_kind_t link,
    const transport_endpoint_t *src,
    const transport_endpoint_t *dst)
{
    memset(event, 0, sizeof(*event));
    event->type = type;
    event->link = link;
    if (message)
    {
        event->cmd = message->header.cmd;
        snprintf(event->msg_id, sizeof(event->msg_id), "%s", message->header.msg_id);
    }
    if (src)
    {
        event->src = *src;
    }
    if (dst)
    {
        event->dst = *dst;
    }
}

static int simple_d2d_bus_dispatch(simple_d2d_bus_t *bus,
                                   const d2d_message_t *request,
                                   d2d_link_kind_t ingress_link,
                                   const transport_endpoint_t *src,
                                   d2d_message_t *reply)
{
    const simple_d2d_bus_app_t *app;
    simple_d2d_bus_event_t event;
    int ret;

    if (!bus || !request || !reply)
    {
        return -EINVAL;
    }

    app = simple_d2d_bus_find_app(bus, request->header.cmd);
    simple_d2d_bus_fill_event_from_message(&event,
                                           SIMPLE_D2D_BUS_EVENT_RX,
                                           request,
                                           ingress_link,
                                           src,
                                           NULL);
    (void)simple_d2d_bus_fill_app_fields(bus, &event, request->header.cmd);
    simple_d2d_bus_emit(bus, &event);
    if (!app || !app->handler)
    {
        event.type = SIMPLE_D2D_BUS_EVENT_APP_REJECT;
        event.ret = -ENOENT;
        event.reason = simple_d2d_bus_reason_unknown;
        simple_d2d_bus_emit(bus, &event);
        return -ENOENT;
    }

    event.type = SIMPLE_D2D_BUS_EVENT_DISPATCH;
    event.reply_cmd = app->reply_cmd;
    event.app_name = app->app_name;
    event.ret = 0;
    event.reason = "dispatch";
    simple_d2d_bus_emit(bus, &event);

    ret = app->handler(app->user_ctx, request, ingress_link, src, reply);
    event.type = ret == 0 ? SIMPLE_D2D_BUS_EVENT_LOCAL_DONE :
                            SIMPLE_D2D_BUS_EVENT_FAIL;
    event.reply_cmd = app->reply_cmd;
    event.ret = ret;
    event.app_name = app->app_name;
    event.reason = ret == 0 ? "ok" : "handler";
    simple_d2d_bus_emit(bus, &event);
    return ret;
}

static void simple_d2d_bus_adapter_event_cb(void *user_data,
                                            const d2d_adapter_event_t *adapter_event)
{
    simple_d2d_bus_t *bus = (simple_d2d_bus_t *)user_data;
    simple_d2d_bus_event_t event;

    if (!bus || !adapter_event)
    {
        return;
    }

    switch (adapter_event->type)
    {
        case D2D_ADAPTER_EVENT_REQUEST_IN:
        {
            d2d_message_t reply;
            int ret;

            memset(&reply, 0, sizeof(reply));
            ret = simple_d2d_bus_dispatch(bus,
                                          adapter_event->data.request_in.message,
                                          adapter_event->data.request_in.ingress_link,
                                          &adapter_event->data.request_in.src_endpoint,
                                          &reply);
            (void)d2d_adapter_begin_request(simple_d2d_service_adapter(&bus->service),
                                            adapter_event->data.request_in.handle,
                                            D2D_REQUEST_MODE_LOCAL);
            if (ret == 0)
            {
                (void)d2d_adapter_complete_request(simple_d2d_service_adapter(&bus->service),
                                                   adapter_event->data.request_in.handle,
                                                   &reply);
            }
            else
            {
                (void)d2d_adapter_fail_request(simple_d2d_service_adapter(&bus->service),
                                               adapter_event->data.request_in.handle,
                                               D2D_REQUEST_FAIL_REJECTED);
            }
            break;
        }
        case D2D_ADAPTER_EVENT_REQUEST_DONE:
            simple_d2d_bus_fill_event_from_message(&event,
                                                   SIMPLE_D2D_BUS_EVENT_LOCAL_DONE,
                                                   adapter_event->data.request_done.request,
                                                   adapter_event->data.request_done.target_link,
                                                   NULL,
                                                   &adapter_event->data.request_done.target_endpoint);
            event.cmd = adapter_event->data.request_done.request_cmd;
            (void)simple_d2d_bus_fill_app_fields(bus,
                                                  &event,
                                                  adapter_event->data.request_done.request_cmd);
            if (event.reply_cmd == D2D_CMD_NONE &&
                adapter_event->data.request_done.reply)
            {
                event.reply_cmd = adapter_event->data.request_done.reply->header.cmd;
            }
            snprintf(event.msg_id, sizeof(event.msg_id), "%s",
                     adapter_event->data.request_done.msg_id);
            event.ret = 0;
            event.reason = "ack";
            simple_d2d_bus_emit(bus, &event);
            break;
        case D2D_ADAPTER_EVENT_REQUEST_FAIL:
            simple_d2d_bus_fill_event_from_message(&event,
                                                   SIMPLE_D2D_BUS_EVENT_FAIL,
                                                   adapter_event->data.request_fail.request,
                                                   adapter_event->data.request_fail.target_link,
                                                   NULL,
                                                   &adapter_event->data.request_fail.target_endpoint);
            event.cmd = adapter_event->data.request_fail.request_cmd;
            (void)simple_d2d_bus_fill_app_fields(bus,
                                                  &event,
                                                  adapter_event->data.request_fail.request_cmd);
            if (event.reply_cmd == D2D_CMD_NONE &&
                adapter_event->data.request_fail.reply)
            {
                event.reply_cmd = adapter_event->data.request_fail.reply->header.cmd;
            }
            snprintf(event.msg_id, sizeof(event.msg_id), "%s",
                     adapter_event->data.request_fail.msg_id);
            event.ret = -EIO;
            event.reason = "reply_fail";
            simple_d2d_bus_emit(bus, &event);
            break;
        case D2D_ADAPTER_EVENT_REQUEST_TIMEOUT:
            simple_d2d_bus_fill_event_from_message(&event,
                                                   SIMPLE_D2D_BUS_EVENT_TIMEOUT,
                                                   adapter_event->data.request_timeout.request,
                                                   adapter_event->data.request_timeout.target_link,
                                                   NULL,
                                                   &adapter_event->data.request_timeout.target_endpoint);
            event.cmd = adapter_event->data.request_timeout.request_cmd;
            (void)simple_d2d_bus_fill_app_fields(bus,
                                                  &event,
                                                  adapter_event->data.request_timeout.request_cmd);
            snprintf(event.msg_id, sizeof(event.msg_id), "%s",
                     adapter_event->data.request_timeout.msg_id);
            event.ret = -ETIMEDOUT;
            event.reason = "timeout";
            simple_d2d_bus_emit(bus, &event);
            break;
        case D2D_ADAPTER_EVENT_LINK_ERROR:
            memset(&event, 0, sizeof(event));
            event.type = SIMPLE_D2D_BUS_EVENT_LINK_ERROR;
            event.link = adapter_event->data.link_error.link_kind;
            event.src = adapter_event->data.link_error.local_endpoint;
            event.dst = adapter_event->data.link_error.remote_endpoint;
            event.ret = adapter_event->data.link_error.error_code;
            event.reason = adapter_event->data.link_error.operation;
            (void)simple_d2d_bus_fill_app_fields(bus,
                                                  &event,
                                                  (d2d_cmd_t)adapter_event->data.link_error.opaque);
            simple_d2d_bus_emit(bus, &event);
            break;
        default:
            break;
    }
}

static int simple_d2d_bus_route_cb(void *user_data,
                                   d2d_request_handle_t handle,
                                   const d2d_message_t *message,
                                   d2d_link_kind_t ingress_link,
                                   const transport_endpoint_t *src_endpoint,
                                   d2d_forwarder_route_result_t *result)
{
    simple_d2d_bus_t *bus = (simple_d2d_bus_t *)user_data;
    const simple_d2d_bus_app_t *app;
    simple_d2d_bus_event_t event;

    (void)handle;
    if (!bus || !message || !src_endpoint || !result)
    {
        return -EINVAL;
    }

    memset(result, 0, sizeof(*result));
    app = simple_d2d_bus_find_app(bus, message->header.cmd);
    simple_d2d_bus_fill_event_from_message(&event,
                                           SIMPLE_D2D_BUS_EVENT_RX,
                                           message,
                                           ingress_link,
                                           src_endpoint,
                                           NULL);
    (void)simple_d2d_bus_fill_app_fields(bus, &event, message->header.cmd);
    simple_d2d_bus_emit(bus, &event);

    if (!app)
    {
        result->decision = D2D_FORWARDER_ROUTE_REJECT_FORWARD;
        event.type = SIMPLE_D2D_BUS_EVENT_APP_REJECT;
        event.ret = -ENOENT;
        event.reason = simple_d2d_bus_reason_unknown;
        simple_d2d_bus_emit(bus, &event);
        return 0;
    }

    if (app->bridge_local)
    {
        result->decision = D2D_FORWARDER_ROUTE_LOCAL;
        event.type = SIMPLE_D2D_BUS_EVENT_DISPATCH;
        (void)simple_d2d_bus_fill_app_fields(bus, &event, message->header.cmd);
        event.ret = 0;
        event.reason = "local";
        simple_d2d_bus_emit(bus, &event);
        return 0;
    }

    if (ingress_link == D2D_LINK_PLC)
    {
        bus->last_plc_src = *src_endpoint;
        result->decision = D2D_FORWARDER_ROUTE_FORWARD;
        result->tx.link_kind = D2D_LINK_ETH;
        result->tx.remote_endpoint = bus->cfg.eth_peer;
        result->tx.opaque = message->header.cmd;
        event.type = SIMPLE_D2D_BUS_EVENT_DISPATCH;
        event.link = D2D_LINK_ETH;
        event.dst = result->tx.remote_endpoint;
        (void)simple_d2d_bus_fill_app_fields(bus, &event, message->header.cmd);
        event.ret = 0;
        event.reason = "forward";
        simple_d2d_bus_emit(bus, &event);
        return 0;
    }

    if (bus->last_plc_src.addr != 0 || bus->last_plc_src.port != 0)
    {
        result->decision = D2D_FORWARDER_ROUTE_FORWARD;
        result->tx.link_kind = D2D_LINK_PLC;
        result->tx.remote_endpoint = bus->last_plc_src;
        result->tx.opaque = message->header.cmd;
        event.type = SIMPLE_D2D_BUS_EVENT_DISPATCH;
        event.link = D2D_LINK_PLC;
        event.dst = result->tx.remote_endpoint;
        (void)simple_d2d_bus_fill_app_fields(bus, &event, message->header.cmd);
        event.ret = 0;
        event.reason = "forward";
        simple_d2d_bus_emit(bus, &event);
        return 0;
    }

    result->decision = D2D_FORWARDER_ROUTE_REJECT_FORWARD;
    event.type = SIMPLE_D2D_BUS_EVENT_FAIL;
    event.ret = -ENOTCONN;
    event.reason = "no_plc_peer";
    (void)simple_d2d_bus_fill_app_fields(bus, &event, message->header.cmd);
    simple_d2d_bus_emit(bus, &event);
    return 0;
}

static void simple_d2d_bus_notify_cb(void *user_data,
                                     const d2d_forwarder_notify_t *notify)
{
    simple_d2d_bus_t *bus = (simple_d2d_bus_t *)user_data;
    simple_d2d_bus_event_t event;

    if (!bus || !notify)
    {
        return;
    }

    switch (notify->type)
    {
        case D2D_FORWARDER_NOTIFY_LOCAL_REQUEST:
        {
            d2d_message_t reply;
            int ret;

            memset(&reply, 0, sizeof(reply));
            ret = simple_d2d_bus_dispatch(bus,
                                          notify->data.local_request.message,
                                          notify->data.local_request.ingress_link,
                                          &notify->data.local_request.src_endpoint,
                                          &reply);
            if (ret == 0)
            {
                (void)d2d_forwarder_complete_local(simple_d2d_service_forwarder(&bus->service),
                                                   notify->data.local_request.handle,
                                                   &reply);
            }
            else
            {
                (void)d2d_forwarder_fail_local(simple_d2d_service_forwarder(&bus->service),
                                               notify->data.local_request.handle,
                                               D2D_REQUEST_FAIL_REJECTED);
            }
            break;
        }
        case D2D_FORWARDER_NOTIFY_OUTBOUND_DONE:
            simple_d2d_bus_fill_event_from_message(&event,
                                                   SIMPLE_D2D_BUS_EVENT_FORWARD_DONE,
                                                   notify->data.outbound_done.request,
                                                   notify->data.outbound_done.target_link,
                                                   NULL,
                                                   &notify->data.outbound_done.target_endpoint);
            event.cmd = notify->data.outbound_done.request_cmd;
            (void)simple_d2d_bus_fill_app_fields(bus,
                                                  &event,
                                                  notify->data.outbound_done.request_cmd);
            if (event.reply_cmd == D2D_CMD_NONE &&
                notify->data.outbound_done.reply)
            {
                event.reply_cmd = notify->data.outbound_done.reply->header.cmd;
            }
            snprintf(event.msg_id, sizeof(event.msg_id), "%s",
                     notify->data.outbound_done.msg_id);
            event.ret = 0;
            event.reason = "forward_ack";
            simple_d2d_bus_emit(bus, &event);
            break;
        case D2D_FORWARDER_NOTIFY_OUTBOUND_FAIL:
            simple_d2d_bus_fill_event_from_message(&event,
                                                   SIMPLE_D2D_BUS_EVENT_FAIL,
                                                   notify->data.outbound_fail.request,
                                                   notify->data.outbound_fail.target_link,
                                                   NULL,
                                                   &notify->data.outbound_fail.target_endpoint);
            event.cmd = notify->data.outbound_fail.request_cmd;
            (void)simple_d2d_bus_fill_app_fields(bus,
                                                  &event,
                                                  notify->data.outbound_fail.request_cmd);
            if (event.reply_cmd == D2D_CMD_NONE &&
                notify->data.outbound_fail.reply)
            {
                event.reply_cmd = notify->data.outbound_fail.reply->header.cmd;
            }
            snprintf(event.msg_id, sizeof(event.msg_id), "%s",
                     notify->data.outbound_fail.msg_id);
            event.ret = -EIO;
            event.reason = "forward_fail";
            simple_d2d_bus_emit(bus, &event);
            break;
        case D2D_FORWARDER_NOTIFY_OUTBOUND_TIMEOUT:
            simple_d2d_bus_fill_event_from_message(&event,
                                                   SIMPLE_D2D_BUS_EVENT_TIMEOUT,
                                                   notify->data.outbound_timeout.request,
                                                   notify->data.outbound_timeout.target_link,
                                                   NULL,
                                                   &notify->data.outbound_timeout.target_endpoint);
            event.cmd = notify->data.outbound_timeout.request_cmd;
            (void)simple_d2d_bus_fill_app_fields(bus,
                                                  &event,
                                                  notify->data.outbound_timeout.request_cmd);
            snprintf(event.msg_id, sizeof(event.msg_id), "%s",
                     notify->data.outbound_timeout.msg_id);
            event.ret = -ETIMEDOUT;
            event.reason = "forward_timeout";
            simple_d2d_bus_emit(bus, &event);
            break;
        case D2D_FORWARDER_NOTIFY_LINK_ERROR:
            memset(&event, 0, sizeof(event));
            event.type = SIMPLE_D2D_BUS_EVENT_LINK_ERROR;
            event.link = notify->data.link_error.link_kind;
            event.src = notify->data.link_error.local_endpoint;
            event.dst = notify->data.link_error.remote_endpoint;
            event.ret = notify->data.link_error.error_code;
            event.reason = notify->data.link_error.operation;
            (void)simple_d2d_bus_fill_app_fields(bus,
                                                  &event,
                                                  (d2d_cmd_t)notify->data.link_error.opaque);
            simple_d2d_bus_emit(bus, &event);
            break;
        default:
            break;
    }
}

void simple_d2d_bus_config_init(simple_d2d_bus_config_t *cfg)
{
    if (!cfg)
    {
        return;
    }
    memset(cfg, 0, sizeof(*cfg));
    cfg->timeout_ms = 5000U;
}

int simple_d2d_bus_register_app(simple_d2d_bus_t *bus,
                                const simple_d2d_bus_app_t *app)
{
    size_t i;

    if (!bus || !app || app->request_cmd == D2D_CMD_NONE ||
        app->reply_cmd == D2D_CMD_NONE || !app->handler ||
        !app->app_name || app->app_name[0] == '\0' ||
        bus->app_count >= SIMPLE_D2D_BUS_MAX_APPS || bus->started)
    {
        return -EINVAL;
    }
    for (i = 0; i < bus->app_count; ++i)
    {
        if (bus->apps[i].request_cmd == app->request_cmd)
        {
            return -EEXIST;
        }
    }
    bus->apps[bus->app_count++] = *app;
    return 0;
}

int simple_d2d_bus_start(simple_d2d_bus_t *bus,
                         const simple_d2d_bus_config_t *cfg)
{
    simple_d2d_service_config_t service_cfg;
    int ret;

    if (!bus || !cfg || cfg->mode == 0 || bus->started)
    {
        return -EINVAL;
    }

    bus->cfg = *cfg;
    if (bus->cfg.timeout_ms == 0)
    {
        bus->cfg.timeout_ms = 5000U;
    }

    memset(&service_cfg, 0, sizeof(service_cfg));
    service_cfg.mode = bus->cfg.mode;
    service_cfg.local_dev_type = bus->cfg.local_dev_type;
    service_cfg.eth_if = bus->cfg.eth_if;
    service_cfg.plc_if = bus->cfg.plc_if;
    service_cfg.eth_local = bus->cfg.eth_local;
    service_cfg.plc_local = bus->cfg.plc_local;
    service_cfg.timeout_ms = bus->cfg.timeout_ms;
    service_cfg.event_cb = simple_d2d_bus_adapter_event_cb;
    service_cfg.route_cb = simple_d2d_bus_route_cb;
    service_cfg.notify_cb = simple_d2d_bus_notify_cb;
    service_cfg.user_data = bus;

    ret = simple_d2d_service_start(&bus->service, &service_cfg);
    if (ret != 0)
    {
        return ret;
    }
    bus->started = true;
    return 0;
}

void simple_d2d_bus_stop(simple_d2d_bus_t *bus)
{
    if (!bus)
    {
        return;
    }
    simple_d2d_service_stop(&bus->service);
    bus->started = false;
}

int simple_d2d_bus_send_request(simple_d2d_bus_t *bus,
                                const d2d_message_t *message,
                                const d2d_adapter_request_t *tx)
{
    if (!bus || !message || !tx || !bus->started)
    {
        return -EINVAL;
    }
    if (simple_d2d_service_adapter(&bus->service))
    {
        return d2d_adapter_send_request(simple_d2d_service_adapter(&bus->service),
                                        message,
                                        tx);
    }
    if (simple_d2d_service_forwarder(&bus->service))
    {
        return d2d_forwarder_send_request(simple_d2d_service_forwarder(&bus->service),
                                          message,
                                          tx);
    }
    return -EINVAL;
}

simple_d2d_service_t *simple_d2d_bus_service(simple_d2d_bus_t *bus)
{
    return bus ? &bus->service : NULL;
}

#ifdef SIMPLE_D2D_BUS_ENABLE_TEST_HOOKS
int simple_d2d_bus_dispatch_for_test(simple_d2d_bus_t *bus,
                                     const d2d_message_t *request,
                                     d2d_link_kind_t ingress_link,
                                     const transport_endpoint_t *src,
                                     d2d_message_t *reply)
{
    return simple_d2d_bus_dispatch(bus, request, ingress_link, src, reply);
}

int simple_d2d_bus_route_for_test(simple_d2d_bus_t *bus,
                                  const d2d_message_t *message,
                                  d2d_link_kind_t ingress_link,
                                  const transport_endpoint_t *src,
                                  d2d_forwarder_route_result_t *result)
{
    return simple_d2d_bus_route_cb(bus, 0, message, ingress_link, src, result);
}

void simple_d2d_bus_adapter_event_for_test(simple_d2d_bus_t *bus,
                                           const d2d_adapter_event_t *event)
{
    simple_d2d_bus_adapter_event_cb(bus, event);
}

void simple_d2d_bus_notify_for_test(simple_d2d_bus_t *bus,
                                    const d2d_forwarder_notify_t *notify)
{
    simple_d2d_bus_notify_cb(bus, notify);
}
#endif
