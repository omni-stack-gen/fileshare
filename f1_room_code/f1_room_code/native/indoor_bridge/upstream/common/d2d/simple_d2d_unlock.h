/**
 * @file
 * @brief simple 示例应用公共模块 simple_d2d_unlock 的公共头文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef SIMPLE_D2D_UNLOCK_H
#define SIMPLE_D2D_UNLOCK_H

#include <stdbool.h>
#include <stdint.h>

#include "simple_d2d_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief D2D 开锁以太网/SPE 侧默认 TCP 端口，按二线设备 TCP/JSON 协议使用 0x9982。 */
#define SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT (0x9982U)
/** @brief D2D 开锁 PLC 侧默认端口，统一使用 simple PLC D2D 控制端口。 */
#define SIMPLE_D2D_UNLOCK_DEFAULT_PLC_PORT SIMPLE_D2D_PLC_CONTROL_PORT
/** @brief 默认开锁端口，当前等同于以太网侧端口。 */
#define SIMPLE_D2D_UNLOCK_DEFAULT_PORT SIMPLE_D2D_UNLOCK_DEFAULT_ETH_PORT

/**
 * @brief simple D2D 开锁运行角色。
 */
typedef enum
{
    /** 室内机等发起开锁请求的一侧。 */
    SIMPLE_D2D_UNLOCK_ROLE_CLIENT = 1,
    /** 门口机等接收开锁请求的一侧。 */
    SIMPLE_D2D_UNLOCK_ROLE_SERVER = 2,
    /** 转换盒等在以太网和 PLC 之间转发开锁消息的一侧。 */
    SIMPLE_D2D_UNLOCK_ROLE_BRIDGE = 3
} simple_d2d_unlock_role_t;

/**
 * @brief D2D 开锁 client 发送链路。
 */
typedef enum
{
    /** 默认 PLC 链路，保持既有室内机 -> converter/door 开锁行为。 */
    SIMPLE_D2D_UNLOCK_CLIENT_LINK_PLC = 0,
    /** 以太网/SPE 链路，用于室内机 -> 二次确认机开锁。 */
    SIMPLE_D2D_UNLOCK_CLIENT_LINK_ETH = 1
} simple_d2d_unlock_client_link_t;

/**
 * @brief D2D 开锁 server 收到的业务请求。
 */
typedef struct
{
    /** 请求来源 topology device_id。 */
    const char *orig_device_id;
    /** 请求目标 topology device_id。 */
    const char *dest_device_id;
    /** 房号或开锁对象 id。 */
    const char *id;
    /** 开锁位置/门点编号。 */
    int place;
    /** 请求进入 server 的链路。 */
    d2d_link_kind_t ingress_link;
    /** 请求来源 endpoint；未知时字段为 0。 */
    transport_endpoint_t source_endpoint;
} simple_d2d_unlock_request_t;

/**
 * @brief D2D 开锁 server 硬件动作回调。
 *
 * @retval 0 硬件动作成功，server 回复 result=1。
 * @return 非 0 表示硬件动作失败，server 回复 result=0。
 */
typedef int (*simple_d2d_unlock_server_fn)(
    void *user_ctx,
    const simple_d2d_unlock_request_t *request);

/** @brief D2D 开锁 server/observer 回调集合。 */
typedef struct
{
    /** 结构化 bus observer；默认可为 NULL。 */
    simple_d2d_observer_config_t observer;
    /** server 收到合法开锁请求后的硬件动作回调；NULL 表示仅 ACK+日志。 */
    simple_d2d_unlock_server_fn server_unlock_cb;
    /** server_unlock_cb 上下文。 */
    void *server_unlock_user_ctx;
} simple_d2d_unlock_callbacks_t;

/** @brief D2D 开锁 bridge/adapter 额外业务 app 集合。 */
typedef struct
{
    /** bridge/adapter 启动时额外注册到 simple_d2d_bus 的业务 app。 */
    const simple_d2d_bus_app_t *apps;
    /** 额外业务 app 数量。 */
    size_t count;
} simple_d2d_unlock_extra_apps_t;

/**
 * @brief D2D 开锁 runtime 配置。
 *
 * 字符串指针由调用方持有，必须在 simple_d2d_unlock_start() 调用期间保持有效。
 */
typedef struct
{
    /** 当前节点角色。 */
    simple_d2d_unlock_role_t role;
    /** client 发送链路；0 表示默认 PLC。 */
    simple_d2d_unlock_client_link_t client_link;
    /** 本机/对端身份。 */
    simple_d2d_identity_config_t identity;
    /** 以太网/SPE endpoint。 */
    simple_d2d_endpoint_config_t eth;
    /** PLC endpoint。 */
    simple_d2d_endpoint_config_t plc;
    /** 请求超时时间，单位毫秒。 */
    uint32_t timeout_ms;
    /** 共享 D2D service；非 NULL 时不单独打开 ETH/PLC D2D 端口。 */
    simple_d2d_service_t *shared_service;
    /** 回调集合。 */
    simple_d2d_unlock_callbacks_t callbacks;
    /** 额外业务 app 集合。 */
    simple_d2d_unlock_extra_apps_t extra_apps;
} simple_d2d_unlock_config_t;

