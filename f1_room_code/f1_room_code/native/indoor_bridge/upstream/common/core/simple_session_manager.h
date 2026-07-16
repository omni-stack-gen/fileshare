/**
 * @file
 * @brief simple 层多会话管理器。
 *
 * 该模块属于 examples 应用层重构骨架，当前负责控制态记录和第一轮 busy/preempt 决策。
 */

#ifndef __SIMPLE_SESSION_MANAGER_H__
#define __SIMPLE_SESSION_MANAGER_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "call_proto.h"
#include "simple_session.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief session manager 当前最多保存的活动 session 数量。 */
#define SIMPLE_SESSION_MANAGER_MAX_SESSIONS 8
/** @brief release history 中最多保存的最近释放记录数量。 */
#define SIMPLE_SESSION_MANAGER_RELEASE_HISTORY_MAX 8
/** @brief session manager reason 字符串缓冲区长度，单位为字节。 */
#define SIMPLE_SESSION_MANAGER_REASON_MAX  64

/**
 * @brief session manager 记录类型。
 */
typedef enum
{
    /** 无效/空记录。 */
    SIMPLE_SESSION_RECORD_KIND_NONE = 0,
    /** CALL 会话记录。 */
    SIMPLE_SESSION_RECORD_KIND_CALL,
    /** MONITOR 会话记录。 */
    SIMPLE_SESSION_RECORD_KIND_MONITOR,
} simple_session_record_kind_t;

/**
 * @brief session manager 记录状态。
 */
typedef enum
{
    /** 空 slot。 */
    SIMPLE_SESSION_RECORD_STATE_EMPTY = 0,
    /** 已创建或 pending。 */
    SIMPLE_SESSION_RECORD_STATE_CREATED,
    /** 呼叫振铃中。 */
    SIMPLE_SESSION_RECORD_STATE_RINGING,
    /** 已接受但未必已进入媒体 active。 */
    SIMPLE_SESSION_RECORD_STATE_ACCEPTED,
    /** 会话处于 active。 */
    SIMPLE_SESSION_RECORD_STATE_ACTIVE,
    /** 已释放，通常只出现在 release history。 */
    SIMPLE_SESSION_RECORD_STATE_RELEASED,
} simple_session_record_state_t;

/**
 * @brief admission 检查请求。
 */
typedef struct
{
    /** 待进入的会话类型。 */
    simple_session_record_kind_t kind;
    /** 待进入的 session id。 */
    uint64_t session_id;
} simple_session_manager_admission_request_t;

/**
 * @brief admission 决策结果。
 */
typedef struct
{
    /** true 表示允许进入。 */
    bool admitted;
    /** true 表示需要抢占；当前第一轮策略通常不触发真实抢占。 */
    bool preempt;
    /** 决策返回值，0 表示允许，负值表示拒绝原因。 */
    int ret;
    /** 映射到 CALL/MONITOR result_code 的结果码。 */
    uint8_t result_code;
    /** 冲突 session id；无冲突时为 0。 */
    uint64_t conflict_session_id;
    /** 冲突记录类型。 */
    simple_session_record_kind_t conflict_kind;
    /** 冲突记录状态。 */
    simple_session_record_state_t conflict_state;
    /** 决策原因字符串。 */
    char reason[SIMPLE_SESSION_MANAGER_REASON_MAX];
} simple_session_manager_admission_decision_t;

/**
 * @brief session manager 保存的一条控制态记录。
 */
typedef struct
{
    /** session id。 */
    uint64_t session_id;
    /** 创建顺序号，用于 latest/current 查询。 */
    uint64_t created_seq;
    /** 最近关联的事务 id。 */
    uint32_t txn_id;
    /** 对端 numeric peer id。 */
    uint32_t peer_id;
    /** 记录类型。 */
    simple_session_record_kind_t kind;
    /** 控制态状态。 */
    simple_session_record_state_t state;
    /** caller device_id。 */
    char caller_device_id[CALL_DEVICE_ID_MAX];
    /** callee device_id。 */
    char callee_device_id[CALL_DEVICE_ID_MAX];
    /** 逻辑媒体源 device_id。 */
    char logical_source_id[CALL_DEVICE_ID_MAX];
    /** 拓扑 domain id。 */
    char domain_id[CALL_DEVICE_ID_MAX];
    /** directory gateway id。 */
    char gateway_id[CALL_DEVICE_ID_MAX];
    /** 释放原因。 */
    char release_reason[SIMPLE_SESSION_MANAGER_REASON_MAX];
} simple_session_record_t;

