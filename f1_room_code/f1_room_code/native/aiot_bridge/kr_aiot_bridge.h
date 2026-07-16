#ifndef KR_AIOT_BRIDGE_H
#define KR_AIOT_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kr_aiot_handle kr_aiot_handle_t;

typedef struct kr_aiot_config
{
    const char *interface;
    const char *sn;
    const char *model;
    const char *server_addr;
    const char *mqtt_server_addr;
    const char *application_version;
    const char *system_version;
    const char *hardware_version;
    const char *mac;
    int keepalive;
    bool is_mqtts;
} kr_aiot_config_t;

enum
{
    KR_AIOT_EVENT_NONE = 0,
    KR_AIOT_EVENT_CONN_STATUS = 1,
    KR_AIOT_EVENT_MESSAGE = 2,
    KR_AIOT_EVENT_CALLX = 3,
    KR_AIOT_EVENT_OPEN_LOCK = 4,
    KR_AIOT_EVENT_SYNC_TIME = 5,
    KR_AIOT_EVENT_REBOOT = 6,
    KR_AIOT_EVENT_RESET = 7,
    KR_AIOT_EVENT_ERROR = 8,
    KR_AIOT_EVENT_WEBRTC_ACCOUNT = 9,
    KR_AIOT_EVENT_UPGRADE = 10,
};

typedef struct kr_aiot_event
{
    int kind;
    int conn_status;
    int msg_type;
    int callx_type;
    int ack_code;
    int is_cloud_request;
    uint64_t seq;
    char callid[64];
    char text1[128];
    char text2[256];
    char text3[1024];
    int value0;
    int value1;
    char callee_kind[32];
    char callee_user_id[64];
    char callee_web_username[64];
    char callee_web_session_id[64];
    int callee_channel;
    int callee_count;
    char caller_device_kind[64];
    char snapshot_download_url[1024];
} kr_aiot_event_t;

int kr_aiot_create(kr_aiot_handle_t **out_handle, const kr_aiot_config_t *cfg);
int kr_aiot_start(kr_aiot_handle_t *handle);
int kr_aiot_stop(kr_aiot_handle_t *handle);
int kr_aiot_destroy(kr_aiot_handle_t *handle);
int kr_aiot_wait_event(kr_aiot_handle_t *handle, kr_aiot_event_t *out_event, int timeout_ms);
int kr_aiot_request_webrtc_account(kr_aiot_handle_t *handle);
int kr_aiot_set_secondary_confirm_online(
    kr_aiot_handle_t *handle,
    bool online,
    const char *proxy_sn,
    const char *proxy_model
);
int kr_aiot_set_call_busy(kr_aiot_handle_t *handle, bool busy);
int kr_aiot_set_call_timeouts(
    kr_aiot_handle_t *handle,
    int ring_timeout,
    int converse_timeout
);
int kr_aiot_send_open_ask(
    kr_aiot_handle_t *handle,
    const char *callid,
    const char *sn
);
int kr_aiot_send_callx_invited_ack(
    kr_aiot_handle_t *handle,
    const char *callid,
    bool able,
    const char *unable_cause,
    int ring_timeout,
    int converse_timeout
);
int kr_aiot_send_callx_accept(
    kr_aiot_handle_t *handle,
    const char *callid,
    const char *caller_media,
    const char *callee_media
);
int kr_aiot_send_callx_hangup(
    kr_aiot_handle_t *handle,
    const char *callid,
    bool entirely,
    const char *cause
);
int kr_aiot_send_callx_initiate(
    kr_aiot_handle_t *handle,
    const char *callee_sn,
    int callee_channel
);
int kr_aiot_send_callx_initiate_manager(kr_aiot_handle_t *handle);
int kr_aiot_send_callx_initiate_resident(
    kr_aiot_handle_t *handle,
    const char *callee_apartment
);
int kr_aiot_send_callx_invite(
    kr_aiot_handle_t *handle,
    const char *callid,
    const char *callee_sn,
    int callee_channel,
    const char *mode,
    int ring_timeout,
    int converse_timeout
);
int kr_aiot_send_callx_terminate(kr_aiot_handle_t *handle, const char *callid);
int kr_aiot_download_file(
    kr_aiot_handle_t *handle,
    const char *url,
    const char *output_path,
    int timeout_ms
);

#ifdef __cplusplus
}
#endif

#endif
