/**
 * @file
 * @brief simple D2D helper 公共配置类型。
 *
 * 本文件属于 examples/common/d2d helper 层，只用于收口 simple D2D 业务 helper
 * 的配置形态，不作为 lib/ public API。
 */

#ifndef SIMPLE_D2D_CONFIG_H
#define SIMPLE_D2D_CONFIG_H

#include <stdint.h>

#include "simple_d2d_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief D2D helper 使用的链路 endpoint 配置。 */
typedef struct
{
    /** 接口名，可为 NULL。 */
    const char *if_name;
    /** 本地地址，host order。 */
    uint32_t local_addr;
    /** 对端地址，host order。 */
    uint32_t peer_addr;
    /** 端口；0 表示使用对应协议默认端口。 */
    uint16_t port;
} simple_d2d_endpoint_config_t;

/** @brief D2D helper 使用的本机/对端身份配置。 */
typedef struct
{
    /** 本地 topology device_id。 */
    const char *local_device_id;
    /** 对端 topology device_id。 */
    const char *peer_device_id;
    /** 本地 D2D devType；0 表示由具体 helper/role 使用默认值。 */
    int local_dev_type;
} simple_d2d_identity_config_t;

/** @brief D2D helper 使用的周期、TTL 和请求超时配置。 */
typedef struct
{
    /** 周期，单位毫秒；0 表示使用 helper 默认值。 */
    uint32_t interval_ms;
    /** 在线 TTL，单位毫秒；0 表示使用 helper 默认值。 */
    uint32_t ttl_ms;
    /** 请求超时，单位毫秒；0 表示使用 helper 默认值。 */
    uint32_t timeout_ms;
} simple_d2d_timing_config_t;

/** @brief D2D helper 使用的设备文本字段配置。 */
typedef struct
{
    /** devModel 字段。 */
    const char *dev_model;
    /** sysVer 字段。 */
    const char *sys_ver;
    /** appVer 字段。 */
    const char *app_ver;
    /** syncVer 字段。 */
    const char *sync_ver;
} simple_d2d_device_text_config_t;

/** @brief D2D helper 使用的 bus observer 配置。 */
typedef struct
{
    /** bus observer，可为 NULL。 */
    simple_d2d_bus_observer_fn observer;
    /** observer 上下文。 */
    void *observer_user_ctx;
} simple_d2d_observer_config_t;

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_D2D_CONFIG_H */
