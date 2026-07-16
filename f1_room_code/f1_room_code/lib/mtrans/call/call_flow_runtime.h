/**
 * @file call_flow_runtime.h
 * @brief `call_flow` 线程化运行时门面（收包、自动 ACK/ERROR、重试/超时、心跳与回调）。
 */
#ifndef __CALL_FLOW_RUNTIME_H__
#define __CALL_FLOW_RUNTIME_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include "mtrans/call/call_proto.h"
#include "transport/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 呼叫控制链路类型。
 */
typedef enum
{
    /** @brief 未知或无效链路。 */
    CALL_LINK_NONE = 0,
    /** @brief UDP 链路。 */
    CALL_LINK_UDP = 1,
    /** @brief PLC 链路。 */
    CALL_LINK_PLC = 2,
} call_link_kind_t;

/**
 * @brief 呼叫控制端点。
 */
typedef struct
{
    /** @brief 链路类型。 */
    call_link_kind_t link_kind;
    /** @brief 端点地址（主机字节序）。 */
    uint32_t addr;
    /** @brief 端口号。 */
    uint16_t port;
} call_endpoint_t;

/**
 * @brief `call_flow` 节点角色。
 */
typedef enum
{
    /** @brief 混合角色（默认）。 */
    CALL_RUNTIME_ROLE_HYBRID = 0,
    /** @brief 仅本地参与角色。 */
    CALL_RUNTIME_ROLE_PARTICIPANT,
    /** @brief 仅中继优先角色。 */
    CALL_RUNTIME_ROLE_RELAY,
} call_runtime_role_t;

/**
 * @brief 本地处理与中继同时可用时的冲突策略。
 */
typedef enum
{
    /** @brief 本地优先（默认，兼容历史行为）。 */
    CALL_RUNTIME_CONFLICT_LOCAL_FIRST = 0,
    /** @brief 中继优先。 */
    CALL_RUNTIME_CONFLICT_RELAY_FIRST,
} call_runtime_conflict_policy_t;

/**
 * @brief 下一跳查询回调。
 *
 * @param user_ctx 用户上下文指针。
 * @param callee_device_id 被叫 `device_id`。
 * @param next_hop 输出下一跳端点，不可为 `NULL`。
 * @return 找到路由返回 `true`；否则返回 `false`。
 */
typedef bool (*call_next_hop_lookup_cb)(void *user_ctx, const char *callee_device_id,
                                        call_endpoint_t *next_hop);

/**
 * @brief 本地 ID 匹配回调。
 *
 * @param user_ctx 用户上下文。
 * @param callee_device_id 待匹配被叫 `device_id`。
 * @return 匹配成功返回 `true`，否则返回 `false`。
 */
typedef bool (*call_local_id_match_cb)(void *user_ctx, const char *callee_device_id);

/**
 * @brief 出站发送参数。
 */
typedef struct
{
    /** @brief 是否显式提供 `callee_device_id`。 */
    bool has_callee_device_id;
    /** @brief 被叫设备 `device_id`。 */
    char callee_device_id[CALL_DEVICE_ID_MAX];
    /** @brief 是否要求 ACK。 */
    bool ack_required;
    /** @brief 事务 ID（0 表示自动分配）。 */
    uint32_t txn_id;
    /** @brief 会话 ID（0 表示自动分配）。 */
    uint64_t session_id;
    /** @brief 报文序号。 */
    uint16_t seq;
    /** @brief 可选负载指针，可为 `NULL`。 */
    const call_payload_t *payload;
    /** @brief 可选媒体参数，可为 `NULL`。 */
    const call_media_profile_t *media_profile;
    /** @brief 是否显式提供入站端点。 */
    bool has_ingress_ep;
    /** @brief 入站端点（用于回程映射）。 */
    call_endpoint_t ingress_ep;
    /** @brief 是否显式覆盖目标端点。 */
    bool has_target_override;
    /** @brief 目标端点覆盖值。 */
    call_endpoint_t target_override;
} call_send_params_t;

/**
 * @brief 媒体状态更新事件数据。
 */
