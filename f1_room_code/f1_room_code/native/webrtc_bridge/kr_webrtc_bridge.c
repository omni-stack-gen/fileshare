#include "kr_webrtc_bridge.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <utils/circlebuf.h>
#define LOG_TAG "webrtc-native"
#include <utils/log.h>
#include <utils/bmem.h>
#include <utils/event_queue.h>
#include <utils/util.h>
#include <AudioCore.h>
#include <video/vd/VDecApi.h>
#include <video/vd/VoApi.h>
#include "webrtc_streamer.h"

#define KR_WEBRTC_STREAM_MAX 2
#define KR_WEBRTC_SESSION_ID_MAX 128
#define KR_WEBRTC_AUDIO_SAMPLE_RATE 8000
#define KR_WEBRTC_AUDIO_SAMPLE_BITS 16
#define KR_WEBRTC_AUDIO_CHANNELS 1
#define KR_WEBRTC_AUDIO_BLOCK_SAMPLES 160
#define KR_WEBRTC_AUDIO_BLOCK_COUNT 4
#define KR_WEBRTC_AUDIO_PLAYBACK_MAX_BYTES 8000U
#define KR_WEBRTC_AUDIO_VOLUME 100
#define KR_WEBRTC_VIDEO_CHN 0
#define KR_WEBRTC_VIDEO_LAYER 0
#define KR_WEBRTC_VIDEO_X 24
#define KR_WEBRTC_VIDEO_Y 50
#define KR_WEBRTC_VIDEO_W 850
#define KR_WEBRTC_VIDEO_H 420
#define KR_WEBRTC_VIDEO_WIDTH 1920
#define KR_WEBRTC_VIDEO_HEIGHT 1088
#define KR_WEBRTC_VIDEO_BUF_SIZE 0x100000
#define KR_WEBRTC_VIDEO_FRAME_QUEUE_CAPACITY 6U
#define KR_WEBRTC_BRIDGE_STATS_INTERVAL_MS 2000ULL
#define KR_WEBRTC_BRIDGE_H264_DUMP_ENV "KR_ROOM_DUMP_BRIDGE_H264"
#define KR_WEBRTC_BRIDGE_H264_DUMP_DIR_ENV "KR_ROOM_DUMP_BRIDGE_H264_DIR"
#define KR_WEBRTC_BRIDGE_H264_DUMP_DEFAULT_DIR "/tmp/kr_room_bridge_dump"
#define KR_WEBRTC_BRIDGE_H264_DUMP_TAG_MAX 96
#define KR_WEBRTC_BRIDGE_H264_DUMP_PATH_MAX 256
#define KR_WEBRTC_TEST_H264_ENV "KR_ROOM_WEBRTC_TEST_H264"
#define KR_WEBRTC_TEST_H264_PATH_ENV "KR_ROOM_WEBRTC_TEST_H264_PATH"
#define KR_WEBRTC_TEST_H264_FPS_ENV "KR_ROOM_WEBRTC_TEST_H264_FPS"
#define KR_WEBRTC_TEST_H264_DEFAULT_PATH "bridge_test.h264"
#define KR_WEBRTC_TEST_H264_PATH_MAX 256
#define KR_WEBRTC_TEST_H264_DEFAULT_FPS 15
#define KR_WEBRTC_TEST_H264_MIN_FPS 1
#define KR_WEBRTC_TEST_H264_MAX_FPS 60
#define KR_WEBRTC_TEST_H264_LOG_INTERVAL_MS 5000ULL

typedef struct kr_webrtc_video_frame_node
{
    uint8_t *data;
    size_t len;
    uint32_t ts;
    struct kr_webrtc_video_frame_node *next;
} kr_webrtc_video_frame_node_t;

typedef struct kr_webrtc_video_decoder
{
    bool initialized;
    bool vo_enabled;
    bool layer_enabled;
    bool stop_requested;
    bool worker_started;
    bool first_packet_logged;
    bool first_frame_logged;
    int channel;
    unsigned int packet_count;
    unsigned int frame_count;
    unsigned int dropped_frames;
    size_t queued_frames;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_t worker;
    kr_webrtc_video_frame_node_t *queue_head;
    kr_webrtc_video_frame_node_t *queue_tail;
} kr_webrtc_video_decoder_t;

struct kr_webrtc_handle;

typedef struct kr_webrtc_session
{
    struct kr_webrtc_handle *owner;
    char session_id[KR_WEBRTC_SESSION_ID_MAX];
    bool enabled;
    bool is_callout;
    bool audio_running;
    bool audio_first_rx_logged;
    bool audio_first_playback_logged;
    int playback_stream;
    int record_stream;
    unsigned int audio_rx_count;
    unsigned int audio_playback_count;
    size_t audio_high_watermark;
    pthread_mutex_t audio_lock;
    struct circlebuf audio_packets;
    kr_webrtc_video_decoder_t video;
} kr_webrtc_session_t;

struct kr_webrtc_handle
{
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool lock_ready;
    bool running;
    bool logged_in;
    bool callbacks_enabled;
    bool call_busy;
    int preview_brightness;
    int playback_volume;
    bool media_bridge_mode;
    bool media_bridge_video_ready;
    bool media_bridge_video_drop_logged;
    bool media_bridge_video_wait_logged;
    uint64_t bridge_video_stats_start_ms;
    uint64_t bridge_video_stats_last_log_ms;
    unsigned int bridge_video_input_count;
    unsigned int bridge_video_ok_count;
    unsigned int bridge_video_fail_count;
    size_t bridge_video_bytes;
    size_t bridge_video_max_len;
    unsigned int send_queue_full_count;
    int send_queue_last_queue;
    int send_queue_last_bufsize;
    char bridge_h264_dump_tag[KR_WEBRTC_BRIDGE_H264_DUMP_TAG_MAX];
    FILE *bridge_h264_dump_file;
    char bridge_h264_dump_path[KR_WEBRTC_BRIDGE_H264_DUMP_PATH_MAX];
    unsigned int bridge_h264_dump_frames;
    size_t bridge_h264_dump_bytes;
    bool bridge_h264_dump_failed;
    bool test_h264_thread_running;
    bool test_h264_thread_joinable;
    bool test_h264_stop_requested;
    pthread_t test_h264_thread;
    char test_h264_path[KR_WEBRTC_TEST_H264_PATH_MAX];
    int test_h264_fps;
    kr_webrtc_remote_audio_sink_t remote_audio_sink;
    void *remote_audio_user;
    unsigned int active_callbacks;
    kr_webrtc_config_t cfg;
    unsigned int video_callback_frames;
    kr_webrtc_session_t sessions[KR_WEBRTC_STREAM_MAX];
    event_queue_t events;
};

static void kr_webrtc_session_stop_media(kr_webrtc_session_t *session, const char *reason);

static uint64_t kr_webrtc_monotonic_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return 0;
    }
    return ((uint64_t)ts.tv_sec * 1000ULL) + ((uint64_t)ts.tv_nsec / 1000000ULL);
}

static bool kr_webrtc_bridge_h264_dump_enabled(void)
{
    const char *value = getenv(KR_WEBRTC_BRIDGE_H264_DUMP_ENV);

    return value != NULL &&
           value[0] != '\0' &&
           strcmp(value, "0") != 0 &&
           strcmp(value, "false") != 0 &&
           strcmp(value, "FALSE") != 0 &&
           strcmp(value, "off") != 0 &&
           strcmp(value, "OFF") != 0;
}

static int kr_webrtc_mkdir_p(const char *path)
{
    char tmp[KR_WEBRTC_BRIDGE_H264_DUMP_PATH_MAX];
    size_t len;
    char *p;

    if (path == NULL || path[0] == '\0')
    {
        return -EINVAL;
    }

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (len == 0)
    {
        return -EINVAL;
    }
    if (tmp[len - 1] == '/')
    {
        tmp[len - 1] = '\0';
    }

    for (p = tmp + 1; *p != '\0'; ++p)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (mkdir(tmp, 0775) != 0 && errno != EEXIST)
            {
                return -errno;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, 0775) != 0 && errno != EEXIST)
    {
        return -errno;
    }
    return 0;
}

static void kr_webrtc_bridge_h264_dump_close_locked(kr_webrtc_handle_t *handle)
{
    FILE *file;
    unsigned int frames;
    size_t bytes;
    char path[KR_WEBRTC_BRIDGE_H264_DUMP_PATH_MAX];

    if (handle == NULL || handle->bridge_h264_dump_file == NULL)
    {
        return;
    }

    file = handle->bridge_h264_dump_file;
    frames = handle->bridge_h264_dump_frames;
    bytes = handle->bridge_h264_dump_bytes;
    snprintf(path, sizeof(path), "%s", handle->bridge_h264_dump_path);
    handle->bridge_h264_dump_file = NULL;
    handle->bridge_h264_dump_path[0] = '\0';
    handle->bridge_h264_dump_frames = 0;
    handle->bridge_h264_dump_bytes = 0;
    fclose(file);
    LOGI("bridge h264 dump closed stream=webrtc path=%s frames=%u bytes=%zu\n",
         path,
         frames,
         bytes);
}

static void kr_webrtc_bridge_h264_dump_open_locked(kr_webrtc_handle_t *handle)
{
    const char *dir;
    const char *tag;
    int ret;

    if (handle == NULL || handle->bridge_h264_dump_file != NULL)
    {
        return;
    }
    if (!kr_webrtc_bridge_h264_dump_enabled())
    {
        return;
    }

    dir = getenv(KR_WEBRTC_BRIDGE_H264_DUMP_DIR_ENV);
    if (dir == NULL || dir[0] == '\0')
    {
        dir = KR_WEBRTC_BRIDGE_H264_DUMP_DEFAULT_DIR;
    }

    ret = kr_webrtc_mkdir_p(dir);
    if (ret < 0)
    {
        LOGE("bridge h264 dump mkdir failed stream=webrtc dir=%s ret=%d\n", dir, ret);
        handle->bridge_h264_dump_failed = true;
        return;
    }

    tag = handle->bridge_h264_dump_tag[0] != '\0' ? handle->bridge_h264_dump_tag : "bridge_unknown";
    snprintf(handle->bridge_h264_dump_path,
             sizeof(handle->bridge_h264_dump_path),
             "%s/%s_webrtc.h264",
             dir,
             tag);
    handle->bridge_h264_dump_file = fopen(handle->bridge_h264_dump_path, "wb");
    if (handle->bridge_h264_dump_file == NULL)
    {
        LOGE("bridge h264 dump open failed stream=webrtc path=%s errno=%d\n",
             handle->bridge_h264_dump_path,
             errno);
        handle->bridge_h264_dump_failed = true;
        handle->bridge_h264_dump_path[0] = '\0';
        return;
    }

    handle->bridge_h264_dump_frames = 0;
    handle->bridge_h264_dump_bytes = 0;
    handle->bridge_h264_dump_failed = false;
    LOGI("bridge h264 dump open stream=webrtc path=%s\n", handle->bridge_h264_dump_path);
}

