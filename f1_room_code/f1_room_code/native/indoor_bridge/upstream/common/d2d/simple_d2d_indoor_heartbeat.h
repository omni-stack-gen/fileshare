/**
 * @file
 * @brief simple 室内机 20010 心跳 presence 来源公共头文件。
 *
 * 本文件属于 examples 应用层，用于室内机通过 PLC D2D 向 converter/信号扩展器
 * 上报 20010 心跳，并让 converter 把该心跳转换为 presence 在线事实。
 */

#ifndef SIMPLE_D2D_INDOOR_HEARTBEAT_H
#define SIMPLE_D2D_INDOOR_HEARTBEAT_H

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

/** @brief 室内机 20010 心跳默认 PLC 端口。 */
#define SIMPLE_D2D_INDOOR_HEARTBEAT_DEFAULT_PLC_PORT SIMPLE_D2D_PLC_CONTROL_PORT
/** @brief 默认室内机心跳周期，单位毫秒。 */
#define SIMPLE_D2D_INDOOR_HEARTBEAT_DEFAULT_INTERVAL_MS (180000U)
/** @brief 默认室内机心跳在线 TTL，单位毫秒。 */
#define SIMPLE_D2D_INDOOR_HEARTBEAT_DEFAULT_TTL_MS (450000U)
/** @brief devModel/sysVer/appVer/syncVer 缓冲区长度。 */
#define SIMPLE_D2D_INDOOR_HEARTBEAT_TEXT_MAX (64U)
/** @brief room_id 缓冲区长度。 */
#define SIMPLE_D2D_INDOOR_HEARTBEAT_ROOM_ID_MAX (32U)

typedef enum
{
    /** 室内机侧，周期发送 20010。 */
    SIMPLE_D2D_INDOOR_HEARTBEAT_ROLE_CLIENT = 1,
    /** converter/信号扩展器侧，接收 20010 并维护在线事实。 */
    SIMPLE_D2D_INDOOR_HEARTBEAT_ROLE_SERVER = 2
} simple_d2d_indoor_heartbeat_role_t;

/**
 * @brief 室内机 20010 heartbeat 在线状态。
 */
typedef struct
{
    /** 房号字符串，来自 20010 `id`。 */
    char room_id[SIMPLE_D2D_INDOOR_HEARTBEAT_ROOM_ID_MAX];
    /** 室内机 topology device_id，来自 D2D header orig_addr。 */
    char device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** 设备型号字符串，来自 20010 `devModel`。 */
    char dev_model[SIMPLE_D2D_INDOOR_HEARTBEAT_TEXT_MAX];
    /** 系统版本字符串，来自 20010 `sysVer`。 */
    char sys_ver[SIMPLE_D2D_INDOOR_HEARTBEAT_TEXT_MAX];
    /** 应用版本字符串，来自 20010 `appVer`。 */
    char app_ver[SIMPLE_D2D_INDOOR_HEARTBEAT_TEXT_MAX];
    /** 同步版本字符串，来自 20010 `syncVer`。 */
    char sync_ver[SIMPLE_D2D_INDOOR_HEARTBEAT_TEXT_MAX];
    /** 最近一次 heartbeat 来源 endpoint。 */
    transport_endpoint_t endpoint;
    /** 最近一次 heartbeat 来源 PLC 地址。 */
    uint32_t plc_addr;
    /** 最近一次观测时间，单位毫秒。 */
    uint64_t observed_ms;
    /** 在线 TTL，单位毫秒。 */
    uint32_t ttl_ms;
    /** 20010 `devType`，室内机使用 1。 */
    int dev_type;
    /** true 表示室内机 app heartbeat 在线。 */
    bool online;
} simple_d2d_indoor_heartbeat_status_t;

/**
 * @brief 室内机 heartbeat 状态更新回调。
 *
 * @param user_ctx 用户上下文。
 * @param status 当前状态快照；仅在回调期间有效。
 */
typedef void (*simple_d2d_indoor_heartbeat_update_fn)(
    void *user_ctx,
    const simple_d2d_indoor_heartbeat_status_t *status);

/**
 * @brief 室内机 heartbeat 回调配置。
 */
typedef struct
{
    /** D2D bus observer 配置，用于协议收发诊断。 */
    simple_d2d_observer_config_t observer;
    /** server 侧 online/offline 状态更新回调。 */
    simple_d2d_indoor_heartbeat_update_fn update_cb;
    /** 回调用户上下文。 */
    void *user_ctx;
} simple_d2d_indoor_heartbeat_callbacks_t;

/**
 * @brief 室内机 heartbeat runtime 配置。
 */
typedef struct
{
    /** runtime 角色：client 发送 20010，server 接收 20010。 */
    simple_d2d_indoor_heartbeat_role_t role;
    /** 本机/对端 device_id 与 devType 配置。 */
    simple_d2d_identity_config_t identity;
    /** PLC D2D endpoint 配置。 */
    simple_d2d_endpoint_config_t plc;
    /** 室内机房号，作为 20010 `id`。 */
    const char *room_id;
    /** 20010 文本字段配置。 */
    simple_d2d_device_text_config_t text;
    /** 心跳周期、TTL 和请求超时配置。 */
    simple_d2d_timing_config_t timing;
    /** 可选共享 D2D service；非 NULL 时不创建独立 bus/service。 */
    simple_d2d_service_t *shared_service;
    /** observer 与状态更新回调。 */
    simple_d2d_indoor_heartbeat_callbacks_t callbacks;
} simple_d2d_indoor_heartbeat_config_t;

