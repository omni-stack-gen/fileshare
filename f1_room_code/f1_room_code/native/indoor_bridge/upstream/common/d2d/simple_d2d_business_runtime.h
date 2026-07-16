/**
 * @file
 * @brief simple D2D business runtime 公共头文件。
 *
 * 本文件属于 examples 应用层，用于收口 simple main 中默认 D2D business
 * wiring。它只组合现有 unlock、室内机 heartbeat 和 indoor registry，不改变
 * 各业务协议的 wire 语义。
 */

#ifndef SIMPLE_D2D_BUSINESS_RUNTIME_H
#define SIMPLE_D2D_BUSINESS_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "simple_d2d_config.h"
#include "simple_d2d_indoor_heartbeat.h"
#include "simple_d2d_indoor_registry.h"
#include "simple_d2d_unlock.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief D2D business runtime 运行角色。 */
typedef enum
{
    /** 室内机侧：unlock client，可选 20010/20014 client。 */
    SIMPLE_D2D_BUSINESS_ROLE_INDOOR_CLIENT = 1,
    /** 门口机侧：unlock server。 */
    SIMPLE_D2D_BUSINESS_ROLE_UNIT_DOOR_SERVER = 2,
    /** converter 侧：unlock bridge，挂载 20010/20014 server app。 */
    SIMPLE_D2D_BUSINESS_ROLE_CONVERTER_BRIDGE = 3,
    /** 二次确认机侧：SPE/TCP unlock server。 */
    SIMPLE_D2D_BUSINESS_ROLE_SECONDARY_CONFIRM_SERVER = 4
} simple_d2d_business_role_t;

/** @brief 室内机默认 converter/PLC 主链路业务配置。 */
typedef struct
{
    /** primary unlock peer topology device_id，client 路径使用。 */
    const char *unlock_peer_device_id;
    /** indoor registry/heartbeat peer topology device_id，client 路径使用。 */
    const char *registry_peer_device_id;
    /** 室内机 room id，client 路径使用。 */
    const char *room_id;
} simple_d2d_business_primary_t;

/** @brief 二次确认机 SPE/TCP overlay 业务配置。 */
typedef struct
{
    /** true 表示 indoor role 额外启用二次确认机 SPE unlock client。 */
    bool enabled;
    /** 二次确认机 unlock peer topology device_id，secondary client 路径使用。 */
    const char *unlock_peer_device_id;
    /** 二次确认机 SPE endpoint；peer_addr 为 0 表示暂未学习到。 */
    simple_d2d_endpoint_config_t endpoint;
    /** 二次确认机 unlock client 共享的 ETH/SPE service；NULL 表示独立打开端口。 */
    simple_d2d_service_t *shared_service;
} simple_d2d_business_secondary_t;

/** @brief D2D business 回调集合。 */
typedef struct
{
    /** bus observer，可为 NULL。 */
    simple_d2d_bus_observer_fn observer;
    /** observer 上下文。 */
    void *observer_user_ctx;
    /** unlock server 收到合法请求后的硬件动作回调；NULL 表示仅 ACK+日志。 */
    simple_d2d_unlock_server_fn unlock_server_cb;
    /** unlock_server_cb 上下文。 */
    void *unlock_server_user_ctx;
    /** 20010 heartbeat update 回调，converter server 路径使用。 */
    simple_d2d_indoor_heartbeat_update_fn heartbeat_update_cb;
    /** 20010 heartbeat update 上下文。 */
    void *heartbeat_user_ctx;
    /** 20014 indoor registry update 回调，converter server 路径使用。 */
    simple_d2d_indoor_registry_update_fn registry_update_cb;
    /** 20014 indoor registry update 上下文。 */
    void *registry_user_ctx;
} simple_d2d_business_callbacks_t;

/**
 * @brief D2D business runtime 配置。
 *
 * 字符串指针和回调上下文由调用方持有，必须在 runtime 运行期间保持有效。
 */
typedef struct
{
    /** 当前 runtime role。 */
    simple_d2d_business_role_t role;
    /** 本机身份。 */
    simple_d2d_identity_config_t identity;
    /** true 表示 indoor role 启用默认 PLC unlock client。 */
    bool primary_unlock_enabled;
    /** ETH/SPE business endpoint。 */
    simple_d2d_endpoint_config_t eth;
    /** PLC business endpoint。 */
    simple_d2d_endpoint_config_t plc;
    /** 室内机默认 converter/PLC 主链路业务配置。 */
    simple_d2d_business_primary_t primary;
    /** 二次确认机 SPE/TCP overlay 业务配置。 */
    simple_d2d_business_secondary_t secondary;
    /** 回调集合。 */
    simple_d2d_business_callbacks_t callbacks;
} simple_d2d_business_runtime_config_t;