static void kr_webrtc_bridge_h264_dump_write(kr_webrtc_handle_t *handle,
                                             const unsigned char *data,
                                             size_t len)
{
    if (handle == NULL || data == NULL || len == 0)
    {
        return;
    }

    pthread_mutex_lock(&handle->lock);
    if (handle->bridge_h264_dump_file != NULL)
    {
        if (fwrite(data, 1, len, handle->bridge_h264_dump_file) != len)
        {
            LOGE("bridge h264 dump write failed stream=webrtc path=%s errno=%d\n",
                 handle->bridge_h264_dump_path,
                 errno);
            handle->bridge_h264_dump_failed = true;
            kr_webrtc_bridge_h264_dump_close_locked(handle);
        }
        else
        {
            handle->bridge_h264_dump_frames++;
            handle->bridge_h264_dump_bytes += len;
        }
    }
    pthread_mutex_unlock(&handle->lock);
}

static bool kr_webrtc_env_enabled(const char *name)
{
    const char *value = getenv(name);

    return value != NULL &&
           value[0] != '\0' &&
           strcmp(value, "0") != 0 &&
           strcmp(value, "false") != 0 &&
           strcmp(value, "FALSE") != 0 &&
           strcmp(value, "off") != 0 &&
           strcmp(value, "OFF") != 0;
}

static int kr_webrtc_test_h264_parse_fps(void)
{
    const char *value = getenv(KR_WEBRTC_TEST_H264_FPS_ENV);
    char *end = NULL;
    long fps;

    if (value == NULL || value[0] == '\0')
    {
        return KR_WEBRTC_TEST_H264_DEFAULT_FPS;
    }

    fps = strtol(value, &end, 10);
    if (end == value || fps < KR_WEBRTC_TEST_H264_MIN_FPS)
    {
        return KR_WEBRTC_TEST_H264_DEFAULT_FPS;
    }
    if (fps > KR_WEBRTC_TEST_H264_MAX_FPS)
    {
        return KR_WEBRTC_TEST_H264_MAX_FPS;
    }
    return (int)fps;
}

static bool kr_webrtc_test_h264_stop_requested(kr_webrtc_handle_t *handle)
{
    bool stop_requested;

    pthread_mutex_lock(&handle->lock);
    stop_requested = handle->test_h264_stop_requested || !handle->running || !handle->logged_in;
    pthread_mutex_unlock(&handle->lock);
    return stop_requested;
}

static size_t kr_webrtc_test_h264_start_code_len(const unsigned char *data, size_t len, size_t pos)
{
    if (pos + 4 <= len &&
        data[pos] == 0x00 &&
        data[pos + 1] == 0x00 &&
        data[pos + 2] == 0x00 &&
        data[pos + 3] == 0x01)
    {
        return 4;
    }
    if (pos + 3 <= len &&
        data[pos] == 0x00 &&
        data[pos + 1] == 0x00 &&
        data[pos + 2] == 0x01)
    {
        return 3;
    }
    return 0;
}

static size_t kr_webrtc_test_h264_find_start_code(const unsigned char *data, size_t len, size_t pos)
{
    while (pos + 3 <= len)
    {
        if (kr_webrtc_test_h264_start_code_len(data, len, pos) > 0)
        {
            return pos;
        }
        pos++;
    }
    return len;
}

static int kr_webrtc_test_h264_load_file(const char *path, unsigned char **out_data, size_t *out_len)
{
    FILE *file;
    long file_size;
    unsigned char *data;
    size_t read_len;

    if (path == NULL || out_data == NULL || out_len == NULL)
    {
        return -EINVAL;
    }

    file = fopen(path, "rb");
    if (file == NULL)
    {
        LOGE("test h264 open failed path=%s errno=%d\n", path, errno);
        return -errno;
    }
    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return -errno;
    }
    file_size = ftell(file);
    if (file_size <= 0)
    {
        fclose(file);
        LOGE("test h264 invalid file size path=%s size=%ld\n", path, file_size);
        return -EINVAL;
    }
    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return -errno;
    }

    data = (unsigned char *)bmalloc((size_t)file_size);
    if (data == NULL)
    {
        fclose(file);
        return -ENOMEM;
    }

    read_len = fread(data, 1, (size_t)file_size, file);
    fclose(file);
    if (read_len != (size_t)file_size)
    {
        LOGE("test h264 read failed path=%s read=%zu expect=%ld\n", path, read_len, file_size);
        bfree(data);
        return -EIO;
    }

    *out_data = data;
    *out_len = read_len;
    return 0;
}

static int kr_webrtc_test_h264_send_nal(
    const unsigned char *sps,
    size_t sps_len,
    const unsigned char *pps,
    size_t pps_len,
    const unsigned char *nal,
    size_t nal_len,
    bool is_idr
)
{
    unsigned char *merged = NULL;
    size_t merged_len;
    int ret;

    if (!is_idr || (sps_len == 0 && pps_len == 0))
    {
        return webrtc_streamer_input_video_data(
            WEBRTC_STREAM_MAIN,
            WEBRTC_VIDEO_H264,
            (unsigned char *)nal,
            nal_len
        );
    }

    merged_len = sps_len + pps_len + nal_len;
    merged = (unsigned char *)bmalloc(merged_len);
    if (merged == NULL)
    {
        return -ENOMEM;
    }
    if (sps_len > 0)
    {
        memcpy(merged, sps, sps_len);
    }
    if (pps_len > 0)
    {
        memcpy(merged + sps_len, pps, pps_len);
    }
    memcpy(merged + sps_len + pps_len, nal, nal_len);

    ret = webrtc_streamer_input_video_data(
        WEBRTC_STREAM_MAIN,
        WEBRTC_VIDEO_H264,
        merged,
        merged_len
    );
    bfree(merged);
    return ret;
}

static void *kr_webrtc_test_h264_thread_main(void *arg)
{
    kr_webrtc_handle_t *handle = (kr_webrtc_handle_t *)arg;
    unsigned char *data = NULL;
    unsigned char *sps = NULL;
    unsigned char *pps = NULL;
    size_t data_len = 0;
    size_t sps_len = 0;
    size_t pps_len = 0;
    char path[KR_WEBRTC_TEST_H264_PATH_MAX];
    int fps;
    useconds_t frame_delay_us;
    unsigned int sent_count = 0;
    unsigned int fail_count = 0;
    size_t sent_bytes = 0;
    uint64_t last_log_ms;
    int ret;

    pthread_mutex_lock(&handle->lock);
    snprintf(path, sizeof(path), "%s", handle->test_h264_path);
    fps = handle->test_h264_fps > 0 ? handle->test_h264_fps : KR_WEBRTC_TEST_H264_DEFAULT_FPS;
    pthread_mutex_unlock(&handle->lock);

    ret = kr_webrtc_test_h264_load_file(path, &data, &data_len);
    if (ret < 0)
    {
        goto exit_thread;
    }
    if (kr_webrtc_test_h264_find_start_code(data, data_len, 0) >= data_len)
    {
        LOGE("test h264 parse failed no annex-b start code path=%s len=%zu\n", path, data_len);
        goto exit_thread;
    }

    frame_delay_us = (useconds_t)(1000000U / (unsigned int)fps);
    last_log_ms = kr_webrtc_monotonic_ms();
    LOGI("test h264 thread started path=%s fps=%d bytes=%zu\n", path, fps, data_len);

    while (!kr_webrtc_test_h264_stop_requested(handle))
    {
        size_t pos = kr_webrtc_test_h264_find_start_code(data, data_len, 0);
        unsigned int pass_sent_start = sent_count;

        while (pos < data_len && !kr_webrtc_test_h264_stop_requested(handle))
        {
            size_t start_len = kr_webrtc_test_h264_start_code_len(data, data_len, pos);
            size_t nal_start;
            size_t next;
            size_t nal_len;
            unsigned char nal_type;
            uint64_t now_ms;

            if (start_len == 0)
            {
                break;
            }
            nal_start = pos + start_len;
            next = kr_webrtc_test_h264_find_start_code(data, data_len, nal_start);
            nal_len = next > pos ? next - pos : data_len - pos;
            if (nal_start >= data_len || nal_len <= start_len)
            {
                pos = next;
                continue;
            }

            nal_type = data[nal_start] & 0x1f;
            if (nal_type == 7)
            {
                unsigned char *copy = (unsigned char *)bmalloc(nal_len);
                if (copy != NULL)
                {
                    memcpy(copy, data + pos, nal_len);
                    bfree(sps);
                    sps = copy;
                    sps_len = nal_len;
                }
            }
            else if (nal_type == 8)
            {
                unsigned char *copy = (unsigned char *)bmalloc(nal_len);
                if (copy != NULL)
                {
                    memcpy(copy, data + pos, nal_len);
                    bfree(pps);
                    pps = copy;
                    pps_len = nal_len;
                }
            }
            else if (nal_type == 5 || nal_type == 1)
            {
                ret = kr_webrtc_test_h264_send_nal(
                    sps,
                    sps_len,
                    pps,
                    pps_len,
                    data + pos,
                    nal_len,
                    nal_type == 5
                );
                if (ret < 0)
                {
                    fail_count++;
                }
                else
                {
                    sent_count++;
                    sent_bytes += nal_len;
                }

                now_ms = kr_webrtc_monotonic_ms();
                if (now_ms != 0 &&
                    last_log_ms != 0 &&
                    now_ms - last_log_ms >= KR_WEBRTC_TEST_H264_LOG_INTERVAL_MS)
                {
                    LOGI("test h264 send stats path=%s sent=%u fail=%u bytes=%zu\n",
                         path,
                         sent_count,
                         fail_count,
                         sent_bytes);
                    last_log_ms = now_ms;
                }
                usleep(frame_delay_us);
            }

            pos = next;
        }
        if (sent_count == pass_sent_start && !kr_webrtc_test_h264_stop_requested(handle))
        {
            LOGE("test h264 parse failed no sendable video frame path=%s len=%zu\n", path, data_len);
            break;
        }
    }

exit_thread:
    LOGI("test h264 thread exit path=%s sent=%u fail=%u bytes=%zu\n",
         path,
         sent_count,
         fail_count,
         sent_bytes);
    bfree(data);
    bfree(sps);
    bfree(pps);

    pthread_mutex_lock(&handle->lock);
    handle->test_h264_thread_running = false;
    handle->test_h264_stop_requested = false;
    pthread_mutex_unlock(&handle->lock);
    return NULL;
}

