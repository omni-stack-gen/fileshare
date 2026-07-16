/**
 * @file call_proto.h
 * @brief 呼叫信令协议数据结构与编解码接口。
 */
#ifndef __CALL_PROTO_H__
#define __CALL_PROTO_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 协议魔数（ASCII: "CALL"）。 */
#define CALL_PROTO_MAGIC   0x43414C4CU /* "CALL" */
/** @brief 协议版本号。 */
#define CALL_PROTO_VERSION 1U

/** @brief 头部标记位：请求报文。 */
#define CALL_PROTO_FLAG_REQUEST      0x01U
/** @brief 头部标记位：响应报文。 */
#define CALL_PROTO_FLAG_RESPONSE     0x02U
/** @brief 头部标记位：请求需要 ACK。 */
#define CALL_PROTO_FLAG_ACK_REQUIRED 0x04U
/** @brief 头部标记位：当前报文为重传。 */
#define CALL_PROTO_FLAG_RETRANSMIT   0x08U

/**
 * @brief 呼叫协议消息类型。
 */
typedef enum
{
	/** @brief 呼叫邀请。 */
	CALL_MSG_CALL_INVITE       = 0x01,
	/** @brief 振铃通知（被叫业务进入待接听状态）。 */
	CALL_MSG_CALL_RING         = 0x02,
	/** @brief 呼叫接受。 */
	CALL_MSG_CALL_ACCEPT       = 0x03,
	/** @brief 呼叫拒绝。 */
	CALL_MSG_CALL_REJECT       = 0x04,
	/** @brief 呼叫挂断。 */
	CALL_MSG_CALL_BYE          = 0x05,
	/** @brief 监看发起。 */
	CALL_MSG_MONITOR_START     = 0x06,
	/** @brief 监看接受。 */
	CALL_MSG_MONITOR_ACCEPT    = 0x07,
	/** @brief 监看拒绝。 */
	CALL_MSG_MONITOR_REJECT    = 0x08,
	/** @brief 监看停止。 */
	CALL_MSG_MONITOR_STOP      = 0x09,
	/** @brief ACK 响应。 */
	CALL_MSG_ACK               = 0x0A,
	/** @brief ERROR 响应。 */
	CALL_MSG_ERROR             = 0x0B,
	/** @brief 心跳。 */
	CALL_MSG_HEARTBEAT         = 0x0C,
	/** @brief 会话状态通知。 */
	CALL_MSG_SESSION_STATE     = 0x0D,
	/** @brief 抢占通知。 */
	CALL_MSG_PREEMPT_NOTICE    = 0x0E,
} call_msg_type_t;

/**
 * @brief 协议结果码。
 */
typedef enum
{
	/** @brief 成功。 */
	CALL_RESULT_OK = 0,
	/** @brief 忙线。 */
	CALL_RESULT_BUSY = 1,
	/** @brief 超时。 */
	CALL_RESULT_TIMEOUT = 2,
	/** @brief 请求错误。 */
	CALL_RESULT_BAD_REQ = 3,
	/** @brief 不支持。 */
	CALL_RESULT_UNSUPPORTED = 4,
	/** @brief 状态冲突。 */
	CALL_RESULT_CONFLICT = 5,
	/** @brief 内部错误。 */
	CALL_RESULT_INTERNAL_ERR = 6,
} call_result_code_t;

/**
 * @brief 媒体协商模式。
 */
typedef enum
{
	/** @brief 通告模式。 */
	CALL_MEDIA_PROFILE_MODE_ANNOUNCE = 1,
	/** @brief Offer 模式。 */
	CALL_MEDIA_PROFILE_MODE_OFFER = 2,
	/** @brief Answer 模式。 */
	CALL_MEDIA_PROFILE_MODE_ANSWER = 3,
} call_media_profile_mode_t;

/**
 * @brief 媒体编解码类型。
 */
typedef enum
{
	/** @brief 未知编解码。 */
	CALL_MEDIA_CODEC_UNKNOWN = 0,
	/** @brief H.264 视频。 */
	CALL_MEDIA_CODEC_H264 = 1,
	/** @brief H.265 视频。 */
	CALL_MEDIA_CODEC_H265 = 2,
	/** @brief Opus 音频。 */
	CALL_MEDIA_CODEC_OPUS = 10,
	/** @brief G.711 A-law 音频。 */
	CALL_MEDIA_CODEC_G711A = 11,
	/** @brief G.711 u-law 音频。 */
	CALL_MEDIA_CODEC_G711U = 12,
} call_media_codec_t;

