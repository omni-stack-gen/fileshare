#ifndef __MTRANS_API_H__
#define __MTRANS_API_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#include "transport/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief mtrans 日志级别。
 */
typedef enum mtrans_log_level
{
	MTRANS_DEBUG = 1,
	MTRANS_INFO  = 2,
	MTRANS_WARN  = 3,
	MTRANS_ERROR = 4,
} mtrans_log_level_t;

/**
 * @brief mtrans 日志处理回调。
 */
typedef void (*mtrans_log_func_t)(mtrans_log_level_t level,
                                  const char *file,
                                  const long line,
                                  const char *func,
                                  const char *format,
                                  va_list ap);

/**
 * @brief mtrans 内存分配钩子集合。
 */
struct mtrans_mem_hooks
{
	/** 内存申请函数；size 单位为字节。 */
	void *(*malloc)(size_t size, const char *file, int line);
	/** 内存重分配函数；size 单位为字节。 */
	void *(*realloc)(void *ptr, size_t size, const char *file, int line);
	/** 内存释放函数。 */
	void (*free)(void *ptr, const char *file, int line);
};

/**
 * @brief 设置 mtrans 全局日志回调。
 *
 * 该接口影响后续 mtrans 内部日志输出。传入 NULL 表示关闭 mtrans 日志回调。
 *
 * @param[in] function 日志处理回调，可为 NULL。
 */
void mtrans_log_set_handler(mtrans_log_func_t function);

/**
 * @brief 注册 mtrans 全局内存分配钩子。
 *
 * 注册后 mtrans 内部会通过 hooks 中的 malloc/realloc/free 分配和释放内存。
 * 只有 hooks 及其中三个函数指针均有效时才会更新当前钩子；传入 NULL 不会改变当前钩子。
 *
 * @param[in] hooks 内存分配钩子集合，可为 NULL；函数指针本身需要在后续使用期间有效。
 */
void mtrans_mem_register_hooks(const struct mtrans_mem_hooks *hooks);

/* ==========================================================
 * 1. 核心枚举与结构体定义 (Data Types & Structures)
 * ========================================================== */

/**
 * @brief 链路封装/传输类型（决定 mtrans 如何组帧）
 *
 * UDP 表示通过 UDP datagram 直传 RTP/RTCP，不是泛化的以太网承载。
 * PLC 表示使用 PLC 固定帧与 RFC4571 聚合/拆分。
 */
typedef enum
{
	MTRANS_LINK_TYPE_UDP = 0,
	MTRANS_LINK_TYPE_PLC = 1,
} mtrans_link_type_t;

/**
 * @brief 链路画像（用于 profile match 路由策略，默认按 link_type 推断）
 */
typedef enum
{
	MTRANS_LINK_PROFILE_DEFAULT = 0,
	MTRANS_LINK_PROFILE_UDP = 1, /* UDP 通道画像 */
	MTRANS_LINK_PROFILE_PLC = 2, /* PLC 通道画像 */
} mtrans_link_profile_t;

/**
 * @brief 媒体类型
 */
typedef enum
{
	MTRANS_MEDIA_TYPE_VIDEO_H264 = 0,
	MTRANS_MEDIA_TYPE_VIDEO_H265 = 1,
	MTRANS_MEDIA_TYPE_AUDIO_OPUS = 2,
	MTRANS_MEDIA_TYPE_AUDIO_G711A = 3,
} mtrans_media_type_t;

/**
 * @brief 流事件类型
 */
typedef enum
{
	MTRANS_STREAM_EVENT_NETWORK_POOR     = 0, /* 丢包率偏高，建议应用层降低视频码率 */
	MTRANS_STREAM_EVENT_PLI_REQUESTED    = 1, /* 接收端画面严重卡死，请求发送端立刻编码一个 I 帧 (关键帧)！*/
	MTRANS_STREAM_EVENT_JITTER_TIMEOUT   = 2, /* 接收端 Jitter Buffer 超时放弃等待，画面可能出现花屏跳跃 */
} mtrans_stream_event_t;

/**
 * @brief RTCP 事件类型
 */
