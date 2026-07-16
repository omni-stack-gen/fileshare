/**
 * @file
 * @brief simple 示例应用公共模块 simple_call_app 的公共头文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_CALL_APP_H__
#define __SIMPLE_CALL_APP_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "call_flow_runtime.h"
#include "simple_control_route_runtime.h"
#include "simple_media.h"
#include "simple_media_profile_shape.h"
#include "simple_plc_ports.h"
#include "simple_session.h"
#include "simple_session_media_plan.h"
#include "transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief simple call control link 最大数量。 */
#define SIMPLE_CALL_MAX_LINKS  (2U)
/** @brief simple call UDP 控制默认端口。 */
#define SIMPLE_CALL_CTRL_UDP_PORT (10001U)
/** @brief simple call PLC 控制默认端口。 */
#define SIMPLE_CALL_CTRL_PLC_PORT SIMPLE_PLC_PORT_CALL_CONTROL

/**
 * @brief simple 示例设备角色。
 */
typedef enum
{
    /** 管理机。 */
    SIMPLE_DEVICE_MANAGE = 0,
    /** 1号单元机/门口机。 */
    SIMPLE_DEVICE_DOOR_MAIN,
    /** 转换盒。 */
    SIMPLE_DEVICE_CONVERTER,
    /** 室内机。 */
    SIMPLE_DEVICE_INDOOR,
    /** 二次确认机。 */
    SIMPLE_DEVICE_SECONDARY_CONFIRM,
} simple_device_role_t;

/**
 * @brief simple call 控制链路配置。
 */
typedef struct
{
    /** call route 链路类型。 */
    call_link_kind_t link_kind;
    /** transport 协议类型。 */
    transport_proto_t proto;
    /** 绑定接口名，可为 NULL。 */
    const char *interface;
    /** 本地监听 endpoint。 */
    transport_endpoint_t local_ep;
} simple_control_link_t;

/**
 * @brief link decision 建链前媒体 profile dry-run 策略。
 *
 * @param[in] user_ctx 用户上下文，调用方保持所有权。
 * @param[in] msg_type 即将发送的 call 消息类型。
 * @param[in,out] profile 可修改的媒体 profile。
 */
typedef void (*simple_link_profile_policy_t)(void *user_ctx,
                                                uint8_t msg_type,
                                                call_media_profile_t *profile);

/**
 * @brief simple call app 初始化配置。
 */
typedef struct
{
    /** 当前设备角色。 */
    simple_device_role_t role;
    /** 当前设备 device_id，不能为 NULL。 */
    const char *self_device_id;
    /** 初始路由表数组，可为 NULL。 */
    const simple_route_entry_t *routes;
    /** 初始路由表条目数。 */
    size_t route_count;
    /** true 表示找不到精确 callee route 时不回退到第一条 route。 */
    bool disable_route_fallback;
    /** 控制链路数组，不能为 NULL。 */
    const simple_control_link_t *links;
    /** 控制链路数量。 */
    size_t link_count;
    /** 媒体操作集合，可为 NULL。 */
    const simple_media_ops_t *media_ops;
    /** 媒体 endpoint provider，可为 NULL；为空时使用 simple static fallback。 */
    simple_media_endpoint_provider_t endpoint_provider;
    /** 传给 endpoint_provider 的上下文。 */
    void *endpoint_provider_ctx;
    /** endpoint/session 绑定释放回调，可为 NULL。 */
    void (*endpoint_release)(void *user_ctx, uint64_t session_id, const char *reason);
    /** 传给 endpoint_release 的上下文。 */
    void *endpoint_release_ctx;
#ifdef SIMPLE_CALL_APP_TESTING
    /** 测试专用心跳发送间隔，单位毫秒；生产构建不开放该配置。 */
    uint32_t test_hb_period_ms;
    /** 测试专用连续丢失心跳次数；生产构建不开放该配置。 */
    uint8_t test_hb_miss_limit;
#endif
    /** 启动随机数/nonce，用于日志或协议去重。 */
    uint32_t boot_nonce;
    /** link decision 建链前媒体 profile dry-run 策略，可为 NULL。 */
    simple_link_profile_policy_t link_profile_policy;
    /** 传给 link_profile_policy 的上下文。 */
    void *link_profile_policy_ctx;
    /** TX media_profile 形态策略，可为 NULL。 */
    simple_tx_profile_shape_policy_t tx_profile_shape_policy;
    /** 传给 tx_profile_shape_policy 的上下文。 */
    void *tx_profile_shape_policy_ctx;
    /** 入站事件旁路观察回调，可为 NULL。 */
    void (*inbound_observer)(void *user_ctx,
                             const call_flow_runtime_inbound_event_t *event);
    /** 外部呼叫占用判断回调，可为 NULL；返回 true 时新 CALL_INVITE 将被 busy 拒绝。 */
    bool (*external_call_busy)(void *user_ctx);
    /** 传给 inbound_observer 的上下文。 */
    void *user_ctx;
} simple_call_app_config_t;

/**
 * @brief simple call app 不透明句柄。
 */
typedef struct simple_call_app simple_call_app_t;

/**
 * @brief 创建 simple call app。
 *
 * @param[out] app_out app 句柄输出，不能为 NULL；成功后由 deinit 释放。
 * @param[in] cfg 初始化配置，不能为 NULL。
 * @retval 0 创建成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOMEM 内存不足。
 */
int simple_call_app_init(simple_call_app_t **app_out, const simple_call_app_config_t *cfg);
/** @brief 启动 simple call app runtime。 */
int simple_call_app_start(simple_call_app_t *app);
/** @brief 停止 simple call app runtime。 */
void simple_call_app_stop(simple_call_app_t *app);
/** @brief 释放 simple call app。 */
void simple_call_app_deinit(simple_call_app_t *app);
/**
 * @brief 写入或删除明确来源的 callee route fact。
 *
 * @param[in,out] app app 句柄，不能为 NULL。
 * @param[in] fact route fact，不能为 NULL。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 */
