#include "kr_sound_bridge.h"

#include <AudioCore.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <utils/wave_file_fmt.h>

#define KR_SOUND_DEFAULT_VOLUME 80
#define KR_SOUND_TOUCH_THROTTLE_MS 45
#define KR_SOUND_BLOCK_TIME_MS 20
#define KR_SOUND_CACHE_BLOCKS 4

typedef struct kr_sound_play_request
{
    char path[PATH_MAX];
    char label[16];
    unsigned int play_id;
    int volume;
    bool loop;
    volatile int *running;
} kr_sound_play_request_t;

struct kr_sound_handle
{
    char asset_dir[PATH_MAX];
    int volume_percent;
    pthread_mutex_t lock;
    pthread_t ring_thread;
    bool ring_thread_valid;
    volatile int ring_running;
    long long last_touch_ms;
    unsigned int next_play_id;
};

static int kr_sound_clamp_volume(int volume)
{
    if (volume < 0)
    {
        return 0;
    }
    if (volume > 100)
    {
        return 100;
    }
    return volume;
}

static long long kr_sound_now_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

static int kr_sound_copy_asset_dir(char *dst, size_t dst_size, const char *asset_dir)
{
    const char *src = (asset_dir && asset_dir[0] != '\0') ? asset_dir : "/system/resources/sound/";
    int written = snprintf(dst, dst_size, "%s", src);
    return (written > 0 && (size_t)written < dst_size) ? 0 : -ENAMETOOLONG;
}

static int kr_sound_build_path(char *dst, size_t dst_size, const char *asset_dir, const char *file)
{
    const char *separator = "/";
    size_t len = asset_dir ? strlen(asset_dir) : 0;
    if (len > 0 && asset_dir[len - 1] == '/')
    {
        separator = "";
    }
    int written = snprintf(dst, dst_size, "%s%s%s", asset_dir, separator, file);
    return (written > 0 && (size_t)written < dst_size) ? 0 : -ENAMETOOLONG;
}

static int kr_sound_build_ring_path(
    char *dst,
    size_t dst_size,
    const char *asset_dir,
    const char *kind,
    int melody)
{
    (void)kind;
    (void)melody;
    return kr_sound_build_path(dst, dst_size, asset_dir, "Door Bell.wav");
}

