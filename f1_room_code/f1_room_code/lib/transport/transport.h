#ifndef __TRANSPORT_API_H__
#define __TRANSPORT_API_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include "transport_endpoint.h"

typedef enum transport_log_level
{
	TRANSPORT_DEBUG = 1,
	TRANSPORT_INFO = 2,
	TRANSPORT_WARNING = 3,
	TRANSPORT_ERROR = 4,
} transport_log_level_t;

typedef void (*transport_log_func_t)(transport_log_level_t level,
                                     const char *file,
                                     const long line,
                                     const char *func,
                                     const char *format,
                                     va_list ap);

struct transport_mem_hooks
{
    void *(*malloc)(size_t size, const char *file, int line);
    void *(*realloc)(void *ptr, size_t size, const char *file, int line);
    void (*free)(void *ptr, const char *file, int line);
};

void transport_log_set_handler(transport_log_func_t function);
void transport_mem_register_hooks(const struct transport_mem_hooks *hooks);

// 单向 (收/发) 统计指标抽象结构
typedef struct transport_stat
{
	uint64_t total_pkts;        // 总包数
	uint64_t total_bytes;       // 总字节数

	uint64_t total_interval_ms; // 总间隔时间
	uint64_t avg_interval_ms;   // 平均每个包的间隔时间
	uint64_t min_interval_ms;   // 最小包间隔
	uint64_t max_interval_ms;   // 最大包间隔

	// ================= 速率统计 (Bytes/s) =================
	uint32_t current_rate_bps;  // 每秒平均速率 (Bytes/s)
	uint32_t max_rate_bps;      // 最大每秒平均速率 (Bytes/s)
	uint32_t min_rate_bps;      // 最小每秒平均速率 (Bytes/s)
	uint32_t avg_total_bps;     // 全局平均每次/每秒速率

	// ================= 速率统计 (Packets/s) =================
	uint32_t current_rate_pps;  // 当前每秒包数 (Packets/s)
	uint32_t max_rate_pps;      // 最大每秒包数 (Packets/s)
	uint32_t min_rate_pps;      // 最小每秒包数 (Packets/s)
	uint32_t avg_total_pps;     // 全局平均每秒包数 (Packets/s)
	// ========================================================

	float payload_utilization_pct; // 负载有效使用率 (百分比 0.0 ~ 100.0)
} transport_stat_t;

// 统一的传输层统计信息结构体
typedef struct transport_info
{
	// 队列与内存统计 (部分包含内存池的协议适用)
	size_t current_mem;
	size_t peak_mem;
	uint32_t current_rx_pkts;
	uint32_t peak_rx_pkts;

	uint64_t total_run_time_ms; // Socket 运行总时长

	// 接收与发送统计
	transport_stat_t rx;
	transport_stat_t tx;
} transport_info_t;

// 支持的底层协议类型
typedef enum transport_proto
{
	TRANSPORT_PROTO_PLC = 0,
	TRANSPORT_PROTO_UDP = 1,
	TRANSPORT_PROTO_TCP = 2
} transport_proto_t;

// 不透明的传输层句柄
typedef struct transport_handle transport_handle_t;

// 抽象底层的 socket 设置选项
typedef enum transport_opt
{
	TRANSPORT_OPT_RCVBUF = 1,       // 设置接收缓冲区大小
	TRANSPORT_OPT_SNDBUF = 2,       // 设置发送缓冲区大小
	TRANSPORT_OPT_REUSEADDR = 3,    // 设置地址复用
	TRANSPORT_OPT_BINDTODEVICE = 4, // 绑定特定网卡
	TRANSPORT_OPT_MCAST_JOIN = 5,   // 加入组播组
	TRANSPORT_OPT_MCAST_LEAVE = 6,  // 离开组播组
	TRANSPORT_OPT_MCAST_LOOP = 7,   // 设置组播本机回环(0关闭/1开启)
	TRANSPORT_OPT_NONBLOCK = 8,     // 设置底层 fd 非阻塞模式(0关闭/1开启)
} transport_opt_t;

