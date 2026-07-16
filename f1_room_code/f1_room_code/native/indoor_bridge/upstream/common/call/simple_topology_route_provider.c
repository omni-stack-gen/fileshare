#include "simple_topology_route_provider.h"

#include <errno.h>
#include <string.h>

#include "simple_call_app.h"
#include "topology/topology_ip.h"

int simple_topology_route_provider_build_udp_control(
    const char *device_id,
    call_endpoint_t *next_hop)
{
    topology_ip_info_t info;
    uint32_t addr;

    if (!device_id || !next_hop)
    {
        return -EINVAL;
    }
    if (topology_ip_parse_device_id(device_id, &info) != 0)
    {
        return -EINVAL;
    }
    if (info.device_type != TOPOLOGY_DEVICE_MANAGEMENT_MACHINE &&
        info.device_type != TOPOLOGY_DEVICE_UNIT_MACHINE)
    {
        return -ENOTSUP;
    }
    if (info.raw_z > 255U)
    {
        return -EINVAL;
    }

    addr = ((uint32_t)10U << 24) |
           ((uint32_t)info.building << 16) |
           ((uint32_t)info.raw_y << 8) |
           (uint32_t)info.raw_z;
    if (addr == 0)
    {
        return -EINVAL;
    }

    memset(next_hop, 0, sizeof(*next_hop));
    next_hop->link_kind = CALL_LINK_UDP;
    next_hop->addr = addr;
    next_hop->port = SIMPLE_CALL_CTRL_UDP_PORT;
    return 0;
}
