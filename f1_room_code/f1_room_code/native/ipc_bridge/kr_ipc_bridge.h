#ifndef KR_IPC_BRIDGE_H
#define KR_IPC_BRIDGE_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kr_ipc_handle kr_ipc_handle_t;

typedef struct kr_ipc_config
{
    const char *interface;
    int preview_x;
    int preview_y;
    int preview_width;
    int preview_height;
} kr_ipc_config_t;

typedef struct kr_ipc_monitor_config
{
    const char *name;
    const char *ip;
    const char *user;
    const char *password;
    const char *rtsp_url;
    const char *sub_rtsp_url;
    bool prefer_substream;
} kr_ipc_monitor_config_t;

enum
{
    KR_IPC_EVENT_NONE = 0,
    KR_IPC_EVENT_MONITOR_STARTED = 1,
    KR_IPC_EVENT_MONITOR_STOPPED = 2,
    KR_IPC_EVENT_STREAM_READY = 3,
    KR_IPC_EVENT_OFFLINE = 4,
    KR_IPC_EVENT_CAPTURE_SAVED = 5,
    KR_IPC_EVENT_INFO = 6,
    KR_IPC_EVENT_ERROR = 7,
};

typedef struct kr_ipc_event
{
    int kind;
    int value0;
    int value1;
    char text1[128];
    char text2[256];
    char text3[512];
} kr_ipc_event_t;

int kr_ipc_create(kr_ipc_handle_t **out_handle, const kr_ipc_config_t *cfg);
int kr_ipc_destroy(kr_ipc_handle_t *handle);
int kr_ipc_wait_event(kr_ipc_handle_t *handle, kr_ipc_event_t *out_event, int timeout_ms);
int kr_ipc_start_monitor(kr_ipc_handle_t *handle, const kr_ipc_monitor_config_t *cfg);
int kr_ipc_stop_monitor(kr_ipc_handle_t *handle);
int kr_ipc_capture_jpeg(kr_ipc_handle_t *handle, const char *output_path);
int kr_ipc_is_monitoring(kr_ipc_handle_t *handle);
int kr_ipc_set_preview_brightness(kr_ipc_handle_t *handle, int level);

#ifdef __cplusplus
}
#endif

#endif
