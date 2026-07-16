/**
 * @file
 * @brief simple call control route runtime.
 *
 * 本文件属于 examples 应用层，用于汇总 bootstrap、directory、registry 等来源的
 * callee 到下一跳 route facts。
 */

#ifndef SIMPLE_CONTROL_ROUTE_RUNTIME_H
#define SIMPLE_CONTROL_ROUTE_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "call_flow_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief simple call route 表最大条目数。 */
#define SIMPLE_CALL_MAX_ROUTES (16U)
/** @brief simple control route runtime 最大 fact 数。 */
#define SIMPLE_CONTROL_ROUTE_RUNTIME_MAX_FACTS (32U)

/**
 * @brief callee device_id 到下一跳的呼叫路由条目。
 */
typedef struct
{
    /** 被叫 device_id，NUL 结尾。 */
    char callee_device_id[CALL_DEVICE_ID_MAX];
    /** 下一跳 route endpoint。 */
    call_endpoint_t next_hop;
} simple_route_entry_t;

/**
 * @brief route fact 来源。
 */
typedef enum
{
    /** 未指定来源。 */
    SIMPLE_CONTROL_ROUTE_SOURCE_NONE = 0,
    /** 启动静态 bootstrap route。 */
    SIMPLE_CONTROL_ROUTE_SOURCE_STATIC_BOOTSTRAP = 1,
    /** directory/downlink 运行态 route。 */
    SIMPLE_CONTROL_ROUTE_SOURCE_DIRECTORY = 2,
    /** direct PLC registry route。 */
    SIMPLE_CONTROL_ROUTE_SOURCE_REGISTRY = 3,
    /** generic topology 推导 route。 */
    SIMPLE_CONTROL_ROUTE_SOURCE_TOPOLOGY = 4,
    /** 入站观测到的 peer route。 */
    SIMPLE_CONTROL_ROUTE_SOURCE_OBSERVED_PEER = 5,
    /** 兼容手动 update/remove route。 */
    SIMPLE_CONTROL_ROUTE_SOURCE_MANUAL = 6,
    /** backend/manual 配置 route。 */
    SIMPLE_CONTROL_ROUTE_SOURCE_BACKEND = 7
} simple_control_route_source_t;

/**
 * @brief route fact 写入请求。
 */
typedef struct
{
    /** route fact 来源。 */
    simple_control_route_source_t source;
    /** 被叫 device_id。 */
    const char *callee_device_id;
    /** true 表示 upsert；false 表示删除同来源同 callee fact。 */
    bool online;
    /** 下一跳 endpoint，online=true 时使用。 */
    call_endpoint_t next_hop;
    /** 观测时间，单位毫秒；0 表示未设置。 */
    uint64_t observed_ms;
    /** 有效期，单位毫秒；0 表示不过期。 */
    uint32_t ttl_ms;
} simple_control_route_fact_t;

/**
 * @brief control route runtime 配置。
 */
typedef struct
{
    /** true 表示 route miss 时兼容回退到第一条 active route。 */
    bool legacy_fallback_enabled;
} simple_control_route_runtime_config_t;

typedef struct
{
    /** route fact 来源。 */
    simple_control_route_source_t source;
    /** 被叫 device_id，NUL 结尾。 */
    char callee_device_id[CALL_DEVICE_ID_MAX];
    /** 当前选中的下一跳 endpoint。 */
    call_endpoint_t next_hop;
    /** 最近一次观测或写入时间，单位毫秒；0 表示未设置。 */
    uint64_t observed_ms;
    /** route fact 有效期，单位毫秒；0 表示不过期。 */
    uint32_t ttl_ms;
    /** runtime 内部递增序号，用于同优先级来源的稳定选择。 */
    uint32_t seq;
    /** true 表示该 snapshot 条目在线有效。 */
    bool online;
} simple_control_route_runtime_entry_t;

/**
 * @brief control route runtime。
 */
typedef struct
{
    simple_control_route_runtime_config_t cfg;
    simple_control_route_runtime_entry_t
        entries[SIMPLE_CONTROL_ROUTE_RUNTIME_MAX_FACTS];
    uint32_t next_seq;
} simple_control_route_runtime_t;

/**
 * @brief 初始化 route runtime。
 *
 * @param runtime route runtime 对象。
 * @param cfg 可选配置；为 NULL 时使用默认配置。
 */