typedef struct
{
    /** @brief 视频是否应处于激活态。 */
    bool video_active;
    /** @brief 音频是否应处于激活态。 */
    bool audio_active;
    /** @brief 触发该更新的消息类型。 */
    uint8_t trigger_msg_type;
    /** @brief 关联事务 ID。 */
    uint32_t txn_id;
    /** @brief 关联会话 ID。 */
    uint64_t session_id;
} call_runtime_media_update_event_t;

/**
 * @brief FSM 状态事件数据。
 */
typedef struct
{
    /** @brief 当前呼叫主状态。 */
    uint8_t call_state;
    /** @brief 当前监看会话数。 */
    uint8_t monitor_sessions;
    /** @brief FSM 动作位。 */
    uint32_t action_flags;
    /** @brief 结果码。 */
    call_result_code_t result_code;
} call_runtime_fsm_state_event_t;

/**
 * @brief call runtime 日志级别。
 */
typedef enum call_log_level
{
    /** @brief 调试级别日志。 */
    CALL_DEBUG = 1,
    /** @brief 信息级别日志。 */
    CALL_INFO = 2,
    /** @brief 警告级别日志。 */
    CALL_WARNING = 3,
    /** @brief 错误级别日志。 */
    CALL_ERROR = 4,
} call_log_level_t;

/**
 * @brief call runtime 日志处理回调。
 */
typedef void (*call_log_func_t)(call_log_level_t level,
                                const char *file,
                                const long line,
                                const char *func,
                                const char *format,
                                va_list ap);

/**
 * @brief call runtime 内存分配钩子集合。
 */
struct call_mem_hooks
{
    void *(*malloc)(size_t size, const char *file, int line);
    void *(*realloc)(void *ptr, size_t size, const char *file, int line);
    void (*free)(void *ptr, const char *file, int line);
};

/**
 * @brief 运行时错误阶段。
 */
typedef enum
{
    /** @brief 发送报文阶段。 */
    CALL_FLOW_RT_ERR_STAGE_SEND = 0,
    /** @brief 入站处理阶段。 */
    CALL_FLOW_RT_ERR_STAGE_HANDLE_PACKET,
    /** @brief 轮询阶段。 */
    CALL_FLOW_RT_ERR_STAGE_TICK,
    /** @brief 内部阶段。 */
    CALL_FLOW_RT_ERR_STAGE_INTERNAL,
} call_flow_runtime_error_stage_t;

/**
 * @brief 回调事件类型。
 */
typedef enum
{
    /** @brief 入站业务消息事件。 */
    CALL_FLOW_RT_EVENT_INBOUND_MESSAGE = 0,
    /** @brief 出站请求响应事件（ACK/ERROR）。 */
    CALL_FLOW_RT_EVENT_RESPONSE_MESSAGE,
    /** @brief 媒体状态更新事件。 */
    CALL_FLOW_RT_EVENT_MEDIA_UPDATE,
    /** @brief FSM 状态事件。 */
    CALL_FLOW_RT_EVENT_FSM_STATE,
    /** @brief 会话错误事件（超时/心跳失联）。 */
    CALL_FLOW_RT_EVENT_SESSION_ERROR,
    /** @brief 运行时错误事件。 */
    CALL_FLOW_RT_EVENT_RUNTIME_ERROR,
} call_flow_runtime_event_type_t;

/**
 * @brief 入站消息分发决策。
 */
typedef enum
{
    /** @brief 丢弃消息。 */
    CALL_RUNTIME_DISPATCH_DROP = 0,
    /** @brief 转发消息。 */
    CALL_RUNTIME_DISPATCH_FORWARD,
    /** @brief 发送 ACK。 */
    CALL_RUNTIME_DISPATCH_SEND_ACK,
    /** @brief 发送 ERROR。 */
    CALL_RUNTIME_DISPATCH_SEND_ERROR,
    /** @brief 事务重试事件。 */
    CALL_RUNTIME_DISPATCH_TXN_RETRY,
    /** @brief 事务超时事件。 */
    CALL_RUNTIME_DISPATCH_TXN_TIMEOUT,
} call_runtime_dispatch_decision_t;