/** @brief D2D business runtime 实例。 */
typedef struct
{
    /** 启动配置快照。 */
    simple_d2d_business_runtime_config_t cfg;
    /** unlock runtime。 */
    simple_d2d_unlock_t unlock;
    /** 二次确认机 SPE unlock client runtime。 */
    simple_d2d_unlock_t secondary_unlock;
    /** indoor heartbeat runtime。 */
    simple_d2d_indoor_heartbeat_t heartbeat;
    /** indoor registry runtime。 */
    simple_d2d_indoor_registry_t registry;
    /** converter bridge extra app 表。 */
    simple_d2d_bus_app_t extra_apps[2];
    /** converter bridge extra app 数量。 */
    size_t extra_app_count;
    /** true 表示 runtime 已启动。 */
    bool started;
} simple_d2d_business_runtime_t;

/**
 * @brief 填充 D2D business runtime 默认配置。
 *
 * @param cfg 配置对象，不能为 NULL。
 */
void simple_d2d_business_runtime_config_init(
    simple_d2d_business_runtime_config_t *cfg);

/**
 * @brief 启动 D2D business runtime。
 *
 * @param runtime runtime 对象，不能为 NULL。
 * @param cfg 启动配置，不能为 NULL。
 * @retval 0 启动成功。
 * @retval -EINVAL 参数非法。
 * @return 其它负数表示子模块启动失败。
 */
int simple_d2d_business_runtime_start(
    simple_d2d_business_runtime_t *runtime,
    const simple_d2d_business_runtime_config_t *cfg);

/**
 * @brief 停止 D2D business runtime。
 *
 * @param runtime runtime 对象，可为 NULL。
 */
void simple_d2d_business_runtime_stop(simple_d2d_business_runtime_t *runtime);

/**
 * @brief 返回 runtime 内部 primary unlock 句柄。
 *
 * @param runtime runtime 对象。
 * @return unlock runtime 指针；参数非法时返回 NULL。
 */
simple_d2d_unlock_t *simple_d2d_business_runtime_unlock(
    simple_d2d_business_runtime_t *runtime);

/**
 * @brief 返回 runtime 内部二次确认机 unlock client 句柄。
 *
 * @param runtime runtime 对象。
 * @return secondary unlock runtime 指针；参数非法时返回 NULL。
 */
simple_d2d_unlock_t *simple_d2d_business_runtime_secondary_unlock(
    simple_d2d_business_runtime_t *runtime);

/**
 * @brief 更新二次确认机 unlock client 的 peer SPE 地址。
 *
 * @param runtime runtime 对象。
 * @param peer_addr 二次确认机 SPE IP，host byte order。
 * @return 0 表示成功，负数表示失败。
 */
int simple_d2d_business_runtime_update_secondary_unlock_peer(
    simple_d2d_business_runtime_t *runtime,
    uint32_t peer_addr);

/**
 * @brief 根据目标 device_id 选择 primary/secondary unlock client 并发送。
 *
 * @param runtime runtime 对象。
 * @param target_device_id 开锁目标 device_id。
 * @param id 房号或开锁对象 id。
 * @param place 开锁位置/门点编号。
 * @return 0 表示发送成功，负数表示失败。
 */
int simple_d2d_business_runtime_send_unlock(
    simple_d2d_business_runtime_t *runtime,
    const char *target_device_id,
    const char *id,
    int place);

/**
 * @brief 返回 runtime 内部 indoor heartbeat 句柄。
 *
 * @param runtime runtime 对象。
 * @return heartbeat runtime 指针；参数非法时返回 NULL。
 */
simple_d2d_indoor_heartbeat_t *simple_d2d_business_runtime_heartbeat(
    simple_d2d_business_runtime_t *runtime);

/**
 * @brief 返回 runtime 内部 indoor registry 句柄。
 *
 * @param runtime runtime 对象。
 * @return registry runtime 指针；参数非法时返回 NULL。
 */
simple_d2d_indoor_registry_t *simple_d2d_business_runtime_registry(
    simple_d2d_business_runtime_t *runtime);

/**
 * @brief 返回 runtime 内部 D2D bus service 句柄。
 *
 * @param runtime runtime 对象。
 * @return bus service 指针；未启动或参数非法时返回 NULL。
 */
simple_d2d_service_t *simple_d2d_business_runtime_bus_service(
    simple_d2d_business_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_D2D_BUSINESS_RUNTIME_H */