void simple_control_route_runtime_init(simple_control_route_runtime_t *runtime,
                                       const simple_control_route_runtime_config_t *cfg);
/**
 * @brief 清空 route runtime 中的所有 route facts。
 *
 * @param runtime route runtime 对象。
 */
void simple_control_route_runtime_clear(simple_control_route_runtime_t *runtime);
/**
 * @brief 获取 route source 的诊断名称。
 *
 * @param source route fact 来源。
 * @return 常量字符串名称；未知来源返回 "unknown"。
 */
const char *simple_control_route_source_name(simple_control_route_source_t source);
/**
 * @brief 导入静态 bootstrap route 表。
 *
 * @param runtime route runtime 对象。
 * @param routes 静态 route 表。
 * @param route_count route 表条目数。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOSPC route fact 表容量不足。
 */
int simple_control_route_runtime_load_bootstrap(
    simple_control_route_runtime_t *runtime,
    const simple_route_entry_t *routes,
    size_t route_count);
/**
 * @brief 写入或删除一条 route fact。
 *
 * online fact 会按 `(source, callee_device_id)` upsert；offline fact 会删除同来源同
 * callee 的 fact。
 *
 * @param runtime route runtime 对象。
 * @param fact route fact 请求。
 * @retval 0 成功。
 * @retval -EINVAL 参数非法。
 * @retval -ENOSPC 容量不足或字符串截断。
 */
int simple_control_route_runtime_apply_fact(
    simple_control_route_runtime_t *runtime,
    const simple_control_route_fact_t *fact);
/**
 * @brief 写入 online route fact。
 *
 * @param runtime route runtime 对象。
 * @param source route fact 来源。
 * @param callee_device_id 被叫 device_id。
 * @param next_hop 下一跳 endpoint。
 * @param observed_ms 观测时间，单位毫秒；0 表示未设置。
 * @param ttl_ms 有效期，单位毫秒；0 表示不过期。
 * @return 0 表示成功，负数表示失败。
 */
int simple_control_route_runtime_apply_online(
    simple_control_route_runtime_t *runtime,
    simple_control_route_source_t source,
    const char *callee_device_id,
    const call_endpoint_t *next_hop,
    uint64_t observed_ms,
    uint32_t ttl_ms);
/**
 * @brief 删除同来源同 callee 的 route fact。
 *
 * @param runtime route runtime 对象。
 * @param source route fact 来源。
 * @param callee_device_id 被叫 device_id。
 * @return 0 表示成功，负数表示失败。
 */
int simple_control_route_runtime_apply_offline(
    simple_control_route_runtime_t *runtime,
    simple_control_route_source_t source,
    const char *callee_device_id);
/**
 * @brief 过期 TTL route fact。
 *
 * 只删除 `ttl_ms != 0 && observed_ms != 0` 且已经超时的 fact；静态 bootstrap
 * 默认不过期。
 *
 * @param runtime route runtime 对象。
 * @param now_ms 当前时间，单位毫秒。
 * @return 被删除的 route fact 数量。
 */
size_t simple_control_route_runtime_expire(simple_control_route_runtime_t *runtime,
                                           uint64_t now_ms);
/**
 * @brief 获取当前 active route fact 数量。
 *
 * @param runtime route runtime 对象。
 * @return active route fact 数量。
 */
size_t simple_control_route_runtime_count(const simple_control_route_runtime_t *runtime);
/**
 * @brief 拷贝 active route fact snapshot。
 *
 * @param runtime route runtime 对象。
 * @param entries 输出数组；为 NULL 时只返回可拷贝数量。
 * @param entry_capacity 输出数组容量。
 * @return 实际拷贝的 active route fact 数量。
 */
size_t simple_control_route_runtime_snapshot(
    const simple_control_route_runtime_t *runtime,
    simple_control_route_runtime_entry_t *entries,
    size_t entry_capacity);
/**
 * @brief 查询 callee 的下一跳 route。
 *
 * @param runtime route runtime 对象。
 * @param callee_device_id 被叫 device_id。
 * @param next_hop 输出下一跳 endpoint。
 * @retval true 查询成功。
 * @retval false route miss。
 */
bool simple_control_route_runtime_lookup(simple_control_route_runtime_t *runtime,
                                         const char *callee_device_id,
                                         call_endpoint_t *next_hop);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_CONTROL_ROUTE_RUNTIME_H */
