/**
 * @file
 * @brief simple 室内机到 PLC CCO 的房号注册服务公共头文件。
 *
 * 本文件属于 examples 应用层，用于 NX5 室内机通过 PLC D2D 向 F1 converter 或
 * X9 这类 PLC CCO 上报房号在线状态。
 */

#ifndef SIMPLE_D2D_INDOOR_REGISTRY_H
#define SIMPLE_D2D_INDOOR_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <pthread.h>

#include "simple_d2d_config.h"
#include "simple_d2d_service.h"
#include "topology_ip.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 房号注册 D2D PLC 默认端口，统一使用 simple PLC D2D 控制端口。 */
#define SIMPLE_D2D_INDOOR_REGISTRY_DEFAULT_PLC_PORT SIMPLE_D2D_PLC_CONTROL_PORT
/** @brief PLC CCO 最多维护的室内机注册数量。 */
#define SIMPLE_D2D_INDOOR_REGISTRY_MAX_ROOMS (32U)
/** @brief 房号字符串缓冲区长度，包含结尾 NUL。 */
#define SIMPLE_D2D_INDOOR_REGISTRY_ROOM_ID_MAX (32U)
/** @brief 默认注册心跳周期，单位毫秒。 */
#define SIMPLE_D2D_INDOOR_REGISTRY_DEFAULT_HEARTBEAT_MS (10000U)
/** @brief 默认在线超时时间，单位毫秒。 */
#define SIMPLE_D2D_INDOOR_REGISTRY_DEFAULT_TTL_MS (30000U)

/** @brief 房号注册运行角色。 */
typedef enum
{
    /** 室内机侧，周期发送 20014 注册/心跳。 */
    SIMPLE_D2D_INDOOR_REGISTRY_ROLE_CLIENT = 1,
    /** PLC CCO 侧，接收 20014 并维护在线表。 */
    SIMPLE_D2D_INDOOR_REGISTRY_ROLE_SERVER = 2
} simple_d2d_indoor_registry_role_t;

/** @brief PLC CCO 侧维护的单个室内机房号状态。 */
typedef struct
{
    /** 房号，例如 1701。 */
    char room_id[SIMPLE_D2D_INDOOR_REGISTRY_ROOM_ID_MAX];
    /** 室内机 topology device_id，例如 1:10.2.17.1。 */
    char device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** 室内机 PLC 逻辑地址，host byte order。 */
    uint32_t plc_addr;
    /** true 表示在线。 */
    bool online;
    /** 最近一次注册/心跳时间，单位毫秒，monotonic。 */
    uint64_t last_seen_ms;
    /** 当前条目的有效期，单位毫秒。 */
    uint32_t ttl_ms;
} simple_d2d_indoor_registry_room_t;

/**
 * @brief PLC CCO 在线表变化回调。
 *
 * @param[in] user_ctx 调用方上下文。
 * @param[in] rooms 当前在线房间数组；count 为 0 时可为 NULL。
 * @param[in] count 当前在线房间数量。
 */
typedef void (*simple_d2d_indoor_registry_update_fn)(
    void *user_ctx,
    const simple_d2d_indoor_registry_room_t *rooms,
    size_t count);

/** @brief 房号注册回调集合。 */
typedef struct
{
    /** server 在线表变化回调。 */
    simple_d2d_indoor_registry_update_fn update_cb;
    /** 回调上下文。 */
    void *user_ctx;
} simple_d2d_indoor_registry_callbacks_t;

/** @brief 房号注册服务配置。 */
typedef struct
{
    /** 当前角色。 */
    simple_d2d_indoor_registry_role_t role;
    /** 本机/对端身份。 */
    simple_d2d_identity_config_t identity;
    /** PLC endpoint。 */
    simple_d2d_endpoint_config_t plc;
    /** client 上报的房号。server 可为 NULL。 */
    const char *room_id;
    /** 周期、TTL 和请求超时。 */
    simple_d2d_timing_config_t timing;
    /** 共享 D2D service；非 NULL 时不单独打开 PLC D2D 控制端口。 */
    simple_d2d_service_t *shared_service;
    /** 回调集合。 */
    simple_d2d_indoor_registry_callbacks_t callbacks;
} simple_d2d_indoor_registry_config_t;

