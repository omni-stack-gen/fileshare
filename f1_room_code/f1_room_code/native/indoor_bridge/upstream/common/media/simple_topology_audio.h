/**
 * @file
 * @brief simple 示例应用公共模块 simple_topology_audio 的公共头文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_TOPOLOGY_AUDIO_H__
#define __SIMPLE_TOPOLOGY_AUDIO_H__

#include <stdbool.h>
#include <stdint.h>

#include "simple_media.h"
#include "simple_topology_profile.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief simple topology audio 运行上下文。
 */
typedef struct
{
    /** 是否启用 topology audio。 */
    bool enabled;
    /** 日志节点名，NUL 结尾。 */
    char node_name[32];
    /** 当前 simple 设备 device_id，NUL 结尾。 */
    char self_device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** topology 图上下文，由 init 创建、deinit 释放。 */
    topology_ctx_t *topology;
} simple_topology_audio_t;

/**
 * @brief 初始化 topology audio 运行上下文。
 *
 * @param[out] ctx 上下文对象，不能为 NULL。
 * @param[in] node_name 日志节点名，不能为 NULL。
 * @param[in] self_device_id 当前设备 device_id，不能为 NULL。
 * @retval 0 初始化成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOMEM 内存不足。
 */
int simple_topology_audio_init(simple_topology_audio_t *ctx,
                               const char *node_name,
                               const char *self_device_id);

/**
 * @brief 释放 topology audio 上下文。
 *
 * @param[in,out] ctx 上下文对象，可为 NULL。
 */
void simple_topology_audio_deinit(simple_topology_audio_t *ctx);

/**
 * @brief 根据 topology 图解析音频 endpoint facts，不应用到 media。
 *
 * @param[in,out] ctx topology audio 上下文，不能为 NULL。
 * @param[in] request endpoint provider 请求，不能为 NULL。
 * @param[in] expected_link_kind 期望 TX 链路类型，用于 fail closed 校验。
 * @param[in] expected_rx_port 期望 RX 端口；0 表示不校验。
 * @param[out] audio_out topology audio 查询结果，不能为 NULL。
 * @param[out] local_endpoint_out 本端声明 endpoint，不能为 NULL。
 * @retval 0 endpoint facts 查询成功。
 * @retval <0 查询或校验失败。
 */
int simple_topology_audio_resolve_binding(
    simple_topology_audio_t *ctx,
    const simple_media_endpoint_request_t *request,
    topology_link_kind_t expected_link_kind,
    uint16_t expected_rx_port,
    topology_audio_endpoint_result_t *audio_out,
    topology_endpoint_t *local_endpoint_out);

#ifdef __cplusplus
}
#endif

#endif