typedef enum
{
	MTRANS_RTCP_EVENT_UNKNOWN = 0,
	MTRANS_RTCP_EVENT_NACK,
	MTRANS_RTCP_EVENT_PLI,
	MTRANS_RTCP_EVENT_FIR,
	MTRANS_RTCP_EVENT_RR,
	MTRANS_RTCP_EVENT_SR,
	MTRANS_RTCP_EVENT_AFB,
} mtrans_rtcp_event_type_t;

/**
 * @brief RTCP 事件摘要
 */
typedef struct
{
	mtrans_rtcp_event_type_t type;
	uint8_t rtcp_type;
	uint8_t fmt;
	uint32_t sender_ssrc;
	uint32_t media_ssrc;
	uint32_t nack_count;
	uint8_t rr_fraction_lost;
	uint32_t rr_jitter;
	uint32_t remb_bps;
} mtrans_rtcp_event_t;

/**
 * @brief 系统全局初始化配置
 */
typedef struct
{
	mtrans_log_level_t log_level;               /* 0: Error, 1: Warn, 2: Info, 3: Debug */
	mtrans_log_func_t log_handler;              /* 日志输出回调 (NULL 则不输出 mtrans 日志) */
	const struct mtrans_mem_hooks *mem_hooks;   /* 内存分配钩子 (NULL 则使用标准 malloc/free) */
} mtrans_engine_config_t;

/**
 * @brief 媒体发送端配置参数
 */
typedef struct
{
	uint32_t ssrc;               /* 同步源标识符 (传 0 则系统自动随机生成) */
	uint32_t target_bitrate_bps; /* 预期目标码率 (用于计算 RTCP 5% 频率，及 Opus 动态调整) */
	bool tx_seq_log_enable;      /* 可选：发送端输出 RTP ssrc/seq/ts 日志（默认 false） */
} mtrans_media_tx_params_t;

/**
 * @brief 媒体接收端配置参数
 */
typedef struct
{
	uint32_t expected_ssrc;      /* 期望接收的 SSRC (传 0 则自动锁定第一个到达的合法流) */
	uint32_t jitter_buffer_ms;   /* 防抖动队列最大容忍延迟 (视频建议 200ms, 100ms音频建议 300ms) */
	uint32_t jitter_wallclock_timeout_ms; /* 可选：缺包场景下真实时间等待上限 (0 关闭) */
	uint32_t jitter_max_queue_packets;    /* 可选：队列积压上限触发快进 (0 关闭) */
} mtrans_media_rx_params_t;

/**
 * @brief 接收帧标志位 (由回调 flags 参数携带，可按位或组合)
 */
enum
{
	MTRANS_RX_FLAG_PACKET_LOST    = 0x0100, /* 该帧之前存在丢包 */
	MTRANS_RX_FLAG_PACKET_CORRUPT = 0x0200, /* 帧数据损坏 */
	MTRANS_RX_FLAG_END_OF_FRAME   = 0x0400, /* 帧结束标记 */
};

/**
 * @brief Channel 完整配置 (含 transport 自动管理)
 */
typedef struct
{
	mtrans_link_type_t link_type;
	size_t mtu;

	/* Transport 配置 */
	transport_proto_t proto;
	const char *interface;          /* 绑定网卡名 (UDP: "eth0", PLC: NULL) */
	transport_endpoint_t local_ep;  /* 本地绑定地址 (addr 通常填 ADDR_ANY) */
	transport_endpoint_t remote_ep; /* 远端目标地址 / 接收过滤地址 */
	bool rx_match_ep_enabled;       /* 可选扩展：接收匹配地址与发送目标解耦（默认 false） */
	transport_endpoint_t rx_match_ep;/* 可选扩展：启用后用于 recv 路由匹配 */
	bool mcast_enabled;             /* 可选扩展：是否启用组播生命周期控制 */
	transport_mcast_opt_t mcast_opt;/* 可选扩展：组播参数（主机字节序） */
	bool mcast_loop_disable;        /* 可选扩展：关闭本机组播回环（默认 false） */
	bool rtcp_sr_rr_disable;        /* 可选扩展：关闭 SR/RR 周期报告（默认 false） */
	bool rfc4571_video_marker_flush_disable; /* 可选扩展：禁用视频 marker 即时 flush（默认 false） */
	bool rfc4571_ts_switch_flush_disable; /* 可选扩展：禁用按 RTP timestamp 切换即时 flush（默认 false） */
	bool rfc4571_urgent_flush_disable; /* 可选扩展：禁用紧急包即时 flush（默认 false） */
	bool rfc4571_interval_flush_disable; /* 可选扩展：禁用周期 flush（默认 false） */
	uint32_t rfc4571_flush_interval_ms; /* 可选扩展：周期 flush 间隔（毫秒，0 表示默认 100ms） */
	bool rfc4571_flush_reason_log_enable; /* 可选扩展：启用 flush reason 日志（默认 false） */

	void *user_ctx;                 /* 透传给接收回调的用户上下文 (可选) */
	mtrans_link_profile_t link_profile; /* 可选扩展：链路画像，默认自动推断 */
} mtrans_channel_config_t;


