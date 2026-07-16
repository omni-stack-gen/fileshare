/**
 * @file
 * @brief simple topology-derived control route helper.
 *
 * 本文件属于 examples 应用层，用于把少量 topology device_id 推导为可尝试控制面下一跳。
 */

#ifndef SIMPLE_TOPOLOGY_ROUTE_PROVIDER_H
#define SIMPLE_TOPOLOGY_ROUTE_PROVIDER_H

#include "call_flow_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 从管理机或单元机 device_id 推导 UDP control next-hop。
 *
 * v1 只支持管理机和单元机。推导结果只是可尝试 route，不代表 presence online。
 *
 * @param[in] device_id topology device_id 字符串，不能为 NULL。
 * @param[out] next_hop route endpoint 输出，不能为 NULL。
 * @retval 0 推导成功。
 * @retval -EINVAL 参数非法或 device_id 无法解析。
 * @retval -ENOTSUP 当前设备类型不支持 topology-derived UDP route。
 */
int simple_topology_route_provider_build_udp_control(
    const char *device_id,
    call_endpoint_t *next_hop);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_TOPOLOGY_ROUTE_PROVIDER_H */