/**
 * @brief session manager 内部 slot。
 */
typedef struct
{
    /** true 表示 slot 正在使用。 */
    bool in_use;
    /** slot 中保存的记录。 */
    simple_session_record_t record;
} simple_session_manager_slot_t;

/**
 * @brief simple 层控制态 manager。
 */
typedef struct
{
    /** 当前活动 session slot。 */
    simple_session_manager_slot_t slots[SIMPLE_SESSION_MANAGER_MAX_SESSIONS];
    /** 最近释放记录环形缓冲区。 */
    simple_session_record_t release_history[SIMPLE_SESSION_MANAGER_RELEASE_HISTORY_MAX];
    /** 下一条记录的创建顺序号。 */
    uint64_t next_created_seq;
    /** release history 下一次写入位置。 */
    size_t release_history_next;
    /** 当前 release history 有效记录数量。 */
    size_t release_history_count;
} simple_session_manager_t;

/**
 * @brief session manager dump 回调。
 *
 * @param[in] user_ctx 用户上下文，可为 NULL。
 * @param[in] record 当前遍历到的记录，不能为 NULL。
 */
typedef void (*simple_session_manager_dump_cb)(void *user_ctx,
                                               const simple_session_record_t *record);

/**
 * @brief 初始化 session manager。
 *
 * @param[out] manager 待初始化的 manager，不能为 NULL。
 */
void simple_session_manager_init(simple_session_manager_t *manager);

/**
 * @brief 清空 manager 中的活动记录与 release history。
 *
 * @param[in,out] manager 已初始化的 manager，不能为 NULL。
 */
void simple_session_manager_clear(simple_session_manager_t *manager);

/**
 * @brief 新增或更新指定 session 的控制态记录。
 *
 * @param[in,out] manager manager 实例，不能为 NULL。
 * @param[in] kind 记录类型，CALL 或 MONITOR。
 * @param[in] session_id session id。
 * @param[in] txn_id 最近关联的事务 id。
 * @param[in] peer_id 对端 numeric peer id。
 * @param[in] caller_device_id caller device_id，可为 NULL。
 * @param[in] callee_device_id callee device_id，可为 NULL。
 * @param[in] logical_source_id 逻辑媒体源 device_id，可为 NULL。
 * @param[in] domain_id 拓扑 domain id，可为 NULL。
 * @param[in] gateway_id directory gateway id，可为 NULL。
 * @param[in] state 要写入的控制态。
 * @param[out] created_out 可选输出；为 true 表示本次创建了新 slot。
 *
 * @retval 0 写入成功。
 * @retval -EINVAL 参数非法或 session_id 为 0。
 * @retval -ENOSPC manager slot 已满。
 */
int simple_session_manager_upsert(simple_session_manager_t *manager,
                                  simple_session_record_kind_t kind,
                                  uint64_t session_id,
                                  uint32_t txn_id,
                                  uint32_t peer_id,
                                  const char *caller_device_id,
                                  const char *callee_device_id,
                                  const char *logical_source_id,
                                  const char *domain_id,
                                  const char *gateway_id,
                                  simple_session_record_state_t state,
                                  bool *created_out);

/**
 * @brief 只执行 admission 检查，不写入 session 记录。
 *
 * @param[in] manager manager 实例，不能为 NULL。
 * @param[in] request admission 请求，不能为 NULL。
 * @param[out] decision 决策输出，不能为 NULL。
 *
 * @retval 0 允许进入。
 * @retval -EBUSY 因已有 CALL 或容量策略被拒绝。
 * @retval -EINVAL 参数非法。
 */
int simple_session_manager_check_admission(
    const simple_session_manager_t *manager,
    const simple_session_manager_admission_request_t *request,
    simple_session_manager_admission_decision_t *decision);

/**
 * @brief 执行 admission 检查并在通过时 reserve session 记录。
 *
 * CALL 与 CALL 互斥，MONITOR 可并存；具体策略由当前 manager 实现决定。
 *
 * @param[in,out] manager manager 实例，不能为 NULL。
 * @param[in] kind 记录类型。
 * @param[in] session_id session id。
 * @param[in] txn_id 事务 id。
 * @param[in] peer_id 对端 numeric peer id。
 * @param[in] caller_device_id caller device_id，可为 NULL。
 * @param[in] callee_device_id callee device_id，可为 NULL。
 * @param[in] logical_source_id 逻辑媒体源 device_id，可为 NULL。
 * @param[in] domain_id 拓扑 domain id，可为 NULL。
 * @param[in] gateway_id directory gateway id，可为 NULL。
 * @param[in] state reserve 后的初始状态。
 * @param[out] decision admission 决策输出，不能为 NULL。
 * @param[out] created_out 可选输出；为 true 表示创建了新记录。
 *
 * @retval 0 reserve 成功。
 * @retval -EBUSY admission 拒绝。
 * @retval -EINVAL 参数非法。
 * @retval -ENOSPC manager slot 已满。
 */
