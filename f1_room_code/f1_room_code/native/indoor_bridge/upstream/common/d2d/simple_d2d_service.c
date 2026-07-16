/**
 * @file
 * @brief simple D2D runtime 服务实现。
 */

#include "simple_d2d_service.h"

#include <errno.h>
#include <string.h>

#include "transport.h"

static const char *simple_d2d_service_link_name(d2d_link_kind_t link)
{
    return link == D2D_LINK_PLC ? "plc" : "eth";
}

static int simple_d2d_service_open_link(simple_d2d_service_t *service,
                                        d2d_link_kind_t kind,
                                        transport_proto_t proto,
                                        const char *interface,
                                        const transport_endpoint_t *local,
                                        d2d_adapter_link_t *link,
                                        transport_handle_t **handle)
{
    if (!service || !local || !link || !handle)
    {
        return -EINVAL;
    }

    *handle = transport_open(proto, interface, (transport_endpoint_t *)local);
    if (!*handle)
    {
        return -EIO;
    }

    memset(link, 0, sizeof(*link));
    link->link_kind = kind;
    link->transport = *handle;
    link->local_endpoint = *local;
    link->name = simple_d2d_service_link_name(kind);
    link->user_data = service->cfg.user_data;
    return 0;
}

static uint32_t simple_d2d_service_timeout_ms(const simple_d2d_service_config_t *cfg)
{
    return cfg->timeout_ms != 0 ? cfg->timeout_ms : 5000U;
}

static int simple_d2d_service_start_adapter(simple_d2d_service_t *service,
                                            d2d_adapter_link_t *link)
{
    d2d_adapter_config_t adapter_cfg;
    uint32_t timeout_ms = simple_d2d_service_timeout_ms(&service->cfg);
    int ret;

    memset(&adapter_cfg, 0, sizeof(adapter_cfg));
    adapter_cfg.local_dev_type = service->cfg.local_dev_type;
    adapter_cfg.heartbeat_timeout_ms = timeout_ms;
    adapter_cfg.sync_timeout_ms = timeout_ms;
    adapter_cfg.request_timeout_ms = timeout_ms;
    adapter_cfg.inbound_timeout_ms = timeout_ms;
    adapter_cfg.poll_wait_ms = 20;
    adapter_cfg.links = link;
    adapter_cfg.link_count = 1;
    adapter_cfg.event_cb = service->cfg.event_cb;
    adapter_cfg.user_data = service->cfg.user_data;

    ret = d2d_adapter_create(&adapter_cfg, &service->adapter);
    if (ret != 0)
    {
        return ret;
    }
    return d2d_adapter_start(service->adapter) == 0 ? 0 : -EIO;
}

static int simple_d2d_service_start_forwarder(simple_d2d_service_t *service)
{
    d2d_forwarder_config_t fwd_cfg;
    uint32_t timeout_ms = simple_d2d_service_timeout_ms(&service->cfg);
    int ret;

    memset(&fwd_cfg, 0, sizeof(fwd_cfg));
    fwd_cfg.local_dev_type = service->cfg.local_dev_type;
    fwd_cfg.heartbeat_timeout_ms = timeout_ms;
    fwd_cfg.sync_timeout_ms = timeout_ms;
    fwd_cfg.request_timeout_ms = timeout_ms;
    fwd_cfg.inbound_timeout_ms = timeout_ms;
    fwd_cfg.poll_wait_ms = 20;
    fwd_cfg.links = service->links;
    fwd_cfg.link_count = 2;
    fwd_cfg.route_cb = service->cfg.route_cb;
    fwd_cfg.notify_cb = service->cfg.notify_cb;
    fwd_cfg.user_data = service->cfg.user_data;

    ret = d2d_forwarder_create(&fwd_cfg, &service->forwarder);
    if (ret != 0)
    {
        return ret;
    }
    return d2d_forwarder_start(service->forwarder) == 0 ? 0 : -EIO;
}

int simple_d2d_service_start(simple_d2d_service_t *service,
                             const simple_d2d_service_config_t *cfg)
{
    int ret = 0;

    if (!service || !cfg || cfg->mode == 0 || service->started)
    {
        return -EINVAL;
    }

    memset(service, 0, sizeof(*service));
    service->cfg = *cfg;

    switch (cfg->mode)
    {
        case SIMPLE_D2D_SERVICE_MODE_ADAPTER_PLC:
            ret = simple_d2d_service_open_link(service,
                                               D2D_LINK_PLC,
                                               TRANSPORT_PROTO_PLC,
                                               cfg->plc_if,
                                               &cfg->plc_local,
                                               &service->links[0],
                                               &service->plc_transport);
            if (ret == 0)
            {
                ret = simple_d2d_service_start_adapter(service, &service->links[0]);
            }
            break;
        case SIMPLE_D2D_SERVICE_MODE_ADAPTER_ETH:
            ret = simple_d2d_service_open_link(service,
                                               D2D_LINK_ETH,
                                               TRANSPORT_PROTO_TCP,
                                               cfg->eth_if,
                                               &cfg->eth_local,
                                               &service->links[0],
                                               &service->eth_transport);
            if (ret == 0)
            {
                ret = simple_d2d_service_start_adapter(service, &service->links[0]);
            }
            break;
        case SIMPLE_D2D_SERVICE_MODE_BRIDGE:
            ret = simple_d2d_service_open_link(service,
                                               D2D_LINK_ETH,
                                               TRANSPORT_PROTO_TCP,
                                               cfg->eth_if,
                                               &cfg->eth_local,
                                               &service->links[0],
                                               &service->eth_transport);
            if (ret != 0)
            {
                break;
            }
            ret = simple_d2d_service_open_link(service,
                                               D2D_LINK_PLC,
                                               TRANSPORT_PROTO_PLC,
                                               cfg->plc_if,
                                               &cfg->plc_local,
                                               &service->links[1],
                                               &service->plc_transport);
            if (ret == 0)
            {
                ret = simple_d2d_service_start_forwarder(service);
            }
            break;
        default:
            ret = -EINVAL;
            break;
    }

    if (ret != 0)
    {
        simple_d2d_service_stop(service);
        return ret;
    }
    service->started = true;
    return 0;
}

void simple_d2d_service_stop(simple_d2d_service_t *service)
{
    if (!service)
    {
        return;
    }
    if (service->adapter)
    {
        d2d_adapter_destroy(service->adapter);
    }
    if (service->forwarder)
    {
        d2d_forwarder_destroy(service->forwarder);
    }
    if (service->eth_transport)
    {
        transport_close(service->eth_transport);
    }
    if (service->plc_transport)
    {
        transport_close(service->plc_transport);
    }
    memset(service, 0, sizeof(*service));
}

d2d_adapter_t *simple_d2d_service_adapter(simple_d2d_service_t *service)
{
    return service ? service->adapter : NULL;
}

d2d_forwarder_t *simple_d2d_service_forwarder(simple_d2d_service_t *service)
{
    return service ? service->forwarder : NULL;
}