static void kr_webrtc_test_h264_start(kr_webrtc_handle_t *handle)
{
    const char *path;
    int fps;
    int ret;

    if (handle == NULL || !kr_webrtc_env_enabled(KR_WEBRTC_TEST_H264_ENV))
    {
        return;
    }

    path = getenv(KR_WEBRTC_TEST_H264_PATH_ENV);
    if (path == NULL || path[0] == '\0')
    {
        path = KR_WEBRTC_TEST_H264_DEFAULT_PATH;
    }
    fps = kr_webrtc_test_h264_parse_fps();

    pthread_mutex_lock(&handle->lock);
    if (!handle->running ||
        !handle->logged_in ||
        handle->test_h264_thread_running ||
        handle->test_h264_thread_joinable)
    {
        pthread_mutex_unlock(&handle->lock);
        return;
    }
    snprintf(handle->test_h264_path, sizeof(handle->test_h264_path), "%s", path);
    handle->test_h264_fps = fps;
    handle->test_h264_stop_requested = false;
    handle->test_h264_thread_running = true;
    handle->test_h264_thread_joinable = true;
    ret = pthread_create(&handle->test_h264_thread, NULL, kr_webrtc_test_h264_thread_main, handle);
    if (ret != 0)
    {
        handle->test_h264_thread_running = false;
        handle->test_h264_thread_joinable = false;
        handle->test_h264_stop_requested = false;
        pthread_mutex_unlock(&handle->lock);
        LOGE("test h264 pthread_create failed path=%s ret=%d\n", path, ret);
        return;
    }
    pthread_mutex_unlock(&handle->lock);

    LOGI("test h264 start requested path=%s fps=%d\n", path, fps);
}

static void kr_webrtc_test_h264_stop(kr_webrtc_handle_t *handle)
{
    pthread_t thread;
    bool should_join;

    if (handle == NULL)
    {
        return;
    }

    pthread_mutex_lock(&handle->lock);
    should_join = handle->test_h264_thread_joinable;
    thread = handle->test_h264_thread;
    if (handle->test_h264_thread_running)
    {
        handle->test_h264_stop_requested = true;
    }
    pthread_mutex_unlock(&handle->lock);

    if (should_join)
    {
        pthread_join(thread, NULL);
        pthread_mutex_lock(&handle->lock);
        handle->test_h264_thread_joinable = false;
        pthread_mutex_unlock(&handle->lock);
    }
}

static int kr_webrtc_clamp_level(int level)
{
    if (level < 0)
    {
        return 0;
    }
    if (level > 100)
    {
        return 100;
    }
    return level;
}

static int kr_webrtc_clamp_volume(int volume)
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

static void kr_webrtc_apply_preview_cm(int brightness)
{
    VO_LAYER_CM cm;

    memset(&cm, 0, sizeof(cm));
    cm.Brightness = kr_webrtc_clamp_level(brightness);
    cm.Saturation = 50;
    cm.Hue = 50;
    cm.Contrast = 50;
    (void)VO_ChnSetCM(KR_WEBRTC_VIDEO_CHN, KR_WEBRTC_VIDEO_LAYER, &cm);
}

static bool kr_webrtc_callback_enter(kr_webrtc_handle_t *handle)
{
    if (handle == NULL)
    {
        return false;
    }

    pthread_mutex_lock(&handle->lock);
    if (!handle->callbacks_enabled)
    {
        pthread_mutex_unlock(&handle->lock);
        return false;
    }
    handle->active_callbacks++;
    pthread_mutex_unlock(&handle->lock);
    return true;
}

static void kr_webrtc_callback_leave(kr_webrtc_handle_t *handle)
{
    if (handle == NULL)
    {
        return;
    }

    pthread_mutex_lock(&handle->lock);
    if (handle->active_callbacks > 0)
    {
        handle->active_callbacks--;
    }
    if (handle->active_callbacks == 0)
    {
        pthread_cond_broadcast(&handle->cond);
    }
    pthread_mutex_unlock(&handle->lock);
}

static void kr_webrtc_wait_callbacks_idle(kr_webrtc_handle_t *handle)
{
    if (handle == NULL)
    {
        return;
    }

    pthread_mutex_lock(&handle->lock);
    while (handle->active_callbacks > 0)
    {
        pthread_cond_wait(&handle->cond, &handle->lock);
    }
    pthread_mutex_unlock(&handle->lock);
}

static void kr_webrtc_copy_session_id(char *dst, size_t dst_len, const char *src, size_t src_len)
{
    size_t copy_len;

    if (dst_len == 0)
    {
        return;
    }
    if (src == NULL || src_len == 0)
    {
        dst[0] = '\0';
        return;
    }

    copy_len = src_len < dst_len - 1 ? src_len : dst_len - 1;
    memcpy(dst, src, copy_len);
    dst[copy_len] = '\0';
}

static bool kr_webrtc_session_id_equals(
    const kr_webrtc_session_t *session,
    const char *session_id,
    size_t session_id_len
)
{
    size_t stored_len;

    if (session == NULL || !session->enabled || session_id == NULL || session_id_len == 0)
    {
        return false;
    }

    stored_len = strlen(session->session_id);
    return stored_len == session_id_len && memcmp(session->session_id, session_id, session_id_len) == 0;
}

static void kr_webrtc_video_queue_clear_locked(kr_webrtc_video_decoder_t *decoder)
{
    kr_webrtc_video_frame_node_t *node;

    if (decoder == NULL)
    {
        return;
    }

    node = decoder->queue_head;
    while (node != NULL)
    {
        kr_webrtc_video_frame_node_t *next = node->next;
        bfree(node->data);
        bfree(node);
        node = next;
    }
    decoder->queue_head = NULL;
    decoder->queue_tail = NULL;
    decoder->queued_frames = 0;
}

static int kr_webrtc_video_queue_push_locked(
    kr_webrtc_video_decoder_t *decoder,
    uint8_t *data,
    size_t len,
    uint32_t ts
)
{
    kr_webrtc_video_frame_node_t *node;

    if (decoder == NULL || data == NULL || len == 0)
    {
        return -EINVAL;
    }

    node = (kr_webrtc_video_frame_node_t *)bzalloc(sizeof(*node));
    if (node == NULL)
    {
        return -ENOMEM;
    }
    node->data = data;
    node->len = len;
    node->ts = ts;

    if (decoder->queued_frames >= KR_WEBRTC_VIDEO_FRAME_QUEUE_CAPACITY && decoder->queue_head != NULL)
    {
        kr_webrtc_video_frame_node_t *drop = decoder->queue_head;
        decoder->queue_head = drop->next;
        if (decoder->queue_head == NULL)
        {
            decoder->queue_tail = NULL;
        }
        bfree(drop->data);
        bfree(drop);
        decoder->queued_frames--;
        decoder->dropped_frames++;
        if (decoder->dropped_frames == 1 || (decoder->dropped_frames % 10U) == 0U)
        {
            LOGW("video queue drop_old count=%u\n", decoder->dropped_frames);
        }
    }

    if (decoder->queue_tail != NULL)
    {
        decoder->queue_tail->next = node;
    }
    else
    {
        decoder->queue_head = node;
    }
    decoder->queue_tail = node;
    decoder->queued_frames++;
    pthread_cond_signal(&decoder->cond);
    return 0;
}

static kr_webrtc_video_frame_node_t *kr_webrtc_video_queue_pop_locked(
    kr_webrtc_video_decoder_t *decoder
)
{
    kr_webrtc_video_frame_node_t *node;

    if (decoder == NULL || decoder->queue_head == NULL)
    {
        return NULL;
    }

    node = decoder->queue_head;
    decoder->queue_head = node->next;
    if (decoder->queue_head == NULL)
    {
        decoder->queue_tail = NULL;
    }
    decoder->queued_frames--;
    node->next = NULL;
    return node;
}

static int kr_webrtc_video_decoder_process_frame(
    kr_webrtc_session_t *session,
    const uint8_t *frame,
    size_t len,
    uint32_t ts
)
{
    kr_webrtc_video_decoder_t *decoder = &session->video;
    VDEC_STREAM_S stream;
    VDEC_FRAME_S image;
    VDEC_CHN_STAT_S stat;
    int ret;

    if (session == NULL || frame == NULL || len == 0 || !decoder->initialized || decoder->channel < 0)
    {
        return -EINVAL;
    }

    memset(&stream, 0, sizeof(stream));
    stream.pu8Addr = (unsigned char *)frame;
    stream.u32Len = (unsigned int)len;
    stream.u64PTS = ts;

    if (VDEC_SendStream(decoder->channel, &stream) != 1)
    {
        LOGE("VDEC_SendStream failed channel=%d len=%u ts=%u\n",
                decoder->channel, stream.u32Len, ts);
        return -EIO;
    }

    memset(&stat, 0, sizeof(stat));
    if (VDEC_Query(decoder->channel, &stat) != 1 || stat.u32FrameNum == 0)
    {
        return 0;
    }

    ret = VDEC_StartRecvStream(decoder->channel, 0, 0, 0);
    if (ret != VDEC_FRAME_DECODED && ret != VDEC_KEYFRAME_DECODED)
    {
        if (ret == VDEC_NO_FRAME_BUFFER || ret == VDEC_NO_BITSTREAM ||
            ret == VDEC_RESOLUTION_CHANGE || ret == VDEC_UNSUPPORTED ||
            ret == VDEC_CHANNEL_ERROR)
        {
            LOGE("VDEC_StartRecvStream status=%d channel=%d queued_frames=%u\n",
                    ret, decoder->channel, stat.u32FrameNum);
        }
        return 0;
    }

    memset(&image, 0, sizeof(image));
    if (VDEC_GetImage(decoder->channel, &image) != 1)
    {
        LOGE("VDEC_GetImage failed channel=%d\n", decoder->channel);
        return -EIO;
    }

    if (!VO_SetZoomInWindow(KR_WEBRTC_VIDEO_CHN, KR_WEBRTC_VIDEO_LAYER, &image.SrcInfo))
    {
        (void)VDEC_ReleaseImage(decoder->channel, &image);
        return -EIO;
    }

    if (!VO_ChnShow(KR_WEBRTC_VIDEO_CHN, KR_WEBRTC_VIDEO_LAYER, &image))
    {
        (void)VDEC_ReleaseImage(decoder->channel, &image);
        LOGE("VO_ChnShow failed channel=%d layer=%d\n",
                KR_WEBRTC_VIDEO_CHN, KR_WEBRTC_VIDEO_LAYER);
        return -EIO;
    }

    decoder->frame_count++;
    if (!decoder->first_frame_logged)
    {
        decoder->first_frame_logged = true;
        LOGI("first video frame shown session=%s channel=%d src=%ux%u stride=%u pts=%llu\n",
                session->session_id,
                decoder->channel,
                image.SrcInfo.W,
                image.SrcInfo.H,
                image.u32Stride,
                image.u64pts);
    }

    if (!VDEC_ReleaseImage(decoder->channel, &image))
    {
        LOGE("VDEC_ReleaseImage failed channel=%d\n", decoder->channel);
        return -EIO;
    }

    return 0;
}