/*
 * 组播控制参数（主机字节序）
 * - group_addr: 必填，组播地址
 * - if_addr: 可选，本机网卡 IP（0 表示未指定）
 * - ifindex: 可选，网卡索引（<=0 表示未指定）
 *
 * transport_setopt 的 optval/optlen 建议传该结构体；
 * 兼容历史调用也可传 uint32_t group_addr。
 */
typedef struct transport_mcast_opt
{
	uint32_t group_addr;
	uint32_t if_addr;
	int32_t ifindex;
} transport_mcast_opt_t;

// 定义底层的操作集接口
typedef struct transport_ops
{
	const char *name;
	int (*open)(const char *interface, transport_endpoint_t *ep);
	int (*send)(int fd, transport_endpoint_t *ep, const void *data, uint32_t len);
	int (*recv)(int fd, transport_endpoint_t *ep, void *buf, uint32_t buf_len, int timeout_ms);
	void (*close)(int fd);
	int (*get_info)(int fd, transport_info_t *info);
	int (*setopt)(int fd, transport_opt_t optname, const void *optval, uint32_t optlen);
} transport_ops_t;

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 动态注册传输层协议
 * @param proto 协议枚举类型
 * @param ops 底层操作接口集
 * @return 0 成功，-1 失败（如参数错误或已存在重复协议）
 */
int transport_register(transport_proto_t proto, const transport_ops_t *ops);

/**
 * @brief 注销传输层协议注册
 *
 * 仅从协议注册表移除 proto，不主动关闭已经通过 transport_open()
 * 创建的 handle；已打开 handle 仍需由调用方使用 transport_close() 关闭。
 *
 * @param proto 协议枚举类型
 * @return 0 成功，-1 失败（如未找到对应协议）
 */
int transport_unregister(transport_proto_t proto);

/**
 * @brief 统一打开接口
 * @param proto 底层协议类型 (PLC、UDP 或 TCP)
 * @param addr 监听的本地/源设备地址 (如 ADDR_ANY，主机字节序)
 * @param port 监听端口
 * @return 成功返回句柄指针，失败返回 NULL
 */
transport_handle_t *transport_open(transport_proto_t proto, const char *interface, transport_endpoint_t *ep);

/**
 * @brief 统一发送接口
 * @param h 传输层句柄
 * @param dst_ep 目标传输层端点
 * @param data 待发送数据
 * @param len 数据长度
 * @return 0 成功，-1 失败
 */
int transport_send(transport_handle_t *h, transport_endpoint_t *dst_ep, const void *data, uint32_t len);

/**
 * @brief 统一接收接口 (自带超时机制)
 * @param h 传输层句柄
 * @param src_ep [输出] 发送方传输层端点
 * @param buf 接收缓冲区
 * @param buf_len 缓冲区大小
 * @param timeout_ms 超时时间 (ms)
 * @return >0 实际长度; 0 逻辑关闭; -1 失败; -2 超时; -3 队列空
 */
int transport_recv(transport_handle_t *h, transport_endpoint_t *src_ep, void *buf, uint32_t buf_len, int timeout_ms);

/**
 * @brief 获取统计信息
 */
int transport_get_info(transport_handle_t *h, transport_info_t *info);

/**
 * @brief 统一关闭接口
 */
void transport_close(transport_handle_t *h);

/**
 * @brief 获取底层真实的系统 fd，用于外部将其加入系统的 select/epoll 循环中
 */
int transport_get_fd(transport_handle_t *h);

/**
 * @brief 统一的属性设置接口 (类似 setsockopt)
 * @param h 传输层句柄
 * @param optname 选项名称
 * @param optval 选项值指针
 * @param optlen 选项值长度
 * @return 0 成功，-1 失败
 */
int transport_setopt(transport_handle_t *h, transport_opt_t optname, const void *optval, uint32_t optlen);

#ifdef __cplusplus
}
#endif

#endif /* __TRANSPORT_API_H__ */