static int kr_sound_play_wav_file(
    const char *path,
    const char *label,
    unsigned int play_id,
    int volume,
    bool loop,
    volatile int *running)
{
    wav_reader_t reader;
    int stream_id = 0;
    int bytes_per_sample;
    int samples_per_block;
    int block_size;
    char *buffer = NULL;
    int ret = 0;
    unsigned int write_blocks = 0;
    unsigned int loop_count = 0;
    size_t total_bytes = 0;
    bool stop_ok;
    bool close_ok;

    memset(&reader, 0, sizeof(reader));
    if (wav_reader_open(&reader, path) != 0)
    {
        fprintf(stderr,
                "kr_sound: open wav failed id=%u label=%s path=%s\n",
                play_id,
                label ? label : "",
                path ? path : "");
        return -ENOENT;
    }

    if (reader.fmt_tag != WAVE_FMT_TAG_PCM ||
        reader.channel <= 0 ||
        reader.sample_rate == 0 ||
        (reader.bits_per_sample != 8 && reader.bits_per_sample != 16 && reader.bits_per_sample != 24 && reader.bits_per_sample != 32))
    {
        fprintf(stderr,
                "kr_sound: unsupported wav path=%s fmt=%u ch=%u rate=%u bits=%u\n",
                path,
                reader.fmt_tag,
                reader.channel,
                reader.sample_rate,
                reader.bits_per_sample);
        wav_reader_close(&reader);
        return -EINVAL;
    }

    bytes_per_sample = reader.bits_per_sample / 8;
    samples_per_block = (int)(reader.sample_rate * KR_SOUND_BLOCK_TIME_MS / 1000U);
    if (samples_per_block <= 0)
    {
        samples_per_block = 1;
    }
    block_size = samples_per_block * reader.channel * bytes_per_sample;
    buffer = (char *)malloc((size_t)block_size);
    if (!buffer)
    {
        wav_reader_close(&reader);
        return -ENOMEM;
    }

    stream_id = Audio_Player_Open();
    if (stream_id <= 0)
    {
        fprintf(stderr,
                "kr_sound: Audio_Player_Open failed id=%u label=%s path=%s\n",
                play_id,
                label ? label : "",
                path ? path : "");
        free(buffer);
        wav_reader_close(&reader);
        return -EIO;
    }

    fprintf(stderr,
            "kr_sound: play begin id=%u label=%s stream=%d loop=%d volume=%d "
            "path=%s rate=%u ch=%u bits=%u block=%d\n",
            play_id,
            label ? label : "",
            stream_id,
            loop ? 1 : 0,
            kr_sound_clamp_volume(volume),
            path ? path : "",
            reader.sample_rate,
            reader.channel,
            reader.bits_per_sample,
            block_size);

    if (!Audio_Player_SetDevice(stream_id, "default") ||
        !Audio_Player_SetFormat(stream_id, reader.channel, reader.bits_per_sample, reader.sample_rate) ||
        !Audio_Player_SetCache(stream_id, KR_SOUND_CACHE_BLOCKS, samples_per_block) ||
        !Audio_Player_SetMode(stream_id, true, NULL, NULL) ||
        !Audio_Player_SetVolume(stream_id, kr_sound_clamp_volume(volume)) ||
        !Audio_Player_Start(stream_id))
    {
        close_ok = Audio_Player_Close(stream_id);
        fprintf(stderr,
                "kr_sound: player setup failed id=%u label=%s stream=%d close_ok=%d\n",
                play_id,
                label ? label : "",
                stream_id,
                close_ok ? 1 : 0);
        free(buffer);
        wav_reader_close(&reader);
        return -EIO;
    }

    while (!running || *running)
    {
        size_t read_bytes = wav_reader_read(&reader, buffer, (size_t)block_size);
        if (read_bytes == 0)
        {
            if (!loop)
            {
                break;
            }
            // 铃声读到文件末尾后立即回到开头，直到接听或挂断时 stop_ring 停止。
            if (running && !*running)
            {
                break;
            }
            if (reader.data_len == 0)
            {
                ret = -EINVAL;
                break;
            }
            if (wav_reader_seek(&reader, 0) != 0)
            {
                ret = -EIO;
                break;
            }
            loop_count++;
            continue;
        }

        if (read_bytes < (size_t)block_size)
        {
            memset(buffer + read_bytes, 0, (size_t)block_size - read_bytes);
            read_bytes = (size_t)block_size;
        }

        if (!Audio_Player_Write(stream_id, buffer, (int)read_bytes, 500))
        {
            ret = -EIO;
            break;
        }
        write_blocks++;
        total_bytes += read_bytes;
    }

    fprintf(stderr,
            "kr_sound: player stopping id=%u label=%s stream=%d blocks=%u "
            "bytes=%zu loops=%u running=%d ret=%d\n",
            play_id,
            label ? label : "",
            stream_id,
            write_blocks,
            total_bytes,
            loop_count,
            running ? *running : -1,
            ret);
    stop_ok = Audio_Player_Stop(stream_id);
    close_ok = Audio_Player_Close(stream_id);
    fprintf(stderr,
            "kr_sound: player closed id=%u label=%s stream=%d stop_ok=%d close_ok=%d ret=%d\n",
            play_id,
            label ? label : "",
            stream_id,
            stop_ok ? 1 : 0,
            close_ok ? 1 : 0,
            ret);
    free(buffer);
    wav_reader_close(&reader);
    return ret;
}

static void *kr_sound_play_thread(void *arg)
{
    kr_sound_play_request_t *request = (kr_sound_play_request_t *)arg;
    if (request)
    {
        (void)kr_sound_play_wav_file(
            request->path,
            request->label,
            request->play_id,
            request->volume,
            request->loop,
            request->running);
        free(request);
    }
    return NULL;
}

static int kr_sound_start_detached_play(
    const char *path,
    const char *label,
    unsigned int play_id,
    int volume)
{
    pthread_t thread;
    kr_sound_play_request_t *request = (kr_sound_play_request_t *)calloc(1, sizeof(*request));
    if (!request)
    {
        return -ENOMEM;
    }
    snprintf(request->path, sizeof(request->path), "%s", path);
    snprintf(request->label, sizeof(request->label), "%s", label ? label : "");
    request->play_id = play_id;
    request->volume = volume;
    request->loop = false;
    request->running = NULL;

    if (pthread_create(&thread, NULL, kr_sound_play_thread, request) != 0)
    {
        free(request);
        return -EIO;
    }
    pthread_detach(thread);
    fprintf(stderr,
            "kr_sound: detached play thread started id=%u label=%s path=%s volume=%d\n",
            play_id,
            label ? label : "",
            path ? path : "",
            volume);
    return 0;
}