static void *kr_webrtc_video_decoder_worker(void *user_ctx)
{
    kr_webrtc_session_t *session = (kr_webrtc_session_t *)user_ctx;
    kr_webrtc_video_decoder_t *decoder;

    if (session == NULL)
    {
        return NULL;
    }

    decoder = &session->video;
    LOGI("video worker start session=%s tid=%lu\n",
            session->session_id, (unsigned long)pthread_self());

    for (;;)
    {
        kr_webrtc_video_frame_node_t *node;

        pthread_mutex_lock(&decoder->lock);
        while (!decoder->stop_requested && decoder->queue_head == NULL)
        {
            pthread_cond_wait(&decoder->cond, &decoder->lock);
        }
        if (decoder->stop_requested && decoder->queue_head == NULL)
        {
            pthread_mutex_unlock(&decoder->lock);
            break;
        }

        node = kr_webrtc_video_queue_pop_locked(decoder);
        pthread_mutex_unlock(&decoder->lock);

        if (node == NULL)
        {
            continue;
        }

        (void)kr_webrtc_video_decoder_process_frame(session, node->data, node->len, node->ts);
        bfree(node->data);
        bfree(node);
    }

    LOGI("video worker exit session=%s\n", session->session_id);
    return NULL;
}

static int kr_webrtc_video_decoder_start(kr_webrtc_session_t *session)
{
    kr_webrtc_video_decoder_t *decoder;
    VO_LAYER_INFO layer_info;
    VDEC_CHN_ATTR_S dec_attr;
    int channel;
    int brightness;
    int ret;

    if (session == NULL)
    {
        return -EINVAL;
    }

    decoder = &session->video;
    if (decoder->initialized)
    {
        return 0;
    }

    LOGI("video decoder start session=%s area={x=%d,y=%d,w=%d,h=%d} stream=%ux%u\n",
            session->session_id,
            KR_WEBRTC_VIDEO_X,
            KR_WEBRTC_VIDEO_Y,
            KR_WEBRTC_VIDEO_W,
            KR_WEBRTC_VIDEO_H,
            KR_WEBRTC_VIDEO_WIDTH,
            KR_WEBRTC_VIDEO_HEIGHT);

    if (VO_Enable() != 1)
    {
        LOGE("VO_Enable failed\n");
        return -EIO;
    }
    decoder->vo_enabled = true;

    memset(&layer_info, 0, sizeof(layer_info));
    layer_info.Rect.X = KR_WEBRTC_VIDEO_X;
    layer_info.Rect.Y = KR_WEBRTC_VIDEO_Y;
    layer_info.Rect.W = KR_WEBRTC_VIDEO_W;
    layer_info.Rect.H = KR_WEBRTC_VIDEO_H;
    layer_info.Rotate = VO_ROTATE_NONE;

    if (VO_EnableVideoLayer(KR_WEBRTC_VIDEO_CHN, KR_WEBRTC_VIDEO_LAYER, &layer_info) != 1)
    {
        LOGE("VO_EnableVideoLayer failed ch=%d layer=%d\n",
                KR_WEBRTC_VIDEO_CHN, KR_WEBRTC_VIDEO_LAYER);
        VO_Disable();
        decoder->vo_enabled = false;
        return -EIO;
    }
    decoder->layer_enabled = true;
    pthread_mutex_lock(&session->owner->lock);
    brightness = session->owner->preview_brightness;
    pthread_mutex_unlock(&session->owner->lock);
    kr_webrtc_apply_preview_cm(brightness);

    if (VDEC_Init() != 1)
    {
        LOGE("VDEC_Init failed\n");
        VO_DisableVideoLayer(KR_WEBRTC_VIDEO_CHN, KR_WEBRTC_VIDEO_LAYER);
        VO_Disable();
        decoder->layer_enabled = false;
        decoder->vo_enabled = false;
        return -EIO;
    }

    memset(&dec_attr, 0, sizeof(dec_attr));
    dec_attr.deType = VDEC_PAYLOAD_TYPE_H264;
    dec_attr.FormatType = FORMAT_NV12;
    dec_attr.BufSize = KR_WEBRTC_VIDEO_BUF_SIZE;
    dec_attr.u32PicWidth = KR_WEBRTC_VIDEO_WIDTH;
    dec_attr.u32PicHight = KR_WEBRTC_VIDEO_HEIGHT;

    channel = VDEC_CreateChn(&dec_attr);
    if (channel < 0)
    {
        LOGE("VDEC_CreateChn failed width=%u height=%u buf=0x%x\n",
                dec_attr.u32PicWidth, dec_attr.u32PicHight, dec_attr.BufSize);
        VDEC_DeInit();
        VO_DisableVideoLayer(KR_WEBRTC_VIDEO_CHN, KR_WEBRTC_VIDEO_LAYER);
        VO_Disable();
        decoder->layer_enabled = false;
        decoder->vo_enabled = false;
        return -EIO;
    }

    decoder->channel = channel;
    decoder->initialized = true;
    decoder->stop_requested = false;
    decoder->first_packet_logged = false;
    decoder->first_frame_logged = false;
    decoder->packet_count = 0;
    decoder->frame_count = 0;
    decoder->dropped_frames = 0;
    ret = pthread_create(&decoder->worker, NULL, kr_webrtc_video_decoder_worker, session);
    if (ret != 0)
    {
        LOGE("video worker pthread_create failed ret=%d\n", ret);
        (void)VDEC_DestroyChn(channel);
        (void)VDEC_DeInit();
        (void)VO_DisableVideoLayer(KR_WEBRTC_VIDEO_CHN, KR_WEBRTC_VIDEO_LAYER);
        (void)VO_Disable();
        decoder->channel = -1;
        decoder->initialized = false;
        decoder->layer_enabled = false;
        decoder->vo_enabled = false;
        return -ret;
    }
    decoder->worker_started = true;
    LOGI("video decoder started session=%s channel=%d\n",
            session->session_id, channel);
    return 0;
}

static void kr_webrtc_video_decoder_stop(kr_webrtc_session_t *session)
{
    kr_webrtc_video_decoder_t *decoder;

    if (session == NULL)
    {
        return;
    }

    decoder = &session->video;
    LOGI("video decoder stop session=%s channel=%d packets=%u frames=%u\n",
            session->session_id, decoder->channel, decoder->packet_count, decoder->frame_count);

    pthread_mutex_lock(&decoder->lock);
    decoder->stop_requested = true;
    kr_webrtc_video_queue_clear_locked(decoder);
    pthread_cond_broadcast(&decoder->cond);
    pthread_mutex_unlock(&decoder->lock);

    if (decoder->worker_started)
    {
        pthread_join(decoder->worker, NULL);
        decoder->worker_started = false;
    }

    if (decoder->channel >= 0)
    {
        (void)VDEC_DestroyChn(decoder->channel);
        decoder->channel = -1;
    }
    if (decoder->initialized)
    {
        (void)VDEC_DeInit();
    }
    if (decoder->layer_enabled)
    {
        (void)VO_DisableVideoLayer(KR_WEBRTC_VIDEO_CHN, KR_WEBRTC_VIDEO_LAYER);
        decoder->layer_enabled = false;
    }
    if (decoder->vo_enabled)
    {
        (void)VO_Disable();
        decoder->vo_enabled = false;
    }

    decoder->initialized = false;
    decoder->stop_requested = false;
}

static bool kr_webrtc_h264_has_start_code(const uint8_t *data, size_t len)
{
    if (data == NULL)
    {
        return false;
    }
    if (len >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01)
    {
        return true;
    }
    return len >= 3 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01;
}

static int kr_webrtc_video_decoder_input(
    kr_webrtc_session_t *session,
    const uint8_t *frame,
    size_t len,
    uint32_t ts
)
{
    static const uint8_t start_code[] = {0x00, 0x00, 0x00, 0x01};
    kr_webrtc_video_decoder_t *decoder;
    uint8_t *queued_frame;
    size_t queued_len;
    bool has_start_code;
    int ret;

    if (session == NULL || frame == NULL || len == 0)
    {
        return -EINVAL;
    }

    decoder = &session->video;
    if (!decoder->initialized)
    {
        ret = kr_webrtc_video_decoder_start(session);
        if (ret < 0)
        {
            return ret;
        }
    }

    has_start_code = kr_webrtc_h264_has_start_code(frame, len);
    queued_len = len + (has_start_code ? 0 : sizeof(start_code));
    queued_frame = (uint8_t *)bmalloc(queued_len);
    if (queued_frame == NULL)
    {
        return -ENOMEM;
    }

    if (!has_start_code)
    {
        memcpy(queued_frame, start_code, sizeof(start_code));
        memcpy(queued_frame + sizeof(start_code), frame, len);
    }
    else
    {
        memcpy(queued_frame, frame, len);
    }

    pthread_mutex_lock(&decoder->lock);
    if (!decoder->initialized || decoder->channel < 0 || decoder->stop_requested)
    {
        pthread_mutex_unlock(&decoder->lock);
        bfree(queued_frame);
        return -EINVAL;
    }
    decoder->packet_count++;
    if (!decoder->first_packet_logged)
    {
        decoder->first_packet_logged = true;
        LOGI("first video packet session=%s channel=%d len=%zu ts=%u start_code=%u\n",
                session->session_id, decoder->channel, queued_len, ts, has_start_code ? 1U : 0U);
    }
    ret = kr_webrtc_video_queue_push_locked(decoder, queued_frame, queued_len, ts);
    pthread_mutex_unlock(&decoder->lock);
    if (ret < 0)
    {
        bfree(queued_frame);
        return ret;
    }

    return 0;
}

static int kr_webrtc_audio_playback_cb(char *data_buffer, int data_size, void *ctx)
{
    kr_webrtc_session_t *session = (kr_webrtc_session_t *)ctx;
    kr_webrtc_handle_t *handle = session != NULL ? session->owner : NULL;
    size_t copy_len;
    size_t queued_before;
    size_t queued_after;
    bool bridge_mode;

    if (session == NULL || handle == NULL || data_buffer == NULL || data_size <= 0)
    {
        return 0;
    }

    memset(data_buffer, 0, (size_t)data_size);
    if (!session->audio_running)
    {
        return data_size;
    }
    pthread_mutex_lock(&handle->lock);
    bridge_mode = handle->media_bridge_mode;
    pthread_mutex_unlock(&handle->lock);
    if (bridge_mode)
    {
        return data_size;
    }

    pthread_mutex_lock(&session->audio_lock);
    queued_before = session->audio_packets.size;
    copy_len = queued_before >= (size_t)data_size ? (size_t)data_size : queued_before;
    if (copy_len > 0)
    {
        circlebuf_pop_front(&session->audio_packets, data_buffer, copy_len);
    }
    queued_after = session->audio_packets.size;
    session->audio_playback_count++;
    if (!session->audio_first_playback_logged)
    {
        session->audio_first_playback_logged = true;
        LOGI("first audio playback callback session=%s request=%d queued_before=%zu queued_after=%zu\n",
                session->session_id, data_size, queued_before, queued_after);
    }
    pthread_mutex_unlock(&session->audio_lock);

    return data_size;
}

