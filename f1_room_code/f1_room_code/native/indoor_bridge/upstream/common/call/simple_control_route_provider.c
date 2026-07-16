#include "simple_control_route_provider.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <utils/util.h>

#include "simple_call_app.h"
#include "simple_topology_route_provider.h"
#include "topology/topology_ip.h"

static int route_provider_expect_type(const char *device_id,
                                      topology_device_type_t expected)
{
    topology_ip_info_t info;

    if (!IS_VALID_STRING(device_id))
    {
        return -EINVAL;
    }
    if (topology_ip_parse_device_id(device_id, &info) != 0)
    {
        return -EINVAL;
    }
    return info.device_type == expected ? 0 : -ENOTSUP;
}

static int route_provider_expect_secondary_confirm(const char *device_id)
{
    if (!IS_VALID_STRING(device_id))
    {
        return -EINVAL;
    }
    return strncmp(device_id, "3:", 2) == 0 ? 0 : -ENOTSUP;
}

static int route_provider_build_udp(simple_control_route_fact_t *fact,
                                    simple_control_route_source_t source,
                                    const char *callee_device_id,
                                    uint32_t addr)
{
    if (!fact || !IS_VALID_STRING(callee_device_id) || addr == 0)
    {
        return -EINVAL;
    }

    memset(fact, 0, sizeof(*fact));
    fact->source = source;
    fact->callee_device_id = callee_device_id;
    fact->online = true;
    fact->next_hop.link_kind = CALL_LINK_UDP;
    fact->next_hop.addr = addr;
    fact->next_hop.port = SIMPLE_CALL_CTRL_UDP_PORT;
    return 0;
}

int simple_control_route_provider_build_converter_downlink(
    simple_control_route_fact_t *fact,
    simple_control_route_source_t source,
    const char *indoor_device_id,
    uint32_t indoor_plc_addr)
{
    int ret;

    if (source != SIMPLE_CONTROL_ROUTE_SOURCE_STATIC_BOOTSTRAP &&
        source != SIMPLE_CONTROL_ROUTE_SOURCE_BACKEND)
    {
        return -EINVAL;
    }
    ret = route_provider_expect_type(indoor_device_id, TOPOLOGY_DEVICE_INDOOR_MACHINE);
    if (ret != 0)
    {
        return ret;
    }
    if (!fact || indoor_plc_addr == 0)
    {
        return -EINVAL;
    }

    memset(fact, 0, sizeof(*fact));
    fact->source = source;
    fact->callee_device_id = indoor_device_id;
    fact->online = true;
    fact->next_hop.link_kind = CALL_LINK_PLC;
    fact->next_hop.addr = indoor_plc_addr;
    fact->next_hop.port = SIMPLE_CALL_CTRL_PLC_PORT;
    return 0;
}

int simple_control_route_provider_build_converter_uplink(
    simple_control_route_fact_t *fact,
    const char *unit_device_id,
    uint32_t door_ip)
{
    int ret;

    ret = route_provider_expect_type(unit_device_id, TOPOLOGY_DEVICE_UNIT_MACHINE);
    if (ret != 0)
    {
        return ret;
    }
    return route_provider_build_udp(fact,
                                    SIMPLE_CONTROL_ROUTE_SOURCE_BACKEND,
                                    unit_device_id,
                                    door_ip);
}

int simple_control_route_provider_build_directory_converter_online(
    simple_control_route_fact_t *fact,
    const char *indoor_device_id,
    uint32_t converter_ip)
{
    int ret;

    ret = route_provider_expect_type(indoor_device_id, TOPOLOGY_DEVICE_INDOOR_MACHINE);
    if (ret != 0)
    {
        return ret;
    }
    return route_provider_build_udp(fact,
                                    SIMPLE_CONTROL_ROUTE_SOURCE_DIRECTORY,
                                    indoor_device_id,
                                    converter_ip);
}

int simple_control_route_provider_build_directory_converter_offline(
    simple_control_route_fact_t *fact,
    const char *indoor_device_id)
{
    int ret;

    ret = route_provider_expect_type(indoor_device_id, TOPOLOGY_DEVICE_INDOOR_MACHINE);
    if (ret != 0)
    {
        return ret;
    }
    if (!fact)
    {
        return -EINVAL;
    }

    memset(fact, 0, sizeof(*fact));
    fact->source = SIMPLE_CONTROL_ROUTE_SOURCE_DIRECTORY;
    fact->callee_device_id = indoor_device_id;
    fact->online = false;
    return 0;
}

int simple_control_route_provider_build_observed_manage_peer(
    simple_control_route_fact_t *fact,
    const char *manage_device_id,
    const call_endpoint_t *src_ep)
{
    int ret;

    ret = route_provider_expect_type(manage_device_id,
                                     TOPOLOGY_DEVICE_MANAGEMENT_MACHINE);
    if (ret != 0)
    {
        return ret;
    }
    if (!fact || !src_ep || src_ep->link_kind != CALL_LINK_UDP ||
        src_ep->addr == 0 || src_ep->port == 0)
    {
        return -EINVAL;
    }

    memset(fact, 0, sizeof(*fact));
    fact->source = SIMPLE_CONTROL_ROUTE_SOURCE_OBSERVED_PEER;
    fact->callee_device_id = manage_device_id;
    fact->online = true;
    fact->next_hop = *src_ep;
    return 0;
}

