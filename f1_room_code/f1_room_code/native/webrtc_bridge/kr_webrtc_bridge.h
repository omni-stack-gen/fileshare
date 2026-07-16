#ifndef KR_WEBRTC_BRIDGE_H
#define KR_WEBRTC_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kr_webrtc_handle kr_webrtc_handle_t;
typedef void (*kr_webrtc_remote_audio_sink_t)(const unsigned char *data, size_t len, void *user);

typedef struct kr_webrtc_config
{
    const char *initstring;
    const char *serno;
    const char *server_addr;
    const char *customer_serno;
    const char *playback_device;
    const char *record_device;
    int signal_reconnect_timeout_ms;
    int max_sdk_use_mem;
    int max_session_use_mem;
    int max_session_buffer_size;
    bool enable_audio_input;
    bool enable_audio_output;
} kr_webrtc_config_t;

enum
{
    KR_WEBRTC_EVENT_NONE = 0,
    KR_WEBRTC_EVENT_ONLINE = 1,
    KR_WEBRTC_EVENT_OFFLINE = 2,
    KR_WEBRTC_EVENT_ASK_IFRAME = 3,
    KR_WEBRTC_EVENT_CALL_START = 4,
    KR_WEBRTC_EVENT_CALL_LINK = 5,
    KR_WEBRTC_EVENT_CALL_DISCONNECT = 6,
    KR_WEBRTC_EVENT_CALL_DESTROY = 7,
    KR_WEBRTC_EVENT_CONNECT_FAILED = 8,
    KR_WEBRTC_EVENT_CONNECT_RECONNECT = 9,
    KR_WEBRTC_EVENT_CALL_FAILED = 10,
    KR_WEBRTC_EVENT_DATA_CHANNEL_OPEN = 14,
    KR_WEBRTC_EVENT_ERROR = 15,
};

typedef struct kr_webrtc_event
{
    int kind;
    int value0;
    int value1;
    char text1[128];
    char text2[128];
} kr_webrtc_event_t;

int kr_webrtc_create(kr_webrtc_handle_t **out_handle, const kr_webrtc_config_t *cfg);
int kr_webrtc_start(kr_webrtc_handle_t *handle);
int kr_webrtc_stop(kr_webrtc_handle_t *handle);
int kr_webrtc_destroy(kr_webrtc_handle_t *handle);
int kr_webrtc_wait_event(kr_webrtc_handle_t *handle, kr_webrtc_event_t *out_event, int timeout_ms);
int kr_webrtc_is_logged_in(kr_webrtc_handle_t *handle);
int kr_webrtc_send_video_frame(
    kr_webrtc_handle_t *handle,
    int stream_type,
    int video_codec,
    const unsigned char *data,
    size_t len
);
int kr_webrtc_send_audio_frame(
    kr_webrtc_handle_t *handle,
    const unsigned char *data,
    size_t len
);
int kr_webrtc_call(
    kr_webrtc_handle_t *handle,
    const char *session_id,
    const char *to,
    const char *audio,
    const char *video,
    int datachannel
);
int kr_webrtc_close_session(kr_webrtc_handle_t *handle, const char *session_id);
int kr_webrtc_set_call_busy(kr_webrtc_handle_t *handle, bool busy);
int kr_webrtc_set_preview_brightness(kr_webrtc_handle_t *handle, int level);
int kr_webrtc_set_playback_volume(kr_webrtc_handle_t *handle, int volume_percent);
int kr_webrtc_set_remote_audio_sink(
    kr_webrtc_handle_t *handle,
    kr_webrtc_remote_audio_sink_t sink,
    void *user
);
int kr_webrtc_set_bridge_h264_dump_tag(kr_webrtc_handle_t *handle, const char *tag);
int kr_webrtc_set_media_bridge_mode(kr_webrtc_handle_t *handle, bool enabled);

#ifdef __cplusplus
}
#endif

#endif