static int kr_webrtc_audio_capture_cb(char *data_buffer, int data_size, void *ctx)
{
    kr_webrtc_session_t *session = (kr_webrtc_session_t *)ctx;
    kr_webrtc_handle_t *handle = session != NULL ? session->owner : NULL;
    bool bridge_mode;

    if (session == NULL || handle == NULL || !handle->running || !session->audio_running)
    {
        return 0;
    }

    pthread_mutex_lock(&handle->lock);
    bridge_mode = handle->media_bridge_mode;
    pthread_mutex_unlock(&handle->lock);
    if (bridge_mode)
    {
        return data_size > 0 ? data_size : 0;
    }

    if (data_buffer != NULL && data_size > 0)
    {
        (void)webrtc_streamer_input_audio_data((unsigned char *)data_buffer, (size_t)data_size);
    }

    return data_size > 0 ? data_size : 0;
}

static int kr_webrtc_open_audio_for_session(kr_webrtc_session_t *session, const char *reason)
{
    kr_webrtc_handle_t *handle;
    const char *playback_device;
    const char *record_device;
    int playback_volume;
    bool bridge_mode;
    int stream_id;

    if (session == NULL || session->owner == NULL)
    {
        return -EINVAL;
    }

    handle = session->owner;
    playback_device = handle->cfg.playback_device != NULL ? handle->cfg.playback_device : "default";
    record_device = handle->cfg.record_device != NULL ? handle->cfg.record_device : "default";
    pthread_mutex_lock(&handle->lock);
    playback_volume = handle->playback_volume;
    bridge_mode = handle->media_bridge_mode;
    pthread_mutex_unlock(&handle->lock);

    if (!bridge_mode && handle->cfg.enable_audio_output && session->playback_stream == 0)
    {
        stream_id = Audio_Player_Open();
        if (stream_id <= 0)
        {
            LOGE("audio playback open failed session=%s reason=%s\n",
                 session->session_id,
                 reason);
            return -EINVAL;
        }

        if (!Audio_Player_SetDevice(stream_id, playback_device) ||
            !Audio_Player_SetFormat(stream_id,
                                    KR_WEBRTC_AUDIO_CHANNELS,
                                    KR_WEBRTC_AUDIO_SAMPLE_BITS,
                                    KR_WEBRTC_AUDIO_SAMPLE_RATE) ||
            !Audio_Player_SetCache(stream_id,
                                   KR_WEBRTC_AUDIO_BLOCK_COUNT,
                                   KR_WEBRTC_AUDIO_BLOCK_SAMPLES) ||
            !Audio_Player_SetMode(stream_id, false, kr_webrtc_audio_playback_cb, session) ||
            !Audio_Player_SetVolume(stream_id, playback_volume) ||
            !Audio_Player_Start(stream_id))
        {
            bool close_ok = Audio_Player_Close(stream_id);
            LOGE("audio playback setup failed session=%s reason=%s stream=%d close_ok=%d\n",
                 session->session_id,
                 reason,
                 stream_id,
                 close_ok ? 1 : 0);
            return -EINVAL;
        }
        session->playback_stream = stream_id;
    }

    if (!bridge_mode && handle->cfg.enable_audio_input && session->record_stream == 0)
    {
        stream_id = Audio_Recorder_Open();
        if (stream_id <= 0)
        {
            LOGE("audio record open failed session=%s reason=%s\n",
                 session->session_id,
                 reason);
            kr_webrtc_session_stop_media(session, "open_record_failed");
            return -EINVAL;
        }

        if (!Audio_Recorder_SetDevice(stream_id, record_device) ||
            !Audio_Recorder_SetFormat(stream_id,
                                      KR_WEBRTC_AUDIO_CHANNELS,
                                      KR_WEBRTC_AUDIO_SAMPLE_BITS,
                                      KR_WEBRTC_AUDIO_SAMPLE_RATE) ||
            !Audio_Recorder_SetCache(stream_id,
                                     KR_WEBRTC_AUDIO_BLOCK_COUNT,
                                     KR_WEBRTC_AUDIO_BLOCK_SAMPLES) ||
            !Audio_Recorder_SetMode(stream_id, false, kr_webrtc_audio_capture_cb, session) ||
            !Audio_Recorder_SetVolume(stream_id, KR_WEBRTC_AUDIO_VOLUME) ||
            !Audio_Recorder_Start(stream_id))
        {
            bool close_ok = Audio_Recorder_Close(stream_id);
            LOGE("audio record setup failed session=%s reason=%s stream=%d close_ok=%d\n",
                 session->session_id,
                 reason,
                 stream_id,
                 close_ok ? 1 : 0);
            kr_webrtc_session_stop_media(session, "open_record_setup_failed");
            return -EINVAL;
        }
        session->record_stream = stream_id;
    }

    session->audio_running = true;
    session->audio_first_rx_logged = false;
    session->audio_first_playback_logged = false;
    session->audio_rx_count = 0;
    session->audio_playback_count = 0;
    session->audio_high_watermark = 0;
    LOGI("audio open session=%s reason=%s playback=%d record=%d playback_dev=%s record_dev=%s\n",
            session->session_id,
            reason,
            session->playback_stream,
            session->record_stream,
            playback_device,
            record_device);
    return 0;
}

static void kr_webrtc_close_audio_for_session(kr_webrtc_session_t *session, const char *reason)
{
    if (session == NULL)
    {
        return;
    }

    LOGI("audio close session=%s reason=%s playback=%d record=%d rx=%u play_cb=%u high=%zu\n",
            session->session_id,
            reason,
            session->playback_stream,
            session->record_stream,
            session->audio_rx_count,
            session->audio_playback_count,
            session->audio_high_watermark);

    session->audio_running = false;
    if (session->record_stream != 0)
    {
        int stream_id = session->record_stream;
        bool stop_ok = Audio_Recorder_Stop(stream_id);
        bool close_ok = Audio_Recorder_Close(stream_id);
        LOGI("audio record closed session=%s reason=%s stream=%d stop_ok=%d close_ok=%d\n",
             session->session_id,
             reason,
             stream_id,
             stop_ok ? 1 : 0,
             close_ok ? 1 : 0);
        session->record_stream = 0;
    }
    if (session->playback_stream != 0)
    {
        int stream_id = session->playback_stream;
        bool stop_ok = Audio_Player_Stop(stream_id);
        bool close_ok = Audio_Player_Close(stream_id);
        LOGI("audio playback closed session=%s reason=%s stream=%d stop_ok=%d close_ok=%d\n",
             session->session_id,
             reason,
             stream_id,
             stop_ok ? 1 : 0,
             close_ok ? 1 : 0);
        session->playback_stream = 0;
    }

    pthread_mutex_lock(&session->audio_lock);
    circlebuf_free(&session->audio_packets);
    circlebuf_init(&session->audio_packets);
    circlebuf_reserve(&session->audio_packets, KR_WEBRTC_AUDIO_PLAYBACK_MAX_BYTES);
    pthread_mutex_unlock(&session->audio_lock);
}

static void kr_webrtc_session_reset(kr_webrtc_session_t *session)
{
    if (session == NULL)
    {
        return;
    }

    session->session_id[0] = '\0';
    session->enabled = false;
    session->is_callout = false;
    session->audio_running = false;
    session->audio_first_rx_logged = false;
    session->audio_first_playback_logged = false;
    session->playback_stream = 0;
    session->record_stream = 0;
    session->audio_rx_count = 0;
    session->audio_playback_count = 0;
    session->audio_high_watermark = 0;
}

static void kr_webrtc_session_stop_media(kr_webrtc_session_t *session, const char *reason)
{
    if (session == NULL)
    {
        return;
    }

    kr_webrtc_close_audio_for_session(session, reason);
    kr_webrtc_video_decoder_stop(session);
}

static void kr_webrtc_session_release(kr_webrtc_session_t *session, const char *reason)
{
    if (session == NULL)
    {
        return;
    }

    kr_webrtc_session_stop_media(session, reason);
    kr_webrtc_session_reset(session);
}

static void kr_webrtc_session_init(kr_webrtc_handle_t *handle, kr_webrtc_session_t *session)
{
    memset(session, 0, sizeof(*session));
    session->owner = handle;
    session->video.channel = -1;
    circlebuf_init(&session->audio_packets);
    circlebuf_reserve(&session->audio_packets, KR_WEBRTC_AUDIO_PLAYBACK_MAX_BYTES);
    (void)pthread_mutex_init(&session->audio_lock, NULL);
    (void)pthread_mutex_init(&session->video.lock, NULL);
    (void)pthread_cond_init(&session->video.cond, NULL);
}

static void kr_webrtc_session_destroy(kr_webrtc_session_t *session)
{
    if (session == NULL)
    {
        return;
    }

    kr_webrtc_session_release(session, "destroy");
    circlebuf_free(&session->audio_packets);
    pthread_cond_destroy(&session->video.cond);
    pthread_mutex_destroy(&session->video.lock);
    pthread_mutex_destroy(&session->audio_lock);
}

static kr_webrtc_session_t *kr_webrtc_find_session_locked(
    kr_webrtc_handle_t *handle,
    const char *session_id,
    size_t session_id_len
)
{
    int i;

    for (i = 0; i < KR_WEBRTC_STREAM_MAX; i++)
    {
        if (kr_webrtc_session_id_equals(&handle->sessions[i], session_id, session_id_len))
        {
            return &handle->sessions[i];
        }
    }
    return NULL;
}

static kr_webrtc_session_t *kr_webrtc_alloc_session_locked(
    kr_webrtc_handle_t *handle,
    const char *session_id,
    size_t session_id_len,
    bool is_callout
)
{
    int i;

    for (i = 0; i < KR_WEBRTC_STREAM_MAX; i++)
    {
        kr_webrtc_session_t *session = &handle->sessions[i];
        if (!session->enabled)
        {
            kr_webrtc_session_reset(session);
            kr_webrtc_copy_session_id(session->session_id, sizeof(session->session_id), session_id, session_id_len);
            session->enabled = true;
            session->is_callout = is_callout;
            return session;
        }
    }
    return NULL;
}

static kr_webrtc_session_t *kr_webrtc_get_or_create_session(
    kr_webrtc_handle_t *handle,
    const char *session_id,
    size_t session_id_len,
    bool is_callout
)
{
    kr_webrtc_session_t *session;

    if (handle == NULL || session_id == NULL || session_id_len == 0)
    {
        return NULL;
    }

    pthread_mutex_lock(&handle->lock);
    session = kr_webrtc_find_session_locked(handle, session_id, session_id_len);
    if (session == NULL)
    {
        session = kr_webrtc_alloc_session_locked(handle, session_id, session_id_len, is_callout);
    }
    pthread_mutex_unlock(&handle->lock);
    return session;
}

