/**
 * @file
 * @brief simple 示例应用公共 endpoint provider。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_ENDPOINT_PROVIDER_H__
#define __SIMPLE_ENDPOINT_PROVIDER_H__

#include <stdbool.h>
#include <stdint.h>

#include "simple_media.h"
#include "simple_media_session_endpoint.h"
#include "topology/topology.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief endpoint provider 所在设备角色。
 */
typedef enum
{
    /** 管理机。 */
    SIMPLE_ENDPOINT_PROVIDER_ROLE_MANAGE = 0,
    /** 室内机。 */
    SIMPLE_ENDPOINT_PROVIDER_ROLE_INDOOR,
    /** 单元门口机。 */
    SIMPLE_ENDPOINT_PROVIDER_ROLE_UNIT_DOOR,
    /** 二次确认机。 */
    SIMPLE_ENDPOINT_PROVIDER_ROLE_SECONDARY_CONFIRM,
    /** host 回归节点。 */
    SIMPLE_ENDPOINT_PROVIDER_ROLE_HOST,
} simple_endpoint_provider_role_t;

/**
 * @brief audio binding-only resolver 回调。
 *
 * provider 通过该回调向 topology/directory 查询音频 binding facts，不直接 apply media。
 *
 * @param[in] user_ctx 用户上下文，可为 NULL。
 * @param[in] request endpoint 请求，不能为 NULL。
 * @param[out] audio_out audio endpoint 结果，不能为 NULL。
 * @param[out] local_endpoint_out 本地拓扑 endpoint，可为 NULL。
 *
 * @retval 0 查询成功。
 * @return 负值表示 topology/directory 未就绪或 binding 失败。
 */
typedef int (*simple_endpoint_provider_audio_resolver_t)(
    void *user_ctx,
    const simple_media_endpoint_request_t *request,
    topology_audio_endpoint_result_t *audio_out,
    topology_endpoint_t *local_endpoint_out);

/**
 * @brief endpoint provider 配置。
 */
typedef struct
{
    /** 当前节点角色。 */
    simple_endpoint_provider_role_t role;
    /** 节点名称，仅用于诊断，可为 NULL。 */
    const char *node_name;
    /** 管理机 device_id，可为 NULL。 */
    const char *manage_device_id;
    /** 本地 UDP media IPv4 地址，主机字节序。 */
    uint32_t local_media_addr;
    /** true 表示允许生成 video binding。 */
    bool video_enabled;
    /** true 表示允许生成 audio binding。 */
    bool audio_enabled;
    /** true 表示当前业务希望 video active。 */
    bool video_desired_active;
    /** true 表示强制覆盖 video link kind。 */
    bool video_link_override_enabled;
    /** video link override 值。 */
    topology_link_kind_t video_link_kind;
    /** video 所属 domain id，可为 NULL。 */
    const char *video_domain_id;
    /** true 表示当前角色使用静态 UDP/SPE 对端，不走 topology audio resolver。 */
    bool static_udp_enabled;
    /** 静态 UDP/SPE domain id，可为 NULL。 */
    const char *static_udp_domain_id;
    /** true 表示可复用已存在 PLC video endpoint facts。 */
    bool existing_plc_video_endpoint_enabled;
    /** 已存在 PLC video 地址。 */
    uint32_t existing_plc_video_addr;
    /** 已存在 PLC video 端口。 */
    uint16_t existing_plc_video_port;
    /** 可选 audio resolver。 */
    simple_endpoint_provider_audio_resolver_t audio_resolver;
    /** 透传给 audio_resolver 的用户上下文。 */
    void *audio_resolver_ctx;
} simple_endpoint_provider_config_t;

/**
 * @brief endpoint provider 实例。
 */
typedef struct
{
    /** session endpoint slot 分配器。 */
    simple_media_session_endpoint_allocator_t allocator;
} simple_endpoint_provider_t;

/**
 * @brief 初始化 endpoint provider。
 *
 * @param[out] provider provider 实例，不能为 NULL。
 *
 * @retval 0 初始化成功。
 * @retval -EINVAL 参数非法。
 */
int simple_endpoint_provider_init(simple_endpoint_provider_t *provider);

/**
 * @brief 根据角色、请求和可选 topology resolver 生成媒体 endpoint binding。
 *
 * provider 只返回 `simple_media_endpoint_result_t`，实际 media apply/start 由调用方继续执行。
 *
 * @param[in,out] provider provider 实例，不能为 NULL。
 * @param[in] cfg provider 配置，不能为 NULL。
 * @param[in] request endpoint 请求，不能为 NULL。
 * @param[out] result endpoint 结果，不能为 NULL。
 *
 * @retval 0 解析成功。
 * @retval -EINVAL 参数非法。
 * @return 其它负值表示 binding/resolver 失败。
 */
int simple_endpoint_provider_resolve(
    simple_endpoint_provider_t *provider,
    const simple_endpoint_provider_config_t *cfg,
    const simple_media_endpoint_request_t *request,
    simple_media_endpoint_result_t *result);

/**
 * @brief 释放指定 session 占用的 endpoint slot。
 *
 * @param[in,out] provider provider 实例，不能为 NULL。
 * @param[in] session_id 要释放的 session id。
 */
void simple_endpoint_provider_release_session(simple_endpoint_provider_t *provider,
                                              uint64_t session_id);

#ifdef __cplusplus
}
#endif

#endif /* __SIMPLE_ENDPOINT_PROVIDER_H__ */