/**
 * @brief 室内机 heartbeat runtime。
 *
 * 字段由 runtime 内部维护，调用方应通过 init/start/stop/deinit 管理生命周期。
 */
typedef struct
{
    /** 启动时保存的配置。 */
    simple_d2d_indoor_heartbeat_config_t cfg;
    /** owned mode 下使用的 D2D bus。 */
    simple_d2d_bus_t bus;
    /** server 侧单房间 heartbeat 状态。 */
    simple_d2d_indoor_heartbeat_status_t rooms[1];
    /** 保护状态和停止标记的互斥锁。 */
    pthread_mutex_t lock;
    /** client 周期发送或 server 过期检查线程。 */
    pthread_t thread;
    /** true 表示 lock 已初始化。 */
    bool lock_ready;
    /** true 表示线程已启动。 */
    bool thread_started;
    /** true 表示 runtime 已启动。 */
    bool started;
    /** true 表示请求线程退出。 */
    bool stop_requested;
} simple_d2d_indoor_heartbeat_t;

/**
 * @brief 初始化室内机 heartbeat 配置默认值。
 *
 * @param cfg 配置对象。
 */
void simple_d2d_indoor_heartbeat_config_init(
    simple_d2d_indoor_heartbeat_config_t *cfg);

/**
 * @brief 初始化室内机 heartbeat runtime。
 *
 * @param heartbeat runtime 对象。
 * @param cfg runtime 配置。
 * @return 0 表示成功，负数表示失败。
 */
int simple_d2d_indoor_heartbeat_init(
    simple_d2d_indoor_heartbeat_t *heartbeat,
    const simple_d2d_indoor_heartbeat_config_t *cfg);

/**
 * @brief 释放室内机 heartbeat runtime。
 *
 * @param heartbeat runtime 对象。
 */
void simple_d2d_indoor_heartbeat_deinit(
    simple_d2d_indoor_heartbeat_t *heartbeat);

/**
 * @brief 初始化并启动室内机 heartbeat runtime。
 *
 * @param heartbeat runtime 对象。
 * @param cfg runtime 配置。
 * @return 0 表示成功，负数表示失败。
 */
int simple_d2d_indoor_heartbeat_start(
    simple_d2d_indoor_heartbeat_t *heartbeat,
    const simple_d2d_indoor_heartbeat_config_t *cfg);

/**
 * @brief 启动 server 侧 heartbeat 过期检查线程。
 *
 * @param heartbeat server runtime。
 * @return 0 表示成功，负数表示失败。
 */
int simple_d2d_indoor_heartbeat_start_expire_thread(
    simple_d2d_indoor_heartbeat_t *heartbeat);

/**
 * @brief 停止室内机 heartbeat runtime。
 *
 * @param heartbeat runtime 对象。
 */
void simple_d2d_indoor_heartbeat_stop(
    simple_d2d_indoor_heartbeat_t *heartbeat);

/**
 * @brief 构造 20010 heartbeat request。
 *
 * @param cfg client 配置。
 * @param message 输出 D2D 消息。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法或必填字段缺失。
 */
int simple_d2d_indoor_heartbeat_build_request(
    const simple_d2d_indoor_heartbeat_config_t *cfg,
    d2d_message_t *message);

/**
 * @brief 处理 20010 heartbeat request 并生成 20011 reply。
 *
 * @param heartbeat server runtime。
 * @param request 输入 20010 request。
 * @param ingress_link 收包链路类型。
 * @param src 来源 endpoint。
 * @param reply 输出 20011 reply。
 * @return 0 表示处理完成，负数表示参数或协议错误。
 */
int simple_d2d_indoor_heartbeat_handle_request(
    simple_d2d_indoor_heartbeat_t *heartbeat,
    const d2d_message_t *request,
    d2d_link_kind_t ingress_link,
    const transport_endpoint_t *src,
    d2d_message_t *reply);

/**
 * @brief 过期室内机 heartbeat 状态。
 *
 * @param heartbeat server runtime。
 * @param now_ms 当前时间，单位毫秒。
 * @return 0 表示完成，负数表示失败。
 */
int simple_d2d_indoor_heartbeat_expire(
    simple_d2d_indoor_heartbeat_t *heartbeat,
    uint64_t now_ms);

/**
 * @brief 拷贝当前室内机 heartbeat 状态快照。
 *
 * @param heartbeat server runtime。
 * @param out 输出数组。
 * @param capacity 输出数组容量。
 * @return 实际拷贝的状态数量。
 */
size_t simple_d2d_indoor_heartbeat_snapshot(
    simple_d2d_indoor_heartbeat_t *heartbeat,
    simple_d2d_indoor_heartbeat_status_t *out,
    size_t capacity);

/**
 * @brief 初始化 20010/20011 D2D bus app。
 *
 * @param app 输出 bus app。
 * @param heartbeat heartbeat runtime。
 * @param bridge_local true 表示用于 converter bridge extra app，本地回复由 bridge 发送。
 */
void simple_d2d_indoor_heartbeat_bus_app_init(
    simple_d2d_bus_app_t *app,
    simple_d2d_indoor_heartbeat_t *heartbeat,
    bool bridge_local);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_D2D_INDOOR_HEARTBEAT_H */
