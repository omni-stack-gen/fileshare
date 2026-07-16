#ifndef KR_INDOOR_BRIDGE_H
#define KR_INDOOR_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kr_indoor_handle kr_indoor_handle_t;

typedef enum
{
    KR_INDOOR_EVENT_NONE = 0,
    KR_INDOOR_EVENT_CALL_INCOMING = 1,
    KR_INDOOR_EVENT_CALL_STATE_CHANGED = 2,
    KR_INDOOR_EVENT_CALL_MEDIA_CHANGED = 3,
    KR_INDOOR_EVENT_CALL_REMOTE_HANGUP = 4,
    KR_INDOOR_EVENT_CALL_ERROR = 5,
    KR_INDOOR_EVENT_UNLOCK_DONE = 101,
    KR_INDOOR_EVENT_UNLOCK_FAIL = 102,
    KR_INDOOR_EVENT_UNLOCK_TIMEOUT = 103,
    KR_INDOOR_EVENT_SECONDARY_CONFIRM_ONLINE = 150,
    KR_INDOOR_EVENT_SECONDARY_CONFIRM_OFFLINE = 151,
    KR_INDOOR_EVENT_ERROR = 200,
} kr_indoor_event_kind_t;

typedef struct
{
    bool enabled;
    const char *interface;
    const char *target_device_id;
    const char *room_id;
    const char *unlock_peer_device_id;
    const char *registry_peer_device_id;
    uint32_t direct_plc_cco_addr;
    int playback_volume_percent;
} kr_indoor_config_t;

typedef struct
{
    kr_indoor_event_kind_t kind;
    uint32_t peer_id;
    uint64_t session_id;
    int session_state;
    int result_code;
    bool video_active;
    bool audio_active;
    char text[64];
    char text2[64];
} kr_indoor_event_t;

int kr_indoor_create(kr_indoor_handle_t **out, const kr_indoor_config_t *cfg);
int kr_indoor_start(kr_indoor_handle_t *handle);
void kr_indoor_stop(kr_indoor_handle_t *handle);
void kr_indoor_destroy(kr_indoor_handle_t *handle);

int kr_indoor_accept(kr_indoor_handle_t *handle);
int kr_indoor_reject(kr_indoor_handle_t *handle);
int kr_indoor_hangup(kr_indoor_handle_t *handle);
int kr_indoor_call(kr_indoor_handle_t *handle, const char *target_device_id);
int kr_indoor_monitor(kr_indoor_handle_t *handle, const char *target_device_id);
int kr_indoor_monitor_stop(kr_indoor_handle_t *handle);
int kr_indoor_unlock(kr_indoor_handle_t *handle, const char *target_device_id, const char *room_id, int place);
int kr_indoor_set_playback_volume(kr_indoor_handle_t *handle, int volume_percent);
int kr_indoor_set_preview_brightness(kr_indoor_handle_t *handle, int level);
int kr_indoor_set_external_call_busy(kr_indoor_handle_t *handle, bool busy);
int kr_indoor_set_webrtc_bridge(kr_indoor_handle_t *handle, void *webrtc_handle);
int kr_indoor_set_media_bridge(kr_indoor_handle_t *handle, bool enabled);
int kr_indoor_request_video_recovery(kr_indoor_handle_t *handle, bool use_fir);

int kr_indoor_wait_event(kr_indoor_handle_t *handle, kr_indoor_event_t *out, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