static kr_webrtc_session_t *kr_webrtc_find_session(
    kr_webrtc_handle_t *handle,
    const char *session_id,
    size_t session_id_len
)
{
    kr_webrtc_session_t *session;

    if (handle == NULL || session_id == NULL || session_id_len == 0)
    {
        return NULL;
    }

    pthread_mutex_lock(&handle->lock);
    session = kr_webrtc_find_session_locked(handle, session_id, session_id_len);
    pthread_mutex_unlock(&handle->lock);
    return session;
}

static void kr_webrtc_release_session_by_id(
    kr_webrtc_handle_t *handle,
    const char *session_id,
    size_t session_id_len,
    const char *reason
)
{
    kr_webrtc_session_t *session;

    session = kr_webrtc_find_session(handle, session_id, session_id_len);
    if (session == NULL)
    {
        return;
    }

    kr_webrtc_session_release(session, reason);
}

static void kr_webrtc_release_all_sessions(kr_webrtc_handle_t *handle, const char *reason)
{
    int i;

    if (handle == NULL)
    {
        return;
    }

    for (i = 0; i < KR_WEBRTC_STREAM_MAX; i++)
    {
        if (handle->sessions[i].enabled)
        {
            kr_webrtc_session_release(&handle->sessions[i], reason);
        }
    }
}

static void kr_webrtc_event_callback(webrtc_event_type_t event, void *user, int *result)
{
    kr_webrtc_handle_t *handle = (kr_webrtc_handle_t *)user;
    kr_webrtc_event_t queued_event;

    (void)result;

    if (!kr_webrtc_callback_enter(handle))
    {
        return;
    }

    memset(&queued_event, 0, sizeof(queued_event));

    switch (event)
    {
    case WEBRTC_EVENT_ONLINE:
        handle->logged_in = true;
        queued_event.kind = KR_WEBRTC_EVENT_ONLINE;
        LOGI("event online\n");
        kr_webrtc_test_h264_start(handle);
        break;
    case WEBRTC_EVENT_OFFLINE:
        handle->logged_in = false;
        queued_event.kind = KR_WEBRTC_EVENT_OFFLINE;
        LOGI("event offline\n");
        kr_webrtc_test_h264_stop(handle);
        break;
    default:
        kr_webrtc_callback_leave(handle);
        return;
    }

    (void)event_queue_push(&handle->events, &queued_event);
    kr_webrtc_callback_leave(handle);
}

static void kr_webrtc_call_income_callback(
    char *session_id,
    size_t session_id_len,
    char *mode,
    size_t mode_len,
    char *source,
    size_t source_len,
    void *user
)
{
    //return;
    kr_webrtc_handle_t *handle = (kr_webrtc_handle_t *)user;
    kr_webrtc_session_t *session;
    bool is_callout;
    bool call_busy;
    bool bridge_mode;
    int rc;

    if (!kr_webrtc_callback_enter(handle))
    {
        return;
    }
    if (session_id == NULL || session_id_len == 0)
    {
        kr_webrtc_callback_leave(handle);
        return;
    }

    is_callout = !(mode_len > 0 || source_len > 0);
    pthread_mutex_lock(&handle->lock);
    call_busy = handle->call_busy;
    bridge_mode = handle->media_bridge_mode;
    pthread_mutex_unlock(&handle->lock);
    if (call_busy && !is_callout)
    {
        LOGI("call_income busy close session=%.*s mode=%.*s source=%.*s\n",
                (int)session_id_len,
                session_id,
                (int)mode_len,
                SAFE_STRING(mode),
                (int)source_len,
                SAFE_STRING(source));
        (void)webrtc_streamer_close_session(session_id, session_id_len);
        kr_webrtc_callback_leave(handle);
        return;
    }

    session = kr_webrtc_get_or_create_session(handle, session_id, session_id_len, is_callout);
    LOGI("call_income session=%.*s mode=%.*s source=%.*s is_callout=%u session=%p\n",
            (int)session_id_len,
            session_id,
            (int)mode_len,
            SAFE_STRING(mode),
            (int)source_len,
            SAFE_STRING(source),
            is_callout ? 1U : 0U,
            (void *)session);

    if (session == NULL)
    {
        LOGW("call_income no free session, closing %.*s\n",
                (int)session_id_len, session_id);
        (void)webrtc_streamer_close_session(session_id, session_id_len);
        kr_webrtc_callback_leave(handle);
        return;
    }

    rc = kr_webrtc_open_audio_for_session(session, "call_income");
    LOGI("call_income audio_open rc=%d session=%s\n", rc, session->session_id);
    if (bridge_mode)
    {
        LOGI("call_income skip remote video decoder in bridge mode session=%s\n",
             session->session_id);
    }
    else
    {
        rc = kr_webrtc_video_decoder_start(session);
        LOGI("call_income video_start rc=%d session=%s\n", rc, session->session_id);
    }
    kr_webrtc_callback_leave(handle);
}

static void kr_webrtc_call_destory_callback(char *session_id, size_t session_id_len, void *user)
{
    kr_webrtc_handle_t *handle = (kr_webrtc_handle_t *)user;
    kr_webrtc_event_t event;

    if (!kr_webrtc_callback_enter(handle))
    {
        return;
    }
    if (session_id == NULL || session_id_len == 0)
    {
        kr_webrtc_callback_leave(handle);
        return;
    }

    LOGI("call_destory session=%.*s\n", (int)session_id_len, session_id);
    memset(&event, 0, sizeof(event));
    event.kind = KR_WEBRTC_EVENT_CALL_DESTROY;
    safe_strncpy(event.text1,
                 session_id_len > 0 ? session_id : NULL,
                 sizeof(event.text1));
    (void)event_queue_push(&handle->events, &event);
    kr_webrtc_release_session_by_id(handle, session_id, session_id_len, "call_destory");
    kr_webrtc_callback_leave(handle);
}

static void kr_webrtc_audio_callback(
    char *data,
    size_t len,
    char *session_id,
    size_t session_id_len,
    void *user
)
{
    //return;
    kr_webrtc_handle_t *handle = (kr_webrtc_handle_t *)user;
    kr_webrtc_session_t *session;
    kr_webrtc_remote_audio_sink_t remote_audio_sink = NULL;
    void *remote_audio_user = NULL;
    bool bridge_mode = false;
    size_t overflow;

    if (!kr_webrtc_callback_enter(handle))
    {
        return;
    }
    if (data == NULL || len == 0 || session_id == NULL || session_id_len == 0)
    {
        kr_webrtc_callback_leave(handle);
        return;
    }

    session = kr_webrtc_find_session(handle, session_id, session_id_len);
    if (session == NULL || !session->audio_running)
    {
        kr_webrtc_callback_leave(handle);
        return;
    }

    pthread_mutex_lock(&handle->lock);
    bridge_mode = handle->media_bridge_mode;
    remote_audio_sink = handle->remote_audio_sink;
    remote_audio_user = handle->remote_audio_user;
    pthread_mutex_unlock(&handle->lock);
    if (bridge_mode && remote_audio_sink != NULL)
    {
        remote_audio_sink((const unsigned char *)data, len, remote_audio_user);
        kr_webrtc_callback_leave(handle);
        return;
    }

    pthread_mutex_lock(&session->audio_lock);
    if (session->audio_packets.size + len > KR_WEBRTC_AUDIO_PLAYBACK_MAX_BYTES)
    {
        overflow = session->audio_packets.size + len - KR_WEBRTC_AUDIO_PLAYBACK_MAX_BYTES;
        if (overflow > session->audio_packets.size)
        {
            overflow = session->audio_packets.size;
        }
        if (overflow > 0)
        {
            circlebuf_pop_front(&session->audio_packets, NULL, overflow);
        }
    }
    circlebuf_push_back(&session->audio_packets, data, len);
    session->audio_rx_count++;
    if (session->audio_packets.size > session->audio_high_watermark)
    {
        session->audio_high_watermark = session->audio_packets.size;
    }
    if (!session->audio_first_rx_logged)
    {
        session->audio_first_rx_logged = true;
        LOGI("first audio rx session=%s len=%zu queued=%zu\n",
                session->session_id, len, session->audio_packets.size);
    }
    pthread_mutex_unlock(&session->audio_lock);
    kr_webrtc_callback_leave(handle);
}

static void kr_webrtc_video_callback(
    webrtc_stream_type_t stream_type,
    webrtc_video_code_type_t type,
    char *data,
    size_t len,
    char *session_id,
    size_t session_id_len,
    void *user
)
{
    kr_webrtc_handle_t *handle = (kr_webrtc_handle_t *)user;
    kr_webrtc_session_t *session;
    unsigned int frame_count;
    bool bridge_mode;
    bool log_bridge_drop;

    (void)stream_type;

    if (!kr_webrtc_callback_enter(handle))
    {
        return;
    }
    if (data == NULL || len == 0 || session_id == NULL || session_id_len == 0)
    {
        kr_webrtc_callback_leave(handle);
        return;
    }

    pthread_mutex_lock(&handle->lock);
    handle->video_callback_frames++;
    frame_count = handle->video_callback_frames;
    bridge_mode = handle->media_bridge_mode;
    log_bridge_drop = bridge_mode && !handle->media_bridge_video_drop_logged;
    if (log_bridge_drop)
    {
        handle->media_bridge_video_drop_logged = true;
    }
    pthread_mutex_unlock(&handle->lock);

    if (bridge_mode)
    {
        if (log_bridge_drop)
        {
            LOGI("media bridge drop remote webrtc video len=%zu frames=%u\n",
                 len,
                 frame_count);
        }
        kr_webrtc_callback_leave(handle);
        return;
    }

    if (type != WEBRTC_VIDEO_H264)
    {
        if (frame_count == 1 || (frame_count % 300U) == 0U)
        {
            LOGW("ignore non-h264 video callback codec=%d len=%zu frames=%u\n",
                    (int)type, len, frame_count);
        }
        kr_webrtc_callback_leave(handle);
        return;
    }

    session = kr_webrtc_find_session(handle, session_id, session_id_len);
    if (session == NULL)
    {
        kr_webrtc_callback_leave(handle);
        return;
    }

    if (frame_count == 1 || (frame_count % 300U) == 0U)
    {
        LOGI("video callback stream=%d codec=%d len=%zu session=%s frames=%u\n",
                (int)stream_type,
                (int)type,
                len,
                session->session_id,
                frame_count);
    }

    (void)kr_webrtc_video_decoder_input(session, (const uint8_t *)data, len, frame_count);
    kr_webrtc_callback_leave(handle);
}