/**
 * @brief 会话状态通知类型。
 */
typedef enum
{
	/** @brief 会话因超时而结束。 */
	CALL_SESSION_STATE_TIMEOUT = 1,
} call_session_state_t;

/**
 * @brief 媒体状态快照。
 */
typedef enum
{
	/** @brief 媒体已停止。 */
	CALL_MEDIA_STATE_STOPPED = 1,
	/** @brief 媒体启动中。 */
	CALL_MEDIA_STATE_STARTING = 2,
	/** @brief 媒体已激活。 */
	CALL_MEDIA_STATE_ACTIVE = 3,
	/** @brief 媒体停止中。 */
	CALL_MEDIA_STATE_STOPPING = 4,
} call_media_state_t;

/**
 * @brief 监视源类型。
 */
typedef enum
{
	/** @brief 未声明监视源类型。 */
	CALL_MONITOR_SOURCE_UNKNOWN = 0,
	/** @brief 监视单元机 / 门口机本机视频源。 */
	CALL_MONITOR_SOURCE_UNIT_MACHINE = 1,
	/** @brief 监视二次确认机视频源。 */
	CALL_MONITOR_SOURCE_SECONDARY_CONFIRM = 2,
	/** @brief 由业务节点代理监视公区 IPC / RTSP 源。 */
	CALL_MONITOR_SOURCE_PUBLIC_IPC = 3,
} call_monitor_source_type_t;

/**
 * @brief 媒体方向。
 */
typedef enum
{
	/** @brief 未声明方向；V2 活跃媒体校验不接受该值。 */
	CALL_MEDIA_DIR_UNKNOWN = 0,
	/** @brief 本侧发送并接收。 */
	CALL_MEDIA_DIR_SENDRECV = 1,
	/** @brief 本侧只发送。 */
	CALL_MEDIA_DIR_SENDONLY = 2,
	/** @brief 本侧只接收。 */
	CALL_MEDIA_DIR_RECVONLY = 3,
	/** @brief 本侧不收不发。 */
	CALL_MEDIA_DIR_INACTIVE = 4,
} call_media_dir_t;

/**
 * @brief 媒体掩码、标志及固定长度常量。
 */
enum
{
	/** @brief 媒体掩码：视频。 */
	CALL_MEDIA_MASK_VIDEO = 0x01,
	/** @brief 媒体掩码：音频。 */
	CALL_MEDIA_MASK_AUDIO = 0x02,
	/** @brief 媒体 profile 标志：RTCP 复用。 */
	CALL_MEDIA_PROFILE_FLAG_RTCP_MUX = 0x01,
	/** @brief 设备 `device_id` 最大长度（含尾零空间）。 */
	CALL_DEVICE_ID_MAX = 96,
	/** @brief 公区 IPC IP 字符串最大长度（含尾零空间）。 */
	CALL_PAYLOAD_IPC_IP_MAX = 64,
	/** @brief RTSP URL 最大长度（含尾零空间）。 */
	CALL_PAYLOAD_RTSP_URL_MAX = 256,
	/** @brief 公区 IPC 用户名 / 密码最大长度（含尾零空间）。 */
	CALL_PAYLOAD_IPC_CREDENTIAL_MAX = 64,
};

/**
 * @brief 媒体 endpoint 链路类型。
 */
typedef enum
{
	/** @brief 未声明链路类型。 */
	CALL_MEDIA_ENDPOINT_LINK_NONE = 0,
	/** @brief UDP 链路。 */
	CALL_MEDIA_ENDPOINT_LINK_UDP = 1,
	/** @brief PLC 链路。 */
	CALL_MEDIA_ENDPOINT_LINK_PLC = 2,
} call_media_endpoint_link_kind_t;

/**
 * @brief 媒体 endpoint 单播/组播模式。
 */
typedef enum
{
	/** @brief 未声明模式。 */
	CALL_MEDIA_CAST_NONE = 0,
	/** @brief 单播 endpoint。 */
	CALL_MEDIA_CAST_UNICAST = 1,
	/** @brief 组播 endpoint。 */
	CALL_MEDIA_CAST_MULTICAST = 2,
} call_media_cast_mode_t;

/**
 * @brief 建链前 dry-run 决策中的 endpoint 来源。
 */
