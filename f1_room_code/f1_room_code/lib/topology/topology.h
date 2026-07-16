/**
 * @file topology.h
 * @brief 拓扑图模型与下一跳查询接口。
 */
#ifndef __TOPOLOGY_H__
#define __TOPOLOGY_H__

#include "topology_ip.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief domain_id 字符串最大长度，包含字符串结束符。 */
#define TOPOLOGY_DOMAIN_ID_MAX (64U)

/** @brief profile/tag 字符串最大长度，包含字符串结束符。 */
#define TOPOLOGY_PROFILE_MAX (32U)

/** @brief 路径解释字符串最大长度，包含字符串结束符。 */
#define TOPOLOGY_EXPLAIN_MAX (256U)

/** @brief 图模型第一版内部节点容量上限。 */
#define TOPOLOGY_MAX_NODES (64U)

/** @brief 图模型第一版内部 domain 容量上限。 */
#define TOPOLOGY_MAX_DOMAINS (64U)

/** @brief 图模型第一版内部 link 容量上限。 */
#define TOPOLOGY_MAX_LINKS (128U)

/** @brief 图模型第一版内部 endpoint 容量上限。 */
#define TOPOLOGY_MAX_ENDPOINTS (128U)

/** @brief 图模型第一版内部 role 容量上限。 */
#define TOPOLOGY_MAX_ROLES (128U)

/** @brief 链路支持控制面路由。 */
#define TOPOLOGY_CAP_CONTROL (1U << 0)

/** @brief 链路支持音频媒体。 */
#define TOPOLOGY_CAP_AUDIO (1U << 1)

/** @brief 链路支持视频媒体。 */
#define TOPOLOGY_CAP_VIDEO (1U << 2)

/** @brief 链路支持监视业务。 */
#define TOPOLOGY_CAP_MONITOR (1U << 3)

/** @brief 链路支持发现或诊断业务。 */
#define TOPOLOGY_CAP_DIAGNOSTIC (1U << 4)

/**
 * @brief 拓扑图上下文。
 *
 * 调用方通过 topology_create() 创建，通过 topology_destroy() 释放。
 */
typedef struct topology_ctx topology_ctx_t;

/**
 * @brief 拓扑上下文配置。
 *
 * 第一版使用固定内部容量，当前字段预留给后续扩展。
 */
typedef struct
{
    /** @brief 预留字段，当前必须填 `0`。 */
    uint32_t reserved;
} topology_config_t;

/**
 * @brief 链路类型。
 */
typedef enum
{
    /** @brief 未知链路。 */
    TOPOLOGY_LINK_UNKNOWN = 0,
    /** @brief 小区 LAN / 标准以太网链路。 */
    TOPOLOGY_LINK_LAN,
    /** @brief SPE 单对以太网链路，现场可称 2 线链路。 */
    TOPOLOGY_LINK_SPE,
    /** @brief PLC 链路。 */
    TOPOLOGY_LINK_PLC,
    /** @brief WiFi 链路。 */
    TOPOLOGY_LINK_WIFI,
} topology_link_kind_t;

/**
 * @brief domain 类型。
 */
typedef enum
{
    /** @brief 未知 domain。 */
    TOPOLOGY_DOMAIN_UNKNOWN = 0,
    /** @brief 小区 LAN / 标准以太网 domain。 */
    TOPOLOGY_DOMAIN_COMMUNITY_LAN,
    /** @brief 单元机到转换盒的 SPE 下行 domain。 */
    TOPOLOGY_DOMAIN_UNIT_DOWNLINK_SPE,
    /** @brief 转换盒下行 PLC domain。 */
    TOPOLOGY_DOMAIN_CONVERTER_PLC,
    /** @brief 小高层单元机 / 门口机直连 PLC domain。 */
    TOPOLOGY_DOMAIN_LOWRISE_PLC,
    /** @brief 别墅型室内机 PLC 手拉手 domain。 */
    TOPOLOGY_DOMAIN_VILLA_PLC,
    /** @brief 室内机到二次确认机的户内 SPE domain。 */
    TOPOLOGY_DOMAIN_HOME_SPE,
    /** @brief 室内机 WiFi 访问公区 IPC 的业务侧 domain。 */
    TOPOLOGY_DOMAIN_HOME_WIFI,
} topology_domain_kind_t;

