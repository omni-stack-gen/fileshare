/**
 * @file
 * @brief simple D2D 0x03 bus 公共头文件。
 *
 * 本文件属于 examples 应用层，用于统一 PLC D2D 控制端口上的业务注册、
 * 分发和结构化观测。
 */

#ifndef SIMPLE_D2D_BUS_H
#define SIMPLE_D2D_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "simple_d2d_service.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 单个 bus 可注册的最大业务 app 数量。 */
#define SIMPLE_D2D_BUS_MAX_APPS (8U)

/**
 * @brief D2D bus typed observer 事件类型。
 */
typedef enum
{
    /** 收到底层 request。 */
    SIMPLE_D2D_BUS_EVENT_RX = 1,
    /** request 已匹配到本地 app 并进入分发。 */
    SIMPLE_D2D_BUS_EVENT_DISPATCH = 2,
    /** 本地 app 处理完成。 */
    SIMPLE_D2D_BUS_EVENT_LOCAL_DONE = 3,
    /** forwarder 转发完成。 */
    SIMPLE_D2D_BUS_EVENT_FORWARD_DONE = 4,
    /** 通用失败事件。 */
    SIMPLE_D2D_BUS_EVENT_FAIL = 5,
    /** 请求等待超时。 */
    SIMPLE_D2D_BUS_EVENT_TIMEOUT = 6,
    /** 链路或 adapter 错误。 */
    SIMPLE_D2D_BUS_EVENT_LINK_ERROR = 7,
    /** app 拒绝处理 request。 */
    SIMPLE_D2D_BUS_EVENT_APP_REJECT = 8
} simple_d2d_bus_event_type_t;

/**
 * @brief D2D bus typed observer 事件。
 */
typedef struct
{
    /** 事件类型。 */
    simple_d2d_bus_event_type_t type;
    /** request cmd。 */
    d2d_cmd_t cmd;
    /** reply cmd；未知或无回复时为 0。 */
    d2d_cmd_t reply_cmd;
    /** D2D message id，NUL 结尾。 */
    char msg_id[D2D_MSG_ID_MAX_LEN + 1];
    /** 事件关联链路。 */
    d2d_link_kind_t link;
    /** 来源 endpoint。 */
    transport_endpoint_t src;
    /** 目标 endpoint。 */
    transport_endpoint_t dst;
    /** 事件结果码，0 表示成功，负值表示失败。 */
    int ret;
    /** 事件原因字符串，生命周期由 bus 内部静态字符串或调用点保证。 */
    const char *reason;
    /** 处理该 cmd 的 app 名称。 */
    const char *app_name;
} simple_d2d_bus_event_t;

/**
 * @brief D2D bus observer 回调。
 *
 * @param[in] user_ctx 用户上下文，可为 NULL。
 * @param[in] event 事件内容，不能为 NULL。
 */
typedef void (*simple_d2d_bus_observer_fn)(
    void *user_ctx,
    const simple_d2d_bus_event_t *event);

/**
 * @brief D2D bus app request 处理回调。
 *
 * @param[in] user_ctx app 用户上下文，可为 NULL。
 * @param[in] request 收到的 request message，不能为 NULL。
 * @param[in] ingress_link request 进入 bus 的链路。
 * @param[in] src request 来源 endpoint，可为 NULL。
 * @param[out] reply app 生成的 reply message，不能为 NULL。
 *
 * @retval 0 处理成功。
 * @return 负值表示 app 拒绝或处理失败。
 */
typedef int (*simple_d2d_bus_handler_fn)(
    void *user_ctx,
    const d2d_message_t *request,
    d2d_link_kind_t ingress_link,
    const transport_endpoint_t *src,
    d2d_message_t *reply);

/**
 * @brief D2D bus 业务 app 描述。
 */
typedef struct
{
    /** app 处理的 request cmd。 */
    d2d_cmd_t request_cmd;
    /** app 返回的 reply cmd。 */
    d2d_cmd_t reply_cmd;
    /** app 名称，必须非空，用于 observer 和诊断。 */
    const char *app_name;
    /** request 处理回调，不能为 NULL。 */
    simple_d2d_bus_handler_fn handler;
    /** 透传给 handler 的用户上下文。 */
    void *user_ctx;
    /** true 表示 bridge 场景下 request 可在本地 app 处理。 */
    bool bridge_local;
} simple_d2d_bus_app_t;

/**
 * @brief D2D bus 启动配置。
 */
typedef struct
{
    /** 底层 service 工作模式。 */
    simple_d2d_service_mode_t mode;
    /** 本地 D2D device type。 */
    int local_dev_type;
    /** Ethernet 接口名，可为 NULL。 */
    const char *eth_if;
    /** PLC 接口名，可为 NULL。 */
    const char *plc_if;
    /** Ethernet 本地 endpoint。 */
    transport_endpoint_t eth_local;
    /** PLC 本地 endpoint。 */
    transport_endpoint_t plc_local;
    /** Ethernet peer endpoint。 */
    transport_endpoint_t eth_peer;
    /** request 超时时间，单位为毫秒。 */
    uint32_t timeout_ms;
    /** typed observer 回调，可为 NULL。 */
    simple_d2d_bus_observer_fn observer;
    /** 透传给 observer 的用户上下文。 */
    void *observer_user_ctx;
} simple_d2d_bus_config_t;