/* ==========================================================
 * 不透明句柄定义 (Opaque Handles - 隐藏内部复杂实现)
 * ========================================================== */
typedef void *mtrans_channel_t;    /* 物理通道句柄 (Socket/网卡 的抽象) */
typedef void *mtrans_stream_tx_t;  /* 媒体发送流句柄 (包含 Muxer, RTP/RTCP 打包, 发送重传缓存) */
typedef void *mtrans_stream_rx_t;  /* 媒体接收流句柄 (包含 Demuxer, Jitter Buffer, 解码, NACK 生成) */


/* ==========================================================
 * 2. 核心引擎 API (Engine Core) - 所有设备通用
 * ========================================================== */

/**
 * @brief 初始化流媒体引擎 (进程启动时调用一次)
 */
int mtrans_engine_init(const mtrans_engine_config_t *config);

/**
 * @brief 销毁引擎，释放所有内部内存和线程
 */
void mtrans_engine_deinit(void);


/* ==========================================================
 * 3. 物理通道 API (Channel Layer)
 * ========================================================== */

/**
 * @brief 底层物理发送回调函数原型
 * @param ctx  用户上下文
 * @param data 准备好发送的物理层数据
 * @param len  数据长度
 */
typedef int (*mtrans_channel_tx_cb_t)(void *ctx, const uint8_t *data, size_t len);

/**
 * @brief 创建 channel 并自动管理 transport + recv 线程
 * @param config 通道配置
 * @return 通道句柄，失败返回 NULL
 *
 * @note 相同 proto + local_ep 的 channel 共享同一个 transport 和 recv 线程。
 *       recv 线程按 remote_ep 路由收到的数据到匹配的 channel。
 *       销毁时调用 mtrans_channel_destroy() 即可自动回收 transport 资源。
 */
mtrans_channel_t mtrans_channel_open(const mtrans_channel_config_t *config);

/**
 * @brief 创建一个传输通道 (手动 TX 回调模式，向后兼容)
 * @param type  链路封装/传输类型 (决定了底层是否开启 RFC 4571 聚合)
 * @param mtu   最大传输单元
 * @param tx_cb 绑定该通道的物理网卡发送接口
 * @param ctx   透传给 tx_cb 的用户上下文
 * @return 通道句柄，失败返回 NULL
 */
mtrans_channel_t mtrans_channel_create(mtrans_link_type_t type, size_t mtu,
                                 mtrans_channel_tx_cb_t tx_cb, void *ctx);

/**
 * @brief 销毁传输通道 (同时回收自动管理的 transport 资源)
 */
void mtrans_channel_destroy(mtrans_channel_t channel);

/**
 * @brief 【供底层网卡中断/recv 线程调用】将从网络收到的裸数据注入通道
 * @param channel 通道句柄
 * @param data    收到的字节流
 * @param len     数据长度
 *
 * @note 如果该通道绑定了接收流，底层会触发音视频就绪回调。
 * @note 如果该通道绑定了网关路由 (Bridge)，底层会自动转发给目标通道。
 */
int mtrans_channel_input_data(mtrans_channel_t channel, const uint8_t *data, size_t len);

/**
 * @brief 设置通道链路画像（用于 MTRANS_ROUTE_FLAG_REQUIRE_PROFILE_MATCH）
 */