int simple_call_apply_route_fact(simple_call_app_t *app,
                                 const simple_control_route_fact_t *fact);
/**
 * @brief 写入明确来源的 online route fact。
 *
 * @param[in,out] app app 句柄，不能为 NULL。
 * @param[in] source route fact 来源。
 * @param[in] callee_device_id 被叫 device_id，不能为 NULL。
 * @param[in] next_hop 下一跳，不能为 NULL。
 * @param[in] observed_ms 观测时间，单位毫秒；0 表示未设置。
 * @param[in] ttl_ms 有效期，单位毫秒；0 表示不过期。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 */
int simple_call_apply_route_online(simple_call_app_t *app,
                                   simple_control_route_source_t source,
                                   const char *callee_device_id,
                                   const call_endpoint_t *next_hop,
                                   uint64_t observed_ms,
                                   uint32_t ttl_ms);
/**
 * @brief 删除同来源同 callee 的 route fact。
 *
 * @param[in,out] app app 句柄，不能为 NULL。
 * @param[in] source route fact 来源。
 * @param[in] callee_device_id 被叫 device_id，不能为 NULL。
 * @retval 0 成功或本就不存在。
 * @retval -EINVAL 参数非法。
 */
int simple_call_apply_route_offline(simple_call_app_t *app,
                                    simple_control_route_source_t source,
                                    const char *callee_device_id);
/**
 * @brief 过期 TTL route facts。
 *
 * @param[in,out] app app 句柄，不能为 NULL。
 * @param[in] now_ms 当前时间，单位毫秒。
 * @return 删除的 route fact 数量。
 */
size_t simple_call_expire_route_facts(simple_call_app_t *app, uint64_t now_ms);
/** @brief 发起普通呼叫。 */
int simple_call_start(simple_call_app_t *app, const char *callee_device_id);
/** @brief 接受当前普通呼叫。 */
int simple_call_accept(simple_call_app_t *app);
/** @brief 拒绝当前普通呼叫。 */
int simple_call_reject(simple_call_app_t *app);
/** @brief 挂断当前呼叫。 */
int simple_call_hangup(simple_call_app_t *app);

/** @brief 发起设备监视。 */
int simple_monitor_start(simple_call_app_t *app, const char *target_device_id);
/**
 * @brief 发起公区 IPC 监视。
 *
 * @param[in,out] app app 句柄，不能为 NULL。
 * @param[in] unit_device_id 处理 monitor 请求的单元机 device_id。
 * @param[in] ipc_ip IPC IPv4 字符串，可为 NULL。
 * @param[in] rtsp_url IPC RTSP URL，可为 NULL；ipc_ip/rtsp_url 至少一个有效。
 * @param[in] ipc_username IPC 用户名，可为 NULL。
 * @param[in] ipc_password IPC 密码，可为 NULL，日志不会输出明文。
 * @retval 0 发送成功。
 * @retval -EINVAL 参数非法。
 */
int simple_monitor_start_public_ipc(simple_call_app_t *app,
                                    const char *unit_device_id,
                                    const char *ipc_ip,
                                    const char *rtsp_url,
                                    const char *ipc_username,
                                    const char *ipc_password);
/** @brief 按 session 停止指定监视。 */
int simple_monitor_stop_session(simple_call_app_t *app, uint64_t session_id);
/** @brief 停止当前或最近一路监视。 */
int simple_monitor_stop(simple_call_app_t *app);

/** @brief 获取当前 simple session 状态。 */
simple_session_state_t simple_call_get_state(simple_call_app_t *app);
/** @brief 输出 simple call 诊断状态，包括 session manager、media plan 和 resource policy。 */
void simple_call_dump_diagnostics(simple_call_app_t *app);
/** @brief simple 媒体 bridge 执行结果诊断。 */
typedef struct
{
    /** bridge 执行返回值。 */
    int ret;
    /** ingress endpoint。 */
    call_media_endpoint_t ingress_endpoint;
    /** true 表示 ingress_endpoint 有效。 */
    bool has_ingress_endpoint;
    /** egress endpoint。 */
    call_media_endpoint_t egress_endpoint;
    /** true 表示 egress_endpoint 有效。 */
    bool has_egress_endpoint;
    /** RX match 地址。 */
    uint32_t rx_match_addr;
    /** RX match 端口。 */
    uint16_t rx_match_port;
    /** true 表示 RX match endpoint 有效。 */
    bool has_rx_match_endpoint;
    /** 诊断原因字符串，可为 NULL。 */
    const char *reason;
} simple_call_media_bridge_result_t;
/** @brief 记录 simple 媒体 bridge plan fact，不改变实际 bridge 行为。 */
int simple_call_note_media_bridge_plan(simple_call_app_t *app,
                                       uint64_t session_id,
                                       bool video,
                                       const simple_media_bridge_plan_t *bridge_plan);
/** @brief 读取 simple 媒体 bridge plan fact，供应用层实际 bridge 创建使用。 */
int simple_call_get_media_bridge_plan(simple_call_app_t *app,
                                      uint64_t session_id,
                                      bool video,
                                      simple_media_bridge_plan_t *bridge_plan);
/** @brief 记录 simple 媒体 bridge 执行结果诊断，不改变实际 bridge 行为。 */
void simple_call_note_media_bridge_result(simple_call_app_t *app,
                                          uint64_t session_id,
                                          bool video,
                                          const simple_call_media_bridge_result_t *result);

#ifdef __cplusplus
}
#endif

#endif