/**
 * @brief 节点在 domain 内的角色。
 */
typedef enum
{
    /** @brief 无特殊角色。 */
    TOPOLOGY_ROLE_NONE = 0,
    /** @brief PLC 主机 / coordinator。 */
    TOPOLOGY_ROLE_PLC_COORDINATOR,
    /** @brief PLC 从设备。 */
    TOPOLOGY_ROLE_PLC_STATION,
    /** @brief 二线协议目录上游节点。 */
    TOPOLOGY_ROLE_DIRECTORY_UPSTREAM,
    /** @brief 控制面网关。 */
    TOPOLOGY_ROLE_CONTROL_GATEWAY,
    /** @brief 媒体面网关。 */
    TOPOLOGY_ROLE_MEDIA_GATEWAY,
} topology_role_kind_t;

/**
 * @brief 路由查询意图。
 */
typedef enum
{
    /** @brief 呼叫控制面路由。 */
    TOPOLOGY_INTENT_CALL_CONTROL = 0,
    /** @brief 监视控制面路由。 */
    TOPOLOGY_INTENT_MONITOR_CONTROL,
    /** @brief 发现或诊断路由。 */
    TOPOLOGY_INTENT_DISCOVERY_DIAGNOSTIC,
} topology_route_intent_t;

/**
 * @brief 音频会话中本机角色。
 */
typedef enum
{
    /** @brief 本机是呼叫发起方。 */
    TOPOLOGY_AUDIO_SESSION_CALLER = 0,
    /** @brief 本机是呼叫接收方。 */
    TOPOLOGY_AUDIO_SESSION_CALLEE,
    /** @brief 本机是中继 / 网关节点。 */
    TOPOLOGY_AUDIO_SESSION_RELAY,
    /** @brief 本机是监视媒体源。 */
    TOPOLOGY_AUDIO_SESSION_MONITOR_SOURCE,
    /** @brief 本机是监视查看方。 */
    TOPOLOGY_AUDIO_SESSION_MONITOR_VIEWER,
} topology_audio_session_role_t;

/**
 * @brief 拓扑节点。
 */
typedef struct
{
    /** @brief 设备 `device_id`，不可为空。 */
    char device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** @brief 设备类型；填 UNKNOWN 时 upsert 会根据 `device_id` 推导。 */
    topology_device_type_t device_type;
    /** @brief profile/tag，当前仅用于展示、导入和诊断。 */
    char topology_profile[TOPOLOGY_PROFILE_MAX];
    /** @brief 绑定状态，取值由上层定义。 */
    uint32_t binding_state;
    /** @brief 在线状态，取值由上层定义。 */
    uint32_t presence_state;
} topology_node_t;

/**
 * @brief 拓扑 domain。
 */
typedef struct
{
    /** @brief domain 标识，必须在同一上下文内唯一。 */
    char domain_id[TOPOLOGY_DOMAIN_ID_MAX];
    /** @brief domain 类型。 */
    topology_domain_kind_t domain_kind;
    /** @brief domain 归属设备，可为空。 */
    char owner_device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** @brief profile/tag，当前仅用于展示、导入和诊断。 */
    char topology_profile[TOPOLOGY_PROFILE_MAX];
} topology_domain_t;

/**
 * @brief 节点 role。
 */
typedef struct
{
    /** @brief 设备 `device_id`，必须已存在于上下文。 */
    char device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** @brief domain 标识，必须已存在于上下文。 */
    char domain_id[TOPOLOGY_DOMAIN_ID_MAX];
    /** @brief 节点在该 domain 内的角色。 */
    topology_role_kind_t role_kind;
} topology_role_t;

