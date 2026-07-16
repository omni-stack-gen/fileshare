#include "simple_media_profile_shape.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "simple_log.h"

static void simple_media_profile_shape_copy_reason(
    simple_media_profile_shape_result_t *result,
    const char *reason)
{
    if (!result)
    {
        return;
    }

    snprintf(result->reason, sizeof(result->reason), "%s", reason ? reason : "");
}

static void simple_media_profile_shape_disable_video(call_media_profile_t *profile)
{
    if (!profile)
    {
        return;
    }

    profile->media_mask &= (uint8_t)~CALL_MEDIA_MASK_VIDEO;
    profile->video_dir = (uint8_t)CALL_MEDIA_DIR_INACTIVE;
    profile->video_codec = (uint8_t)CALL_MEDIA_CODEC_UNKNOWN;
    profile->video_pt = 0;
    profile->video_width = 0;
    profile->video_height = 0;
    profile->video_fps = 0;
    profile->video_bitrate_kbps = 0;
    profile->video_ssrc = 0;
    profile->has_video_endpoint = false;
    memset(&profile->video_endpoint, 0, sizeof(profile->video_endpoint));
}

static void simple_media_profile_shape_disable_audio(call_media_profile_t *profile)
{
    if (!profile)
    {
        return;
    }

    profile->media_mask &= (uint8_t)~CALL_MEDIA_MASK_AUDIO;
    profile->audio_dir = (uint8_t)CALL_MEDIA_DIR_INACTIVE;
    profile->audio_codec = (uint8_t)CALL_MEDIA_CODEC_UNKNOWN;
    profile->audio_pt = 0;
    profile->audio_sample_rate = 0;
    profile->audio_channels = 0;
    profile->audio_ptime_ms = 0;
    profile->audio_bitrate_kbps = 0;
    profile->audio_ssrc = 0;
    profile->has_audio_endpoint = false;
    memset(&profile->audio_endpoint, 0, sizeof(profile->audio_endpoint));
}

void simple_media_profile_shape_fill_default(call_media_profile_t *profile,
                                             uint8_t msg_type)
{
    uint8_t mode = 0;

    if (!profile)
    {
        return;
    }

    memset(profile, 0, sizeof(*profile));
    if (!call_proto_media_profile_mode_for_msg(msg_type, &mode))
    {
        mode = (uint8_t)CALL_MEDIA_PROFILE_MODE_ANNOUNCE;
    }
    profile->mode = mode;
    profile->media_mask = call_proto_default_media_mask_for_msg(msg_type);
    profile->flags = CALL_MEDIA_PROFILE_FLAG_RTCP_MUX;
    profile->video_codec = CALL_MEDIA_CODEC_H264;
    profile->video_pt = 96;
    profile->video_width = 1920;
    profile->video_height = 1088;
    profile->video_fps = 30;
    profile->video_bitrate_kbps = 1000;
    profile->audio_codec = CALL_MEDIA_CODEC_OPUS;
    profile->audio_pt = 111;
    profile->audio_sample_rate = 16000;
    profile->audio_channels = 1;
    profile->audio_ptime_ms = 20;
    profile->audio_bitrate_kbps = 24;
    (void)call_proto_media_profile_dirs_for_msg(msg_type, profile->media_mask,
                                                &profile->video_dir,
                                                &profile->audio_dir);
}

static int simple_media_profile_shape_apply_tx_policy(
    const simple_media_profile_shape_request_t *request,
    call_media_profile_t *profile)
{
    simple_tx_profile_shape_request_t policy_request;

    if (!request->tx_policy)
    {
        return 0;
    }

    memset(&policy_request, 0, sizeof(policy_request));
    policy_request.msg_type = request->msg_type;
    policy_request.session_id = request->session_id;
    policy_request.caller_device_id = request->caller_device_id;
    policy_request.callee_device_id = request->callee_device_id;
    policy_request.peer_id = request->peer_id;
    return request->tx_policy(request->tx_policy_ctx, &policy_request, profile);
}

static void simple_media_profile_shape_apply_answer_offer(
    const simple_media_profile_shape_request_t *request,
    call_media_profile_t *profile,
    simple_media_profile_shape_result_t *result)
{
    const call_media_profile_t *offer;

    if (!request || !profile || request->msg_type != CALL_MSG_CALL_ACCEPT ||
        !request->has_offer_profile || !request->offer_profile)
    {
        return;
    }

    offer = request->offer_profile;
    if ((offer->media_mask & CALL_MEDIA_MASK_VIDEO) == 0)
    {
        simple_media_profile_shape_disable_video(profile);
    }
    if ((offer->media_mask & CALL_MEDIA_MASK_AUDIO) == 0)
    {
        simple_media_profile_shape_disable_audio(profile);
    }

    (void)call_proto_media_profile_dirs_for_msg(request->msg_type,
                                                profile->media_mask,
                                                &profile->video_dir,
                                                &profile->audio_dir);
    if (result)
    {
        result->offer_mask = offer->media_mask;
        result->answer_shaped_from_offer = true;
        simple_media_profile_shape_copy_reason(result, "answer_from_offer");
    }

    SIMPLE_LOGI("simple_call answer profile shaped from offer: session=0x%" PRIx64
                " offer_mask=0x%02x answer_mask=0x%02x\n",
                request->session_id, offer->media_mask, profile->media_mask);
}

int simple_media_profile_shape_prepare(
    const simple_media_profile_shape_request_t *request,
    call_media_profile_t *profile,
    simple_media_profile_shape_result_t *result)
{
    int ret;

    if (result)
    {
        memset(result, 0, sizeof(*result));
    }
    if (!request || !profile)
    {
        if (result)
        {
            result->ret = -EINVAL;
            simple_media_profile_shape_copy_reason(result, "invalid");
        }
        return -EINVAL;
    }

    simple_media_profile_shape_fill_default(profile, request->msg_type);
    if (result)
    {
        result->before_mask = profile->media_mask;
        result->after_mask = profile->media_mask;
        simple_media_profile_shape_copy_reason(result, "ok");
    }

    ret = simple_media_profile_shape_apply_tx_policy(request, profile);
    if (result)
    {
        result->after_mask = profile->media_mask;
        result->ret = ret;
    }
    if (ret < 0)
    {
        if (result)
        {
            simple_media_profile_shape_copy_reason(result, "tx_shape_policy");
        }
        return ret;
    }

    simple_media_profile_shape_apply_answer_offer(request, profile, result);
    if (result)
    {
        result->after_mask = profile->media_mask;
        result->ret = 0;
    }
    return 0;
}