typedef enum
{
	/** @brief 未选择 endpoint。 */
	CALL_MEDIA_ENDPOINT_SOURCE_NONE = 0,
	/** @brief 来自 topology/backend profile。 */
	CALL_MEDIA_ENDPOINT_SOURCE_TOPOLOGY = 1,
	/** @brief 来自业务层显式配置。 */
	CALL_MEDIA_ENDPOINT_SOURCE_BUSINESS = 2,
	/** @brief 来自静态 fallback 配置。 */
	CALL_MEDIA_ENDPOINT_SOURCE_STATIC = 3,
	/** @brief 来自本次 session 动态分配。 */
	CALL_MEDIA_ENDPOINT_SOURCE_SESSION = 4,
} call_media_endpoint_source_t;

/**
 * @brief 单路媒体 endpoint。
 */
typedef struct
{
	/** @brief 链路类型，见 `call_media_endpoint_link_kind_t`。 */
	uint8_t link_kind;
	/** @brief 单播/组播模式，见 `call_media_cast_mode_t`。 */
	uint8_t cast_mode;
	/** @brief endpoint 来源，见 `call_media_endpoint_source_t`。 */
	uint8_t source;
	/** @brief endpoint 地址，主机字节序。 */
	uint32_t addr;
	/** @brief endpoint 端口。 */
	uint16_t port;
} call_media_endpoint_t;

/**
 * @brief 媒体参数描述。
 */
typedef struct
{
	/** @brief 协商模式，见 `call_media_profile_mode_t`。 */
	uint8_t mode;
	/** @brief 媒体掩码，见 `CALL_MEDIA_MASK_*`。 */
	uint8_t media_mask;
	/** @brief profile 标志位。 */
	uint8_t flags;
	/** @brief 视频方向，见 `call_media_dir_t`。 */
	uint8_t video_dir;
	/** @brief 音频方向，见 `call_media_dir_t`。 */
	uint8_t audio_dir;
	/** @brief 视频编解码类型。 */
	uint8_t video_codec;
	/** @brief 视频 payload type。 */
	uint8_t video_pt;
	/** @brief 视频宽度。 */
	uint16_t video_width;
	/** @brief 视频高度。 */
	uint16_t video_height;
	/** @brief 视频帧率。 */
	uint16_t video_fps;
	/** @brief 视频码率（kbps）。 */
	uint16_t video_bitrate_kbps;
	/** @brief `video_endpoint` 是否存在。 */
	bool has_video_endpoint;
	/** @brief 视频 endpoint。 */
	call_media_endpoint_t video_endpoint;
	/** @brief 音频编解码类型。 */
	uint8_t audio_codec;
	/** @brief 音频 payload type。 */
	uint8_t audio_pt;
	/** @brief 音频采样率。 */
	uint16_t audio_sample_rate;
	/** @brief 音频声道数。 */
	uint8_t audio_channels;
	/** @brief 音频 ptime（毫秒）。 */
	uint8_t audio_ptime_ms;
	/** @brief 音频码率（kbps）。 */
	uint16_t audio_bitrate_kbps;
	/** @brief `audio_endpoint` 是否存在。 */
	bool has_audio_endpoint;
	/** @brief 音频 endpoint。 */
	call_media_endpoint_t audio_endpoint;
	/** @brief 视频 SSRC。 */
	uint32_t video_ssrc;
	/** @brief 音频 SSRC。 */
	uint32_t audio_ssrc;
} call_media_profile_t;

/**
 * @brief 建链前 dry-run 决策输出。
 */
typedef struct
{
	/** @brief 是否启用视频。 */
	bool video_enabled;
	/** @brief 是否启用音频。 */
	bool audio_enabled;
	/** @brief 视频 endpoint 来源，见 `call_media_endpoint_source_t`。 */
	uint8_t video_source;
	/** @brief 音频 endpoint 来源，见 `call_media_endpoint_source_t`。 */
	uint8_t audio_source;
	/** @brief 视频方向。 */
	uint8_t video_dir;
	/** @brief 音频方向。 */
	uint8_t audio_dir;
	/** @brief 视频 codec。 */
	uint8_t video_codec;
	/** @brief 音频 codec。 */
	uint8_t audio_codec;
	/** @brief 视频 payload type。 */
	uint8_t video_pt;
	/** @brief 音频 payload type。 */
	uint8_t audio_pt;
	/** @brief 视频 endpoint。 */
	call_media_endpoint_t video_endpoint;
	/** @brief 音频 endpoint。 */
	call_media_endpoint_t audio_endpoint;
} call_media_link_decision_t;