/**
 * @brief 拓扑链路。
 */
typedef struct
{
    /** @brief 源设备 `device_id`，必须已存在于上下文。 */
    char src_device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** @brief 目标设备 `device_id`，必须已存在于上下文。 */
    char dst_device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** @brief 链路所属 domain，必须已存在于上下文。 */
    char domain_id[TOPOLOGY_DOMAIN_ID_MAX];
    /** @brief 链路类型。 */
    topology_link_kind_t link_kind;
    /** @brief 链路能力位，使用 `TOPOLOGY_CAP_*`。 */
    uint32_t capabilities;
} topology_link_t;

/**
 * @brief 链路内 endpoint。
 */
typedef struct
{
    /** @brief endpoint 所属设备 `device_id`，必须已存在于上下文。 */
    char device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** @brief endpoint 所属 domain，必须已存在于上下文。 */
    char domain_id[TOPOLOGY_DOMAIN_ID_MAX];
    /** @brief endpoint 所属链路类型。 */
    topology_link_kind_t link_kind;
    /** @brief 链路内地址；PLC 地址需要按完整 32-bit 保存。 */
    uint32_t addr;
    /** @brief 链路内端口；无端口语义时填 `0`。 */
    uint16_t port;
    /** @brief 预留字段，当前填 `0`。 */
    uint16_t reserved;
} topology_endpoint_t;

/**
 * @brief 设备解析结果。
 */
typedef struct
{
    /** @brief 已导入的节点记录。 */
    topology_node_t node;
    /** @brief 由 `device_id` 解析出的身份字段。 */
    topology_ip_info_t ip_info;
} topology_device_info_t;

/**
 * @brief 下一跳查询结果。
 */
typedef struct
{
    /** @brief 下一跳设备 `device_id`。 */
    char next_hop_device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** @brief 下一跳 endpoint。 */
    topology_endpoint_t next_hop_endpoint;
    /** @brief 命中的第一跳 domain。 */
    char domain_id[TOPOLOGY_DOMAIN_ID_MAX];
    /** @brief 命中的第一跳链路类型。 */
    topology_link_kind_t link_kind;
    /** @brief 第一跳之后是否仍有后续跳点。 */
    bool bridge_required;
    /** @brief 从本机到目标的 link 跳数。 */
    uint8_t hop_count;
    /** @brief 简短路径解释，便于日志输出。 */
    char explain[TOPOLOGY_EXPLAIN_MAX];
} topology_next_hop_result_t;

/**
 * @brief 音频 endpoint 查询参数。
 */
typedef struct
{
    /** @brief 本机在当前音频会话中的角色。 */
    topology_audio_session_role_t session_role;
} topology_audio_query_t;

/**
 * @brief 拓扑图只读摘要。
 */
typedef struct
{
    /** @brief 当前节点数量。 */
    size_t node_count;
    /** @brief 当前 domain 数量。 */
    size_t domain_count;
    /** @brief 当前 role 数量。 */
    size_t role_count;
    /** @brief 当前 link 数量。 */
    size_t link_count;
    /** @brief 当前 endpoint 数量。 */
    size_t endpoint_count;
} topology_summary_t;

/**
 * @brief 音频 endpoint 查询结果。
 */
typedef struct
{
    /** @brief 本机音频 RTP 发送目标 endpoint。 */
    topology_endpoint_t tx_endpoint;
    /** @brief 本机接收音频 RTP 时的匹配源；第一版可使用 ADDR_ANY 兼容模式。 */
    topology_endpoint_t rx_match_endpoint;
    /** @brief 当前路径命中的网关设备；直达时为空字符串。 */
    char gateway_device_id[TOPOLOGY_DEVICE_ID_MAX];
    /** @brief 当前本机是否需要依赖后续网关桥接。 */
    bool bridge_required;
    /** @brief 网关在下一段链路上的桥接 endpoint；直达时为全零。 */
    topology_endpoint_t bridge_peer_endpoint;
    /** @brief 从本机到 peer 的音频链路跳数。 */
    uint8_t hop_count;
    /** @brief 简短路径解释，便于日志输出。 */
    char explain[TOPOLOGY_EXPLAIN_MAX];
} topology_audio_endpoint_result_t;