static int kr_webrtc_authentication_callback(
    char *authdata,
    size_t authlen,
    char *password,
    size_t pwdlen
)
{
    (void)authdata;
    (void)authlen;
    (void)password;
    (void)pwdlen;
    return 1;
}

static void kr_webrtc_configuration_callback(char *data, size_t len, int reboot)
{
    (void)data;
    (void)len;
    (void)reboot;
}

static void kr_webrtc_message_callback(
    char *session_id,
    size_t session_id_len,
    char *req_msg,
    size_t req_msg_len,
    char *rsp_msg,
    size_t *rsp_msg_len,
    void *user
)
{
    (void)session_id;
    (void)session_id_len;
    (void)user;

    if (rsp_msg == NULL || rsp_msg_len == NULL)
    {
        return;
    }

    if (req_msg != NULL && req_msg_len > 0)
    {
        memcpy(rsp_msg, req_msg, req_msg_len);
        *rsp_msg_len = req_msg_len;
    }
    else
    {
        *rsp_msg_len = 0;
    }
}

static void kr_webrtc_call_failed_callback(
    char *session_id,
    size_t session_id_len,
    char *message,
    void *user
)
{
    kr_webrtc_handle_t *handle = (kr_webrtc_handle_t *)user;
    kr_webrtc_event_t event;

    if (!kr_webrtc_callback_enter(handle))
    {
        return;
    }

    LOGE("call_failed session=%.*s message=%s\n",
            (int)session_id_len,
            SAFE_STRING(session_id),
            SAFE_STRING(message));

    memset(&event, 0, sizeof(event));
    event.kind = KR_WEBRTC_EVENT_CALL_FAILED;
    safe_strncpy(event.text1,
                 session_id_len > 0 ? session_id : NULL,
                 sizeof(event.text1));
    safe_strncpy(event.text2, message, sizeof(event.text2));
    (void)event_queue_push(&handle->events, &event);
    kr_webrtc_callback_leave(handle);
}

static void kr_webrtc_call_disconnect_callback(
    char *session_id,
    size_t session_id_len,
    char *message,
    void *user
)
{
    kr_webrtc_handle_t *handle = (kr_webrtc_handle_t *)user;
    kr_webrtc_event_t event;

    if (!kr_webrtc_callback_enter(handle))
    {
        return;
    }

    LOGI("call_disconnect session=%.*s message=%s\n",
            (int)session_id_len,
            SAFE_STRING(session_id),
            SAFE_STRING(message));

    memset(&event, 0, sizeof(event));
    event.kind = KR_WEBRTC_EVENT_CALL_DISCONNECT;
    safe_strncpy(event.text1,
                 session_id_len > 0 ? session_id : NULL,
                 sizeof(event.text1));
    safe_strncpy(event.text2, message, sizeof(event.text2));
    (void)event_queue_push(&handle->events, &event);
    kr_webrtc_callback_leave(handle);
}

static void kr_webrtc_session_ask_iframe_callback(
    char *session_id,
    size_t session_id_len,
    void *user
)
{
    kr_webrtc_handle_t *handle = (kr_webrtc_handle_t *)user;
    kr_webrtc_event_t event;

    if (!kr_webrtc_callback_enter(handle))
    {
        return;
    }

    pthread_mutex_lock(&handle->lock);
    if (handle->media_bridge_mode)
    {
        handle->media_bridge_video_ready = true;
        handle->media_bridge_video_wait_logged = false;
    }
    pthread_mutex_unlock(&handle->lock);

    memset(&event, 0, sizeof(event));
    event.kind = KR_WEBRTC_EVENT_ASK_IFRAME;
    safe_strncpy(event.text1,
                 session_id_len > 0 ? session_id : NULL,
                 sizeof(event.text1));
    (void)event_queue_push(&handle->events, &event);
    kr_webrtc_callback_leave(handle);
}

static void kr_webrtc_send_queue_full_callback(
    char *session_id,
    size_t session_id_len,
    int queue,
    int bufsize,
    void *user
)
{
    kr_webrtc_handle_t *handle = (kr_webrtc_handle_t *)user;
    unsigned int count;

    if (!kr_webrtc_callback_enter(handle))
    {
        return;
    }

    pthread_mutex_lock(&handle->lock);
    handle->send_queue_full_count++;
    handle->send_queue_last_queue = queue;
    handle->send_queue_last_bufsize = bufsize;
    count = handle->send_queue_full_count;
    pthread_mutex_unlock(&handle->lock);

    LOGW("send queue full count=%u session=%.*s queue=%d bufsize=%d\n",
         count,
         (int)session_id_len,
         session_id ? session_id : "",
         queue,
         bufsize);
    kr_webrtc_callback_leave(handle);
}

int kr_webrtc_create(kr_webrtc_handle_t **out_handle, const kr_webrtc_config_t *cfg)
{
    kr_webrtc_handle_t *handle;
    int i;
    int rc;

    if (out_handle == NULL || cfg == NULL)
    {
        return -EINVAL;
    }

    handle = (kr_webrtc_handle_t *)bzalloc(sizeof(*handle));
    if (handle == NULL)
    {
        return -ENOMEM;
    }

    rc = pthread_mutex_init(&handle->lock, NULL);
    if (rc != 0)
    {
        bfree(handle);
        return -rc;
    }
    rc = pthread_cond_init(&handle->cond, NULL);
    if (rc != 0)
    {
        pthread_mutex_destroy(&handle->lock);
        bfree(handle);
        return -rc;
    }

    handle->lock_ready = true;
    handle->preview_brightness = 50;
    handle->playback_volume = KR_WEBRTC_AUDIO_VOLUME;
    rc = event_queue_init(&handle->events, sizeof(kr_webrtc_event_t));
    if (rc != 0)
    {
        pthread_cond_destroy(&handle->cond);
        pthread_mutex_destroy(&handle->lock);
        bfree(handle);
        return rc;
    }
    handle->cfg = *cfg;
    handle->cfg.initstring = bstrdup(cfg->initstring);
    handle->cfg.serno = bstrdup(cfg->serno);
    handle->cfg.server_addr = bstrdup(cfg->server_addr);
    handle->cfg.customer_serno = bstrdup(cfg->customer_serno);
    handle->cfg.playback_device = bstrdup(cfg->playback_device);
    handle->cfg.record_device = bstrdup(cfg->record_device);

    if ((cfg->initstring != NULL && handle->cfg.initstring == NULL) ||
        (cfg->serno != NULL && handle->cfg.serno == NULL) ||
        (cfg->server_addr != NULL && handle->cfg.server_addr == NULL) ||
        (cfg->customer_serno != NULL && handle->cfg.customer_serno == NULL) ||
        (cfg->playback_device != NULL && handle->cfg.playback_device == NULL) ||
        (cfg->record_device != NULL && handle->cfg.record_device == NULL))
    {
        (void)kr_webrtc_destroy(handle);
        return -ENOMEM;
    }

    for (i = 0; i < KR_WEBRTC_STREAM_MAX; i++)
    {
        kr_webrtc_session_init(handle, &handle->sessions[i]);
    }

    LOGI(
        "create init_len=%zu serno_len=%zu server_len=%zu customer_len=%zu playback=%s record=%s\n",
        handle->cfg.initstring != NULL ? strlen(handle->cfg.initstring) : 0,
        handle->cfg.serno != NULL ? strlen(handle->cfg.serno) : 0,
        handle->cfg.server_addr != NULL ? strlen(handle->cfg.server_addr) : 0,
        handle->cfg.customer_serno != NULL ? strlen(handle->cfg.customer_serno) : 0,
        handle->cfg.playback_device != NULL ? handle->cfg.playback_device : "(null)",
        handle->cfg.record_device != NULL ? handle->cfg.record_device : "(null)"
    );

    *out_handle = handle;
    return 0;
}