/**
 * @brief 运行时链路配置。
 */
typedef struct
{
    /** @brief 链路类型。 */
    call_link_kind_t link_kind;
    /** @brief 已打开的 transport 句柄。 */
    transport_handle_t *transport;
} call_flow_runtime_link_t;

/**
 * @brief 入站业务消息事件数据。
 */
typedef struct
{
    /** @brief 消息来源端点。 */
    call_endpoint_t src_ep;
    /** @brief 已解析头部。 */
    call_msg_header_t header;
    /** @brief 已解析负载。 */
    call_payload_t payload;
    /** @brief 分发决策。 */
    call_runtime_dispatch_decision_t decision;
    /** @brief 结果码。 */
    call_result_code_t result_code;
    /** @brief 未知字段数量。 */
    size_t unknown_key_count;
    /** @brief 原请求是否要求 ACK。 */
    bool require_ack;
} call_flow_runtime_inbound_event_t;

/**
 * @brief 出站请求响应事件数据。
 */
typedef struct
{
    /** @brief 响应来源端点。 */
    call_endpoint_t src_ep;
    /** @brief 响应头部。 */
    call_msg_header_t header;
    /** @brief 响应负载。 */
    call_payload_t payload;
    /** @brief 结果码。 */
    call_result_code_t result_code;
} call_flow_runtime_response_event_t;

/**
 * @brief 会话错误事件数据。
 */
typedef struct
{
    /** @brief 会话 ID。 */
    uint64_t session_id;
    /** @brief 事务 ID（无事务时为 0）。 */
    uint32_t txn_id;
    /** @brief 关联消息类型。 */
    uint8_t msg_type;
    /** @brief 重试次数（无重试语义时为 0）。 */
    uint8_t retry_count;
    /** @brief 错误结果码。 */
    call_result_code_t result_code;
} call_flow_runtime_session_error_event_t;

/**
 * @brief 运行时错误事件数据。
 */
typedef struct
{
    /** @brief 错误阶段。 */
    call_flow_runtime_error_stage_t stage;
    /** @brief 错误码（负值）。 */
    int error_code;
    /** @brief 关联端点（无关时为零值）。 */
    call_endpoint_t ep;
    /** @brief 关联消息类型（无关时为 0）。 */
    uint8_t msg_type;
    /** @brief 关联事务 ID（无关时为 0）。 */
    uint32_t txn_id;
} call_flow_runtime_error_event_t;

/**
 * @brief 运行时回调事件。
 */
typedef struct
{
    /** @brief 事件类型。 */
    call_flow_runtime_event_type_t type;
    /** @brief 事件载荷。 */
    union
    {
        /** @brief 入站业务消息事件。 */
        call_flow_runtime_inbound_event_t inbound;
        /** @brief 出站请求响应事件。 */
        call_flow_runtime_response_event_t response;
        /** @brief 媒体状态更新事件。 */
        call_runtime_media_update_event_t media_update;
        /** @brief FSM 状态事件。 */
        call_runtime_fsm_state_event_t fsm_state;
        /** @brief 会话错误事件。 */
        call_flow_runtime_session_error_event_t session_error;
        /** @brief 运行时错误事件。 */
        call_flow_runtime_error_event_t runtime_error;
    } u;
} call_flow_runtime_event_t;

/**
 * @brief 事件回调函数。
 *
 * @param user_ctx 用户上下文。
 * @param event 回调事件，不可为 `NULL`。
 */
typedef void (*call_flow_runtime_event_cb)(void *user_ctx,
                                           const call_flow_runtime_event_t *event);

/**
 * @brief 获取本地媒体状态快照。
 *
 * 该回调主要用于在发送 `CALL_MSG_SESSION_STATE`（例如 timeout teardown）
 * 时附带发送端本地媒体阶段快照。
 *
 * @param user_ctx 用户上下文。
 * @param session_id 当前会话 ID。
 * @param media_state_out 输出媒体状态，不可为 `NULL`。
 * @return 成功返回 `true`；无需附带或获取失败返回 `false`。
 */
