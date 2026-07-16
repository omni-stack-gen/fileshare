#ifndef KR_SOUND_BRIDGE_H
#define KR_SOUND_BRIDGE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kr_sound_handle kr_sound_handle_t;

typedef struct kr_sound_config
{
    const char *asset_dir;
    int volume_percent;
} kr_sound_config_t;

int kr_sound_create(kr_sound_handle_t **out_handle, const kr_sound_config_t *cfg);
int kr_sound_destroy(kr_sound_handle_t *handle);
int kr_sound_play_touch(kr_sound_handle_t *handle);
int kr_sound_start_ring(kr_sound_handle_t *handle, const char *kind, int melody, int volume_percent);
int kr_sound_stop_ring(kr_sound_handle_t *handle);
int kr_sound_set_volume(kr_sound_handle_t *handle, int volume_percent);

#ifdef __cplusplus
}
#endif

#endif
