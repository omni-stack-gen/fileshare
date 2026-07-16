/**
 * @file
 * @brief simple 示例应用 PLC 逻辑地址推导工具。
 *
 * 本文件属于 examples 应用层，用于把 topology device_id 转换为当前 simple
 * 上板路径使用的 PLC 逻辑地址。
 */

#ifndef SIMPLE_PLC_ADDR_H
#define SIMPLE_PLC_ADDR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 从 topology device_id 推导 PLC 逻辑地址。
 *
 * @param[in] device_id topology device_id 字符串，不能为 NULL。
 * @param[out] out PLC 地址输出，host byte order，不能为 NULL。
 * @retval 0 推导成功。
 * @retval -EINVAL 参数非法或 device_id 类型不支持。
 */
int simple_plc_addr_from_device_id(const char *device_id, uint32_t *out);

/**
 * @brief 从单元机 device_id 和房号推导室内机 device_id。
 *
 * @param[in] unit_device_id 单元机 device_id，不能为 NULL。
 * @param[in] room_id room-sync 上报的房号，不能为 NULL。
 * @param[out] buf device_id 输出缓冲区，不能为 NULL。
 * @param[in] len 输出缓冲区长度。
 * @retval 0 推导成功。
 * @retval -EINVAL 参数非法或房号不符合当前规则。
 * @retval -ENOSPC 输出缓冲区不足。
 */
int simple_plc_addr_derive_indoor_device_id(const char *unit_device_id,
                                            const char *room_id,
                                            char *buf,
                                            size_t len);

/**
 * @brief 解析 IPv4 字符串为 host byte order 地址。
 *
 * @param[in] ip IPv4 字符串，不能为 NULL。
 * @param[out] out 地址输出，host byte order，不能为 NULL。
 * @retval 0 解析成功。
 * @retval -EINVAL 参数非法或不是有效 IPv4。
 */
int simple_plc_addr_parse_ipv4_host(const char *ip, uint32_t *out);

#ifdef __cplusplus
}
#endif

#endif
