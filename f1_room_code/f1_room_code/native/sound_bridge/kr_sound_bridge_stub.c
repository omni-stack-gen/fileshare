#include "kr_sound_bridge.h"

#include <errno.h>
#include <stdlib.h>

struct kr_sound_handle
{
    int unused;
};

int kr_sound_create(kr_sound_handle_t **out_handle, const kr_sound_config_t *cfg)
{
    (void)cfg;
    if (!out_handle)
    {
        return -EINVAL;
    }
    *out_handle = (kr_sound_handle_t *)calloc(1, sizeof(kr_sound_handle_t));
    return *out_handle ? 0 : -ENOMEM;
}

int kr_sound_destroy(kr_sound_handle_t *handle)
{
    free(handle);
    return 0;
}

int kr_sound_play_touch(kr_sound_handle_t *handle)
{
    return handle ? 0 : -EINVAL;
}

int kr_sound_start_ring(kr_sound_handle_t *handle, const char *kind, int melody, int volume_percent)
{
    (void)kind;
    (void)melody;
    (void)volume_percent;
    return handle ? 0 : -EINVAL;
}

int kr_sound_stop_ring(kr_sound_handle_t *handle)
{
    return handle ? 0 : -EINVAL;
}

int kr_sound_set_volume(kr_sound_handle_t *handle, int volume_percent)
{
    (void)volume_percent;
    return handle ? 0 : -EINVAL;
}