/**
 * @brief 创建拓扑图上下文。
 *
 * @param[in] config 配置参数；可为 `NULL` 表示使用默认配置。当前仅支持默认固定容量。
 * @return 成功返回上下文指针；内存不足返回 `NULL`。
 *
 * @see topology_destroy
 */
topology_ctx_t *topology_create(const topology_config_t *config);

/**
 * @brief 销毁拓扑图上下文。
 *
 * @param[in] ctx 由 topology_create() 返回的上下文；可为 `NULL`。
 */
void topology_destroy(topology_ctx_t *ctx);

/**
 * @brief 清空拓扑图上下文中的所有节点、domain、role、link 和 endpoint。
 *
 * @param[in,out] ctx 拓扑上下文，不可为 `NULL`。
 */
void topology_clear(topology_ctx_t *ctx);

/**
 * @brief 读取拓扑图摘要。
 *
 * @param[in] ctx 拓扑上下文，不可为 `NULL`。
 * @param[out] out 摘要输出，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 读取成功。
 * @retval -EINVAL 参数非法。
 */
int topology_get_summary(const topology_ctx_t *ctx, topology_summary_t *out);

/**
 * @brief 按索引读取节点副本。
 *
 * @param[in] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] index 节点索引。
 * @param[out] out 节点输出，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 读取成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 索引不存在。
 */
int topology_get_node_at(const topology_ctx_t *ctx, size_t index, topology_node_t *out);

/**
 * @brief 按索引读取 domain 副本。
 *
 * @param[in] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] index domain 索引。
 * @param[out] out domain 输出，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 读取成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 索引不存在。
 */
int topology_get_domain_at(const topology_ctx_t *ctx, size_t index, topology_domain_t *out);

/**
 * @brief 按索引读取 role 副本。
 *
 * @param[in] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] index role 索引。
 * @param[out] out role 输出，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 读取成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 索引不存在。
 */
int topology_get_role_at(const topology_ctx_t *ctx, size_t index, topology_role_t *out);

/**
 * @brief 按索引读取 link 副本。
 *
 * @param[in] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] index link 索引。
 * @param[out] out link 输出，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 读取成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 索引不存在。
 */
int topology_get_link_at(const topology_ctx_t *ctx, size_t index, topology_link_t *out);

/**
 * @brief 按索引读取 endpoint 副本。
 *
 * @param[in] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] index endpoint 索引。
 * @param[out] out endpoint 输出，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 读取成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 索引不存在。
 */
int topology_get_endpoint_at(const topology_ctx_t *ctx,
                             size_t index,
                             topology_endpoint_t *out);

/**
 * @brief 新增或更新节点。
 *
 * `node->device_id` 必须是 topology_ip_parse_device_id() 可解析的完整 `device_id`。
 * 如果 `node->device_type` 为 UNKNOWN，函数会自动按 `device_id` 推导设备类型。
 *
 * @param[in,out] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] node 节点信息，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 新增或更新成功。
 * @retval -EINVAL 参数非法或 `device_id` 与设备类型不匹配。
 * @retval -ENOSPC 内部容量不足。
 */
int topology_upsert_node(topology_ctx_t *ctx, const topology_node_t *node);

/**
 * @brief 新增或更新 domain。
 *
 * @param[in,out] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] domain domain 信息，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 新增或更新成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOSPC 内部容量不足。
 */
int topology_upsert_domain(topology_ctx_t *ctx, const topology_domain_t *domain);

/**
 * @brief 新增或更新节点 role。
 *
 * @param[in,out] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] role role 信息，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 新增或更新成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 节点或 domain 尚未导入。
 * @retval -ENOSPC 内部容量不足。
 */
