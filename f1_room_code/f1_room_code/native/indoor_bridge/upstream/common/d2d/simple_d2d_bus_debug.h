/**
 * @file
 * @brief simple D2D 0x03 bus DEBUG observer adapter.
 *
 * 本文件属于 examples 应用层，用于把 typed observer event 渲染为 DEBUG 日志。
 */

#ifndef SIMPLE_D2D_BUS_DEBUG_H
#define SIMPLE_D2D_BUS_DEBUG_H

#include "simple_d2d_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief D2D bus DEBUG 文本渲染 observer。
 *
 * @param[in] user_ctx 未使用的用户上下文，可为 NULL。
 * @param[in] event typed observer 事件，不能为 NULL。
 */
void simple_d2d_bus_debug_observer(void *user_ctx,
                                   const simple_d2d_bus_event_t *event);

#ifdef __cplusplus
}
#endif

#endif
