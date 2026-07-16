/**
 * @file
 * @brief simple 示例应用公共模块 simple_codec 的实现文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_codec.h"

int simple_codec_start_video_tx(const simple_codec_ops_t *ops)
{
    if (!ops || !ops->start_video_encoder)
    {
        return 0;
    }

    return ops->start_video_encoder(ops->user_ctx);
}

void simple_codec_stop_video_tx(const simple_codec_ops_t *ops)
{
    if (ops && ops->stop_video_encoder)
    {
        ops->stop_video_encoder(ops->user_ctx);
    }
}

int simple_codec_start_video_rx(const simple_codec_ops_t *ops)
{
    if (!ops || !ops->start_video_decoder)
    {
        return 0;
    }

    return ops->start_video_decoder(ops->user_ctx);
}

void simple_codec_stop_video_rx(const simple_codec_ops_t *ops)
{
    if (ops && ops->stop_video_decoder)
    {
        ops->stop_video_decoder(ops->user_ctx);
    }
}

int simple_codec_input_video(const simple_codec_ops_t *ops,
                             const uint8_t *frame,
                             size_t len,
                             uint32_t ts,
                             int flags)
{
    if (!ops || !ops->input_video_frame)
    {
        return 0;
    }

    return ops->input_video_frame(ops->user_ctx, frame, len, ts, flags);
}

void simple_codec_on_stream_event(const simple_codec_ops_t *ops,
                                  mtrans_stream_event_t event)
{
    if (!ops || !ops->on_stream_event)
    {
        return;
    }

    ops->on_stream_event(ops->user_ctx, event);
}

int simple_codec_input_audio(const simple_codec_ops_t *ops,
                             const uint8_t *frame,
                             size_t len,
                             uint32_t ts,
                             int flags)
{
    if (!ops || !ops->input_audio_frame)
    {
        return 0;
    }

    return ops->input_audio_frame(ops->user_ctx, frame, len, ts, flags);
}

int simple_codec_start_audio_tx(const simple_codec_ops_t *ops)
{
    if (!ops || !ops->start_audio_encoder)
    {
        return 0;
    }

    return ops->start_audio_encoder(ops->user_ctx);
}

void simple_codec_stop_audio_tx(const simple_codec_ops_t *ops)
{
    if (ops && ops->stop_audio_encoder)
    {
        ops->stop_audio_encoder(ops->user_ctx);
    }
}

int simple_codec_start_audio_rx(const simple_codec_ops_t *ops)
{
    if (!ops || !ops->start_audio_decoder)
    {
        return 0;
    }

    return ops->start_audio_decoder(ops->user_ctx);
}

void simple_codec_stop_audio_rx(const simple_codec_ops_t *ops)
{
    if (ops && ops->stop_audio_decoder)
    {
        ops->stop_audio_decoder(ops->user_ctx);
    }
}

int simple_codec_start_audio(const simple_codec_ops_t *ops)
{
    int ret;

    if (!ops)
    {
        return 0;
    }

    if (ops->start_audio_encoder)
    {
        ret = simple_codec_start_audio_tx(ops);
        if (ret < 0)
        {
            return ret;
        }
    }

    if (ops->start_audio_decoder)
    {
        ret = simple_codec_start_audio_rx(ops);
        if (ret < 0)
        {
            simple_codec_stop_audio_tx(ops);
            return ret;
        }
    }

    return 0;
}

void simple_codec_stop_audio(const simple_codec_ops_t *ops)
{
    if (!ops)
    {
        return;
    }

    if (ops->stop_audio_decoder)
    {
        simple_codec_stop_audio_rx(ops);
    }
    if (ops->stop_audio_encoder)
    {
        simple_codec_stop_audio_tx(ops);
    }
}
