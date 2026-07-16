/**
 * @file
 * @brief simple 层 session media plan 记录。
 */

#ifndef __SIMPLE_SESSION_MEDIA_PLAN_H__
#define __SIMPLE_SESSION_MEDIA_PLAN_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "call_proto.h"
#include "simple_media.h"
#include "simple_media_profile_shape.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief media plan 最多保存的 session/media 记录数量。 */
#define SIMPLE_SESSION_MEDIA_PLAN_MAX_ENTRIES 16
/** @brief media plan reason 字符串缓冲区长度，单位为字节。 */
#define SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX  64

/**
 * @brief media plan 中的媒体类型。
 */
typedef enum
{
    /** 无效媒体类型。 */
    SIMPLE_SESSION_MEDIA_KIND_NONE = 0,
    /** 视频。 */
    SIMPLE_SESSION_MEDIA_KIND_VIDEO,
    /** 音频。 */
    SIMPLE_SESSION_MEDIA_KIND_AUDIO,
} simple_session_media_kind_t;

/**
 * @brief media_profile 保存方向。
 */
typedef enum
{
    /** 无效方向。 */
    SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_NONE = 0,
    /** 本端发送出去的 wire profile。 */
    SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_LOCAL,
    /** 对端发来的 wire profile。 */
    SIMPLE_SESSION_MEDIA_PROFILE_DIRECTION_PEER,
} simple_session_media_profile_direction_t;

/**
 * @brief profile selection 诊断结果。
 */
typedef struct
{
    /** 触发选择的消息类型。 */
    uint8_t trigger_msg_type;
    /** 实际选中的 profile 消息类型。 */
    uint8_t profile_msg_type;
    /** 实际选中的 profile 方向。 */
    simple_session_media_profile_direction_t direction;
    /** 选择原因。 */
    char reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
} simple_session_media_plan_profile_selection_t;

/**
 * @brief Plan Producer 使用的 endpoint resolver。
 */
typedef int (*simple_session_media_plan_endpoint_resolver_t)(
    void *user_ctx,
    const simple_media_endpoint_request_t *request,
    simple_media_endpoint_result_t *result);

/**
 * @brief TX profile producer 请求。
 */
typedef struct
{
    /** 即将发送的消息类型。 */
    uint8_t msg_type;
    /** session id。 */
    uint64_t session_id;
    /** 本端 device_id。 */
    const char *self_device_id;
    /** caller device_id。 */
    const char *caller_device_id;
    /** callee device_id。 */
    const char *callee_device_id;
    /** 对端 numeric peer id。 */
    uint32_t peer_id;
    /** 可选 TX profile shape policy。 */
    simple_tx_profile_shape_policy_t tx_profile_shape_policy;
    /** 透传给 tx_profile_shape_policy 的上下文。 */
    void *tx_profile_shape_policy_ctx;
    /** 可选对端 offer profile。 */
    const call_media_profile_t *offer_profile;
    /** true 表示 offer_profile 有效。 */
    bool has_offer_profile;
    /** endpoint resolver，不能为 NULL。 */
    simple_session_media_plan_endpoint_resolver_t endpoint_resolver;
    /** 透传给 endpoint_resolver 的上下文。 */
    void *endpoint_resolver_ctx;
} simple_session_media_plan_profile_producer_request_t;

/**
 * @brief TX profile producer 结果。
 */
typedef struct
{
    /** producer 返回值。 */
    int ret;
    /** producer 处理的消息类型。 */
    uint8_t msg_type;
    /** shape 阶段结果。 */
    simple_media_profile_shape_result_t shape;
    /** true 表示调用过 endpoint resolver。 */
    bool endpoint_resolver_called;
    /** endpoint resolver 返回值。 */
    int endpoint_resolver_ret;
    /** link decision 结果。 */
    call_media_link_decision_t link_decision;
    /** link decision 返回值。 */
    int link_decision_ret;
    /** producer 原因字符串。 */
    char reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
} simple_session_media_plan_profile_producer_result_t;

/**
 * @brief converter/media bridge plan。
 */
typedef struct
{
    /** ingress endpoint。 */
    call_media_endpoint_t ingress_endpoint;
    /** true 表示 ingress_endpoint 有效。 */
    bool has_ingress_endpoint;
    /** egress endpoint。 */
    call_media_endpoint_t egress_endpoint;
    /** true 表示 egress_endpoint 有效。 */
    bool has_egress_endpoint;
    /** RX match 地址。 */
    uint32_t rx_match_addr;
    /** RX match 端口。 */
    uint16_t rx_match_port;
    /** true 表示 RX match endpoint 有效。 */
    bool has_rx_match_endpoint;
    /** bridge plan 原因字符串。 */
    char reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
} simple_media_bridge_plan_t;

