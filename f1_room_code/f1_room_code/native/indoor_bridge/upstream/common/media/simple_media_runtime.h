/**
 * @file
 * @brief simple 示例应用媒体运行态兼容 facade。
 *
 * 本文件属于 examples 应用层，用于把 simple_call_app 看到的 media ops 与
 * simple_media 底层实现之间加一层稳定边界，并逐步接管 simple_media_t 所有权。
 */

#ifndef __SIMPLE_MEDIA_RUNTIME_H__
#define __SIMPLE_MEDIA_RUNTIME_H__

#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

#include "simple_media.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief simple media runtime 不透明句柄。 */
typedef struct simple_media_runtime simple_media_runtime_t;
/** @brief runtime debug probe 私有状态不透明句柄。 */
typedef struct simple_media_runtime_debug_probe_state simple_media_runtime_debug_probe_state_t;

/** @brief runtime bridge 承载类型。 */
typedef enum
{
    SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_NONE = 0,
    SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_MEDIA,
    SIMPLE_MEDIA_RUNTIME_BRIDGE_KIND_DIRECT,
} simple_media_runtime_bridge_kind_t;

/** @brief runtime MEDIA bridge apply 形态。 */
typedef enum
{
    SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_NONE = 0,
    SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_BIDIRECTIONAL,
    SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_SPLIT,
    SIMPLE_MEDIA_RUNTIME_MEDIA_BRIDGE_AUDIO_ONLY,
} simple_media_runtime_media_bridge_mode_t;

/** @brief runtime endpoint snapshot 来源。 */
typedef enum
{
    SIMPLE_MEDIA_RUNTIME_ENDPOINT_SNAPSHOT_CURRENT = 0,
    SIMPLE_MEDIA_RUNTIME_ENDPOINT_SNAPSHOT_BASE,
} simple_media_runtime_endpoint_snapshot_source_t;

/** @brief runtime bridge route stats 方向。 */
typedef enum
{
    SIMPLE_MEDIA_RUNTIME_BRIDGE_ROUTE_UDP_TO_PLC = 1,
    SIMPLE_MEDIA_RUNTIME_BRIDGE_ROUTE_PLC_TO_UDP,
} simple_media_runtime_bridge_route_direction_t;

/** @brief runtime debug probe event 类型。 */
typedef enum
{
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_EVENT_FRAME = 1,
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_EVENT_RTCP,
} simple_media_runtime_debug_probe_event_type_t;

/** @brief runtime debug probe 链路方向。 */
typedef enum
{
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_LINK_UDP = 1,
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_LINK_PLC,
} simple_media_runtime_debug_probe_link_t;

/** @brief runtime debug probe 媒体类型。 */
typedef enum
{
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_MEDIA_VIDEO = 1,
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_MEDIA_AUDIO,
} simple_media_runtime_debug_probe_media_t;

/** @brief runtime debug probe RTCP 摘要类型。 */
typedef enum
{
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_UNKNOWN = 0,
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_NACK,
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_PLI,
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_FIR,
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_RR,
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_SR,
    SIMPLE_MEDIA_RUNTIME_DEBUG_PROBE_RTCP_AFB,
} simple_media_runtime_debug_probe_rtcp_type_t;

/** @brief runtime debug probe typed event。 */
typedef struct
{
    simple_media_runtime_debug_probe_event_type_t event;
    simple_media_runtime_debug_probe_link_t link;
    simple_media_runtime_debug_probe_media_t media;
    size_t len;
    uint32_t ts;
    int flags;
    simple_media_runtime_debug_probe_rtcp_type_t rtcp_type;
    uint8_t rtcp_packet_type;
    uint8_t rtcp_fmt;
    uint32_t sender_ssrc;
    uint32_t media_ssrc;
    uint32_t nack_count;
    uint8_t rr_fraction_lost;
    uint32_t rr_jitter;
    uint32_t remb_bps;
    const char *reason;
} simple_media_runtime_debug_probe_event_t;

/**
 * @brief runtime debug probe observer。
 *
 * 该回调在 mtrans RX/RTCP 回调链路中同步触发；实现必须快速返回，不要阻塞。
 * event 指针只在本次回调期间有效，调用方如需跨线程使用应自行复制字段。
 */