int topology_upsert_role(topology_ctx_t *ctx, const topology_role_t *role);

/**
 * @brief 新增或更新 link。
 *
 * 第一版将 link 视为可双向寻路的图边。
 *
 * @param[in,out] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] link link 信息，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 新增或更新成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 节点或 domain 尚未导入。
 * @retval -ENOSPC 内部容量不足。
 */
int topology_upsert_link(topology_ctx_t *ctx, const topology_link_t *link);

/**
 * @brief 新增或更新 endpoint。
 *
 * @param[in,out] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] endpoint endpoint 信息，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 新增或更新成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 节点或 domain 尚未导入。
 * @retval -ENOSPC 内部容量不足。
 */
int topology_upsert_endpoint(topology_ctx_t *ctx, const topology_endpoint_t *endpoint);

/**
 * @brief 移除节点以及与该节点相关的 role、link 和 endpoint。
 *
 * @param[in,out] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] device_id 待移除节点的 `device_id`，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 移除成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 节点不存在。
 */
int topology_remove_node(topology_ctx_t *ctx, const char *device_id);

/**
 * @brief 移除 domain 以及与该 domain 相关的 role、link 和 endpoint。
 *
 * @param[in,out] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] domain_id 待移除 domain 标识，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 移除成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT domain 不存在。
 */
int topology_remove_domain(topology_ctx_t *ctx, const char *domain_id);

/**
 * @brief 解析已导入设备。
 *
 * @param[in] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] device_id 待解析设备 `device_id`，不可为 `NULL`。
 * @param[out] out 解析结果，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 解析成功。
 * @retval -EINVAL 参数非法或 `device_id` 格式非法。
 * @retval -ENOENT 设备尚未导入。
 */
int topology_resolve_device(topology_ctx_t *ctx,
                            const char *device_id,
                            topology_device_info_t *out);

/**
 * @brief 查询从本机到目标设备的控制面下一跳。
 *
 * 第一版基于已导入 link 做最短跳数查询，并返回第一跳对端在该 link domain 内的
 * endpoint。查询不会修改拓扑上下文。
 *
 * @param[in] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] local_device_id 本机 `device_id`，不可为 `NULL`，且必须已导入。
 * @param[in] target_device_id 目标 `device_id`，不可为 `NULL`，且必须已导入。
 * @param[in] intent 路由意图。
 * @param[out] out 下一跳结果，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 查询成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 节点或下一跳 endpoint 不存在。
 * @retval -EHOSTUNREACH 当前图中没有可用路径。
 */
int topology_query_next_hop(topology_ctx_t *ctx,
                            const char *local_device_id,
                            const char *target_device_id,
                            topology_route_intent_t intent,
                            topology_next_hop_result_t *out);

/**
 * @brief 查询本机与 peer 之间的音频 RTP endpoint。
 *
 * 第一版基于带 `TOPOLOGY_CAP_AUDIO` 能力的 link 做最短跳数查询。直达路径返回 peer 在
 * 共享 domain 内的 endpoint；多跳路径返回第一跳网关 endpoint，并在结果中标记
 * `bridge_required`。
 *
 * @param[in] ctx 拓扑上下文，不可为 `NULL`。
 * @param[in] local_device_id 本机 `device_id`，不可为 `NULL`，且必须已导入。
 * @param[in] peer_device_id 对端 `device_id`，不可为 `NULL`，且必须已导入。
 * @param[in] query 查询参数，不可为 `NULL`。
 * @param[out] out 查询结果，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负数错误码。
 * @retval 0 查询成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOENT 节点或所需 endpoint 不存在。
 * @retval -EHOSTUNREACH 当前图中没有可用音频路径。
 */
int topology_query_audio_endpoint(topology_ctx_t *ctx,
                                  const char *local_device_id,
                                  const char *peer_device_id,
                                  const topology_audio_query_t *query,
                                  topology_audio_endpoint_result_t *out);

#ifdef __cplusplus
}
#endif

#endif /* __TOPOLOGY_H__ */