/**
 * @brief D2D bus 实例。
 */
typedef struct
{
    /** 当前配置快照。 */
    simple_d2d_bus_config_t cfg;
    /** 底层 D2D service。 */
    simple_d2d_service_t service;
    /** 已注册 app 表。 */
    simple_d2d_bus_app_t apps[SIMPLE_D2D_BUS_MAX_APPS];
    /** 已注册 app 数量。 */
    size_t app_count;
    /** 最近 PLC request 来源，用于 reply/诊断。 */
    transport_endpoint_t last_plc_src;
    /** true 表示 bus 已启动。 */
    bool started;
} simple_d2d_bus_t;

/**
 * @brief 初始化 bus 配置默认值。
 *
 * @param[out] cfg 配置结构，不能为 NULL。
 */
void simple_d2d_bus_config_init(simple_d2d_bus_config_t *cfg);

/**
 * @brief 注册一个 D2D request cmd 处理 app。
 *
 * 只能在 bus start 前注册；同一 request_cmd 不允许重复注册。
 *
 * @param[in,out] bus bus 实例，不能为 NULL。
 * @param[in] app app 描述，不能为 NULL；app_name 和 handler 必须有效。
 *
 * @retval 0 注册成功。
 * @retval -EINVAL 参数非法或 bus 已启动。
 * @retval -EEXIST request_cmd 已被注册。
 * @retval -ENOSPC app 表已满。
 */
int simple_d2d_bus_register_app(simple_d2d_bus_t *bus,
                                const simple_d2d_bus_app_t *app);

/**
 * @brief 启动 D2D bus 和底层 service。
 *
 * @param[in,out] bus bus 实例，不能为 NULL。
 * @param[in] cfg 启动配置，不能为 NULL。
 *
 * @retval 0 启动成功。
 * @retval -EINVAL 参数非法。
 * @return 其它负值表示底层 service 启动失败。
 */
int simple_d2d_bus_start(simple_d2d_bus_t *bus,
                         const simple_d2d_bus_config_t *cfg);

/**
 * @brief 停止 D2D bus。
 *
 * @param[in,out] bus bus 实例，可为 NULL。
 */
void simple_d2d_bus_stop(simple_d2d_bus_t *bus);

/**
 * @brief 通过 bus 发送 request 消息。
 *
 * @param[in,out] bus 已启动的 bus，不能为 NULL。
 * @param[in] message 待发送 D2D message，不能为 NULL。
 * @param[in] tx 发送请求参数，不能为 NULL。
 *
 * @retval 0 发送完成或进入底层请求流程。
 * @retval -EINVAL 参数非法或 bus 未启动。
 * @return 其它负值表示底层 adapter/request 失败。
 */
int simple_d2d_bus_send_request(simple_d2d_bus_t *bus,
                                const d2d_message_t *message,
                                const d2d_adapter_request_t *tx);

/**
 * @brief 返回 bus 内部 simple_d2d_service 句柄。
 *
 * 该接口供少量 wiring 或测试代码访问底层 service；调用方不得释放返回指针。
 *
 * @param[in,out] bus bus 实例，不能为 NULL。
 * @return 底层 service 指针；参数非法时返回 NULL。
 */
simple_d2d_service_t *simple_d2d_bus_service(simple_d2d_bus_t *bus);

#ifdef SIMPLE_D2D_BUS_ENABLE_TEST_HOOKS
/** @brief 测试专用：直接注入 RX request 并执行本地 dispatch。 */
int simple_d2d_bus_dispatch_for_test(simple_d2d_bus_t *bus,
                                     const d2d_message_t *request,
                                     d2d_link_kind_t ingress_link,
                                     const transport_endpoint_t *src,
                                     d2d_message_t *reply);

/** @brief 测试专用：直接执行 route/forward 决策。 */
int simple_d2d_bus_route_for_test(simple_d2d_bus_t *bus,
                                  const d2d_message_t *message,
                                  d2d_link_kind_t ingress_link,
                                  const transport_endpoint_t *src,
                                  d2d_forwarder_route_result_t *result);

/** @brief 测试专用：注入 adapter 事件。 */
void simple_d2d_bus_adapter_event_for_test(simple_d2d_bus_t *bus,
                                           const d2d_adapter_event_t *event);

/** @brief 测试专用：注入 forwarder notify 事件。 */
void simple_d2d_bus_notify_for_test(simple_d2d_bus_t *bus,
                                    const d2d_forwarder_notify_t *notify);
#endif

#ifdef __cplusplus
}
#endif

#endif
