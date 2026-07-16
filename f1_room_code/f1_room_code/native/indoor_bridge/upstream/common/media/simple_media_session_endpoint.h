/**
 * @file
 * @brief simple 示例媒体 session endpoint 分配器。
 *
 * 本文件属于 examples 应用层，用于按 session/source/domain/gateway 为 CALL wire
 * endpoint 和本地媒体 apply 生成一致的地址、端口和 slot。
 */

#ifndef __SIMPLE_MEDIA_SESSION_ENDPOINT_H__
#define __SIMPLE_MEDIA_SESSION_ENDPOINT_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "call_proto.h"
#include "simple_media.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief session endpoint allocator 可分配的每类媒体最大 slot 数量。 */
#define SIMPLE_MEDIA_SESSION_SLOT_MAX (16U)
/** @brief scope slot 最小有效值。 */
#define SIMPLE_MEDIA_SCOPE_SLOT_MIN   (1U)
/** @brief scope slot 最大有效值。 */
#define SIMPLE_MEDIA_SCOPE_SLOT_MAX   (254U)

/**
 * @brief simple session endpoint 媒体类型。
 */
typedef enum
{
    /** 视频 endpoint。 */
    SIMPLE_MEDIA_KIND_VIDEO = 1,
    /** 音频 endpoint。 */
    SIMPLE_MEDIA_KIND_AUDIO = 2,
} simple_media_kind_t;

/**
 * @brief session endpoint 绑定输入。
 */
typedef struct
{
    /** session id。 */
    uint64_t session_id;
    /** 媒体类型，取 simple_media_kind_t。 */
    uint8_t media_kind;
    /** 逻辑媒体源 device_id。 */
    char logical_source_id[CALL_DEVICE_ID_MAX];
    /** 拓扑 domain id。 */
    char domain_id[CALL_DEVICE_ID_MAX];
    /** directory gateway id。 */
    char gateway_id[CALL_DEVICE_ID_MAX];
    /** 首选 scope slot；0 表示由 allocator 根据标识 hash 派生。 */
    uint8_t preferred_scope_slot;
} simple_media_session_endpoint_bind_cfg_t;

/**
 * @brief session endpoint 绑定结果。
 */
typedef struct
{
    /** session id。 */
    uint64_t session_id;
    /** 媒体类型，取 simple_media_kind_t。 */
    uint8_t media_kind;
    /** 逻辑媒体源 device_id。 */
    char logical_source_id[CALL_DEVICE_ID_MAX];
    /** 拓扑 domain id。 */
    char domain_id[CALL_DEVICE_ID_MAX];
    /** directory gateway id。 */
    char gateway_id[CALL_DEVICE_ID_MAX];
    /** session 维度 slot，用于端口生成。 */
    uint8_t session_slot;
    /** scope 维度 slot，用于组地址生成。 */
    uint8_t scope_slot;
    /** true 表示 scope_slot 由 hash 派生。 */
    bool scope_derived;
} simple_media_session_endpoint_binding_t;

/**
 * @brief allocator 内部 entry。
 */
typedef struct
{
    /** true 表示 entry 正在使用。 */
    bool in_use;
    /** entry 保存的绑定。 */
    simple_media_session_endpoint_binding_t binding;
} simple_media_session_endpoint_entry_t;

/**
 * @brief session endpoint slot 分配器。
 */
typedef struct
{
    /** video/audio 共用 entry 表。 */
    simple_media_session_endpoint_entry_t entries[SIMPLE_MEDIA_SESSION_SLOT_MAX * 2U];
} simple_media_session_endpoint_allocator_t;

/** @brief 初始化 session endpoint slot 分配器。 */
void simple_media_session_endpoint_allocator_init(
    simple_media_session_endpoint_allocator_t *allocator);

/**
 * @brief 为指定 session/media 绑定或复用 endpoint slot。
 *
 * @param[in,out] allocator 分配器实例，不能为 NULL。
 * @param[in] cfg 绑定配置，不能为 NULL。
 * @param[out] binding_out 绑定结果，不能为 NULL。
 *
 * @retval 0 绑定成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOSPC slot 已满。
 */
int simple_media_session_endpoint_bind(
    simple_media_session_endpoint_allocator_t *allocator,
    const simple_media_session_endpoint_bind_cfg_t *cfg,
    simple_media_session_endpoint_binding_t *binding_out);

/** @brief 释放指定 session 的所有 endpoint slot。 */
void simple_media_session_endpoint_release_session(
    simple_media_session_endpoint_allocator_t *allocator,
    uint64_t session_id);

/** @brief 根据 scope/session slot 生成 UDP 组播地址。 */
uint32_t simple_media_session_endpoint_udp_group(uint8_t scope_slot,
                                                 uint8_t session_slot);
/** @brief 根据 session slot 生成 UDP video 端口。 */
uint16_t simple_media_session_endpoint_udp_video_port(uint8_t session_slot);
/** @brief 根据 session slot 生成 UDP audio 端口。 */
uint16_t simple_media_session_endpoint_udp_audio_port(uint8_t session_slot);
/** @brief 根据 scope/session slot 生成 PLC 组地址。 */
uint32_t simple_media_session_endpoint_plc_group(uint8_t scope_slot,
                                                 uint8_t session_slot);
/** @brief 返回 simple 示例使用的 PLC media 端口。 */
uint16_t simple_media_session_endpoint_plc_media_port(void);
/** @brief 根据 domain/gateway/source 计算稳定 scope slot。 */
uint8_t simple_media_session_endpoint_hash_scope(const char *domain_id,
                                                 const char *gateway_id,
                                                 const char *logical_source_id);

#ifdef __cplusplus
}
#endif

#endif