typedef void (*simple_media_runtime_debug_probe_observer_fn)(
    void *ctx,
    const simple_media_runtime_debug_probe_event_t *event);

/** @brief runtime debug probe 配置。 */
typedef struct
{
    uint32_t jitter_buffer_ms;
    uint32_t jitter_wallclock_timeout_ms;
    uint32_t jitter_max_queue_packets;
    simple_media_runtime_debug_probe_observer_fn observer;
    void *observer_ctx;
} simple_media_runtime_debug_probe_config_t;

/** @brief runtime video feedback 类型。 */
typedef enum
{
    SIMPLE_MEDIA_RUNTIME_VIDEO_FEEDBACK_PLI = 1,
    SIMPLE_MEDIA_RUNTIME_VIDEO_FEEDBACK_FIR,
} simple_media_runtime_video_feedback_t;

/** @brief 默认 PLI 最小发送间隔，单位为毫秒。 */
#define SIMPLE_MEDIA_RUNTIME_DEFAULT_PLI_MIN_INTERVAL_MS (500U)
/** @brief 默认 FIR 最小发送间隔，单位为毫秒。 */
#define SIMPLE_MEDIA_RUNTIME_DEFAULT_FIR_MIN_INTERVAL_MS (1500U)
/** @brief runtime bridge table 固定容量。 */
#define SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY (8U)

/** @brief runtime 可选 start delegate。 */
typedef int (*simple_media_runtime_start_fn)(void *ctx);
/** @brief runtime 可选 stop delegate。 */
typedef void (*simple_media_runtime_stop_fn)(void *ctx);
/** @brief endpoint apply observer，用于 host 保留兼容日志/校验。 */
typedef void (*simple_media_runtime_apply_observer_fn)(
    void *ctx,
    bool audio,
    const simple_media_endpoint_binding_t *binding,
    int ret,
    const char *reason);

/**
 * @brief simple media runtime 配置。
 */
typedef struct
{
    const char *node_name;
    simple_media_t *media;
    pthread_mutex_t *lock;
    void *delegate_ctx;
    simple_media_runtime_start_fn start_video;
    simple_media_runtime_stop_fn stop_video;
    simple_media_runtime_start_fn start_audio;
    simple_media_runtime_stop_fn stop_audio;
    uint32_t video_feedback_pli_min_interval_ms;
    uint32_t video_feedback_fir_min_interval_ms;
    bool audio_link_match_required;
    simple_media_runtime_apply_observer_fn apply_observer;
    void *apply_observer_ctx;
} simple_media_runtime_config_t;

/**
 * @brief runtime 记录的 bridge 状态。
 */
typedef struct
{
    simple_media_runtime_bridge_kind_t kind;
    bool active;
    bool video_active;
    bool audio_active;
    uint64_t session_id;
    char caller_device_id[CALL_DEVICE_ID_MAX];
    char callee_device_id[CALL_DEVICE_ID_MAX];
} simple_media_runtime_bridge_state_t;

/**
 * @brief runtime 保存的 active MEDIA bridge 描述符。
 *
 * 该描述符只描述 simple_media bridge 的当前会话 apply 结果；DIRECT bridge 使用
 * direct bridge config/state，不复用该结构。
 */
typedef struct
{
    simple_media_runtime_media_bridge_mode_t mode;
    uint32_t udp_remote_addr;
    uint16_t udp_remote_port;
    uint32_t udp_rx_match_addr;
    uint16_t udp_rx_match_port;
    uint32_t plc_remote_addr;
    uint16_t plc_remote_port;
    uint32_t plc_rx_match_addr;
    uint16_t plc_rx_match_port;
    uint32_t video_plc_remote_addr;
    uint16_t video_plc_remote_port;
    uint32_t audio_udp_remote_addr;
    uint16_t audio_udp_remote_port;
} simple_media_runtime_media_bridge_descriptor_t;

/**
 * @brief runtime 内部 bridge table 记录。
 *
 * MEDIA/DIRECT bridge 的 channel/rule 资源均由 record 持有，二者均支持
 * 多个 record 并存。
 */
