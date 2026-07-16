/**
 * @file
 * @brief simple 示例应用公共模块 simple_topology_profile 的公共头文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_TOPOLOGY_PROFILE_H__
#define __SIMPLE_TOPOLOGY_PROFILE_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "topology/topology.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief topology profile 诊断日志回调。
 *
 * @param[in] user_ctx 用户上下文，调用方保持所有权。
 * @param[in] line 已格式化日志字符串，生命周期仅限本次回调。
 */
typedef void (*simple_topology_profile_log_cb)(void *user_ctx, const char *line);

/**
 * @brief 通用 topology profile facts。
 *
 * 调用方直接提供 topology core 的 node/domain/role/link/endpoint facts。
 */
typedef struct
{
	/** 节点 facts。 */
	const topology_node_t *nodes;
	/** 节点数量。 */
	size_t node_count;
	/** domain facts，可为 NULL。 */
	const topology_domain_t *domains;
	/** domain 数量。 */
	size_t domain_count;
	/** role facts，可为 NULL。 */
	const topology_role_t *roles;
	/** role 数量。 */
	size_t role_count;
	/** link facts，可为 NULL。 */
	const topology_link_t *links;
	/** link 数量。 */
	size_t link_count;
	/** endpoint facts，可为 NULL。 */
	const topology_endpoint_t *endpoints;
	/** endpoint 数量。 */
	size_t endpoint_count;
} simple_topology_profile_generic_t;

/**
 * @brief 通用 PLC CCO profile 中单个 PLC STA。
 */
typedef struct
{
	/** STA device_id。 */
	const char *device_id;
	/** STA PLC 地址。 */
	uint32_t plc_addr;
} simple_topology_profile_plc_sta_t;

/**
 * @brief 通用 PLC CCO/domain 构建配置。
 */
typedef struct
{
	/** profile/tag，可为 NULL。 */
	const char *profile_name;
	/** PLC CCO（Central Coordinator）device_id，可为 converter 或 X9。 */
	const char *cco_device_id;
	/** PLC domain id。 */
	const char *domain_id;
	/** PLC domain 类型，例如 CONVERTER_PLC 或 LOWRISE_PLC。 */
	topology_domain_kind_t domain_kind;
	/** PLC CCO 地址。 */
	uint32_t cco_plc_addr;
	/** PLC 媒体端口。 */
	uint16_t plc_port;
	/** link 能力；0 表示默认 CONTROL|AUDIO。 */
	uint32_t capabilities;
	/** STA 数组。 */
	const simple_topology_profile_plc_sta_t *stas;
	/** STA 数量。 */
	size_t sta_count;
} simple_topology_profile_plc_cco_t;

/**
 * @brief converter 双域 topology 构建配置。
 *
 * 用于表达 X9/unit door 到 F1 converter 的下行 2-wire domain，以及 F1 converter
 * 作为 PLC CCO 到室内机 STA 的 PLC domain。
 */
typedef struct
{
	/** profile/tag，可为 NULL。 */
	const char *profile_name;
	/** X9/unit door device_id。 */
	const char *unit_device_id;
	/** F1 converter device_id。 */
	const char *converter_device_id;
	/** 下行 2-wire domain id；为 NULL 时使用默认 `unit-downlink`。 */
	const char *downlink_domain_id;
	/** converter PLC domain id；为 NULL 时使用默认 `converter-plc`。 */
	const char *plc_domain_id;
	/** X9/unit door 下行 2-wire 地址。 */
	uint32_t unit_downlink_addr;
	/** converter 下行 2-wire 地址。 */
	uint32_t converter_downlink_addr;
	/** 下行 2-wire 媒体端口。 */
	uint16_t downlink_port;
	/** converter PLC CCO 地址。 */
	uint32_t converter_plc_addr;
	/** PLC 媒体端口。 */
	uint16_t plc_port;
	/** link 能力；0 表示默认 CONTROL|AUDIO。 */
	uint32_t capabilities;
	/** PLC STA 数组。 */
	const simple_topology_profile_plc_sta_t *stas;
	/** PLC STA 数量。 */
	size_t sta_count;
} simple_topology_profile_converter_dual_domain_t;

/**
 * @brief 根据 topology graph 准备音频 endpoint 的配置。
 */
typedef struct
{
	const char *node_name;
	topology_ctx_t *topology;
	const char *local_device_id;
	const char *caller_device_id;
	const char *callee_device_id;
	const char *peer_device_id;
	uint64_t session_id;
	uint8_t msg_type;
	topology_link_kind_t expected_tx_link_kind;
	bool validate_tx_endpoint;
	uint32_t expected_tx_addr;
	uint16_t expected_tx_port;
	bool validate_rx_port;
	uint16_t expected_rx_port;
	simple_topology_profile_log_cb log_cb;
	void *log_user_ctx;
	topology_audio_endpoint_result_t *audio_out;
} simple_topology_profile_prepare_audio_t;


/**
 * @brief 由通用 topology facts 创建 topology graph。
 *
 * @param[in] profile 通用 profile facts，不能为 NULL。
 * @return topology 上下文；失败返回 NULL。成功后调用方负责 topology_destroy()。
 */
topology_ctx_t *simple_topology_profile_create_generic(
	const simple_topology_profile_generic_t *profile);

/**
 * @brief 向 topology graph 添加一个 PLC CCO/domain 及其 STA。
 *
 * @param[in,out] topology topology 上下文，不能为 NULL。
 * @param[in] cfg PLC CCO 构建配置，不能为 NULL。
 * @retval 0 添加成功。
 * @retval <0 添加失败。
 */
int simple_topology_profile_add_plc_cco(
	topology_ctx_t *topology,
	const simple_topology_profile_plc_cco_t *cfg);

/**
 * @brief 向 topology graph 添加 converter 双域 facts。
 *
 * @param[in,out] topology topology 上下文，不能为 NULL。
 * @param[in] cfg converter 双域构建配置，不能为 NULL。
 * @retval 0 添加成功。
 * @retval <0 添加失败。
 */
int simple_topology_profile_add_converter_dual_domain(
	topology_ctx_t *topology,
	const simple_topology_profile_converter_dual_domain_t *cfg);

/**
 * @brief 根据对端 endpoint 反查本端同 domain/link 的 endpoint。
 *
 * @param[in] topology topology graph，不能为 NULL。
 * @param[in] local_device_id 本端 device_id，不能为 NULL。
 * @param[in] peer 对端 endpoint，不能为 NULL。
 * @param[out] local_out 本端 endpoint 输出，不能为 NULL。
 * @retval 0 查询成功。
 * @retval <0 查询失败。
 */
int simple_topology_profile_find_local_endpoint(topology_ctx_t *topology,
                                                const char *local_device_id,
                                                const topology_endpoint_t *peer,
                                                topology_endpoint_t *local_out);


/**
 * @brief 查询并校验当前音频 endpoint。
 *
 * @param[in] cfg endpoint 准备配置，不能为 NULL。
 * @retval 0 查询与校验成功。
 * @retval <0 查询或校验失败。
 */
int simple_topology_profile_prepare_audio(const simple_topology_profile_prepare_audio_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* __SIMPLE_TOPOLOGY_PROFILE_H__ */
