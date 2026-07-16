#ifndef D2D_PROTOCOL_FORWARDER_H
#define D2D_PROTOCOL_FORWARDER_H

/**
 * @file d2d_forwarder.h
 * @brief d2d 转发编排模块对外接口。
 */

#include "d2d_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct d2d_forwarder d2d_forwarder_t;
typedef struct d2d_forwarder_notify d2d_forwarder_notify_t;

/**
 * @brief 转发编排路由决策枚举。
 */
typedef enum
{
    D2D_FORWARDER_ROUTE_LOCAL = 1,         /**< 按本地处理路径执行。 */
    D2D_FORWARDER_ROUTE_FORWARD = 2,       /**< 按转发路径执行。 */
    D2D_FORWARDER_ROUTE_REJECT_LOCAL = 3,  /**< 按本地失败拒绝，回复 `messageType=2`。 */
    D2D_FORWARDER_ROUTE_REJECT_FORWARD = 4 /**< 按转发失败拒绝，回复 `messageType=3`。 */
} d2d_forwarder_route_decision_t;

/**
 * @brief 转发编排对外通知类型枚举。
 */
typedef enum
{
    D2D_FORWARDER_NOTIFY_LOCAL_REQUEST = 1,  /**< 收到一条需本地处理的入站请求。 */
    D2D_FORWARDER_NOTIFY_OUTBOUND_DONE = 2,  /**< 一条主动出站请求收到成功应答。 */
    D2D_FORWARDER_NOTIFY_OUTBOUND_FAIL = 3,  /**< 一条主动出站请求收到失败应答。 */
    D2D_FORWARDER_NOTIFY_OUTBOUND_TIMEOUT = 4,/**< 一条主动出站请求发生超时。 */
    D2D_FORWARDER_NOTIFY_LINK_ERROR = 5      /**< 某条链路在收发或线程驱动中发生错误。 */
} d2d_forwarder_notify_type_t;

/**
 * @brief 路由决策结果结构。
 */
typedef struct
{
    d2d_forwarder_route_decision_t decision; /**< 路由决策结果。 */
    d2d_adapter_request_t tx;                /**< 当决策为 `FORWARD` 时使用的下一跳发送目标。 */
} d2d_forwarder_route_result_t;

/**
 * @brief 本地处理通知数据结构。
 */
typedef struct
{
    d2d_request_handle_t handle;       /**< 入站请求句柄，用于后续本地回提。 */
    d2d_link_kind_t ingress_link;      /**< 入站链路类型。 */
    transport_endpoint_t src_endpoint; /**< 入站来源端点。 */
    const d2d_message_t *message;      /**< 入站请求消息，仅在当前回调期间有效。 */
} d2d_forwarder_local_request_notify_t;

/**
 * @brief 主动出站成功通知数据结构。
 */
typedef struct
{
    char msg_id[D2D_MSG_ID_MAX_LEN + 1]; /**< 原请求消息 ID。 */
    d2d_cmd_t request_cmd;               /**< 原请求命令字。 */
    d2d_link_kind_t target_link;         /**< 发送该请求时指定的目标链路。 */
    transport_endpoint_t target_endpoint;/**< 发送该请求时指定的目标端点。 */
    uintptr_t opaque;                    /**< 发送该请求时透传的上下文标识。 */
    const d2d_message_t *request;        /**< 原请求消息，仅在当前回调期间有效。 */
    const d2d_message_t *reply;          /**< 成功应答消息，仅在当前回调期间有效。 */
} d2d_forwarder_outbound_done_notify_t;

/**
 * @brief 主动出站失败通知数据结构。
 */
