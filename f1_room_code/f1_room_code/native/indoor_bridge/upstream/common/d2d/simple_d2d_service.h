/**
 * @file
 * @brief simple D2D runtime 服务公共头文件。
 *
 * 本文件属于 examples 应用层，用于统一管理 D2D adapter/forwarder 与底层传输链路。
 */

#ifndef SIMPLE_D2D_SERVICE_H
#define SIMPLE_D2D_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "d2d_forwarder.h"
#include "simple_plc_ports.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief simple 主线所有 PLC D2D 控制协议统一使用的端口。 */
#define SIMPLE_D2D_PLC_CONTROL_PORT SIMPLE_PLC_PORT_D2D_CONTROL

/** @brief simple D2D runtime 工作模式。 */
typedef enum
{
    /** 单链路 adapter，链路类型为 PLC。 */
    SIMPLE_D2D_SERVICE_MODE_ADAPTER_PLC = 1,
    /** 单链路 adapter，链路类型为以太网。 */
    SIMPLE_D2D_SERVICE_MODE_ADAPTER_ETH = 2,
    /** 双链路 forwarder，在以太网和 PLC 之间转发。 */
    SIMPLE_D2D_SERVICE_MODE_BRIDGE = 3
} simple_d2d_service_mode_t;

/** @brief simple D2D runtime 服务配置。 */
typedef struct
{
    /** 工作模式。 */
    simple_d2d_service_mode_t mode;
    /** 传入 D2D adapter/forwarder 的本地设备类型。 */
    int local_dev_type;
    /** 以太网接口名，可为 NULL。 */
    const char *eth_if;
    /** PLC 接口名，可为 NULL。 */
    const char *plc_if;
    /** 以太网本地地址。 */
    transport_endpoint_t eth_local;
    /** PLC 本地地址。 */
    transport_endpoint_t plc_local;
    /** D2D 超时时间，单位毫秒；0 表示使用 5000ms。 */
    uint32_t timeout_ms;
    /** adapter 事件回调，adapter 模式使用。 */
    d2d_adapter_event_fn_t event_cb;
    /** forwarder 路由回调，bridge 模式使用。 */
    d2d_forwarder_route_fn_t route_cb;
    /** forwarder 通知回调，bridge 模式使用。 */
    d2d_forwarder_notify_fn_t notify_cb;
    /** 回调上下文。 */
    void *user_data;
} simple_d2d_service_config_t;

/** @brief simple D2D runtime 服务实例。 */
typedef struct
{
    /** 启动时拷贝的配置。 */
    simple_d2d_service_config_t cfg;
    /** 以太网 transport。 */
    transport_handle_t *eth_transport;
    /** PLC transport。 */
    transport_handle_t *plc_transport;
    /** 单链路 adapter。 */
    d2d_adapter_t *adapter;
    /** 双链路 forwarder。 */
    d2d_forwarder_t *forwarder;
    /** link 缓存，最多 eth/plc 两条。 */
    d2d_adapter_link_t links[2];
    /** 是否已启动。 */
    bool started;
} simple_d2d_service_t;

/**
 * @brief 启动 simple D2D runtime 服务。
 *
 * @param[in,out] service 服务对象，不能为 NULL。
 * @param[in] cfg 服务配置，不能为 NULL。
 * @retval 0 启动成功。
 * @retval -EINVAL 参数非法。
 * @retval -EIO 底层 transport/adapter/forwarder 启动失败。
 */
int simple_d2d_service_start(simple_d2d_service_t *service,
                             const simple_d2d_service_config_t *cfg);

/**
 * @brief 停止并释放 simple D2D runtime 服务。
 *
 * @param[in,out] service 服务对象，可为 NULL。
 */
void simple_d2d_service_stop(simple_d2d_service_t *service);

/**
 * @brief 获取当前 adapter 实例。
 *
 * @param[in] service 服务对象，可为 NULL。
 * @return adapter 指针；未启动 adapter 模式时返回 NULL。
 */
d2d_adapter_t *simple_d2d_service_adapter(simple_d2d_service_t *service);

/**
 * @brief 获取当前 forwarder 实例。
 *
 * @param[in] service 服务对象，可为 NULL。
 * @return forwarder 指针；未启动 bridge 模式时返回 NULL。
 */
d2d_forwarder_t *simple_d2d_service_forwarder(simple_d2d_service_t *service);

#ifdef __cplusplus
}
#endif

#endif