typedef bool (*call_flow_runtime_media_state_snapshot_cb)(void *user_ctx,
                                                          uint64_t session_id,
                                                          uint8_t *media_state_out);

/**
 * @brief 运行时初始化配置。
 */
typedef struct
{
    /** @brief 本端设备 `device_id`，可为 `NULL`。 */
    const char *self_device_id;
    /** @brief 监视会话上限（0 使用默认值）。 */
    uint8_t monitor_limit;
    /** @brief 严格模式开关。 */
    bool strict_mode;
    /** @brief 节点角色策略。 */
    call_runtime_role_t role;
    /** @brief 本地/中继冲突策略。 */
    call_runtime_conflict_policy_t conflict_policy;
    /** @brief 本地 ID 匹配回调，可为 `NULL`。 */
    call_local_id_match_cb local_id_match;
    /** @brief 本地 ID 匹配回调上下文；为 `NULL` 时使用 @p user_ctx。 */
    void *local_id_match_ctx;
    /** @brief 下一跳查询回调，可为 `NULL`。 */
    call_next_hop_lookup_cb next_hop_lookup;
    /** @brief 下一跳查询回调上下文；为 `NULL` 时使用 @p user_ctx。 */
    void *next_hop_lookup_ctx;
    /** @brief 启动随机因子/实例标识（0 使用默认值）。 */
    uint32_t boot_nonce;
    /** @brief call 子系统日志级别（0 使用默认值）。 */
    call_log_level_t log_level;
    /** @brief call 子系统日志回调，可为 `NULL`。 */
    call_log_func_t log_handler;
    /** @brief call 子系统内存钩子，可为 `NULL`。 */
    const struct call_mem_hooks *mem_hooks;
    /** @brief 链路数组。 */
    const call_flow_runtime_link_t *links;
    /** @brief 链路数量。 */
    size_t link_count;
    /** @brief 回调函数，可为 `NULL`。 */
    call_flow_runtime_event_cb on_event;
    /** @brief 本地媒体状态快照回调，可为 `NULL`。 */
    call_flow_runtime_media_state_snapshot_cb get_media_state_snapshot;
    /** @brief 回调用户上下文。 */
    void *user_ctx;
#ifdef CALL_FLOW_RUNTIME_TESTING
    /** @brief 测试专用 ACK 等待超时时间（0 使用生产默认值）。 */
    uint32_t test_ack_timeout_ms;
    /** @brief 测试专用最大重试次数（0 使用生产默认值）。 */
    uint8_t test_max_retry;
    /** @brief 测试专用去重窗口时长（0 使用生产默认值）。 */
    uint32_t test_dedup_window_ms;
    /** @brief 测试专用收包等待周期（0 使用生产默认值）。 */
    uint32_t test_io_wait_period_ms;
    /** @brief 测试专用轮询周期（0 使用生产默认值）。 */
    uint32_t test_tick_period_ms;
    /** @brief 测试专用心跳周期（0 使用生产默认值）。 */
    uint32_t test_hb_period_ms;
    /** @brief 测试专用心跳失联阈值（0 使用生产默认值）。 */
    uint8_t test_hb_miss_limit;
#endif
} call_flow_runtime_config_t;

/**
 * @brief 运行时上下文（不透明）。
 */
typedef struct call_flow_runtime_ctx call_flow_runtime_ctx_t;

/**
 * @brief 初始化运行时上下文。
 *
 * 若配置提供 `mem_hooks` / `log_handler`，初始化过程中会先注册它们，
 * 再创建运行时内部对象；用法与 `mtrans_engine_init()` 的配置传入方式保持一致。
 *
 * @param ctx_out 输出上下文，不可为 `NULL`。
 * @param cfg 初始化配置，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负值错误码。
 */
int call_flow_runtime_init(call_flow_runtime_ctx_t **ctx_out,
                           const call_flow_runtime_config_t *cfg);