typedef struct
{
    bool used;
    bool has_media_descriptor;
    bool owns_media_resources;
    bool owns_direct_resources;
    simple_media_runtime_bridge_state_t state;
    simple_media_runtime_media_bridge_descriptor_t media_descriptor;
    mtrans_channel_t media_bridge_udp_ch;
    mtrans_channel_t media_bridge_plc_ch;
    mtrans_channel_t media_bridge_video_plc_ch;
    mtrans_channel_t media_bridge_audio_udp_ch;
    uint32_t media_bridge_route_group;
    uint32_t media_bridge_udp_to_plc_flags;
    uint32_t media_bridge_plc_to_udp_flags;
    uint32_t media_bridge_udp_to_video_plc_flags;
    uint32_t media_bridge_plc_to_audio_udp_flags;
    bool media_bridge_udp_to_plc_added;
    bool media_bridge_plc_to_udp_added;
    bool media_bridge_udp_to_video_plc_added;
    bool media_bridge_plc_to_audio_udp_added;
    mtrans_channel_t direct_bridge_ingress_ch;
    mtrans_channel_t direct_bridge_egress_ch;
    uint32_t direct_bridge_route_group;
    uint32_t direct_bridge_forward_flags;
    uint32_t direct_bridge_reverse_flags;
    bool direct_bridge_forward_added;
    bool direct_bridge_reverse_added;
} simple_media_runtime_bridge_record_t;

/** @brief runtime bridge table 诊断快照。 */
typedef struct
{
    simple_media_runtime_bridge_state_t state;
    bool has_media_descriptor;
    simple_media_runtime_media_bridge_descriptor_t media_descriptor;
    bool owns_media_resources;
    bool media_udp_open;
    bool media_plc_open;
    bool media_video_plc_open;
    bool media_audio_udp_open;
    uint32_t media_route_group;
    uint32_t media_udp_to_plc_flags;
    uint32_t media_plc_to_udp_flags;
    uint32_t media_udp_to_video_plc_flags;
    uint32_t media_plc_to_audio_udp_flags;
    bool media_udp_to_plc_added;
    bool media_plc_to_udp_added;
    bool media_udp_to_video_plc_added;
    bool media_plc_to_audio_udp_added;
    bool owns_direct_resources;
    bool direct_ingress_open;
    bool direct_egress_open;
    uint32_t direct_route_group;
    uint32_t direct_forward_flags;
    uint32_t direct_reverse_flags;
    bool direct_forward_added;
    bool direct_reverse_added;
} simple_media_runtime_bridge_snapshot_t;

/** @brief runtime 当前或 base endpoint 只读快照。 */
typedef struct
{
    simple_media_endpoint_type_t type;
    const char *udp_interface;
    uint32_t udp_remote_addr;
    uint16_t udp_remote_port;
    bool udp_mcast_enabled;
    uint32_t plc_local_addr;
    uint32_t plc_remote_addr;
    uint16_t plc_remote_port;
    bool plc_rx_match_ep_enabled;
    uint32_t plc_rx_match_addr;
    uint16_t plc_rx_match_port;
    bool audio_tx_remote_ep_enabled;
    uint32_t audio_tx_remote_addr;
    uint16_t audio_tx_remote_port;
} simple_media_runtime_endpoint_snapshot_t;

/**
 * @brief runtime direct bridge 配置。
 *
 * direct bridge 直接用两端 mtrans channel 建 route，不扩展 simple_media endpoint 类型。
 */
typedef struct
{
    mtrans_channel_config_t ingress_channel;
    mtrans_channel_config_t egress_channel;
    uint32_t route_group;
    uint32_t forward_flags;
    uint32_t reverse_flags;
    bool bidirectional;
} simple_media_runtime_direct_bridge_config_t;

/**
 * @brief simple media runtime 对象。
 */