int mtrans_channel_set_link_profile(mtrans_channel_t channel, mtrans_link_profile_t profile);


/* ==========================================================
 * 4. 媒体发送 API (Stream TX)
 * ========================================================== */

/**
 * @brief 发送端事件回调注册表（可选）
 */
typedef struct
{
	/* 流事件通知（如：收到 PLI/FIR，要求应用层尽快编码 I 帧） */
	void (*on_stream_event)(void *ctx, mtrans_stream_event_t event);

	/* RTCP 事件通知（NACK/PLI/FIR/RR/SR/AFB 等） */
	void (*on_rtcp_event)(void *ctx, const mtrans_rtcp_event_t *event);
} mtrans_stream_tx_callbacks_t;

/**
 * @brief 创建媒体发送流。
 *
 * 发送流绑定到指定 channel，并负责 RTP/RTCP 打包、发送和重传缓存管理。
 *
 * @param[in] channel 由 mtrans_channel_open() 或 mtrans_channel_create() 创建的通道句柄。
 * @param[in] media_type 媒体编码类型。
 * @param[in] params 发送端参数，不能为 NULL。
 *
 * @return 发送流句柄；失败返回 NULL。
 * @see mtrans_stream_tx_destroy
 */
mtrans_stream_tx_t mtrans_stream_tx_create(mtrans_channel_t channel,
                                     mtrans_media_type_t media_type,
                                     const mtrans_media_tx_params_t *params);

/**
 * @brief 销毁媒体发送流。
 *
 * 销毁后 stream 句柄不可继续使用；底层 channel 生命周期仍由调用方管理。
 *
 * @param[in] stream 发送流句柄，可为 NULL。
 */
void mtrans_stream_tx_destroy(mtrans_stream_tx_t stream);

/**
 * @brief 设置发送流回调（可选，不设置则保持旧行为）
 */
int mtrans_stream_tx_set_callbacks(mtrans_stream_tx_t stream,
                                   const mtrans_stream_tx_callbacks_t *cbs,
                                   void *ctx);

/**
 * @brief 输入采集并编码好的视频数据
 * @param stream 发送流句柄
 * @param data   数据
 * @param len    数据长度
 * @param ts 	 RTP 时间戳
 */
int mtrans_stream_tx_input_video(mtrans_stream_tx_t stream, const uint8_t *data, size_t len, uint32_t ts);

/**
 * @brief 输入采集并编码好的音频数据
 * @param stream 发送流句柄
 * @param data   数据
 * @param len    数据长度
 * @param ts     RTP 时间戳
 */
int mtrans_stream_tx_input_audio(mtrans_stream_tx_t stream, const uint8_t *data, size_t len, uint32_t ts);


/* ==========================================================
 * 5. 媒体接收 API (Stream RX)
 * ========================================================== */

/**
 * @brief 接收端数据就绪回调注册表
 */
typedef struct
{
	// 收到的视频画面帧
	void (*on_video_frame_ready)(void *ctx, const uint8_t *frame_data, size_t len, uint32_t ts, int flags);

	// 收到的音频
	void (*on_audio_frame_ready)(void *ctx, const uint8_t *frame_data, size_t len, uint32_t ts, int flags);

	// 流事件通知 (如要求应用层立刻编码 I 帧响应对端的 PLI)
	void (*on_stream_event)(void *ctx, mtrans_stream_event_t event);

	// RTCP 事件通知 (NACK/PLI/FIR/RR/SR/AFB 等)
	void (*on_rtcp_event)(void *ctx, const mtrans_rtcp_event_t *event);
} mtrans_stream_rx_callbacks_t;

/**
 * @brief 创建媒体接收流。
 *
 * 接收流绑定到指定 channel，负责从 RTP/RTCP 数据中恢复音视频帧并触发回调。
 *
 * @param[in] channel 由 mtrans_channel_open() 或 mtrans_channel_create() 创建的通道句柄。
 * @param[in] media_type 媒体编码类型。
 * @param[in] params 接收端参数，不能为 NULL。
 * @param[in] cbs 接收回调集合，可为 NULL。
 * @param[in] ctx 透传给回调的用户上下文，可为 NULL。
 *
 * @return 接收流句柄；失败返回 NULL。
 * @see mtrans_stream_rx_destroy
 */
