#include "kr_webrtc_bridge.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <utils/bmem.h>

static void *kr_webrtc_stub_zalloc(size_t size)
{
    void *mem = bmalloc(size);
    if (mem != NULL)
    {
        memset(mem, 0, size);
    }
    return mem;
}

struct kr_webrtc_handle
{
    int unused;
};

int kr_webrtc_create(kr_webrtc_handle_t **out_handle, const kr_webrtc_config_t *cfg)
{
    (void)cfg;
    if (out_handle == NULL)
    {
        return -EINVAL;
    }

    *out_handle = (kr_webrtc_handle_t *)kr_webrtc_stub_zalloc(sizeof(kr_webrtc_handle_t));
    return *out_handle == NULL ? -ENOMEM : 0;
}

int kr_webrtc_start(kr_webrtc_handle_t *handle)
{
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_stop(kr_webrtc_handle_t *handle)
{
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_destroy(kr_webrtc_handle_t *handle)
{
    bfree(handle);
    return 0;
}

int kr_webrtc_wait_event(kr_webrtc_handle_t *handle, kr_webrtc_event_t *out_event, int timeout_ms)
{
    (void)handle;
    (void)out_event;
    (void)timeout_ms;
    return 0;
}

int kr_webrtc_is_logged_in(kr_webrtc_handle_t *handle)
{
    return handle == NULL ? 0 : 1;
}

int kr_webrtc_send_video_frame(
    kr_webrtc_handle_t *handle,
    int stream_type,
    int video_codec,
    const unsigned char *data,
    size_t len
)
{
    (void)stream_type;
    (void)video_codec;
    (void)data;
    (void)len;
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_send_audio_frame(
    kr_webrtc_handle_t *handle,
    const unsigned char *data,
    size_t len
)
{
    (void)data;
    (void)len;
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_call(
    kr_webrtc_handle_t *handle,
    const char *session_id,
    const char *to,
    const char *audio,
    const char *video,
    int datachannel
)
{
    (void)session_id;
    (void)to;
    (void)audio;
    (void)video;
    (void)datachannel;
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_close_session(kr_webrtc_handle_t *handle, const char *session_id)
{
    (void)session_id;
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_set_call_busy(kr_webrtc_handle_t *handle, bool busy)
{
    (void)busy;
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_set_preview_brightness(kr_webrtc_handle_t *handle, int level)
{
    (void)level;
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_set_playback_volume(kr_webrtc_handle_t *handle, int volume_percent)
{
    (void)volume_percent;
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_set_remote_audio_sink(
    kr_webrtc_handle_t *handle,
    kr_webrtc_remote_audio_sink_t sink,
    void *user
)
{
    (void)sink;
    (void)user;
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_set_bridge_h264_dump_tag(kr_webrtc_handle_t *handle, const char *tag)
{
    (void)tag;
    return handle == NULL ? -EINVAL : 0;
}

int kr_webrtc_set_media_bridge_mode(kr_webrtc_handle_t *handle, bool enabled)
{
    (void)enabled;
    return handle == NULL ? -EINVAL : 0;
}