struct simple_media_runtime
{
    simple_media_runtime_config_t cfg;
    simple_media_t owned_media;
    bool owns_media;
    bool has_base_config;
    simple_media_config_t base_config;
    simple_media_runtime_bridge_record_t bridge_table[
        SIMPLE_MEDIA_RUNTIME_BRIDGE_TABLE_CAPACITY];
    bool has_pending_media_bridge;
    simple_media_runtime_media_bridge_descriptor_t pending_media_bridge;
    bool has_pending_video_endpoint;
    simple_media_endpoint_binding_t pending_video_endpoint;
    bool has_active_video_endpoint;
    simple_media_endpoint_binding_t active_video_endpoint;
    bool has_pending_audio_endpoint;
    simple_media_endpoint_binding_t pending_audio_endpoint;
    bool has_active_audio_endpoint;
    simple_media_endpoint_binding_t active_audio_endpoint;
    bool bridge_releasing;
    simple_media_runtime_debug_probe_state_t *debug_probe;
    bool debug_probe_starting;
    uint64_t video_feedback_last_pli_ms;
    uint64_t video_feedback_last_fir_ms;
};

/** @brief 填充默认配置。 */
void simple_media_runtime_config_init(simple_media_runtime_config_t *cfg);
/** @brief 填充默认 direct bridge 配置。 */
void simple_media_runtime_direct_bridge_config_init(
    simple_media_runtime_direct_bridge_config_t *cfg);
/** @brief 填充默认 debug probe 配置。 */
void simple_media_runtime_debug_probe_config_init(
    simple_media_runtime_debug_probe_config_t *cfg);
/** @brief 初始化 runtime facade。 */
int simple_media_runtime_init(simple_media_runtime_t *runtime,
                              const simple_media_runtime_config_t *cfg);
/** @brief 由 runtime 打开并持有 simple_media_t。 */
int simple_media_runtime_open(simple_media_runtime_t *runtime,
                              const simple_media_runtime_config_t *cfg,
                              const simple_media_config_t *media_cfg);
/** @brief 关闭 runtime 持有的 simple_media_t，并清空 runtime。 */
void simple_media_runtime_close(simple_media_runtime_t *runtime);
/** @brief 反初始化 runtime facade；owned runtime 会同时关闭 media。 */
void simple_media_runtime_deinit(simple_media_runtime_t *runtime);
/** @brief 填充兼容 simple_media_ops_t。 */
int simple_media_runtime_fill_ops(simple_media_runtime_t *runtime, simple_media_ops_t *ops);
/** @brief 获取 runtime 初始化时保存的 base media config。 */
const simple_media_config_t *simple_media_runtime_base_config(
    const simple_media_runtime_t *runtime);
/** @brief 获取 runtime base endpoint 类型。 */
bool simple_media_runtime_get_base_endpoint_type(
    const simple_media_runtime_t *runtime,
    simple_media_endpoint_type_t *type);
/** @brief 查询 runtime base config 是否为 bridge endpoint。 */
bool simple_media_runtime_base_bridge_capable(const simple_media_runtime_t *runtime);
/** @brief 查询 runtime base bridge 是否支持双向转发。 */
bool simple_media_runtime_base_bridge_bidirectional(const simple_media_runtime_t *runtime);
/** @brief 查询 runtime base UDP interface。 */
const char *simple_media_runtime_base_udp_interface(const simple_media_runtime_t *runtime);
/** @brief 查询 runtime current UDP interface。 */
const char *simple_media_runtime_current_udp_interface(const simple_media_runtime_t *runtime);
/** @brief runtime 空闲时选择 UDP interface，同时更新 current/base config。 */
int simple_media_runtime_select_udp_interface_idle(simple_media_runtime_t *runtime,
                                                   const char *ifname);
/** @brief 读取 runtime endpoint 快照。 */
bool simple_media_runtime_get_endpoint_snapshot(
    const simple_media_runtime_t *runtime,
    simple_media_runtime_endpoint_snapshot_source_t source,
    simple_media_runtime_endpoint_snapshot_t *snapshot);

/** @brief 查询 runtime video TX stream 是否可发送。 */
bool simple_media_runtime_video_tx_ready(const simple_media_runtime_t *runtime);
/** @brief 查询 runtime audio TX stream 是否可发送。 */
bool simple_media_runtime_audio_tx_ready(const simple_media_runtime_t *runtime);
/** @brief 通过 runtime 输入视频 payload。 */
int simple_media_runtime_input_video(simple_media_runtime_t *runtime,
                                     const uint8_t *payload,
                                     size_t payload_len,
                                     uint32_t rtp_ts);