int kr_webrtc_start(kr_webrtc_handle_t *handle)
{
    int rc;

    if (handle == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    if (handle->running)
    {
        pthread_mutex_unlock(&handle->lock);
        return 0;
    }
    event_queue_reset_cancel(&handle->events);
    handle->callbacks_enabled = true;
    pthread_mutex_unlock(&handle->lock);

    LOGI(
        "start begin init_len=%zu serno=%s server=%s customer_len=%zu\n",
        handle->cfg.initstring != NULL ? strlen(handle->cfg.initstring) : 0,
        SAFE_STRING(handle->cfg.serno),
        SAFE_STRING(handle->cfg.server_addr),
        handle->cfg.customer_serno != NULL ? strlen(handle->cfg.customer_serno) : 0
    );

    rc = webrtc_streamer_init(
        (char *)handle->cfg.initstring,
        NULL,
        (char *)handle->cfg.serno,
        (char *)handle->cfg.server_addr,
        (char *)(SAFE_STRING(handle->cfg.customer_serno))
    );
    LOGI("webrtc_streamer_init rc=%d\n", rc);
    if (rc < 0)
    {
        pthread_mutex_lock(&handle->lock);
        handle->callbacks_enabled = false;
        pthread_cond_broadcast(&handle->cond);
        pthread_mutex_unlock(&handle->lock);
        kr_webrtc_wait_callbacks_idle(handle);
        return rc;
    }

    (void)webrtc_streamer_set_signal_reconnect_timeout(
        handle->cfg.signal_reconnect_timeout_ms > 0 ? handle->cfg.signal_reconnect_timeout_ms : 500
    );
    (void)webrtc_streamer_set_log_level_mask(
        WEBRTC_STREAM_WARNING | WEBRTC_STREAM_ERROR | WEBRTC_STREAM_FATAL
    );
    (void)webrtc_streamer_set_mem_info(
        handle->cfg.max_sdk_use_mem > 0 ? handle->cfg.max_sdk_use_mem : 5 * 1024 * 1024,
        handle->cfg.max_session_use_mem > 0 ? handle->cfg.max_session_use_mem : 1 * 1024 * 1024,
        handle->cfg.max_session_buffer_size > 0 ? handle->cfg.max_session_buffer_size : 128
    );

    printf("webrtc_streamer_register_callback_fun========= %d   %d   %d   \n", handle->cfg.max_sdk_use_mem, handle->cfg.max_session_use_mem, handle->cfg.max_session_buffer_size);
    (void)webrtc_streamer_register_event_callback_fun(kr_webrtc_event_callback, handle);
    (void)webrtc_streamer_register_call_income_callback_fun(kr_webrtc_call_income_callback, handle);
    (void)webrtc_streamer_register_call_destory_callback_fun(kr_webrtc_call_destory_callback, handle);
    (void)webrtc_streamer_register_audio_callback_fun(kr_webrtc_audio_callback, handle);
    (void)webrtc_streamer_register_video_callback_fun(kr_webrtc_video_callback, handle);
    (void)webrtc_streamer_register_authentication_callback_fun(
        kr_webrtc_authentication_callback,
        handle
    );
    (void)webrtc_streamer_register_configuration_callback_fun(
        kr_webrtc_configuration_callback,
        handle
    );
    (void)webrtc_streamer_register_message_callback_fun(kr_webrtc_message_callback, 4 * 1024, handle);
    (void)webrtc_streamer_register_call_failed_callback_fun(kr_webrtc_call_failed_callback, handle);
    (void)webrtc_streamer_register_call_disconnect_callback_fun(
        kr_webrtc_call_disconnect_callback,
        handle
    );
    (void)webrtc_streamer_register_session_ask_iframe_callback_fun(
        kr_webrtc_session_ask_iframe_callback,
        handle
    );
    (void)webrtc_streamer_register_send_queue_full_callback_fun(
        kr_webrtc_send_queue_full_callback,
        handle
    );
    LOGI("callbacks registered\n");
    LOGI("audio/video open deferred until call_income\n");

    pthread_mutex_lock(&handle->lock);
    handle->running = true;
    pthread_mutex_unlock(&handle->lock);
    LOGI("start done running=1\n");
    return 0;
}

int kr_webrtc_stop(kr_webrtc_handle_t *handle)
{
    if (handle == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    handle->running = false;
    handle->logged_in = false;
    handle->callbacks_enabled = false;
    kr_webrtc_bridge_h264_dump_close_locked(handle);
    pthread_cond_broadcast(&handle->cond);
    pthread_mutex_unlock(&handle->lock);
    kr_webrtc_test_h264_stop(handle);
    event_queue_cancel(&handle->events);

    (void)webrtc_streamer_uninit();
    kr_webrtc_wait_callbacks_idle(handle);
    kr_webrtc_release_all_sessions(handle, "stop");
    return 0;
}

int kr_webrtc_destroy(kr_webrtc_handle_t *handle)
{
    int i;

    if (handle == NULL)
    {
        return -EINVAL;
    }

    (void)kr_webrtc_stop(handle);
    event_queue_destroy(&handle->events);

    for (i = 0; i < KR_WEBRTC_STREAM_MAX; i++)
    {
        kr_webrtc_session_destroy(&handle->sessions[i]);
    }

    if (handle->lock_ready)
    {
        pthread_cond_destroy(&handle->cond);
        pthread_mutex_destroy(&handle->lock);
    }

    bfree((void *)handle->cfg.initstring);
    bfree((void *)handle->cfg.serno);
    bfree((void *)handle->cfg.server_addr);
    bfree((void *)handle->cfg.customer_serno);
    bfree((void *)handle->cfg.playback_device);
    bfree((void *)handle->cfg.record_device);
    bfree(handle);
    return 0;
}

int kr_webrtc_wait_event(kr_webrtc_handle_t *handle, kr_webrtc_event_t *out_event, int timeout_ms)
{
    if (handle == NULL || out_event == NULL)
    {
        return -EINVAL;
    }

    return event_queue_wait(&handle->events, out_event, timeout_ms);
}

int kr_webrtc_is_logged_in(kr_webrtc_handle_t *handle)
{
    if (handle == NULL)
    {
        return 0;
    }

    return handle->logged_in ? 1 : 0;
}

int kr_webrtc_send_video_frame(
    kr_webrtc_handle_t *handle,
    int stream_type,
    int video_codec,
    const unsigned char *data,
    size_t len
)
{
    bool bridge_mode;
    uint64_t now_ms;
    int ret;

    if (handle == NULL || !handle->running || data == NULL || len == 0)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    bridge_mode = handle->media_bridge_mode;
    pthread_mutex_unlock(&handle->lock);

    if(!handle->test_h264_thread_running)
    {
        ret = webrtc_streamer_input_video_data(
            (webrtc_stream_type_t)stream_type,
            (webrtc_video_code_type_t)video_codec,
            (unsigned char *)data,
            len
        );
    }
    else
    {
        ret = 0;
    }
    if (bridge_mode && ret >= 0)
    {
        kr_webrtc_bridge_h264_dump_write(handle, data, len);
    }

    if (bridge_mode)
    {
        now_ms = kr_webrtc_monotonic_ms();
        pthread_mutex_lock(&handle->lock);
        if (handle->bridge_video_stats_start_ms == 0)
        {
            handle->bridge_video_stats_start_ms = now_ms;
            handle->bridge_video_stats_last_log_ms = now_ms;
        }
        handle->bridge_video_input_count++;
        handle->bridge_video_bytes += len;
        if (len > handle->bridge_video_max_len)
        {
            handle->bridge_video_max_len = len;
        }
        if (ret < 0)
        {
            handle->bridge_video_fail_count++;
        }
        else
        {
            handle->bridge_video_ok_count++;
        }

        if (now_ms != 0 &&
            handle->bridge_video_stats_last_log_ms != 0 &&
            now_ms - handle->bridge_video_stats_last_log_ms >= KR_WEBRTC_BRIDGE_STATS_INTERVAL_MS)
        {
            handle->bridge_video_stats_last_log_ms = now_ms;
        }
        pthread_mutex_unlock(&handle->lock);
    }

    return ret;
}

int kr_webrtc_send_audio_frame(
    kr_webrtc_handle_t *handle,
    const unsigned char *data,
    size_t len
)
{
    if (handle == NULL || !handle->running || data == NULL || len == 0)
    {
        return -EINVAL;
    }

    return webrtc_streamer_input_audio_data((unsigned char *)data, len);
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
    kr_webrtc_session_t *session;
    int rc;

    if (handle == NULL || !handle->running || session_id == NULL || to == NULL)
    {
        return -EINVAL;
    }

    session = kr_webrtc_get_or_create_session(handle, session_id, strlen(session_id), true);
    if (session == NULL)
    {
        return -EBUSY;
    }

    rc = webrtc_streamer_call(
        WEBRTC_STREAM_MAIN,
        (char *)session_id,
        strlen(session_id),
        (char *)to,
        strlen(to),
        (char *)(audio != NULL ? audio : "video"),
        (char *)(video != NULL ? video : "video"),
        datachannel,
        NULL,
        NULL
    );
    if (rc < 0)
    {
        kr_webrtc_session_release(session, "outgoing_call_failed");
    }
    return rc;
}

int kr_webrtc_close_session(kr_webrtc_handle_t *handle, const char *session_id)
{
    if (handle == NULL || !handle->running || session_id == NULL)
    {
        return -EINVAL;
    }

    return webrtc_streamer_close_session((char *)session_id, strlen(session_id));
}

int kr_webrtc_set_call_busy(kr_webrtc_handle_t *handle, bool busy)
{
    if (handle == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    handle->call_busy = busy;
    pthread_mutex_unlock(&handle->lock);
    LOGI("call_busy set busy=%d\n", busy ? 1 : 0);
    return 0;
}

int kr_webrtc_set_preview_brightness(kr_webrtc_handle_t *handle, int level)
{
    int brightness;
    bool has_layer = false;
    int i;

    if (handle == NULL)
    {
        return -EINVAL;
    }

    brightness = kr_webrtc_clamp_level(level);
    pthread_mutex_lock(&handle->lock);
    handle->preview_brightness = brightness;
    for (i = 0; i < KR_WEBRTC_STREAM_MAX; i++)
    {
        if (handle->sessions[i].video.layer_enabled)
        {
            has_layer = true;
            break;
        }
    }
    pthread_mutex_unlock(&handle->lock);

    if (has_layer)
    {
        kr_webrtc_apply_preview_cm(brightness);
    }
    LOGI("preview brightness set level=%d applied=%d\n", brightness, has_layer ? 1 : 0);
    return 0;
}

int kr_webrtc_set_playback_volume(kr_webrtc_handle_t *handle, int volume_percent)
{
    int volume;
    int ret = 0;
    int i;

    if (handle == NULL)
    {
        return -EINVAL;
    }

    volume = kr_webrtc_clamp_volume(volume_percent);
    pthread_mutex_lock(&handle->lock);
    handle->playback_volume = volume;
    for (i = 0; i < KR_WEBRTC_STREAM_MAX; i++)
    {
        if (handle->sessions[i].playback_stream != 0 &&
            !Audio_Player_SetVolume(handle->sessions[i].playback_stream, volume))
        {
            ret = -EIO;
        }
    }
    pthread_mutex_unlock(&handle->lock);

    LOGI("playback volume set volume=%d ret=%d\n", volume, ret);
    return ret;
}

int kr_webrtc_set_remote_audio_sink(
    kr_webrtc_handle_t *handle,
    kr_webrtc_remote_audio_sink_t sink,
    void *user
)
{
    if (handle == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    handle->remote_audio_sink = sink;
    handle->remote_audio_user = user;
    pthread_mutex_unlock(&handle->lock);
    LOGI("remote audio sink set enabled=%d\n", sink != NULL ? 1 : 0);
    return 0;
}

int kr_webrtc_set_bridge_h264_dump_tag(kr_webrtc_handle_t *handle, const char *tag)
{
    if (handle == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    snprintf(handle->bridge_h264_dump_tag,
             sizeof(handle->bridge_h264_dump_tag),
             "%s",
             tag != NULL ? tag : "");
    pthread_mutex_unlock(&handle->lock);
    LOGI("bridge h264 dump tag set tag=%s\n",
         handle->bridge_h264_dump_tag[0] != '\0' ? handle->bridge_h264_dump_tag : "(empty)");
    return 0;
}

int kr_webrtc_set_media_bridge_mode(kr_webrtc_handle_t *handle, bool enabled)
{
    int i;

    if (handle == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    handle->media_bridge_mode = enabled;
    handle->bridge_video_stats_start_ms = 0;
    handle->bridge_video_stats_last_log_ms = 0;
    handle->bridge_video_input_count = 0;
    handle->bridge_video_ok_count = 0;
    handle->bridge_video_fail_count = 0;
    handle->bridge_video_bytes = 0;
    handle->bridge_video_max_len = 0;
    handle->send_queue_full_count = 0;
    handle->send_queue_last_queue = 0;
    handle->send_queue_last_bufsize = 0;
    kr_webrtc_bridge_h264_dump_close_locked(handle);
    if (enabled)
    {
        handle->media_bridge_video_ready = false;
        handle->media_bridge_video_drop_logged = false;
        handle->media_bridge_video_wait_logged = false;
        kr_webrtc_bridge_h264_dump_open_locked(handle);
    }
    else
    {
        handle->media_bridge_video_ready = false;
        handle->media_bridge_video_wait_logged = false;
    }
    pthread_mutex_unlock(&handle->lock);

    if (enabled)
    {
        for (i = 0; i < KR_WEBRTC_STREAM_MAX; ++i)
        {
            kr_webrtc_close_audio_for_session(&handle->sessions[i], "media_bridge_mode");
            kr_webrtc_video_decoder_stop(&handle->sessions[i]);
        }
    }
    LOGI("media bridge mode set enabled=%d\n", enabled ? 1 : 0);
    return 0;
}