/**
 * @brief 启动运行时线程。
 *
 * @param ctx 运行时上下文，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负值错误码。
 */
int call_flow_runtime_start(call_flow_runtime_ctx_t *ctx);

/**
 * @brief 停止运行时线程。
 *
 * @param ctx 运行时上下文，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负值错误码。
 */
int call_flow_runtime_stop(call_flow_runtime_ctx_t *ctx);

/**
 * @brief 释放运行时上下文。
 *
 * @param ctx 运行时上下文，可为 `NULL`。
 */
void call_flow_runtime_deinit(call_flow_runtime_ctx_t *ctx);

/**
 * @brief 异步发送自定义请求消息。
 *
 * @param ctx 运行时上下文，不可为 `NULL`。
 * @param msg_type 消息类型。
 * @param params 发送参数，不可为 `NULL`。
 * @param now_ms 调用时间（毫秒），传 `0` 时由实现取当前时间。
 * @return 成功返回 `0`；失败返回负值错误码。
 */
int call_flow_runtime_send_custom(call_flow_runtime_ctx_t *ctx, uint8_t msg_type,
                                  const call_send_params_t *params,
                                  uint64_t now_ms);

/**
 * @brief 发送 `CALL_INVITE` 请求。
 */
int call_flow_runtime_invite(call_flow_runtime_ctx_t *ctx,
                             const call_send_params_t *params,
                             uint64_t now_ms);
/**
 * @brief 发送 `CALL_RING` 请求（被叫业务进入待接听状态，触发 video-only 阶段）。
 */
int call_flow_runtime_ring(call_flow_runtime_ctx_t *ctx,
                           const call_send_params_t *params,
                           uint64_t now_ms);
/**
 * @brief 发送 `CALL_ACCEPT` 请求。
 */
int call_flow_runtime_accept(call_flow_runtime_ctx_t *ctx,
                             const call_send_params_t *params,
                             uint64_t now_ms);
/**
 * @brief 发送 `CALL_REJECT` 请求。
 */
int call_flow_runtime_reject(call_flow_runtime_ctx_t *ctx,
                             const call_send_params_t *params,
                             uint64_t now_ms);
/**
 * @brief 发送 `CALL_BYE` 请求。
 */
int call_flow_runtime_bye(call_flow_runtime_ctx_t *ctx,
                          const call_send_params_t *params,
                          uint64_t now_ms);
/**
 * @brief 发送 `MONITOR_START` 请求。
 */
int call_flow_runtime_monitor_start(call_flow_runtime_ctx_t *ctx,
                                    const call_send_params_t *params,
                                    uint64_t now_ms);
/**
 * @brief 发送 `MONITOR_ACCEPT` 请求。
 */
int call_flow_runtime_monitor_accept(call_flow_runtime_ctx_t *ctx,
                                     const call_send_params_t *params,
                                     uint64_t now_ms);
/**
 * @brief 发送 `MONITOR_REJECT` 请求。
 */
int call_flow_runtime_monitor_reject(call_flow_runtime_ctx_t *ctx,
                                     const call_send_params_t *params,
                                     uint64_t now_ms);
/**
 * @brief 发送 `MONITOR_STOP` 请求。
 */
int call_flow_runtime_monitor_stop(call_flow_runtime_ctx_t *ctx,
                                   const call_send_params_t *params,
                                   uint64_t now_ms);
/**
 * @brief 发送 `HEARTBEAT` 请求。
 */
int call_flow_runtime_heartbeat(call_flow_runtime_ctx_t *ctx,
                                const call_send_params_t *params,
                                uint64_t now_ms);
/**
 * @brief 发送 `SESSION_STATE` 请求。
 */
int call_flow_runtime_session_state(call_flow_runtime_ctx_t *ctx,
                                    const call_send_params_t *params,
                                    uint64_t now_ms);
/**
 * @brief 发送 `PREEMPT_NOTICE` 请求。
 */
int call_flow_runtime_preempt_notice(call_flow_runtime_ctx_t *ctx,
                                     const call_send_params_t *params,
                                     uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif
