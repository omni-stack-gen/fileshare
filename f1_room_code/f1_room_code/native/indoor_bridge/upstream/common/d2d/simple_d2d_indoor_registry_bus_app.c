#include "simple_d2d_indoor_registry_bus_app.h"

#include <errno.h>
#include <string.h>

#include "simple_log.h"

static int simple_d2d_indoor_registry_bus_handler(void *user_ctx,
                                                  const d2d_message_t *message,
                                                  d2d_link_kind_t ingress_link,
                                                  const transport_endpoint_t *src,
                                                  d2d_message_t *reply)
{
    simple_d2d_indoor_registry_t *registry =
        (simple_d2d_indoor_registry_t *)user_ctx;
    int ret;

    if (!registry || !message || !reply ||
        ingress_link != D2D_LINK_PLC ||
        message->header.cmd != D2D_CMD_INDOOR_SYNC_REQ)
    {
        return -EINVAL;
    }

    ret = simple_d2d_indoor_registry_handle_sync(registry, message, reply);
    SIMPLE_LOGD("room registry bus dispatch: ret=%d src=0x%08x:%u\n",
                ret,
                src ? src->addr : 0,
                src ? src->port : 0);
    return ret;
}

void simple_d2d_indoor_registry_bus_app_init(
    simple_d2d_bus_app_t *app,
    simple_d2d_indoor_registry_t *registry,
    bool bridge_local)
{
    if (!app)
    {
        return;
    }

    memset(app, 0, sizeof(*app));
    app->request_cmd = D2D_CMD_INDOOR_SYNC_REQ;
    app->reply_cmd = D2D_CMD_INDOOR_SYNC_ACK;
    app->app_name = "indoor_registry";
    app->handler = simple_d2d_indoor_registry_bus_handler;
    app->user_ctx = registry;
    app->bridge_local = bridge_local;
}
