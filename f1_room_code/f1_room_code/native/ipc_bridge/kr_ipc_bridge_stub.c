#include "kr_ipc_bridge.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <utils/bmem.h>

static void *kr_ipc_stub_zalloc(size_t size)
{
    void *mem = bmalloc(size);
    if (mem != NULL)
    {
        memset(mem, 0, size);
    }
    return mem;
}

struct kr_ipc_handle
{
    int unused;
};

int kr_ipc_create(kr_ipc_handle_t **out_handle, const kr_ipc_config_t *cfg)
{
    (void)cfg;
    if (out_handle == NULL)
    {
        return -EINVAL;
    }

    *out_handle = (kr_ipc_handle_t *)kr_ipc_stub_zalloc(sizeof(kr_ipc_handle_t));
    return *out_handle != NULL ? 0 : -ENOMEM;
}

int kr_ipc_destroy(kr_ipc_handle_t *handle)
{
    bfree(handle);
    return 0;
}

int kr_ipc_wait_event(kr_ipc_handle_t *handle, kr_ipc_event_t *out_event, int timeout_ms)
{
    (void)handle;
    (void)timeout_ms;
    if (out_event != NULL)
    {
        memset(out_event, 0, sizeof(*out_event));
    }
    return 0;
}

int kr_ipc_start_monitor(kr_ipc_handle_t *handle, const kr_ipc_monitor_config_t *cfg)
{
    (void)handle;
    (void)cfg;
    return -ENOSYS;
}

int kr_ipc_stop_monitor(kr_ipc_handle_t *handle)
{
    (void)handle;
    return 0;
}

int kr_ipc_capture_jpeg(kr_ipc_handle_t *handle, const char *output_path)
{
    (void)handle;
    (void)output_path;
    return -ENOSYS;
}

int kr_ipc_is_monitoring(kr_ipc_handle_t *handle)
{
    (void)handle;
    return 0;
}

int kr_ipc_set_preview_brightness(kr_ipc_handle_t *handle, int level)
{
    (void)handle;
    (void)level;
    return 0;
}