/**
 * @brief media plan 中的单条 session/media 诊断记录。
 *
 * 字段按 provider/profile/link decision/apply/bridge/resource/runtime 阶段分组保存，
 * 用于 host 断言和板端 diagnostics；它不是 wire protocol 结构。
 */
typedef struct
{
    uint64_t session_id;
    simple_session_media_kind_t media_kind;
    uint8_t last_msg_type;
    uint8_t direction;
    uint8_t codec;
    uint8_t payload_type;
    bool provider_seen;
    uint8_t provider_stage;
    int provider_ret;
    bool provider_has_declared_endpoint;
    call_media_endpoint_t provider_declared_endpoint;
    bool provider_has_tx_remote_endpoint;
    call_media_endpoint_t provider_tx_remote_endpoint;
    bool provider_has_rx_match_endpoint;
    uint32_t provider_rx_match_addr;
    uint16_t provider_rx_match_port;
    char provider_reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
    bool tx_profile_seen;
    bool rx_profile_seen;
    bool local_profile_seen;
    uint8_t local_profile_msg_type;
    call_media_profile_t local_profile;
    bool peer_profile_seen;
    uint8_t peer_profile_msg_type;
    call_media_profile_t peer_profile;
    bool profile_shape_seen;
    int profile_shape_ret;
    uint8_t profile_shape_before_mask;
    uint8_t profile_shape_after_mask;
    char profile_shape_reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
    bool profile_selection_seen;
    int profile_selection_ret;
    uint8_t profile_selection_trigger_msg_type;
    uint8_t profile_selection_profile_msg_type;
    simple_session_media_profile_direction_t profile_selection_direction;
    char profile_selection_reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
    bool profile_producer_seen;
    int profile_producer_ret;
    uint8_t profile_producer_msg_type;
    bool profile_producer_endpoint_resolver_called;
    int profile_producer_endpoint_resolver_ret;
    int profile_producer_link_decision_ret;
    char profile_producer_reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
    bool has_declared_endpoint;
    bool has_peer_endpoint;
    call_media_endpoint_t declared_endpoint;
    call_media_endpoint_t peer_endpoint;
    /** 是否已有 link decision dry-run 记录。 */
    bool has_link_decision;
    /** link decision 返回值。 */
    int link_decision_ret;
    /** link decision 是否启用该 media kind。 */
    bool link_decision_enabled;
    /** link decision 选中的 wire-declared endpoint。 */
    call_media_endpoint_t link_decision_endpoint;
    bool apply_seen;
    int apply_ret;
    bool has_apply_tx_remote_endpoint;
    call_media_endpoint_t apply_tx_remote_endpoint;
    bool has_apply_rx_match_endpoint;
    uint32_t apply_rx_match_addr;
    uint16_t apply_rx_match_port;
    bool bridge_seen;
    int bridge_ret;
    bool bridge_plan_seen;
    bool has_bridge_plan_ingress_endpoint;
    call_media_endpoint_t bridge_plan_ingress_endpoint;
    bool has_bridge_plan_egress_endpoint;
    call_media_endpoint_t bridge_plan_egress_endpoint;
    bool has_bridge_plan_rx_match_endpoint;
    uint32_t bridge_plan_rx_match_addr;
    uint16_t bridge_plan_rx_match_port;
    char bridge_plan_reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
    bool has_bridge_ingress_endpoint;
    call_media_endpoint_t bridge_ingress_endpoint;
    bool has_bridge_egress_endpoint;
    call_media_endpoint_t bridge_egress_endpoint;
    bool has_bridge_rx_match_endpoint;
    uint32_t bridge_rx_match_addr;
    uint16_t bridge_rx_match_port;
    bool resource_checked;
    bool resource_admitted;
    unsigned int resource_used;
    unsigned int resource_limit;
    char resource_reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
    bool runtime_active;
    uint8_t runtime_msg_type;
    int runtime_ret;
    char runtime_reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
    char reason[SIMPLE_SESSION_MEDIA_PLAN_REASON_MAX];
} simple_session_media_plan_record_t;

/**
 * @brief media plan 内部 slot。
 */
typedef struct
{
    /** true 表示 slot 正在使用。 */
    bool in_use;
    /** slot 保存的记录。 */
    simple_session_media_plan_record_t record;
} simple_session_media_plan_slot_t;