/**
 * @brief D2D 开锁 runtime 实例。
 *
 * 调用方可栈上声明该对象；内部资源由 start/stop 成对管理。
 */
typedef struct
{
    /** 启动时拷贝的配置。 */
    simple_d2d_unlock_config_t cfg;
    /** D2D 0x03 bus，持有 adapter/forwarder 和底层链路。 */
    simple_d2d_bus_t bus;
    /** runtime 是否已启动。 */
    bool started;
} simple_d2d_unlock_t;

/**
 * @brief 填充 D2D 开锁默认配置。
 *
 * @param[out] cfg 配置对象，不能为 NULL。
 */
void simple_d2d_unlock_config_init(simple_d2d_unlock_config_t *cfg);

/**
 * @brief 启动 D2D 开锁 runtime。
 *
 * @param[in,out] unlock runtime 对象，不能为 NULL。
 * @param[in] cfg 启动配置，不能为 NULL。
 * @retval 0 启动成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOMEM 内存不足。
 */
int simple_d2d_unlock_start(simple_d2d_unlock_t *unlock,
                            const simple_d2d_unlock_config_t *cfg);

/**
 * @brief 停止并释放 D2D 开锁 runtime 内部资源。
 *
 * @param[in,out] unlock runtime 对象，可为 NULL。
 */
void simple_d2d_unlock_stop(simple_d2d_unlock_t *unlock);

/**
 * @brief 发送一次 D2D 开锁请求。
 *
 * @param[in,out] unlock 已启动的 runtime，不能为 NULL。
 * @param[in] id 房号或开锁对象 id，不能为 NULL。
 * @param[in] place 开锁位置/门点编号。
 * @retval 0 发送成功。
 * @retval -EINVAL 参数非法或 runtime 未启动。
 */
int simple_d2d_unlock_send(simple_d2d_unlock_t *unlock,
                           const char *id,
                           int place);

/**
 * @brief 向指定目标设备发送一次 D2D 开锁请求。
 *
 * @param[in,out] unlock 已启动的 runtime，不能为 NULL。
 * @param[in] peer_id 目标 topology device_id，不能为 NULL。
 * @param[in] id 房号或开锁对象 id，不能为 NULL。
 * @param[in] place 开锁位置/门点编号。
 * @retval 0 发送成功。
 * @retval -EINVAL 参数非法或 runtime 未启动。
 */
int simple_d2d_unlock_send_to(simple_d2d_unlock_t *unlock,
                              const char *peer_id,
                              const char *id,
                              int place);

/**
 * @brief 填充 unlock bus app 描述，供外部共享 bus 注册 20130/20131。
 *
 * @param[out] app app 描述，不能为 NULL。
 * @param[in,out] unlock unlock runtime，不能为 NULL。
 * @param[in] bridge_local true 表示 bridge 模式下可在本地处理 request。
 */
void simple_d2d_unlock_bus_app_init(simple_d2d_bus_app_t *app,
                                    simple_d2d_unlock_t *unlock,
                                    bool bridge_local);

#ifdef SIMPLE_D2D_UNLOCK_ENABLE_TEST_HOOKS
/**
 * @brief 测试专用：按配置构造 unlock request message。
 *
 * @param[in] cfg unlock 配置，不能为 NULL。
 * @param[in] peer_id 目标 topology device_id，不能为 NULL。
 * @param[in] id 房号或开锁对象 id，不能为 NULL。
 * @param[in] place 开锁位置/门点编号。
 * @param[out] message 输出 D2D message，不能为 NULL。
 *
 * @retval 0 构造成功。
 * @retval -EINVAL 参数非法。
 */
int simple_d2d_unlock_build_request_for_test(const simple_d2d_unlock_config_t *cfg,
                                             const char *peer_id,
                                             const char *id,
                                             int place,
                                             d2d_message_t *message);

/**
 * @brief 测试专用：直接调用 unlock request handler。
 *
 * @param[in,out] unlock 已初始化的 unlock runtime，不能为 NULL。
 * @param[in] request 输入 20130 request，不能为 NULL。
 * @param[out] reply 输出 20131 reply，不能为 NULL。
 * @retval 0 处理成功。
 * @retval -EINVAL 参数非法或请求格式非法。
 */
int simple_d2d_unlock_handle_request_for_test(simple_d2d_unlock_t *unlock,
                                              const d2d_message_t *request,
                                              d2d_message_t *reply);
#endif

#ifdef __cplusplus
}
#endif

#endif