/**
 * @brief 协议头部结构。
 */
typedef struct
{
	/** @brief 魔数，固定为 `CALL_PROTO_MAGIC`。 */
	uint32_t magic;
	/** @brief 版本号，固定为 `CALL_PROTO_VERSION`。 */
	uint8_t version;
	/** @brief 消息类型。 */
	uint8_t msg_type;
	/** @brief 标记位，见 `CALL_PROTO_FLAG_*`。 */
	uint8_t flags;
	/** @brief 保留字段。 */
	uint8_t reserved;
	/** @brief 事务 ID。 */
	uint32_t txn_id;
	/** @brief 会话 ID。 */
	uint64_t session_id;
	/** @brief 序列号/重传计数。 */
	uint16_t seq;
	/** @brief 负载长度（字节）。 */
	uint16_t body_len;
} call_msg_header_t;

/**
 * @brief 协议 JSON 负载结构。
 */
typedef struct
{
	/** @brief `caller_device_id` 是否存在。 */
	bool has_caller_device_id;
	/** @brief 主叫设备 `device_id`。 */
	char caller_device_id[CALL_DEVICE_ID_MAX];
	/** @brief `callee_device_id` 是否存在。 */
	bool has_callee_device_id;
	/** @brief 被叫设备 `device_id`。 */
	char callee_device_id[CALL_DEVICE_ID_MAX];
	/** @brief `result_code` 是否存在。 */
	bool has_result_code;
	/** @brief 结果码。 */
	uint8_t result_code;
	/** @brief `reason_code` 是否存在。 */
	bool has_reason_code;
	/** @brief 原因码。 */
	uint32_t reason_code;
	/** @brief `sender_device_id` 是否存在。 */
	bool has_sender_device_id;
	/** @brief 当前发包节点 `device_id`。 */
	char sender_device_id[CALL_DEVICE_ID_MAX];
	/** @brief `ref_msg_type` 是否存在。 */
	bool has_ref_msg_type;
	/** @brief 响应所引用的原请求消息类型。 */
	uint8_t ref_msg_type;
	/** @brief `session_state` 是否存在。 */
	bool has_session_state;
	/** @brief 会话状态。 */
	uint8_t session_state;
	/** @brief `media_state` 是否存在。 */
	bool has_media_state;
	/** @brief 媒体状态。 */
	uint8_t media_state;
	/** @brief `media_profile` 是否存在。 */
	bool has_media_profile;
	/** @brief 媒体参数。 */
	call_media_profile_t media_profile;
	/** @brief `monitor_source_type` 是否存在；当前仅 `MONITOR_START` 可携带。 */
	bool has_monitor_source_type;
	/** @brief 监视源类型，见 `call_monitor_source_type_t`。 */
	uint8_t monitor_source_type;
	/** @brief `ipc_ip` 是否存在；仅公区 IPC 监视时可携带。 */
	bool has_ipc_ip;
	/** @brief 公区 IPC 点分十进制 IP 字符串。 */
	char ipc_ip[CALL_PAYLOAD_IPC_IP_MAX];
	/** @brief `rtsp_url` 是否存在；仅公区 IPC 监视时可携带。 */
	bool has_rtsp_url;
	/** @brief 公区 IPC RTSP URL。 */
	char rtsp_url[CALL_PAYLOAD_RTSP_URL_MAX];
	/** @brief `ipc_username` 是否存在；仅公区 IPC 监视时可携带。 */
	bool has_ipc_username;
	/** @brief 公区 IPC 访问用户名，可用于 ONVIF 或直接 RTSP 访问。 */
	char ipc_username[CALL_PAYLOAD_IPC_CREDENTIAL_MAX];
	/** @brief `ipc_password` 是否存在；仅公区 IPC 监视时可携带。 */
	bool has_ipc_password;
	/** @brief 公区 IPC 访问密码，可用于 ONVIF 或直接 RTSP 访问。 */
	char ipc_password[CALL_PAYLOAD_IPC_CREDENTIAL_MAX];
} call_payload_t;

/**
 * @brief 协议固定头部长度常量。
 */
enum
{
	/** @brief 固定头部长度（字节）。 */
	CALL_PROTO_HEADER_SIZE = 24,
};

/**
 * @brief 判断消息类型是否受支持。
 *
 * @param msg_type 消息类型值。
 * @return 支持返回 `true`，否则返回 `false`。
 */
bool call_proto_msg_type_supported(uint8_t msg_type);