/** @brief 通过 runtime 输入音频 payload。 */
int simple_media_runtime_input_audio(simple_media_runtime_t *runtime,
                                     const uint8_t *payload,
                                     size_t payload_len,
                                     uint32_t rtp_ts);
/**
 * @brief 通过 runtime 向当前 video RX 对端请求关键帧/恢复。
 *
 * @retval 0 请求已发送。
 * @retval -EAGAIN 被 runtime 限频跳过。
 * @retval -ENOENT 当前没有 video RX stream。
 * @retval -EINVAL 参数非法。
 */
int simple_media_runtime_request_video_feedback(
    simple_media_runtime_t *runtime,
    simple_media_runtime_video_feedback_t type,
    const char *reason);
/** @brief 查询 runtime bridge route stats。 */
int simple_media_runtime_get_bridge_route_stats(
    simple_media_runtime_t *runtime,
    simple_media_runtime_bridge_route_direction_t direction,
    mtrans_route_rule_stats_t *stats);
/** @brief 启动 runtime debug RX probe；已启动时返回 -EBUSY。 */
int simple_media_runtime_start_debug_probe(
    simple_media_runtime_t *runtime,
    const simple_media_runtime_debug_probe_config_t *cfg);
/** @brief 停止 runtime debug RX probe，允许重复调用。 */
void simple_media_runtime_stop_debug_probe(simple_media_runtime_t *runtime);
/** @brief 查询 runtime debug RX probe 是否已启动。 */
bool simple_media_runtime_debug_probe_active(const simple_media_runtime_t *runtime);

/** @brief 通过 runtime 启动视频。 */
int simple_media_runtime_start_video(simple_media_runtime_t *runtime);
/** @brief 通过 runtime 停止视频。 */
void simple_media_runtime_stop_video(simple_media_runtime_t *runtime);
/** @brief 通过 runtime 启动音频。 */
int simple_media_runtime_start_audio(simple_media_runtime_t *runtime);
/** @brief 通过 runtime 停止音频。 */
void simple_media_runtime_stop_audio(simple_media_runtime_t *runtime);
/** @brief 通过 runtime 应用视频 endpoint binding。 */
int simple_media_runtime_apply_video_endpoint(
    simple_media_runtime_t *runtime,
    const simple_media_endpoint_binding_t *binding);
/** @brief 通过 runtime 应用音频 endpoint binding。 */
int simple_media_runtime_apply_audio_endpoint(
    simple_media_runtime_t *runtime,
    const simple_media_endpoint_binding_t *binding);

/** @brief 通过 runtime 应用双向 bridge endpoint。 */
int simple_media_runtime_apply_bridge_endpoint(simple_media_runtime_t *runtime,
                                               uint32_t udp_remote_addr,
                                               uint16_t udp_remote_port,
                                               uint32_t udp_rx_match_addr,
                                               uint16_t udp_rx_match_port,
                                               uint32_t plc_remote_addr,
                                               uint16_t plc_remote_port,
                                               uint32_t plc_rx_match_addr,
                                               uint16_t plc_rx_match_port);
/** @brief 通过 runtime 应用 PLC 到 UDP 视频/音频分流 bridge endpoint。 */
int simple_media_runtime_apply_bridge_split_endpoint(simple_media_runtime_t *runtime,
                                                     uint32_t video_udp_remote_addr,
                                                     uint16_t video_udp_remote_port,
                                                     uint32_t audio_udp_remote_addr,
                                                     uint16_t audio_udp_remote_port,
                                                     uint32_t video_plc_remote_addr,
                                                     uint16_t video_plc_remote_port,
                                                     uint32_t udp_rx_match_addr,
                                                     uint16_t udp_rx_match_port,
                                                     uint32_t plc_remote_addr,
                                                     uint16_t plc_remote_port,
                                                     uint32_t plc_rx_match_addr,
                                                     uint16_t plc_rx_match_port);