int kr_sound_create(kr_sound_handle_t **out_handle, const kr_sound_config_t *cfg)
{
    kr_sound_handle_t *handle;
    if (!out_handle)
    {
        return -EINVAL;
    }

    handle = (kr_sound_handle_t *)calloc(1, sizeof(*handle));
    if (!handle)
    {
        return -ENOMEM;
    }
    if (kr_sound_copy_asset_dir(handle->asset_dir, sizeof(handle->asset_dir), cfg ? cfg->asset_dir : NULL) < 0)
    {
        free(handle);
        return -ENAMETOOLONG;
    }
    handle->volume_percent = kr_sound_clamp_volume(cfg ? cfg->volume_percent : KR_SOUND_DEFAULT_VOLUME);
    pthread_mutex_init(&handle->lock, NULL);
    (void)AudioCore_Init();
    *out_handle = handle;
    return 0;
}

int kr_sound_destroy(kr_sound_handle_t *handle)
{
    if (!handle)
    {
        return 0;
    }
    kr_sound_stop_ring(handle);
    pthread_mutex_destroy(&handle->lock);
    free(handle);
    return 0;
}

int kr_sound_play_touch(kr_sound_handle_t *handle)
{
    char path[PATH_MAX];
    long long now;
    int volume;
    unsigned int play_id;

    if (!handle)
    {
        return -EINVAL;
    }
    pthread_mutex_lock(&handle->lock);
    now = kr_sound_now_ms();
    if (now - handle->last_touch_ms < KR_SOUND_TOUCH_THROTTLE_MS)
    {
        pthread_mutex_unlock(&handle->lock);
        return 0;
    }
    handle->last_touch_ms = now;
    play_id = ++handle->next_play_id;
    volume = handle->volume_percent > 70 ? 70 : handle->volume_percent;
    if (kr_sound_build_path(path, sizeof(path), handle->asset_dir, "button.wav") < 0)
    {
        pthread_mutex_unlock(&handle->lock);
        return -ENAMETOOLONG;
    }
    pthread_mutex_unlock(&handle->lock);

    return kr_sound_start_detached_play(path, "touch", play_id, volume);
}

int kr_sound_start_ring(kr_sound_handle_t *handle, const char *kind, int melody, int volume_percent)
{
    kr_sound_play_request_t *request;

    if (!handle)
    {
        return -EINVAL;
    }
    kr_sound_stop_ring(handle);

    request = (kr_sound_play_request_t *)calloc(1, sizeof(*request));
    if (!request)
    {
        return -ENOMEM;
    }

    pthread_mutex_lock(&handle->lock);
    if (kr_sound_build_ring_path(request->path, sizeof(request->path), handle->asset_dir, kind, melody) < 0)
    {
        pthread_mutex_unlock(&handle->lock);
        free(request);
        return -ENAMETOOLONG;
    }
    handle->ring_running = 1;
    request->play_id = ++handle->next_play_id;
    snprintf(request->label, sizeof(request->label), "%s", "ring");
    request->volume = kr_sound_clamp_volume(volume_percent);
    request->loop = true;
    request->running = &handle->ring_running;
    if (pthread_create(&handle->ring_thread, NULL, kr_sound_play_thread, request) != 0)
    {
        handle->ring_running = 0;
        pthread_mutex_unlock(&handle->lock);
        free(request);
        return -EIO;
    }
    handle->ring_thread_valid = true;
    fprintf(stderr,
            "kr_sound: ring thread started id=%u kind=%s melody=%d volume=%d path=%s\n",
            request->play_id,
            kind ? kind : "",
            melody,
            request->volume,
            request->path);
    pthread_mutex_unlock(&handle->lock);
    return 0;
}

int kr_sound_stop_ring(kr_sound_handle_t *handle)
{
    pthread_t thread;
    bool join_thread = false;

    if (!handle)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    handle->ring_running = 0;
    if (handle->ring_thread_valid)
    {
        thread = handle->ring_thread;
        handle->ring_thread_valid = false;
        join_thread = true;
    }
    fprintf(stderr,
            "kr_sound: stop ring requested join=%d thread_valid=%d\n",
            join_thread ? 1 : 0,
            handle->ring_thread_valid ? 1 : 0);
    pthread_mutex_unlock(&handle->lock);

    if (join_thread)
    {
        fprintf(stderr, "kr_sound: stop ring joining thread\n");
        pthread_join(thread, NULL);
        fprintf(stderr, "kr_sound: stop ring joined thread\n");
    }
    return 0;
}

int kr_sound_set_volume(kr_sound_handle_t *handle, int volume_percent)
{
    if (!handle)
    {
        return -EINVAL;
    }
    pthread_mutex_lock(&handle->lock);
    handle->volume_percent = kr_sound_clamp_volume(volume_percent);
    pthread_mutex_unlock(&handle->lock);
    return 0;
}
