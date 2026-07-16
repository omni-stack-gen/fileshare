#ifndef __TRANSPORT_ENDPOINT_H__
#define __TRANSPORT_ENDPOINT_H__

#include <stdint.h>
#include <stdbool.h>

// 通用地址宏定义 表示任意地址
#define ADDR_ANY (0x0)

// 传输层端点 (主机字节序 地址 + 端口号)
typedef struct transport_endpoint
{
	uint32_t addr;    // 主机字节序 地址
	uint16_t port;    // 主机字节序 端口号
} transport_endpoint_t;

/**
 * @brief 判断两个传输端点是否严格相等 (用于路由表的增删查改)
 * @param ep1 端点 1
 * @param ep2 端点 2
 * @return true 相等, false 不等
 */
bool transport_endpoint_is_equal(const transport_endpoint_t *ep1, const transport_endpoint_t *ep2);

/**
 * @brief 路由匹配检查 (支持通配符: addr为ADDR_ANY或port为0时视为匹配任意)
 * @param rule 路由表里配置的规则端点 (允许包含通配符)
 * @param pkt  实际收到的数据包源端点 (具体的 IP 和端口)
 * @return true 匹配成功, false 匹配失败
 */
bool transport_endpoint_match(const transport_endpoint_t *rule, const transport_endpoint_t *pkt);

#endif /* __TRANSPORT_ENDPOINT_H__ */