/** @brief 通过 runtime 应用双向 audio-only bridge endpoint。 */
int simple_media_runtime_apply_bridge_audio_only_endpoint(simple_media_runtime_t *runtime,
                                                          uint32_t udp_remote_addr,
                                                          uint16_t udp_remote_port,
                                                          uint32_t udp_rx_match_addr,
                                                          uint16_t udp_rx_match_port,
                                                          uint32_t plc_remote_addr,
                                                          uint16_t plc_remote_port,
                                                          uint32_t plc_rx_match_addr,
                                                          uint16_t plc_rx_match_port);
/** @brief 通过 runtime 显式启动 bridge。 */
int simple_media_runtime_start_bridge(simple_media_runtime_t *runtime);
/** @brief 通过 runtime 显式停止 bridge。 */
int simple_media_runtime_stop_bridge(simple_media_runtime_t *runtime);
/** @brief 使用兼容 pending descriptor 显式启动 MEDIA bridge；多实例主路径使用 descriptor API。 */
int simple_media_runtime_start_bridge_session(simple_media_runtime_t *runtime,
                                              uint64_t session_id,
                                              const char *caller_device_id,
                                              const char *callee_device_id,
                                              bool video_active,
                                              bool audio_active);
/** @brief 使用 descriptor 原子启动 MEDIA bridge session，避免共用 pending bridge 槽。 */
int simple_media_runtime_start_bridge_session_with_descriptor(
    simple_media_runtime_t *runtime,
    const simple_media_runtime_media_bridge_descriptor_t *descriptor,
    uint64_t session_id,
    const char *caller_device_id,
    const char *callee_device_id,
    bool video_active,
    bool audio_active);
/** @brief 显式启动 DIRECT bridge；允许与 MEDIA session 并存。 */
int simple_media_runtime_start_direct_bridge_session(
    simple_media_runtime_t *runtime,
    const simple_media_runtime_direct_bridge_config_t *bridge_cfg,
    uint64_t session_id,
    const char *caller_device_id,
    const char *callee_device_id,
    bool video_active,
    bool audio_active);
/** @brief 按 session 释放 bridge，session_id 为 0 时释放第一条 active record。 */
int simple_media_runtime_release_bridge(simple_media_runtime_t *runtime,
                                        uint64_t session_id,
                                        simple_media_runtime_bridge_state_t *released);
/** @brief 读取第一条 active bridge 状态，兼容旧单 active 查询。 */
bool simple_media_runtime_get_bridge_state(
    const simple_media_runtime_t *runtime,
    simple_media_runtime_bridge_state_t *state);
/** @brief 读取当前 active MEDIA bridge 描述符。 */
bool simple_media_runtime_get_media_bridge_descriptor(
    const simple_media_runtime_t *runtime,
    simple_media_runtime_media_bridge_descriptor_t *descriptor);
/** @brief 查询 active bridge record 数量。 */
size_t simple_media_runtime_get_bridge_record_count(
    const simple_media_runtime_t *runtime);
/** @brief 按 active ordinal 读取 bridge state。 */
bool simple_media_runtime_get_bridge_state_at(
    const simple_media_runtime_t *runtime,
    size_t ordinal,
    simple_media_runtime_bridge_state_t *state);
/** @brief 按 session_id 读取 active bridge state。 */
bool simple_media_runtime_get_bridge_state_for_session(
    const simple_media_runtime_t *runtime,
    uint64_t session_id,
    simple_media_runtime_bridge_state_t *state);
/** @brief 按 active ordinal 读取 bridge table 诊断快照。 */
bool simple_media_runtime_get_bridge_snapshot_at(
    const simple_media_runtime_t *runtime,
    size_t ordinal,
    simple_media_runtime_bridge_snapshot_t *snapshot);
/** @brief 按 session_id 读取 active bridge table 诊断快照。 */
bool simple_media_runtime_get_bridge_snapshot_for_session(
    const simple_media_runtime_t *runtime,
    uint64_t session_id,
    simple_media_runtime_bridge_snapshot_t *snapshot);
/** @brief 打印完整 bridge table 诊断。 */
void simple_media_runtime_dump_bridge_table(simple_media_runtime_t *runtime,
                                            const char *reason);

#ifdef __cplusplus
}
#endif

#endif