mtrans_stream_rx_t mtrans_stream_rx_create(mtrans_channel_t channel,
                                     mtrans_media_type_t media_type,
                                     const mtrans_media_rx_params_t *params,
                                     const mtrans_stream_rx_callbacks_t *cbs,
                                     void *ctx);

/**
 * @brief 销毁媒体接收流。
 *
 * 销毁后 stream 句柄不可继续使用；底层 channel 生命周期仍由调用方管理。
 *
 * @param[in] stream 接收流句柄，可为 NULL。
 */
void mtrans_stream_rx_destroy(mtrans_stream_rx_t stream);

/**
 * @brief 接收端主动发送 PLI（请求发送端尽快出关键帧）
 * @return 0 成功；负值失败（如尚未锁定媒体 SSRC）
 */
int mtrans_stream_rx_send_pli(mtrans_stream_rx_t stream);

/**
 * @brief 接收端主动发送 FIR（强制请求关键帧）
 * @return 0 成功；负值失败（如尚未锁定媒体 SSRC）
 */
int mtrans_stream_rx_send_fir(mtrans_stream_rx_t stream);


/* ==========================================================
 * 6. 路由 API
 * ========================================================== */

/**
 * @brief 建立两条异构物理链路之间的“透明媒体桥接”
 * @param src_channel 数据来源通道 (如：挂载了 PLC 收发驱动的通道)
 * @param dst_channel 数据去向通道 (如：挂载了 UDP 收发驱动的通道)
 *
 */
int mtrans_route_bridge_channels(mtrans_channel_t src_channel, mtrans_channel_t dst_channel);

/**
 * @brief 解除桥接关系
 * @param src_channel 数据来源通道 (如：挂载了 PLC 收发驱动的通道)
 * @param dst_channel 数据去向通道 (如：挂载了 UDP 收发驱动的通道)
 */
int mtrans_route_unbridge_channels(mtrans_channel_t src_channel, mtrans_channel_t dst_channel);

/**
 * @brief 路由规则描述（兼容扩展，基于 channel 句柄）
 */
typedef struct
{
	mtrans_channel_t src_channel;
	mtrans_channel_t dst_channel;
	uint32_t route_group;   /* 路由分组标识，仅作为规则元数据与统计输出 */
	uint32_t flags;         /* 策略位 */
} mtrans_route_rule_t;

enum
{
	MTRANS_ROUTE_FLAG_REQUIRE_PROFILE_MATCH = 0x00000001, /* src/dst 链路画像需一致 */
	MTRANS_ROUTE_FLAG_MEDIA_VIDEO_ONLY = 0x00000002,      /* 仅转发视频 RTP，RTCP 透传 */
	MTRANS_ROUTE_FLAG_MEDIA_AUDIO_ONLY = 0x00000004,      /* 仅转发音频 RTP，RTCP 透传 */
};

/**
 * @brief 新增一条路由规则
 */
int mtrans_route_add_rule(const mtrans_route_rule_t *rule);

/**
 * @brief 删除一条路由规则
 */
int mtrans_route_del_rule(const mtrans_route_rule_t *rule);

/**
 * @brief 单条路由规则统计（src -> dst）
 */
typedef struct
{
	mtrans_channel_t src_channel;
	mtrans_channel_t dst_channel;
	uint32_t route_group;
	uint32_t flags;
	uint64_t forwarded_pkts;
	uint64_t forwarded_bytes;
	uint64_t dropped_send_fail_pkts;
	uint64_t dropped_send_fail_bytes;
	uint64_t dropped_policy_pkts;
	uint64_t dropped_policy_bytes;
	uint64_t dropped_policy_profile_pkts;
	uint64_t dropped_policy_profile_bytes;
} mtrans_route_rule_stats_t;

/**
 * @brief 获取指定规则（src->dst）的命中与失败统计
 */
int mtrans_route_get_rule_stats(mtrans_channel_t src_channel, mtrans_channel_t dst_channel,
                                mtrans_route_rule_stats_t *stats);


#ifdef __cplusplus
}
#endif

#endif /* __MTRANS_API_H__ */