/**
 * @brief simple session media plan。
 */
typedef struct
{
    /** session/media 记录表。 */
    simple_session_media_plan_slot_t slots[SIMPLE_SESSION_MEDIA_PLAN_MAX_ENTRIES];
} simple_session_media_plan_t;

/**
 * @brief media plan dump 回调。
 */
typedef void (*simple_session_media_plan_dump_cb)(
    void *user_ctx,
    const simple_session_media_plan_record_t *record);

/** @brief 初始化 media plan。 */
void simple_session_media_plan_init(simple_session_media_plan_t *plan);

/** @brief 清空 media plan 中的所有记录。 */
void simple_session_media_plan_clear(simple_session_media_plan_t *plan);

/**
 * @brief 记录一份 media_profile。
 *
 * @param[in,out] plan media plan，不能为 NULL。
 * @param[in] session_id session id。
 * @param[in] msg_type profile 对应消息类型。
 * @param[in] profile profile 内容，不能为 NULL。
 * @param[in] tx_profile true 表示 local profile，false 表示 peer profile。
 */
int simple_session_media_plan_note_profile(simple_session_media_plan_t *plan,
                                           uint64_t session_id,
                                           uint8_t msg_type,
                                           const call_media_profile_t *profile,
                                           bool tx_profile);

/** @brief 记录本端已发送的 local profile。 */
int simple_session_media_plan_note_local_profile(simple_session_media_plan_t *plan,
                                                 uint64_t session_id,
                                                 uint8_t msg_type,
                                                 const call_media_profile_t *profile);

/** @brief 记录对端发来的 peer profile。 */
int simple_session_media_plan_note_peer_profile(simple_session_media_plan_t *plan,
                                                uint64_t session_id,
                                                uint8_t msg_type,
                                                const call_media_profile_t *profile);

/** @brief 按 session/msg/direction 读取完整 profile。 */
int simple_session_media_plan_load_profile(
    const simple_session_media_plan_t *plan,
    uint64_t session_id,
    uint8_t msg_type,
    simple_session_media_profile_direction_t direction,
    call_media_profile_t *profile_out);

/** @brief 为指定消息读取对端 offer/peer profile，包含 CALL/MONITOR fallback 规则。 */
int simple_session_media_plan_load_peer_profile_for_msg(
    const simple_session_media_plan_t *plan,
    uint64_t session_id,
    uint8_t msg_type,
    call_media_profile_t *profile_out,
    simple_session_media_plan_profile_selection_t *selection_out);

/** @brief 为 media start 选择应使用的 profile。 */
int simple_session_media_plan_select_profile_for_media_start(
    const simple_session_media_plan_t *plan,
    uint64_t session_id,
    uint8_t trigger_msg_type,
    call_media_profile_t *profile_out,
    simple_session_media_plan_profile_selection_t *selection_out);

/** @brief 记录 profile selection 诊断。 */
int simple_session_media_plan_note_profile_selection(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    const simple_session_media_plan_profile_selection_t *selection,
    const call_media_profile_t *profile,
    int ret,
    const char *reason);

/** @brief 生成最终 TX wire media_profile。 */
int simple_session_media_plan_prepare_tx_profile(
    const simple_session_media_plan_profile_producer_request_t *request,
    call_media_profile_t *profile,
    simple_session_media_plan_profile_producer_result_t *result);

/** @brief 记录 TX profile producer 诊断。 */
int simple_session_media_plan_note_profile_producer(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    const simple_session_media_plan_profile_producer_result_t *result,
    const call_media_profile_t *profile);

/** @brief 记录 profile shape 阶段诊断。 */
int simple_session_media_plan_note_profile_shape(simple_session_media_plan_t *plan,
                                                 uint64_t session_id,
                                                 uint8_t msg_type,
                                                 int ret,
                                                 uint8_t before_mask,
                                                 uint8_t after_mask,
                                                 const char *reason);

/** @brief 记录 endpoint provider 结果。 */
int simple_session_media_plan_note_provider_endpoint(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    uint8_t stage,
    uint8_t msg_type,
    int ret,
    const call_media_endpoint_t *declared_endpoint,
    bool has_declared_endpoint,
    const call_media_endpoint_t *tx_remote_endpoint,
    bool has_tx_remote_endpoint,
    uint32_t rx_match_addr,
    uint16_t rx_match_port,
    bool has_rx_match_endpoint,
    const char *reason);

/** @brief 记录 media profile link decision dry-run 结果。 */
int simple_session_media_plan_note_link_decision(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    uint8_t msg_type,
    const call_media_link_decision_t *decision,
    int ret);

