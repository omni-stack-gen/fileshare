/**
 * @file
 * @brief simple 室内机 registry 的 D2D bus app 适配头文件。
 *
 * 本文件属于 examples 应用层，用于把 20014/20015 registry 挂到 PLC 0x03
 * simple_d2d_bus 上。
 */

#ifndef SIMPLE_D2D_INDOOR_REGISTRY_BUS_APP_H
#define SIMPLE_D2D_INDOOR_REGISTRY_BUS_APP_H

#include <stdbool.h>

#include "simple_d2d_bus.h"
#include "simple_d2d_indoor_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 indoor_registry bus app 描述。
 *
 * @param[out] app 待填充的 bus app。
 * @param[in,out] registry registry 运行态，由调用方持有。
 * @param[in] bridge_local true 表示 forwarder 收到后本地处理并回 20015。
 */
void simple_d2d_indoor_registry_bus_app_init(
    simple_d2d_bus_app_t *app,
    simple_d2d_indoor_registry_t *registry,
    bool bridge_local);

#ifdef __cplusplus
}
#endif

#endif