typedef struct
{
    char msg_id[D2D_MSG_ID_MAX_LEN + 1]; /**< 原请求消息 ID。 */
    d2d_cmd_t request_cmd;               /**< 原请求命令字。 */
    d2d_message_type_t reply_type;       /**< 失败应答类型。 */
    d2d_link_kind_t target_link;         /**< 发送该请求时指定的目标链路。 */
    transport_endpoint_t target_endpoint;/**< 发送该请求时指定的目标端点。 */
    uintptr_t opaque;                    /**< 发送该请求时透传的上下文标识。 */
    const d2d_message_t *request;        /**< 原请求消息，仅在当前回调期间有效。 */
    const d2d_message_t *reply;          /**< 失败应答消息，仅在当前回调期间有效。 */
} d2d_forwarder_outbound_fail_notify_t;

/**
 * @brief 主动出站超时通知数据结构。
 */
typedef struct
{
    char msg_id[D2D_MSG_ID_MAX_LEN + 1]; /**< 原请求消息 ID。 */
    d2d_cmd_t request_cmd;               /**< 原请求命令字。 */
    d2d_link_kind_t target_link;         /**< 发送该请求时指定的目标链路。 */
    transport_endpoint_t target_endpoint;/**< 发送该请求时指定的目标端点。 */
    uintptr_t opaque;                    /**< 发送该请求时透传的上下文标识。 */
    const d2d_message_t *request;        /**< 原请求消息，仅在当前回调期间有效。 */
} d2d_forwarder_outbound_timeout_notify_t;

/**
 * @brief 链路错误通知数据结构。
 */
typedef struct
{
    d2d_link_kind_t link_kind;           /**< 出错链路类型。 */
    transport_endpoint_t local_endpoint; /**< 本地链路端点。 */
    transport_endpoint_t remote_endpoint;/**< 关联的远端端点，未知时为全 0。 */
    uintptr_t opaque;                    /**< 关联请求透传的上下文标识，未知时为 0。 */
    int error_code;                      /**< 错误码。 */
    const char *operation;               /**< 出错操作名称，仅在当前回调期间有效。 */
    void *link_user_data;                /**< 对应链路的用户私有数据。 */
} d2d_forwarder_link_error_notify_t;

/**
 * @brief 转发编排通知负载联合体。
 */
typedef union
{
    d2d_forwarder_local_request_notify_t local_request;     /**< 本地处理通知负载。 */
    d2d_forwarder_outbound_done_notify_t outbound_done;     /**< 主动出站成功通知负载。 */
    d2d_forwarder_outbound_fail_notify_t outbound_fail;     /**< 主动出站失败通知负载。 */
    d2d_forwarder_outbound_timeout_notify_t outbound_timeout;/**< 主动出站超时通知负载。 */
    d2d_forwarder_link_error_notify_t link_error;           /**< 链路错误通知负载。 */
} d2d_forwarder_notify_data_t;

/**
 * @brief 转发编排通知对象结构。
 */
struct d2d_forwarder_notify
{
    d2d_forwarder_notify_type_t type; /**< 通知类型。 */
    uint64_t now_ms;                  /**< 通知触发时间戳，单位为毫秒。 */
    d2d_forwarder_notify_data_t data; /**< 通知负载。 */
};

/**
 * @brief 路由决策回调函数类型。
 *
 * 回调应根据入站消息、入口链路和来源端点给出路由决策。当回调返回非 0 或
 * 未填入有效决策时，forwarder 会回退为 `REJECT_FORWARD`。
 *
 * @param[in] user_data 创建 forwarder 时传入的用户上下文。
 * @param[in] handle 入站请求句柄。
 * @param[in] message 入站请求消息，仅在当前回调期间有效。
 * @param[in] ingress_link 入站链路类型。
 * @param[in] src_endpoint 入站来源端点。
 * @param[out] result 路由决策结果输出对象。
 * @return 成功返回 `0`，失败返回负值错误码或其他非 0 值。
 */
typedef int (*d2d_forwarder_route_fn_t)(void *user_data,
                                        d2d_request_handle_t handle,
                                        const d2d_message_t *message,
                                        d2d_link_kind_t ingress_link,
                                        const transport_endpoint_t *src_endpoint,
                                        d2d_forwarder_route_result_t *result);

