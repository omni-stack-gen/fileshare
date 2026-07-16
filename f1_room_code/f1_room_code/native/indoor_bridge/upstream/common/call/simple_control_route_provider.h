/**
 * @file
 * @brief simple production control route fact provider.
 *
 * 本文件属于 examples 应用层，用于把生产链路事实转换为 route runtime 可消费的
 * route fact。
 */

#ifndef SIMPLE_CONTROL_ROUTE_PROVIDER_H
#define SIMPLE_CONTROL_ROUTE_PROVIDER_H

#include <stdint.h>

#include "simple_control_route_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 构造 converter 到室内机 PLC 下行 route fact。
 *
 * @param fact 输出 route fact。
 * @param source route 来源，只允许 STATIC_BOOTSTRAP 或 BACKEND。
 * @param indoor_device_id 室内机 device_id。
 * @param indoor_plc_addr 室内机 PLC 地址，host byte order。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法或 source 不支持。
 * @retval -ENOTSUP device_id 类型不是室内机。
 */
int simple_control_route_provider_build_converter_downlink(
    simple_control_route_fact_t *fact,
    simple_control_route_source_t source,
    const char *indoor_device_id,
    uint32_t indoor_plc_addr);

/**
 * @brief 构造 converter 到门口机的 UDP 上行 route fact。
 *
 * @param fact 输出 route fact。
 * @param unit_device_id 门口机 device_id。
 * @param door_ip 门口机 LAN IP，host byte order。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOTSUP device_id 类型不是门口机。
 */
int simple_control_route_provider_build_converter_uplink(
    simple_control_route_fact_t *fact,
    const char *unit_device_id,
    uint32_t door_ip);

/**
 * @brief 构造门口机从 directory 获得的 converter 在线 route fact。
 *
 * @param fact 输出 route fact。
 * @param indoor_device_id 室内机 device_id。
 * @param converter_ip converter 下行侧 IP，host byte order。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOTSUP device_id 类型不是室内机。
 */
int simple_control_route_provider_build_directory_converter_online(
    simple_control_route_fact_t *fact,
    const char *indoor_device_id,
    uint32_t converter_ip);

/**
 * @brief 构造门口机 directory converter 离线 route fact。
 *
 * @param fact 输出 route fact。
 * @param indoor_device_id 室内机 device_id。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOTSUP device_id 类型不是室内机。
 */
int simple_control_route_provider_build_directory_converter_offline(
    simple_control_route_fact_t *fact,
    const char *indoor_device_id);

/**
 * @brief 构造门口机观测到管理机入站 peer 后的 route fact。
 *
 * @param fact 输出 route fact。
 * @param manage_device_id 管理机 device_id。
 * @param src_ep 入站消息来源 UDP endpoint。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法或 endpoint 不是有效 UDP endpoint。
 * @retval -ENOTSUP device_id 类型不是管理机。
 */
int simple_control_route_provider_build_observed_manage_peer(
    simple_control_route_fact_t *fact,
    const char *manage_device_id,
    const call_endpoint_t *src_ep);

/**
 * @brief 构造室内机观测到二次确认机入站 peer 后的 route fact。
 *
 * @param fact 输出 route fact。
 * @param secondary_device_id 二次确认机 device_id。
 * @param src_ep 入站消息来源 UDP endpoint。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法或 endpoint 不是有效 UDP endpoint。
 * @retval -ENOTSUP device_id 类型不是二次确认机。
 */
int simple_control_route_provider_build_observed_secondary_confirm_peer(
    simple_control_route_fact_t *fact,
    const char *secondary_device_id,
    const call_endpoint_t *src_ep);

/**
 * @brief 构造二次确认机 observed peer 离线 route fact。
 *
 * @param fact 输出 route fact。
 * @param secondary_device_id 二次确认机 device_id。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOTSUP device_id 类型不是二次确认机。
 */
int simple_control_route_provider_build_observed_secondary_confirm_offline(
    simple_control_route_fact_t *fact,
    const char *secondary_device_id);

/**
 * @brief 构造显式 backend UDP route fact。
 *
 * @param fact 输出 route fact。
 * @param callee_device_id 被叫 device_id。
 * @param peer_ip 下一跳 IP，host byte order。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 */
int simple_control_route_provider_build_backend_udp(
    simple_control_route_fact_t *fact,
    const char *callee_device_id,
    uint32_t peer_ip);

/**
 * @brief 从 next-hop device_id 推导 UDP route fact。
 *
 * callee 可以不同于 next-hop device_id，例如管理机到室内机经门口机转发。
 *
 * @param fact 输出 route fact。
 * @param callee_device_id 被叫 device_id。
 * @param next_hop_device_id 用于 topology IP 推导的下一跳 device_id。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOTSUP next-hop device_id 类型不支持推导。
 */
int simple_control_route_provider_build_topology_udp(
    simple_control_route_fact_t *fact,
    const char *callee_device_id,
    const char *next_hop_device_id);

/**
 * @brief 构造门口机到 converter 的静态 bootstrap route fact。
 *
 * @param fact 输出 route fact。
 * @param indoor_device_id 室内机 device_id。
 * @param converter_ip converter IP，host byte order。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOTSUP device_id 类型不是室内机。
 */
int simple_control_route_provider_build_door_converter_bootstrap(
    simple_control_route_fact_t *fact,
    const char *indoor_device_id,
    uint32_t converter_ip);

/**
 * @brief 构造终端侧 PLC bootstrap route fact。
 *
 * @param fact 输出 route fact。
 * @param callee_device_id 被叫 device_id。
 * @param gateway_plc_addr PLC gateway 地址，host byte order。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 */
int simple_control_route_provider_build_terminal_plc_bootstrap(
    simple_control_route_fact_t *fact,
    const char *callee_device_id,
    uint32_t gateway_plc_addr);

/**
 * @brief 构造终端侧 UDP bootstrap route fact。
 *
 * @param fact 输出 route fact。
 * @param callee_device_id 被叫 device_id。
 * @param peer_ip 对端 IP，host byte order。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 */
int simple_control_route_provider_build_terminal_udp_bootstrap(
    simple_control_route_fact_t *fact,
    const char *callee_device_id,
    uint32_t peer_ip);

/**
 * @brief 构造二次确认机 heartbeat learned online route fact。
 *
 * @param fact 输出 route fact。
 * @param secondary_device_id 二次确认机 device_id。
 * @param peer_ip heartbeat 来源 IP，host byte order。
 * @param observed_ms 观测时间，单位毫秒。
 * @param ttl_ms learned route 有效期，单位毫秒。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 */
int simple_control_route_provider_build_secondary_confirm_learned_online(
    simple_control_route_fact_t *fact,
    const char *secondary_device_id,
    uint32_t peer_ip,
    uint64_t observed_ms,
    uint32_t ttl_ms);

/**
 * @brief 构造二次确认机 heartbeat learned offline route fact。
 *
 * @param fact 输出 route fact。
 * @param secondary_device_id 二次确认机 device_id。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 */
int simple_control_route_provider_build_secondary_confirm_learned_offline(
    simple_control_route_fact_t *fact,
    const char *secondary_device_id);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_CONTROL_ROUTE_PROVIDER_H */