/** @brief 房号注册服务运行态。 */
typedef struct
{
    /** 启动/配置副本。 */
    simple_d2d_indoor_registry_config_t cfg;
    /** D2D runtime 服务。 */
    simple_d2d_service_t service;
    /** 运行态互斥锁。 */
    pthread_mutex_t lock;
    /** worker 线程。 */
    pthread_t thread;
    /** PLC CCO 侧 room 表。 */
    simple_d2d_indoor_registry_room_t rooms[SIMPLE_D2D_INDOOR_REGISTRY_MAX_ROOMS];
    /** PLC CCO 侧 room 表数量。 */
    size_t room_count;
    /** lock 是否已初始化。 */
    bool lock_ready;
    /** worker 线程是否已启动。 */
    bool thread_started;
    /** runtime 是否已启动。 */
    bool started;
    /** 停止标志。 */
    bool stop_requested;
} simple_d2d_indoor_registry_t;

/**
 * @brief 填充房号注册默认配置。
 *
 * @param cfg 配置对象，不能为 NULL。
 */
void simple_d2d_indoor_registry_config_init(simple_d2d_indoor_registry_config_t *cfg);

/**
 * @brief 初始化房号注册运行态，只做内存/锁初始化。
 *
 * @param registry 注册 runtime 对象，不能为 NULL。
 * @param cfg 初始配置，不能为 NULL。
 * @retval 0 初始化成功。
 * @retval -EINVAL 参数非法。
 */
int simple_d2d_indoor_registry_init(simple_d2d_indoor_registry_t *registry,
                                    const simple_d2d_indoor_registry_config_t *cfg);

/**
 * @brief 释放 init 创建的资源。
 *
 * 已 start 的对象应先调用 simple_d2d_indoor_registry_stop()。
 *
 * @param registry 注册 runtime 对象，可为 NULL。
 */
void simple_d2d_indoor_registry_deinit(simple_d2d_indoor_registry_t *registry);

/**
 * @brief 启动房号注册服务。
 *
 * @param registry 注册 runtime 对象，不能为 NULL。
 * @param cfg 启动配置，不能为 NULL。
 * @return 0 表示成功，负数表示失败。
 */
int simple_d2d_indoor_registry_start(simple_d2d_indoor_registry_t *registry,
                                     const simple_d2d_indoor_registry_config_t *cfg);

/**
 * @brief 停止房号注册服务。
 *
 * @param registry 注册 runtime 对象，可为 NULL。
 */
void simple_d2d_indoor_registry_stop(simple_d2d_indoor_registry_t *registry);

/**
 * @brief 构造一条 20014 室内机房号注册/在线上报请求。
 *
 * @param cfg client 配置，不能为 NULL。
 * @param online true 表示在线上报，false 表示离线。
 * @param message 输出 D2D message，不能为 NULL。
 * @param entry 输出 room/device 条目，不能为 NULL。
 * @retval 0 构造成功。
 * @retval -EINVAL 参数非法或必填字段缺失。
 */
int simple_d2d_indoor_registry_build_request(
    const simple_d2d_indoor_registry_config_t *cfg,
    bool online,
    d2d_message_t *message,
    d2d_device_info_t *entry);

/**
 * @brief PLC CCO 侧处理一条 20014 请求并构造 20015 回复。
 *
 * @param registry server runtime，不能为 NULL。
 * @param message 输入 20014 request，不能为 NULL。
 * @param reply_out 输出 20015 reply，不能为 NULL。
 * @return 0 表示处理成功，负数表示失败。
 */
int simple_d2d_indoor_registry_handle_sync(simple_d2d_indoor_registry_t *registry,
                                           const d2d_message_t *message,
                                           d2d_message_t *reply_out);

/**
 * @brief 按给定时间执行一次超时扫描。
 *
 * @param registry server runtime，不能为 NULL。
 * @param now_ms 当前时间，单位毫秒。
 * @return 0 表示完成，负数表示失败。
 */
int simple_d2d_indoor_registry_expire(simple_d2d_indoor_registry_t *registry,
                                      uint64_t now_ms);

/**
 * @brief 复制当前注册表快照。
 *
 * @param registry server runtime，不能为 NULL。
 * @param rooms 输出 room 数组；为 NULL 时只返回当前数量。
 * @param capacity 输出数组容量。
 * @return 实际复制的 room 数量。
 */
size_t simple_d2d_indoor_registry_snapshot(
    simple_d2d_indoor_registry_t *registry,
    simple_d2d_indoor_registry_room_t *rooms,
    size_t capacity);

#ifdef __cplusplus
}
#endif

#endif