int simple_session_manager_reserve(simple_session_manager_t *manager,
                                   simple_session_record_kind_t kind,
                                   uint64_t session_id,
                                   uint32_t txn_id,
                                   uint32_t peer_id,
                                   const char *caller_device_id,
                                   const char *callee_device_id,
                                   const char *logical_source_id,
                                   const char *domain_id,
                                   const char *gateway_id,
                                   simple_session_record_state_t state,
                                   simple_session_manager_admission_decision_t *decision,
                                   bool *created_out);

/**
 * @brief 更新指定 session 的控制态状态。
 *
 * @param[in,out] manager manager 实例，不能为 NULL。
 * @param[in] session_id session id。
 * @param[in] state 新状态。
 *
 * @retval 0 更新成功。
 * @retval -ENOENT 未找到对应 session。
 * @retval -EINVAL 参数非法。
 */
int simple_session_manager_update_state(simple_session_manager_t *manager,
                                        uint64_t session_id,
                                        simple_session_record_state_t state);

/**
 * @brief 按 session id 释放控制态记录。
 *
 * 释放时会把最后状态写入 release history，便于诊断。
 *
 * @param[in,out] manager manager 实例，不能为 NULL。
 * @param[in] session_id 要释放的 session id。
 * @param[in] reason 释放原因字符串，可为 NULL。
 *
 * @retval 0 找到并释放记录。
 * @retval -ENOENT 未找到对应 session。
 * @retval -EINVAL 参数非法。
 */
int simple_session_manager_release(simple_session_manager_t *manager,
                                   uint64_t session_id,
                                   const char *reason);

/**
 * @brief 查询指定 session 的当前控制态记录。
 *
 * @param[in] manager manager 实例，不能为 NULL。
 * @param[in] session_id session id。
 * @param[out] record_out 记录输出，不能为 NULL。
 *
 * @retval 0 查询成功。
 * @retval -ENOENT 未找到对应 session。
 * @retval -EINVAL 参数非法。
 */
int simple_session_manager_get(const simple_session_manager_t *manager,
                               uint64_t session_id,
                               simple_session_record_t *record_out);

/** @brief 返回当前活动记录数量。 */
size_t simple_session_manager_count(const simple_session_manager_t *manager);

/** @brief 返回指定类型的当前活动记录数量。 */
size_t simple_session_manager_count_kind(const simple_session_manager_t *manager,
                                         simple_session_record_kind_t kind);

/** @brief 返回 release history 中已保存的记录数量。 */
size_t simple_session_manager_release_count(const simple_session_manager_t *manager);

/** @brief 查询指定类型最近创建的当前活动记录。 */
int simple_session_manager_get_latest(const simple_session_manager_t *manager,
                                      simple_session_record_kind_t kind,
                                      simple_session_record_t *record_out);

/** @brief 查询当前最近的 CALL 记录。 */
int simple_session_manager_get_current_call(const simple_session_manager_t *manager,
                                            simple_session_record_t *record_out);

/** @brief 查询当前最近的 MONITOR 记录。 */
int simple_session_manager_get_current_monitor(const simple_session_manager_t *manager,
                                               simple_session_record_t *record_out);

/** @brief 从 manager 当前记录派生旧 simple_session_state_t 兼容状态。 */
simple_session_state_t simple_session_manager_public_state(
    const simple_session_manager_t *manager);

/** @brief 遍历当前活动记录并通过回调输出。 */
void simple_session_manager_dump(const simple_session_manager_t *manager,
                                 simple_session_manager_dump_cb cb,
                                 void *user_ctx);

/** @brief 遍历 release history 并通过回调输出。 */
void simple_session_manager_dump_releases(const simple_session_manager_t *manager,
                                          simple_session_manager_dump_cb cb,
                                          void *user_ctx);

/** @brief 返回记录类型名称字符串。 */
const char *simple_session_record_kind_name(simple_session_record_kind_t kind);

/** @brief 返回记录状态名称字符串。 */
const char *simple_session_record_state_name(simple_session_record_state_t state);

#ifdef __cplusplus
}
#endif

#endif
