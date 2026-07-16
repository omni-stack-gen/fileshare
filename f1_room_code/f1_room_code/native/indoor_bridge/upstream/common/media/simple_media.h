/**
 * @file
 * @brief simple 示例应用公共模块 simple_media 的公共头文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_MEDIA_H__
#define __SIMPLE_MEDIA_H__

#include <stdbool.h>
#include <stdint.h>

#include "call_proto.h"
#include "mtrans.h"
#include "simple_codec.h"
#include "simple_plc_ports.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief simple UDP 媒体组播地址，网络字节序整数。 */
#define SIMPLE_UDP_MEDIA_GROUP_ADDR (0xEF0A0A0AU)
/** @brief simple UDP 媒体组播地址字符串。 */
#define SIMPLE_UDP_MEDIA_GROUP      "239.10.10.10"
/** @brief simple UDP 媒体默认端口。 */
#define SIMPLE_UDP_MEDIA_PORT       (10000U)
/** @brief simple PLC 媒体组播地址。 */
#define SIMPLE_PLC_MEDIA_GROUP_ADDR (0x120001U)
/** @brief simple PLC 媒体默认端口。 */
#define SIMPLE_PLC_MEDIA_PORT       SIMPLE_PLC_PORT_MEDIA
/** @brief simple UDP RTP MTU，单位字节。 */
#define SIMPLE_UDP_RTP_MTU          (980U)
/** @brief simple PLC RTP MTU，单位字节。 */
#define SIMPLE_PLC_RTP_MTU          (2024U)

/**
 * @brief simple media endpoint 工作模式。
 */
typedef enum
{
    /** 不打开媒体 endpoint。 */
    SIMPLE_MEDIA_ENDPOINT_NONE = 0,
    /** UDP 发送 endpoint。 */
    SIMPLE_MEDIA_ENDPOINT_UDP_TX,
    /** UDP 接收 endpoint。 */
    SIMPLE_MEDIA_ENDPOINT_UDP_RX,
    /** PLC 发送 endpoint。 */
    SIMPLE_MEDIA_ENDPOINT_PLC_TX,
    /** PLC 接收 endpoint。 */
    SIMPLE_MEDIA_ENDPOINT_PLC_RX,
    /** UDP 到 PLC bridge。 */
    SIMPLE_MEDIA_ENDPOINT_UDP_TO_PLC_BRIDGE,
    /** PLC 到 UDP bridge。 */
    SIMPLE_MEDIA_ENDPOINT_PLC_TO_UDP_BRIDGE,
} simple_media_endpoint_type_t;

/**
 * @brief simple media 打开配置。
 */
typedef struct
{
    simple_media_endpoint_type_t type;
    const char *udp_interface;
    uint32_t udp_local_addr;
    uint32_t udp_remote_addr;
    uint32_t udp_remote_port;
    bool udp_mcast_enabled;
    uint32_t udp_mcast_group_addr;
    bool udp_mcast_loop_disable;
    bool udp_rx_match_ep_enabled;
    uint32_t udp_rx_match_addr;
    uint32_t udp_rx_match_port;
    uint32_t plc_local_addr;
    uint32_t plc_remote_addr;
    uint32_t plc_remote_port;
    bool plc_rx_match_ep_enabled;
    uint32_t plc_rx_match_addr;
    uint32_t plc_rx_match_port;
    bool plc_mcast_enabled;
    uint32_t plc_mcast_group_addr;
    bool audio_tx_remote_ep_enabled;
    uint32_t audio_tx_remote_addr;
    uint32_t audio_tx_remote_port;
    bool bridge_split_plc_to_udp_enabled;
    bool bridge_audio_only_enabled;
    uint32_t bridge_video_plc_remote_addr;
    uint32_t bridge_video_plc_remote_port;
    uint32_t bridge_audio_udp_remote_addr;
    uint32_t bridge_audio_udp_remote_port;
    bool bidirectional_bridge;
    bool video_tx_enabled;
    bool video_rx_enabled;
    bool audio_tx_enabled;
    bool audio_rx_enabled;
    uint32_t video_ssrc;
    uint32_t video_rx_ssrc;
    uint32_t audio_ssrc;
    uint32_t audio_rx_ssrc;
    const simple_codec_ops_t *codec_ops;
} simple_media_config_t;

/**
 * @brief simple media 运行态。
 *
 * 调用方持有该对象，simple_media_open()/close() 负责内部 channel/stream 生命周期。
 */
typedef struct
{
    simple_media_config_t base_cfg;
    bool configured;
    bool channels_open;
    bool channel_cfg_valid;
    simple_media_config_t channel_cfg;
    bool audio_tx_channel_cfg_valid;
    simple_media_config_t audio_tx_channel_cfg;
    const simple_codec_ops_t *active_video_codec_ops;
    const simple_codec_ops_t *active_audio_codec_ops;
    mtrans_channel_t udp_ch;
    mtrans_channel_t plc_ch;
    mtrans_channel_t audio_tx_ch;
    mtrans_channel_t bridge_video_plc_ch;
    mtrans_channel_t bridge_audio_udp_ch;
    mtrans_stream_tx_t video_tx;
    mtrans_stream_rx_t video_rx;
    mtrans_stream_tx_t audio_tx;
    mtrans_stream_rx_t audio_rx;
} simple_media_t;

/** @brief endpoint provider 调用阶段。 */
typedef enum
{
    /** @brief 构建即将发送的 CALL media_profile。 */
    SIMPLE_MEDIA_ENDPOINT_STAGE_TX_PROFILE = 1,
    /** @brief 根据 media_profile 决策结果启动本地媒体前。 */
    SIMPLE_MEDIA_ENDPOINT_STAGE_MEDIA_START = 2,
} simple_media_endpoint_stage_t;

/**
 * @brief simple call endpoint provider 输入。
 */
typedef struct
{
    /** 当前调用阶段。 */
    simple_media_endpoint_stage_t stage;
    /** 当前 CALL 消息类型。 */
    uint8_t msg_type;
    /** 当前默认/收到的 media_profile。 */
    const call_media_profile_t *profile;
    /** 最近收到的对端 media_profile；没有时为 NULL。 */
    const call_media_profile_t *peer_profile;
    /** peer_profile 存储，provider 不应直接写入。 */
    call_media_profile_t peer_profile_storage;
    /** 当前设备 device_id。 */
    char self_device_id[CALL_DEVICE_ID_MAX];
    /** 对端 numeric peer id。 */
    uint32_t peer_id;
    /** caller device_id。 */
    char caller_device_id[CALL_DEVICE_ID_MAX];
    /** callee device_id。 */
    char callee_device_id[CALL_DEVICE_ID_MAX];
    /** 当前 session id。 */
    uint64_t session_id;
} simple_media_endpoint_request_t;

/**
 * @brief 单路媒体 endpoint provider 输出。
 */
typedef struct
{
    /** 是否提供写入 CALL wire 的本机声明 endpoint。 */
    bool has_declared_endpoint;
    /** 写入 CALL wire 的本机声明 endpoint。 */
    call_media_endpoint_t declared_endpoint;
    /** 是否提供本地 TX 目标 endpoint。 */
    bool has_tx_remote_endpoint;
    /** 本地 TX 目标 endpoint。 */
    call_media_endpoint_t tx_remote_endpoint;
    /** 是否提供本地 RX 匹配 endpoint。 */
    bool has_rx_match_endpoint;
    /** 本地 RX 匹配地址；允许为 ADDR_ANY。 */
    uint32_t rx_match_addr;
    /** 本地 RX 匹配端口。 */
    uint16_t rx_match_port;
} simple_media_endpoint_binding_t;

/**
 * @brief video/audio endpoint provider 输出。
 */
typedef struct
{
    simple_media_endpoint_binding_t video;
    simple_media_endpoint_binding_t audio;
} simple_media_endpoint_result_t;

/** @brief simple call endpoint provider 回调。 */
typedef int (*simple_media_endpoint_provider_t)(
    void *user_ctx,
    const simple_media_endpoint_request_t *request,
    simple_media_endpoint_result_t *result);

/**
 * @brief call app 使用的媒体操作集合。
 */
typedef struct
{
    /** 启动视频。 */
    int (*start_video)(void *user_ctx);
    /** 停止视频。 */
    void (*stop_video)(void *user_ctx);
    /** 启动视频前应用本地 endpoint。 */
    int (*apply_video_endpoint)(void *user_ctx,
                                const simple_media_endpoint_binding_t *binding);
    /** 启动音频前应用本地 endpoint。 */
    int (*apply_audio_endpoint)(void *user_ctx,
                                const simple_media_endpoint_binding_t *binding);
    /** 启动音频。 */
    int (*start_audio)(void *user_ctx);
    /** 停止音频。 */
    void (*stop_audio)(void *user_ctx);
    /** 传给上述回调的用户上下文。 */
    void *user_ctx;
} simple_media_ops_t;

/**
 * @brief 填充 simple media 默认配置。
 *
 * @param[out] cfg 配置对象，不能为 NULL。
 * @param[in] type endpoint 工作模式。
 */
void simple_media_default_config(simple_media_config_t *cfg,
                                 simple_media_endpoint_type_t type);
/**
 * @brief 打开 simple media channel/stream。
 *
 * @param[in,out] media media 对象，不能为 NULL。
 * @param[in] cfg 打开配置，不能为 NULL。
 * @retval 0 打开成功。
 * @retval -EINVAL 参数非法。
 */
int simple_media_open(simple_media_t *media, const simple_media_config_t *cfg);
/** @brief 关闭 simple media 并释放内部 channel/stream。 */
void simple_media_close(simple_media_t *media);
/** @brief 使用显式执行配置启动 bridge channel/route。 */
int simple_media_start_bridge_with_config(simple_media_t *media,
                                          const simple_media_config_t *cfg);
/** @brief 显式停止 bridge channel/route，保留当前配置。 */
int simple_media_stop_bridge(simple_media_t *media);
/** @brief 使用显式执行配置启动视频 stream。 */
int simple_media_start_video_with_config(simple_media_t *media,
                                         const simple_media_config_t *cfg);
/** @brief 停止视频 stream。 */
void simple_media_stop_video(simple_media_t *media);
/** @brief 使用显式执行配置启动音频 stream。 */
int simple_media_start_audio_with_config(simple_media_t *media,
                                         const simple_media_config_t *cfg);
/** @brief 停止音频 stream。 */
void simple_media_stop_audio(simple_media_t *media);

#ifdef __cplusplus
}
#endif

#endif