/**
 * @brief 获取消息类型名称字符串。
 *
 * @param msg_type 消息类型值。
 * @return 名称字符串指针；未知类型返回实现定义字符串。
 */
const char *call_proto_msg_type_name(uint8_t msg_type);

/**
 * @brief 获取媒体状态名称字符串。
 *
 * @param media_state 媒体状态值。
 * @return 名称字符串指针；未知状态返回实现定义字符串。
 */
const char *call_proto_media_state_name(uint8_t media_state);

/**
 * @brief 获取媒体方向名称字符串。
 *
 * @param media_dir 媒体方向值。
 * @return 名称字符串指针；未知方向返回实现定义字符串。
 */
const char *call_proto_media_dir_name(uint8_t media_dir);

/**
 * @brief 获取媒体 profile 模式名称字符串。
 *
 * @param mode 媒体 profile 模式值。
 * @return 名称字符串指针；未知模式返回实现定义字符串。
 */
const char *call_proto_media_profile_mode_name(uint8_t mode);

/**
 * @brief 获取媒体 codec 名称字符串。
 *
 * @param codec 媒体 codec 值。
 * @return 名称字符串指针；未知 codec 返回实现定义字符串。
 */
const char *call_proto_media_codec_name(uint8_t codec);

/**
 * @brief 获取监视源类型名称字符串。
 *
 * @param source_type 监视源类型值。
 * @return 名称字符串指针；未知类型返回实现定义字符串。
 */
const char *call_proto_monitor_source_type_name(uint8_t source_type);

/**
 * @brief 根据名称解析监视源类型。
 *
 * @param name 输入名称字符串，不可为 `NULL`。
 * @param source_type_out 输出监视源类型，不可为 `NULL`。
 * @retval 0 解析成功。
 * @retval -EINVAL 参数非法或名称不支持。
 */
int call_proto_monitor_source_type_from_name(const char *name, uint8_t *source_type_out);

/**
 * @brief 判断消息是否强制携带 `media_profile`。
 *
 * @param msg_type 消息类型值。
 * @return 需要返回 `true`；否则返回 `false`。
 */
bool call_proto_msg_requires_media_profile(uint8_t msg_type);

/**
 * @brief 获取消息对应的 `media_profile.mode`。
 *
 * @param msg_type 消息类型值。
 * @param mode_out 输出模式，不可为 `NULL`。
 * @return 成功返回 `true`；否则返回 `false`。
 */
bool call_proto_media_profile_mode_for_msg(uint8_t msg_type, uint8_t *mode_out);

/**
 * @brief 获取消息对应的默认媒体方向。
 *
 * @param msg_type 消息类型值。
 * @param media_mask 媒体掩码。
 * @param video_dir_out 视频方向输出，可为 `NULL`。
 * @param audio_dir_out 音频方向输出，可为 `NULL`。
 * @return 成功返回 `true`；消息无固定方向语义返回 `false`。
 */
bool call_proto_media_profile_dirs_for_msg(uint8_t msg_type, uint8_t media_mask,
                                           uint8_t *video_dir_out, uint8_t *audio_dir_out);

/**
 * @brief 获取消息对应的默认必需媒体掩码。
 *
 * @param msg_type 消息类型值。
 * @return 默认媒体掩码；无固定要求返回 `0`。
 */
uint8_t call_proto_default_media_mask_for_msg(uint8_t msg_type);

/**
 * @brief 根据 `media_profile` 生成建链前 dry-run 决策。
 *
 * 该接口只消费 `media_profile.video_endpoint/audio_endpoint` 中已经声明的建链输入，
 * 不读取 observation SSRC，不启动媒体，不申请资源，不修改 route 或 endpoint provider 状态。
 *
 * @param msg_type 当前消息类型。
 * @param profile 当前消息携带的媒体 profile，不可为 `NULL`。
 * @param decision_out 输出建链决策，不可为 `NULL`。
 * @retval 0 决策成功。
 * @retval -EINVAL 参数非法或 `media_profile` 与消息语义不匹配。
 * @retval -ENOENT 活跃媒体缺少可用 endpoint。
 */
int call_proto_media_profile_decide_link(uint8_t msg_type,
                                         const call_media_profile_t *profile,
                                         call_media_link_decision_t *decision_out);

/**
 * @brief 根据名称解析消息类型。
 *
 * @param name 输入名称字符串，不可为 `NULL`。
 * @param msg_type_out 输出消息类型，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负值错误码。
 */
