#include "kr_aiot_bridge.h"

#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include <utils/bmem.h>

static void *kr_aiot_stub_zalloc(size_t size)
{
    void *mem = bmalloc(size);
    if (mem != NULL)
    {
        memset(mem, 0, size);
    }
    return mem;
}

struct kr_aiot_handle
{
    int unused;
};

int kr_aiot_create(kr_aiot_handle_t **out_handle, const kr_aiot_config_t *cfg)
{
    (void)cfg;
    if (out_handle == NULL)
    {
        return -EINVAL;
    }
    *out_handle = (kr_aiot_handle_t *)kr_aiot_stub_zalloc(sizeof(kr_aiot_handle_t));
    return *out_handle == NULL ? -ENOMEM : 0;
}

int kr_aiot_start(kr_aiot_handle_t *handle)
{
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_stop(kr_aiot_handle_t *handle)
{
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_destroy(kr_aiot_handle_t *handle)
{
    bfree(handle);
    return 0;
}

int kr_aiot_wait_event(kr_aiot_handle_t *handle, kr_aiot_event_t *out_event, int timeout_ms)
{
    (void)handle;
    (void)out_event;
    (void)timeout_ms;
    return 0;
}

int kr_aiot_request_webrtc_account(kr_aiot_handle_t *handle)
{
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_set_secondary_confirm_online(
    kr_aiot_handle_t *handle,
    bool online,
    const char *proxy_sn,
    const char *proxy_model
)
{
    (void)online;
    (void)proxy_sn;
    (void)proxy_model;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_set_call_busy(kr_aiot_handle_t *handle, bool busy)
{
    (void)busy;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_set_call_timeouts(
    kr_aiot_handle_t *handle,
    int ring_timeout,
    int converse_timeout
)
{
    (void)ring_timeout;
    (void)converse_timeout;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_send_open_ask(
    kr_aiot_handle_t *handle,
    const char *callid,
    const char *sn
)
{
    (void)callid;
    (void)sn;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_send_callx_invited_ack(
    kr_aiot_handle_t *handle,
    const char *callid,
    bool able,
    const char *unable_cause,
    int ring_timeout,
    int converse_timeout
)
{
    (void)callid;
    (void)able;
    (void)unable_cause;
    (void)ring_timeout;
    (void)converse_timeout;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_send_callx_accept(
    kr_aiot_handle_t *handle,
    const char *callid,
    const char *caller_media,
    const char *callee_media
)
{
    (void)callid;
    (void)caller_media;
    (void)callee_media;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_send_callx_hangup(
    kr_aiot_handle_t *handle,
    const char *callid,
    bool entirely,
    const char *cause
)
{
    (void)callid;
    (void)entirely;
    (void)cause;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_send_callx_initiate(
    kr_aiot_handle_t *handle,
    const char *callee_sn,
    int callee_channel
)
{
    (void)callee_sn;
    (void)callee_channel;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_send_callx_initiate_manager(kr_aiot_handle_t *handle)
{
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_send_callx_initiate_resident(
    kr_aiot_handle_t *handle,
    const char *callee_apartment
)
{
    (void)callee_apartment;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_send_callx_invite(
    kr_aiot_handle_t *handle,
    const char *callid,
    const char *callee_sn,
    int callee_channel,
    const char *mode,
    int ring_timeout,
    int converse_timeout
)
{
    (void)callid;
    (void)callee_sn;
    (void)callee_channel;
    (void)mode;
    (void)ring_timeout;
    (void)converse_timeout;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_send_callx_terminate(kr_aiot_handle_t *handle, const char *callid)
{
    (void)callid;
    return handle == NULL ? -EINVAL : 0;
}

int kr_aiot_download_file(
    kr_aiot_handle_t *handle,
    const char *url,
    const char *output_path,
    int timeout_ms
)
{
    (void)url;
    (void)output_path;
    (void)timeout_ms;
    return handle == NULL ? -EINVAL : -ENOSYS;
}