int simple_control_route_provider_build_observed_secondary_confirm_peer(
    simple_control_route_fact_t *fact,
    const char *secondary_device_id,
    const call_endpoint_t *src_ep)
{
    int ret;

    ret = route_provider_expect_secondary_confirm(secondary_device_id);
    if (ret != 0)
    {
        return ret;
    }
    if (!fact || !src_ep || src_ep->link_kind != CALL_LINK_UDP ||
        src_ep->addr == 0 || src_ep->port == 0)
    {
        return -EINVAL;
    }

    memset(fact, 0, sizeof(*fact));
    fact->source = SIMPLE_CONTROL_ROUTE_SOURCE_OBSERVED_PEER;
    fact->callee_device_id = secondary_device_id;
    fact->online = true;
    fact->next_hop = *src_ep;
    return 0;
}

int simple_control_route_provider_build_observed_secondary_confirm_offline(
    simple_control_route_fact_t *fact,
    const char *secondary_device_id)
{
    int ret;

    ret = route_provider_expect_secondary_confirm(secondary_device_id);
    if (ret != 0)
    {
        return ret;
    }
    if (!fact)
    {
        return -EINVAL;
    }

    memset(fact, 0, sizeof(*fact));
    fact->source = SIMPLE_CONTROL_ROUTE_SOURCE_OBSERVED_PEER;
    fact->callee_device_id = secondary_device_id;
    fact->online = false;
    return 0;
}

int simple_control_route_provider_build_backend_udp(
    simple_control_route_fact_t *fact,
    const char *callee_device_id,
    uint32_t peer_ip)
{
    return route_provider_build_udp(fact,
                                    SIMPLE_CONTROL_ROUTE_SOURCE_BACKEND,
                                    callee_device_id,
                                    peer_ip);
}

int simple_control_route_provider_build_topology_udp(
    simple_control_route_fact_t *fact,
    const char *callee_device_id,
    const char *next_hop_device_id)
{
    call_endpoint_t next_hop;
    int ret;

    if (!fact || !IS_VALID_STRING(callee_device_id) ||
        !IS_VALID_STRING(next_hop_device_id))
    {
        return -EINVAL;
    }

    ret = simple_topology_route_provider_build_udp_control(next_hop_device_id, &next_hop);
    if (ret != 0)
    {
        return ret;
    }

    memset(fact, 0, sizeof(*fact));
    fact->source = SIMPLE_CONTROL_ROUTE_SOURCE_TOPOLOGY;
    fact->callee_device_id = callee_device_id;
    fact->online = true;
    fact->next_hop = next_hop;
    return 0;
}

int simple_control_route_provider_build_door_converter_bootstrap(
    simple_control_route_fact_t *fact,
    const char *indoor_device_id,
    uint32_t converter_ip)
{
    int ret;

    ret = route_provider_expect_type(indoor_device_id, TOPOLOGY_DEVICE_INDOOR_MACHINE);
    if (ret != 0)
    {
        return ret;
    }
    return route_provider_build_udp(fact,
                                    SIMPLE_CONTROL_ROUTE_SOURCE_STATIC_BOOTSTRAP,
                                    indoor_device_id,
                                    converter_ip);
}

int simple_control_route_provider_build_terminal_plc_bootstrap(
    simple_control_route_fact_t *fact,
    const char *callee_device_id,
    uint32_t gateway_plc_addr)
{
    if (!fact || !IS_VALID_STRING(callee_device_id) || gateway_plc_addr == 0)
    {
        return -EINVAL;
    }

    memset(fact, 0, sizeof(*fact));
    fact->source = SIMPLE_CONTROL_ROUTE_SOURCE_STATIC_BOOTSTRAP;
    fact->callee_device_id = callee_device_id;
    fact->online = true;
    fact->next_hop.link_kind = CALL_LINK_PLC;
    fact->next_hop.addr = gateway_plc_addr;
    fact->next_hop.port = SIMPLE_CALL_CTRL_PLC_PORT;
    return 0;
}

int simple_control_route_provider_build_terminal_udp_bootstrap(
    simple_control_route_fact_t *fact,
    const char *callee_device_id,
    uint32_t peer_ip)
{
    return route_provider_build_udp(fact,
                                    SIMPLE_CONTROL_ROUTE_SOURCE_STATIC_BOOTSTRAP,
                                    callee_device_id,
                                    peer_ip);
}

int simple_control_route_provider_build_secondary_confirm_learned_online(
    simple_control_route_fact_t *fact,
    const char *secondary_device_id,
    uint32_t peer_ip,
    uint64_t observed_ms,
    uint32_t ttl_ms)
{
    int ret;

    if (!IS_VALID_STRING(secondary_device_id))
    {
        return -EINVAL;
    }
    ret = route_provider_build_udp(fact,
                                   SIMPLE_CONTROL_ROUTE_SOURCE_REGISTRY,
                                   secondary_device_id,
                                   peer_ip);
    if (ret != 0)
    {
        return ret;
    }
    fact->observed_ms = observed_ms;
    fact->ttl_ms = ttl_ms;
    return 0;
}

int simple_control_route_provider_build_secondary_confirm_learned_offline(
    simple_control_route_fact_t *fact,
    const char *secondary_device_id)
{
    if (!IS_VALID_STRING(secondary_device_id))
    {
        return -EINVAL;
    }
    if (!fact)
    {
        return -EINVAL;
    }

    memset(fact, 0, sizeof(*fact));
    fact->source = SIMPLE_CONTROL_ROUTE_SOURCE_REGISTRY;
    fact->callee_device_id = secondary_device_id;
    fact->online = false;
    return 0;
}
