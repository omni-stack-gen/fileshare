/**
 * @file
 * @brief simple 示例应用 PLC 逻辑地址推导工具实现。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_plc_addr.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>

#include "topology_ip.h"

int simple_plc_addr_parse_ipv4_host(const char *ip, uint32_t *out)
{
    struct in_addr in;

    if (!ip || !out || inet_aton(ip, &in) == 0 || in.s_addr == INADDR_NONE)
    {
        return -EINVAL;
    }
    *out = ntohl(in.s_addr);
    return 0;
}

int simple_plc_addr_from_device_id(const char *device_id, uint32_t *out)
{
    topology_ip_info_t info;
    uint32_t addr;
    int ret;

    if (!device_id || !out || topology_ip_parse_device_id(device_id, &info) != 0)
    {
        return -EINVAL;
    }

    if (info.device_type == TOPOLOGY_DEVICE_CONVERTER_BOX)
    {
        return simple_plc_addr_parse_ipv4_host(info.upstream_unit_ip, out);
    }
    if (info.raw_z > 255U)
    {
        return -EINVAL;
    }

    switch (info.device_type)
    {
    case TOPOLOGY_DEVICE_INDOOR_MACHINE:
    case TOPOLOGY_DEVICE_UNIT_MACHINE:
    case TOPOLOGY_DEVICE_MANAGEMENT_MACHINE:
        addr = ((uint32_t)10U << 24) |
               ((uint32_t)info.building << 16) |
               ((uint32_t)info.raw_y << 8) |
               (uint32_t)info.raw_z;
        if (addr == 0)
        {
            return -EINVAL;
        }
        *out = addr;
        return 0;
    default:
        ret = -EINVAL;
        break;
    }

    return ret;
}

int simple_plc_addr_derive_indoor_device_id(const char *unit_device_id,
                                            const char *room_id,
                                            char *buf,
                                            size_t len)
{
    topology_ip_info_t unit;
    char *end = NULL;
    unsigned long room_no;
    unsigned long floor;
    unsigned long room;

    if (!unit_device_id || !room_id || !buf || len == 0 ||
        topology_ip_parse_device_id(unit_device_id, &unit) != 0 ||
        unit.device_type != TOPOLOGY_DEVICE_UNIT_MACHINE)
    {
        return -EINVAL;
    }

    errno = 0;
    room_no = strtoul(room_id, &end, 10);
    if (errno != 0 || !end || *end != '\0' || room_no < 101UL || room_no > 9999UL)
    {
        return -EINVAL;
    }
    floor = room_no / 100UL;
    room = room_no % 100UL;
    if (floor == 0 || floor > 99UL || room == 0 || room > 99UL)
    {
        return -EINVAL;
    }

    return topology_ip_format_indoor_machine(unit.building,
                                             (uint8_t)floor,
                                             (uint8_t)room,
                                             false,
                                             buf,
                                             len);
}
