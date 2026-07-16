/**
 * @file
 * @brief simple 示例应用 IPv4 工具函数。
 *
 * 本文件属于 examples 应用层，用于把 IPv4 字符串或网卡地址转换为 host byte order
 * 的 `uint32_t`，供各 simple main 入口解析启动参数。
 */

#ifndef SIMPLE_IPV4_UTILS_H
#define SIMPLE_IPV4_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief 解析 IPv4 字符串为 host byte order 地址。
 *
 * @param[in] ip IPv4 点分十进制字符串，不能为 NULL。
 * @param[out] out 地址输出，host byte order，不能为 NULL。
 * @retval true 解析成功。
 * @retval false 参数非法或字符串不是合法 IPv4 地址。
 */
bool simple_ipv4_parse_host_order(const char *ip, uint32_t *out);

/**
 * @brief 读取指定网卡 IPv4 地址并转换为 host byte order。
 *
 * @param[in] ifname 网卡名，不能为 NULL。
 * @param[out] out 地址输出，host byte order，不能为 NULL。
 * @retval true 读取成功。
 * @retval false 参数非法、网卡不存在或网卡未配置 IPv4 地址。
 */
bool simple_ipv4_get_if_host_order(const char *ifname, uint32_t *out);

/**
 * @brief 将 host byte order IPv4 地址格式化为点分十进制字符串。
 *
 * 调用方提供输出缓冲区，本函数不持有缓冲区，也不会返回静态存储。格式化成功时，
 * 返回值等于 `buf`；如果 `buf` 为 NULL 或 `buf_len` 为 0，则返回字面量
 * `"<null>"`；如果点分十进制格式化失败，会在缓冲区内写入 `0x%x` 形式的
 * host order 地址作为诊断兜底。
 *
 * @param[in] host_addr host byte order IPv4 地址。
 * @param[out] buf 输出缓冲区，可为 NULL；长度由 `buf_len` 指定，单位为字节。
 * @param[in] buf_len 输出缓冲区长度，单位为字节。
 * @return 成功或兜底格式化时返回 `buf`；缓冲区非法时返回 `"<null>"`。
 */
const char *simple_ipv4_to_str(uint32_t host_addr, char *buf, size_t buf_len);

#endif /* SIMPLE_IPV4_UTILS_H */
