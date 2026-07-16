/**
 * @file
 * @brief simple D2D 0x03 bus DEBUG observer adapter implementation.
 */

#include "simple_d2d_bus_debug.h"

#include "simple_log.h"

static const char *simple_d2d_bus_debug_event_name(simple_d2d_bus_event_type_t type)
{
    switch (type)
    {
        case SIMPLE_D2D_BUS_EVENT_RX:
            return "rx";
        case SIMPLE_D2D_BUS_EVENT_DISPATCH:
            return "dispatch";
        case SIMPLE_D2D_BUS_EVENT_LOCAL_DONE:
            return "local_done";
        case SIMPLE_D2D_BUS_EVENT_FORWARD_DONE:
            return "forward_done";
        case SIMPLE_D2D_BUS_EVENT_FAIL:
            return "fail";
        case SIMPLE_D2D_BUS_EVENT_TIMEOUT:
            return "timeout";
        case SIMPLE_D2D_BUS_EVENT_LINK_ERROR:
            return "link_error";
        case SIMPLE_D2D_BUS_EVENT_APP_REJECT:
            return "app_reject";
        default:
            return "unknown";
    }
}

static const char *simple_d2d_bus_debug_link_name(d2d_link_kind_t link)
{
    switch (link)
    {
        case D2D_LINK_ETH:
            return "eth";
        case D2D_LINK_PLC:
            return "plc";
        default:
            return "none";
    }
}

void simple_d2d_bus_debug_observer(void *user_ctx,
                                   const simple_d2d_bus_event_t *event)
{
    (void)user_ctx;
    if (!event)
    {
        return;
    }

    SIMPLE_LOGD("d2d bus event: event=%s app=%s cmd=%d reply_cmd=%d "
                "msg_id=%s link=%s src=0x%08x:%u dst=0x%08x:%u "
                "ret=%d reason=%s\n",
                simple_d2d_bus_debug_event_name(event->type),
                event->app_name ? event->app_name : "-",
                event->cmd,
                event->reply_cmd,
                event->msg_id[0] ? event->msg_id : "-",
                simple_d2d_bus_debug_link_name(event->link),
                event->src.addr,
                event->src.port,
                event->dst.addr,
                event->dst.port,
                event->ret,
                event->reason ? event->reason : "-");
}