/**
 * @brief 转发编排通知回调函数类型。
 *
 * 通知回调由内部 adapter 线程同步调用。通知中的消息对象仅在当前回调期间
 * 有效。若需跨回调保存，调用方应自行复制。
 *
 * @param[in] user_data 创建 forwarder 时传入的用户上下文。
 * @param[in] notify 当前通知对象。
 */
typedef void (*d2d_forwarder_notify_fn_t)(void *user_data, const d2d_forwarder_notify_t *notify);

/**
 * @brief 转发编排配置结构。
 */
typedef struct
{
    int local_dev_type;                      /**< 本端设备类型。 */
    uint32_t heartbeat_timeout_ms;           /**< 心跳类请求超时时间。 */
    uint32_t sync_timeout_ms;                /**< 同步类请求超时时间。 */
    uint32_t request_timeout_ms;             /**< 普通业务请求超时时间。 */
    uint32_t inbound_timeout_ms;             /**< 入站异步处理超时时间。 */
    uint32_t poll_wait_ms;                   /**< 内部线程 `select` 等待时间。 */
    const d2d_adapter_link_t *links;         /**< 链路配置数组。 */
    size_t link_count;                       /**< 链路数量。 */
    d2d_forwarder_route_fn_t route_cb;       /**< 路由决策回调，不能为空。 */
    d2d_forwarder_notify_fn_t notify_cb;     /**< 对外通知回调，可为 `NULL`。 */
    void *user_data;                         /**< 透传给回调的用户上下文。 */
} d2d_forwarder_config_t;

/**
 * @brief 创建转发编排实例。
 *
 * 内部会创建并持有一个 `d2d_adapter`，但不会立即启动线程。
 *
 * @param[in] config forwarder 配置。
 * @param[out] out 输出 forwarder 实例。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_forwarder_create(const d2d_forwarder_config_t *config, d2d_forwarder_t **out);

/**
 * @brief 启动转发编排内部 adapter 线程。
 *
 * @param[in,out] forwarder forwarder 实例。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_forwarder_start(d2d_forwarder_t *forwarder);

/**
 * @brief 停止转发编排内部 adapter 线程。
 *
 * @param[in,out] forwarder forwarder 实例。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_forwarder_stop(d2d_forwarder_t *forwarder);

/**
 * @brief 销毁转发编排实例并释放内部资源。
 *
 * 销毁前若线程仍在运行，会先执行一次 stop。
 *
 * @param[in,out] forwarder forwarder 实例。
 */
void d2d_forwarder_destroy(d2d_forwarder_t *forwarder);

/**
 * @brief 线程安全地提交一条主动出站业务请求。
 *
 * 该接口仅用于应用主动发起的出站请求，不用于内部转发路径。
 *
 * @param[in,out] forwarder forwarder 实例。
 * @param[in] request 业务请求消息。
 * @param[in] tx 出站目标配置。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_forwarder_send_request(d2d_forwarder_t *forwarder,
                               const d2d_message_t *request,
                               const d2d_adapter_request_t *tx);

/**
 * @brief 回提本地处理成功结果。
 *
 * 仅适用于此前通过 `LOCAL_REQUEST` 通知交给应用本地处理的入站请求。
 *
 * @param[in,out] forwarder forwarder 实例。
 * @param[in] handle 入站请求句柄。
 * @param[in] reply 成功应答消息。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_forwarder_complete_local(d2d_forwarder_t *forwarder,
                                 d2d_request_handle_t handle,
                                 const d2d_message_t *reply);

/**
 * @brief 回提本地处理失败结果。
 *
 * 仅适用于此前通过 `LOCAL_REQUEST` 通知交给应用本地处理的入站请求。
 *
 * @param[in,out] forwarder forwarder 实例。
 * @param[in] handle 入站请求句柄。
 * @param[in] reason 失败原因。
 * @return 成功返回 `0`，失败返回负值错误码。
 */
int d2d_forwarder_fail_local(d2d_forwarder_t *forwarder,
                             d2d_request_handle_t handle,
                             d2d_request_fail_reason_t reason);

#ifdef __cplusplus
}
#endif

#endif
