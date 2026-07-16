/**
 * @file
 * @brief simple 示例应用公共模块 simple_codec 的公共头文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#ifndef __SIMPLE_CODEC_H__
#define __SIMPLE_CODEC_H__

#include <stddef.h>
#include <stdint.h>

#include "mtrans.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief simple 媒体编解码回调集合。
 *
 * 所有回调均由 simple 媒体路径在对应 TX/RX 启停或收到帧时调用；
 * 回调指针可为 NULL，表示该方向没有真实编解码实现。
 */
typedef struct
{
    /** 启动视频编码器。 */
    int (*start_video_encoder)(void *user_ctx);
    /** 停止视频编码器。 */
    void (*stop_video_encoder)(void *user_ctx);
    /** 启动视频解码器。 */
    int (*start_video_decoder)(void *user_ctx);
    /** 停止视频解码器。 */
    void (*stop_video_decoder)(void *user_ctx);
    /** 输入一帧视频码流；len 单位为字节。 */
    int (*input_video_frame)(void *user_ctx, const uint8_t *frame,
                             size_t len, uint32_t ts, int flags);
    /** 接收 mtrans stream 事件。 */
    void (*on_stream_event)(void *user_ctx, mtrans_stream_event_t event);
    /** 启动音频编码器。 */
    int (*start_audio_encoder)(void *user_ctx);
    /** 停止音频编码器。 */
    void (*stop_audio_encoder)(void *user_ctx);
    /** 启动音频解码器。 */
    int (*start_audio_decoder)(void *user_ctx);
    /** 停止音频解码器。 */
    void (*stop_audio_decoder)(void *user_ctx);
    /** 输入一帧音频码流；len 单位为字节。 */
    int (*input_audio_frame)(void *user_ctx, const uint8_t *frame,
                             size_t len, uint32_t ts, int flags);
    /** 透传给上述回调的用户上下文，调用方保持所有权。 */
    void *user_ctx;
} simple_codec_ops_t;

/** @brief 调用视频 TX 启动回调。 */
int simple_codec_start_video_tx(const simple_codec_ops_t *ops);
/** @brief 调用视频 TX 停止回调。 */
void simple_codec_stop_video_tx(const simple_codec_ops_t *ops);
/** @brief 调用视频 RX 启动回调。 */
int simple_codec_start_video_rx(const simple_codec_ops_t *ops);
/** @brief 调用视频 RX 停止回调。 */
void simple_codec_stop_video_rx(const simple_codec_ops_t *ops);
/**
 * @brief 向视频解码回调输入一帧码流。
 *
 * @param[in] ops 编解码回调集合，可为 NULL。
 * @param[in] frame 帧数据指针；len 为 0 时可为 NULL。
 * @param[in] len 帧数据长度，单位为字节。
 * @param[in] ts RTP 时间戳。
 * @param[in] flags mtrans 帧标记。
 * @return 回调返回值；无回调时返回 0。
 */
int simple_codec_input_video(const simple_codec_ops_t *ops,
                             const uint8_t *frame,
                             size_t len,
                             uint32_t ts,
                             int flags);
/** @brief 转发 mtrans stream 事件到编解码回调。 */
void simple_codec_on_stream_event(const simple_codec_ops_t *ops,
                                  mtrans_stream_event_t event);
/**
 * @brief 向音频解码回调输入一帧码流。
 *
 * @param[in] ops 编解码回调集合，可为 NULL。
 * @param[in] frame 帧数据指针；len 为 0 时可为 NULL。
 * @param[in] len 帧数据长度，单位为字节。
 * @param[in] ts RTP 时间戳。
 * @param[in] flags mtrans 帧标记。
 * @return 回调返回值；无回调时返回 0。
 */
int simple_codec_input_audio(const simple_codec_ops_t *ops,
                             const uint8_t *frame,
                             size_t len,
                             uint32_t ts,
                             int flags);
/** @brief 调用音频 TX 启动回调。 */
int simple_codec_start_audio_tx(const simple_codec_ops_t *ops);
/** @brief 调用音频 TX 停止回调。 */
void simple_codec_stop_audio_tx(const simple_codec_ops_t *ops);
/** @brief 调用音频 RX 启动回调。 */
int simple_codec_start_audio_rx(const simple_codec_ops_t *ops);
/** @brief 调用音频 RX 停止回调。 */
void simple_codec_stop_audio_rx(const simple_codec_ops_t *ops);
/** @brief 同时启动音频 TX/RX 相关回调。 */
int simple_codec_start_audio(const simple_codec_ops_t *ops);
/** @brief 同时停止音频 TX/RX 相关回调。 */
void simple_codec_stop_audio(const simple_codec_ops_t *ops);

#ifdef __cplusplus
}
#endif

#endif