int call_proto_msg_type_from_name(const char *name, uint8_t *msg_type_out);

/**
 * @brief 判断消息是否触发视频启动流程。
 *
 * @param msg_type 消息类型值。
 * @return 触发返回 `true`；否则返回 `false`。
 */
bool call_proto_is_video_start_trigger(uint8_t msg_type);

/**
 * @brief 判断消息是否触发音频启用流程。
 *
 * @param msg_type 消息类型值。
 * @return 触发返回 `true`；否则返回 `false`。
 */
bool call_proto_is_audio_enable_trigger(uint8_t msg_type);

/**
 * @brief 判断消息是否触发停止流程。
 *
 * @param msg_type 消息类型值。
 * @return 触发返回 `true`；否则返回 `false`。
 */
bool call_proto_is_stop_trigger(uint8_t msg_type);

/**
 * @brief 判断消息类型是否建议打印常规控制面日志。
 *
 * @param msg_type 消息类型值。
 * @return 建议打印返回 `true`；建议静默返回 `false`。
 */
bool call_proto_should_log_msg(uint8_t msg_type);

/**
 * @brief 判断 CLI 指定消息是否需要启动视频。
 *
 * @param msg_type 消息类型值。
 * @return 需要返回 `true`；否则返回 `false`。
 */
bool call_proto_cli_msg_requires_video_start(uint8_t msg_type);

/**
 * @brief 判断 CLI 指定消息是否需要启用音频。
 *
 * @param msg_type 消息类型值。
 * @return 需要返回 `true`；否则返回 `false`。
 */
bool call_proto_cli_msg_requires_audio_enable(uint8_t msg_type);

/**
 * @brief 将头部与负载编码为协议二进制报文。
 *
 * @param header 输入头部，不可为 `NULL`。
 * @param payload 输入负载，可为 `NULL`（表示空负载）。
 * @param out 输出缓冲区，不可为 `NULL`。
 * @param out_cap @p out 缓冲区容量（字节）。
 * @param out_len 输出报文长度，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负值错误码（如参数错误或缓冲区不足）。
 */
int call_proto_encode(const call_msg_header_t *header,
                      const call_payload_t *payload,
                      uint8_t *out, size_t out_cap, size_t *out_len);

/**
 * @brief 将头部与负载编码为协议二进制报文，但不输出成功的 TX 报文日志。
 *
 * @param header 输入头部，不可为 `NULL`。
 * @param payload 输入负载，可为 `NULL`（表示空负载）。
 * @param out 输出缓冲区，不可为 `NULL`。
 * @param out_cap @p out 缓冲区容量（字节）。
 * @param out_len 输出报文长度，不可为 `NULL`。
 * @return 成功返回 `0`；失败返回负值错误码（如参数错误或缓冲区不足）。
 */
int call_proto_encode_silent(const call_msg_header_t *header,
                             const call_payload_t *payload,
                             uint8_t *out, size_t out_cap, size_t *out_len);

/**
 * @brief 将协议二进制报文解码为头部与负载结构。
 *
 * @param data 输入报文缓冲区，不可为 `NULL`。
 * @param data_len @p data 缓冲区长度（字节）。
 * @param header_out 输出头部，不可为 `NULL`。
 * @param payload_out 输出负载，不可为 `NULL`。
 * @param unknown_key_count 输出未知 JSON 字段数量，可为 `NULL`。
 * @return 成功返回 `0`；失败返回负值错误码。
 */
int call_proto_decode(const uint8_t *data, size_t data_len,
                      call_msg_header_t *header_out,
                      call_payload_t *payload_out,
                      size_t *unknown_key_count);

/**
 * @brief 将协议二进制报文静默解码为头部与负载结构（不输出成功的 RX 报文日志）。
 *
 * @param data 输入报文缓冲区，不可为 `NULL`。
 * @param data_len @p data 缓冲区长度（字节）。
 * @param header_out 输出头部，不可为 `NULL`。
 * @param payload_out 输出负载，不可为 `NULL`。
 * @param unknown_key_count 输出未知 JSON 字段数量，可为 `NULL`。
 * @return 成功返回 `0`；失败返回负值错误码。
 */
int call_proto_decode_silent(const uint8_t *data, size_t data_len,
                             call_msg_header_t *header_out,
                             call_payload_t *payload_out,
                             size_t *unknown_key_count);

#ifdef __cplusplus
}
#endif

#endif