/** @brief 记录 media apply 业务结论。 */
int simple_session_media_plan_note_apply(simple_session_media_plan_t *plan,
                                         uint64_t session_id,
                                         simple_session_media_kind_t media_kind,
                                         int ret,
                                         const char *reason);

/** @brief 记录 media endpoint apply 细节。 */
int simple_session_media_plan_note_apply_endpoint(simple_session_media_plan_t *plan,
                                                  uint64_t session_id,
                                                  simple_session_media_kind_t media_kind,
                                                  int ret,
                                                  const call_media_endpoint_t *tx_remote_endpoint,
                                                  bool has_tx_remote_endpoint,
                                                  uint32_t rx_match_addr,
                                                  uint16_t rx_match_port,
                                                  bool has_rx_match_endpoint,
                                                  const char *reason);

/** @brief 记录 bridge apply 业务结论。 */
int simple_session_media_plan_note_bridge(simple_session_media_plan_t *plan,
                                          uint64_t session_id,
                                          simple_session_media_kind_t media_kind,
                                          int ret,
                                          const char *reason);

/** @brief 记录 bridge plan。 */
int simple_session_media_plan_note_bridge_plan(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    const simple_media_bridge_plan_t *bridge_plan);

/** @brief 记录 bridge endpoint apply 细节。 */
int simple_session_media_plan_note_bridge_endpoint(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    int ret,
    const call_media_endpoint_t *ingress_endpoint,
    bool has_ingress_endpoint,
    const call_media_endpoint_t *egress_endpoint,
    bool has_egress_endpoint,
    uint32_t rx_match_addr,
    uint16_t rx_match_port,
    bool has_rx_match_endpoint,
    const char *reason);

/** @brief 记录资源准入决策。 */
int simple_session_media_plan_note_resource(simple_session_media_plan_t *plan,
                                            uint64_t session_id,
                                            simple_session_media_kind_t media_kind,
                                            bool admitted,
                                            unsigned int used,
                                            unsigned int limit,
                                            const char *reason);
/** @brief 记录 runtime active fact。 */
int simple_session_media_plan_note_runtime_active(
    simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind,
    uint8_t msg_type,
    bool active,
    int ret,
    const char *reason);

/** @brief 查询指定 session/media 是否已经 runtime active。 */
bool simple_session_media_plan_runtime_active(
    const simple_session_media_plan_t *plan,
    uint64_t session_id,
    simple_session_media_kind_t media_kind);

/** @brief 统计指定媒体类型的 runtime active session 数量。 */
unsigned int simple_session_media_plan_count_runtime_active(
    const simple_session_media_plan_t *plan,
    simple_session_media_kind_t media_kind);

/** @brief 统计指定媒体类型的 runtime active 数量，并排除一个 session。 */
unsigned int simple_session_media_plan_count_runtime_active_excluding(
    const simple_session_media_plan_t *plan,
    simple_session_media_kind_t media_kind,
    uint64_t session_id);

/** @brief 按 active facts 派生 CALL_MEDIA_STATE_* 兼容状态。 */
uint8_t simple_session_media_plan_media_state(
    const simple_session_media_plan_t *plan,
    uint64_t session_id);

/** @brief 释放指定 session 的 media plan 记录。 */
void simple_session_media_plan_release(simple_session_media_plan_t *plan,
                                       uint64_t session_id,
                                       const char *reason);

/** @brief 查询指定 session/media 的 plan 记录。 */
int simple_session_media_plan_get(const simple_session_media_plan_t *plan,
                                  uint64_t session_id,
                                  simple_session_media_kind_t media_kind,
                                  simple_session_media_plan_record_t *record_out);

/** @brief 返回当前有效 plan 记录数量。 */
size_t simple_session_media_plan_count(const simple_session_media_plan_t *plan);

/** @brief 统计当前 PLC video active 数量。 */
unsigned int simple_session_media_plan_count_plc_video(
    const simple_session_media_plan_t *plan);

/** @brief 统计当前 PLC video active 数量，并排除一个 session。 */
unsigned int simple_session_media_plan_count_plc_video_excluding(
    const simple_session_media_plan_t *plan,
    uint64_t session_id);

/** @brief 遍历导出所有 plan 记录。 */
void simple_session_media_plan_dump(const simple_session_media_plan_t *plan,
                                    simple_session_media_plan_dump_cb cb,
                                    void *user_ctx);

/** @brief 返回 media kind 名称字符串。 */
const char *simple_session_media_kind_name(simple_session_media_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif
