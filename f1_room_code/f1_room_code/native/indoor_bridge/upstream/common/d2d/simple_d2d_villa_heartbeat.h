/**
 * @file
 * @brief simple 二次确认机/小门口机 25000 心跳来源服务公共头文件。
 *
 * 本文件属于 examples 应用层，用于 X9 二次确认机通过 SPE TCP 向所属室内机
 * 上报 25000 心跳，并让室内机把该心跳转换为在线来源事实。
 */

#ifndef SIMPLE_D2D_VILLA_HEARTBEAT_H
#define SIMPLE_D2D_VILLA_HEARTBEAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pthread.h>

#include "simple_d2d_config.h"
#include "topology_ip.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 二线设备 TCP/JSON 协议默认端口。 */
#define SIMPLE_D2D_VILLA_HEARTBEAT_DEFAULT_ETH_PORT (0x9982U)
/** @brief 默认二次确认机心跳周期，单位毫秒。 */
#define SIMPLE_D2D_VILLA_HEARTBEAT_DEFAULT_INTERVAL_MS (120000U)
/** @brief 默认二次确认机在线 TTL，单位毫秒。 */
#define SIMPLE_D2D_VILLA_HEARTBEAT_DEFAULT_TTL_MS (300000U)
/** @brief devModel/sysVer/appVer 缓冲区长度。 */
#define SIMPLE_D2D_VILLA_HEARTBEAT_TEXT_MAX (64U)

typedef enum
{
    /** 二次确认机侧，周期发送 25000。 */
    SIMPLE_D2D_VILLA_HEARTBEAT_ROLE_CLIENT = 1,
    /** 所属室内机侧，接收 25000 并维护在线事实。 */
    SIMPLE_D2D_VILLA_HEARTBEAT_ROLE_SERVER = 2
} simple_d2d_villa_heartbeat_role_t;

/**
 * @brief 二次确认机 25000 心跳在线状态。
 */
typedef struct
{
    /** 二次确认机 topology device_id。 */
    char device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** 设备 MAC 地址，短横线格式，来自 25000 `mac`。 */
    char mac[TOPOLOGY_MAC_STR_MAX];
    /** 设备型号字符串，来自 25000 `devModel`。 */
    char dev_model[SIMPLE_D2D_VILLA_HEARTBEAT_TEXT_MAX];
    /** 系统版本字符串，来自 25000 `sysVer`。 */
    char sys_ver[SIMPLE_D2D_VILLA_HEARTBEAT_TEXT_MAX];
    /** 应用版本字符串，来自 25000 `appVer`。 */
    char app_ver[SIMPLE_D2D_VILLA_HEARTBEAT_TEXT_MAX];
    /** 最近一次 heartbeat 来源 endpoint。 */
    transport_endpoint_t endpoint;
    /** 最近一次观测时间，单位毫秒。 */
    uint64_t observed_ms;
    /** 在线 TTL，单位毫秒。 */
    uint32_t ttl_ms;
    /** 25000 `devType`，二次确认机 v1 使用 3。 */
    int dev_type;
    /** true 表示 heartbeat 状态在线。 */
    bool online;
} simple_d2d_villa_heartbeat_status_t;

/**
 * @brief 二次确认机 heartbeat 状态更新回调。
 *
 * @param user_ctx 用户上下文。
 * @param status 当前状态快照；仅在回调期间有效。
 */
typedef void (*simple_d2d_villa_heartbeat_update_fn)(
    void *user_ctx,
    const simple_d2d_villa_heartbeat_status_t *status);

/**
 * @brief 二次确认机 heartbeat 回调配置。
 */
typedef struct
{
    /** D2D bus observer 配置，用于协议收发诊断。 */
    simple_d2d_observer_config_t observer;
    /** server 侧 online/offline 状态更新回调。 */
    simple_d2d_villa_heartbeat_update_fn update_cb;
    /** 回调用户上下文。 */
    void *user_ctx;
} simple_d2d_villa_heartbeat_callbacks_t;

/**
 * @brief 二次确认机 heartbeat runtime 配置。
 */
typedef struct
{
    /** runtime 角色：client 发送 25000，server 接收 25000。 */
    simple_d2d_villa_heartbeat_role_t role;
    /** 本机/对端 device_id 与 devType 配置。 */
    simple_d2d_identity_config_t identity;
    /** SPE/LAN TCP endpoint 配置。 */
    simple_d2d_endpoint_config_t eth;
    /** 25000 文本字段配置。 */
    simple_d2d_device_text_config_t text;
    /** 心跳周期、TTL 和请求超时配置。 */
    simple_d2d_timing_config_t timing;
    /** 可选共享 D2D service；非 NULL 时不再创建独立 bus/service。 */
    simple_d2d_service_t *shared_service;
    /** observer 与状态更新回调。 */
    simple_d2d_villa_heartbeat_callbacks_t callbacks;
} simple_d2d_villa_heartbeat_config_t;

/**
 * @brief 二次确认机 heartbeat runtime。
 *
 * 字段由 runtime 内部维护，调用方应通过 init/start/stop/deinit 管理生命周期。
 */
typedef struct
{
    /** 启动时保存的配置。 */
    simple_d2d_villa_heartbeat_config_t cfg;
    /** owned mode 下使用的 D2D bus。 */
    simple_d2d_bus_t bus;
    /** server 侧最近一次 heartbeat 状态。 */
    simple_d2d_villa_heartbeat_status_t status;
    /** 保护状态和停止标记的互斥锁。 */
    pthread_mutex_t lock;
    /** client 周期发送线程。 */
    pthread_t thread;
    /** true 表示 lock 已初始化。 */
    bool lock_ready;
    /** true 表示周期发送线程已启动。 */
    bool thread_started;
    /** true 表示 runtime 已启动。 */
    bool started;
    /** true 表示请求周期发送线程退出。 */
    bool stop_requested;
} simple_d2d_villa_heartbeat_t;

/**
 * @brief 初始化二次确认机 heartbeat 配置默认值。
 *
 * @param cfg 配置对象。
 */
void simple_d2d_villa_heartbeat_config_init(
    simple_d2d_villa_heartbeat_config_t *cfg);

/**
 * @brief 初始化二次确认机 heartbeat runtime。
 *
 * @param heartbeat runtime 对象。
 * @param cfg runtime 配置。
 * @return 0 表示成功，负数表示失败。
 */
int simple_d2d_villa_heartbeat_init(
    simple_d2d_villa_heartbeat_t *heartbeat,
    const simple_d2d_villa_heartbeat_config_t *cfg);

/**
 * @brief 释放二次确认机 heartbeat runtime。
 *
 * @param heartbeat runtime 对象。
 */
void simple_d2d_villa_heartbeat_deinit(
    simple_d2d_villa_heartbeat_t *heartbeat);

/**
 * @brief 初始化并启动二次确认机 heartbeat runtime。
 *
 * @param heartbeat runtime 对象。
 * @param cfg runtime 配置。
 * @return 0 表示成功，负数表示失败。
 */
int simple_d2d_villa_heartbeat_start(
    simple_d2d_villa_heartbeat_t *heartbeat,
    const simple_d2d_villa_heartbeat_config_t *cfg);

/**
 * @brief 停止二次确认机 heartbeat runtime。
 *
 * @param heartbeat runtime 对象。
 */
void simple_d2d_villa_heartbeat_stop(
    simple_d2d_villa_heartbeat_t *heartbeat);

/**
 * @brief 构造 25000 heartbeat request。
 *
 * @param cfg client 配置。
 * @param message 输出 D2D 消息。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法或必填字段缺失。
 */
int simple_d2d_villa_heartbeat_build_request(
    const simple_d2d_villa_heartbeat_config_t *cfg,
    d2d_message_t *message);

/**
 * @brief 处理 25000 heartbeat request 并生成 25001 reply。
 *
 * @param heartbeat server runtime。
 * @param request 输入 25000 request。
 * @param ingress_link 收包链路类型。
 * @param src 来源 endpoint。
 * @param reply 输出 25001 reply。
 * @return 0 表示处理完成，负数表示参数或协议错误。
 */
int simple_d2d_villa_heartbeat_handle_request(
    simple_d2d_villa_heartbeat_t *heartbeat,
    const d2d_message_t *request,
    d2d_link_kind_t ingress_link,
    const transport_endpoint_t *src,
    d2d_message_t *reply);

/**
 * @brief 拷贝当前二次确认机 heartbeat 状态快照。
 *
 * @param heartbeat server runtime。
 * @param out 输出数组。
 * @param capacity 输出数组容量。
 * @return 实际拷贝的状态数量。
 */
size_t simple_d2d_villa_heartbeat_snapshot(
    simple_d2d_villa_heartbeat_t *heartbeat,
    simple_d2d_villa_heartbeat_status_t *out,
    size_t capacity);

/**
 * @brief 获取 heartbeat runtime 当前使用的 D2D service。
 *
 * @param heartbeat runtime 对象。
 * @return D2D service 指针；未启动或参数非法时返回 NULL。
 */
simple_d2d_service_t *simple_d2d_villa_heartbeat_bus_service(
    simple_d2d_villa_heartbeat_t *heartbeat);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_D2D_VILLA_HEARTBEAT_H */
