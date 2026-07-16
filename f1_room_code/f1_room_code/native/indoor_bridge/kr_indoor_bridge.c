#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <net/if.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <goblin_transmission.h>

#include "kr_indoor_bridge.h"
#include "kr_webrtc_bridge.h"

#include <utils/circlebuf.h>
#include <utils/resize_buf.h>
#include <utils/bmem.h>

#include <codec/opus/opus_codec.h>
#include <codec/f1_video_codec.h>

#include <AudioCore.h>

#include "call_utils.h"
#include "mtrans.h"
#include "simple_bmem_backend.h"
#include "simple_call_app.h"
#include "simple_control_route_provider.h"
#include "simple_d2d_bus_debug.h"
#include "simple_d2d_business_runtime.h"
#include "simple_d2d_villa_heartbeat.h"
#include "simple_endpoint_provider.h"
#include "simple_ipv4_utils.h"
#include "simple_log.h"
#include "simple_media.h"
#include "simple_media_runtime.h"
#include "simple_mem.h"
#include "simple_plc_addr.h"
#include "simple_topology_audio.h"
#include "simple_utils_log_backend.h"
#include "transport.h"

#define SIMPLE_INDOOR_DEVICE_ID "1:10.2.17.1"
#define SIMPLE_DEFAULT_DOOR_DEVICE_ID "2:10.2.0.1"
#define SIMPLE_DEFAULT_MANAGE_DEVICE_ID "6:10.2.128.1"
#define SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID "3:10.2.17.1"
#define SIMPLE_DEFAULT_SECONDARY_CONFIRM_IP "192.168.8.111"
#define SIMPLE_DEFAULT_CONVERTER_DEVICE_ID "500:10.2.0.1:aa-bb-cc-dd-ee-ff"
#define SIMPLE_INDOOR_VIDEO_W  (1024)
#define SIMPLE_INDOOR_VIDEO_H  (600)
#define SIMPLE_AUDIO_SAMPLE_RATE (16000)
#define SIMPLE_AUDIO_SAMPLE_BITS (16)
#define SIMPLE_AUDIO_CHANNELS    (1)
#define SIMPLE_AUDIO_BLOCK_TIME  (20)
#define SIMPLE_AUDIO_BLOCK_COUNT (4)
#define SIMPLE_AUDIO_BLOCK_SIZE  (SIMPLE_AUDIO_SAMPLE_RATE * SIMPLE_AUDIO_BLOCK_TIME * \
                                  SIMPLE_AUDIO_CHANNELS * (SIMPLE_AUDIO_SAMPLE_BITS / 8) / 1000)
#define SIMPLE_AUDIO_VOLUME      (100)
#define SIMPLE_PLAYBACK_VOLUME   (100)
#define SIMPLE_WEBRTC_BRIDGE_SAMPLE_RATE (8000)
#define KR_INDOOR_QUEUE_CAPACITY (64U)
#define H264_NAL_TYPE(header) ((header) & 0x1F)
#define SIMPLE_MEDIA_BRIDGE_VIDEO_TARGET_FPS 15U
#define SIMPLE_MEDIA_BRIDGE_VIDEO_MIN_INTERVAL_MS (1000U / SIMPLE_MEDIA_BRIDGE_VIDEO_TARGET_FPS)
#define SIMPLE_MEDIA_BRIDGE_VIDEO_STATS_INTERVAL_MS 2000ULL
#define SIMPLE_MEDIA_BRIDGE_H264_DUMP_ENV "KR_ROOM_DUMP_BRIDGE_H264"
#define SIMPLE_MEDIA_BRIDGE_H264_DUMP_DIR_ENV "KR_ROOM_DUMP_BRIDGE_H264_DIR"
#define SIMPLE_MEDIA_BRIDGE_H264_DUMP_DEFAULT_DIR "/tmp/kr_room_bridge_dump"
#define SIMPLE_MEDIA_BRIDGE_H264_DUMP_TAG_MAX 96
#define SIMPLE_MEDIA_BRIDGE_H264_DUMP_PATH_MAX 256

enum
{
    H264_NAL_SLICE = 1,
    H264_NAL_IDR_SLICE = 5,
    H264_NAL_SPS = 7,
    H264_NAL_PPS = 8,
    H264_NAL_FU_A = 28,
};

extern const transport_ops_t g_plc_transport_ops;
extern const transport_ops_t g_udp_transport_ops;
extern const transport_ops_t g_tcp_transport_ops;

typedef struct
{
    resize_buf_t sps;
    resize_buf_t pps;
    bool unlocked;
    bool wait_idr_logged;
    bool missing_param_logged;
    bool corrupt_logged;
    bool pli_requested;
} simple_indoor_h264_gate_t;

typedef struct
{
    f1_video_codec_data_t codec_data;
    resize_buf_t rb;
    simple_indoor_h264_gate_t h264_gate;
    bool running;
    bool first_packet_logged;
    bool first_frame_logged;
} simple_indoor_video_decoder_t;

typedef enum
{
    SIMPLE_INDOOR_PRIMARY_VIA_CONVERTER = 0,
    SIMPLE_INDOOR_PRIMARY_VIA_UNIT_DOOR_PLC,
    SIMPLE_INDOOR_PRIMARY_NONE
} simple_indoor_primary_path_t;

typedef enum
{
    SIMPLE_INDOOR_SECONDARY_OFF = 0,
    SIMPLE_INDOOR_SECONDARY_OPTIONAL,
    SIMPLE_INDOOR_SECONDARY_REQUIRED
} simple_indoor_secondary_policy_t;

typedef struct
{
    simple_indoor_primary_path_t primary_path;
    simple_indoor_secondary_policy_t secondary_policy;
    bool secondary_active;
} simple_indoor_link_plan_t;

typedef struct
{
    simple_indoor_video_decoder_t video;
    simple_media_runtime_t media_runtime;
    volatile int video_desired_active;
    volatile int audio_desired_active;
    int video_active;
    int audio_active;
    int pb_stream;
    int ca_stream;
    pthread_mutex_t pb_mutex;
    pthread_mutex_t dec_mutex;
    struct circlebuf pb_packet;
    struct circlebuf capture_buf;
    void *opus_encoder;
    void *opus_decoder;
    uint32_t audio_next_rtp_ts;
    bool audio_first_tx_logged;
    bool aec_enabled;
    simple_topology_audio_t *topology_audio;
    simple_endpoint_provider_t endpoint_provider;
    simple_indoor_primary_path_t primary_path;
    bool secondary_active;
    simple_call_app_t *call_app;
    char fallback_peer_device_id[TOPOLOGY_DEVICE_ID_MAX];
    char secondary_proxy_sn[TOPOLOGY_MAC_STR_MAX];
    uint32_t fallback_peer_plc_addr;
    uint32_t fallback_peer_udp_addr;
    uint32_t local_udp_addr;
    int preview_brightness;
    simple_d2d_business_runtime_t *d2d_business;
    kr_webrtc_handle_t *webrtc_bridge;
    volatile int media_bridge_enabled;
    bool media_bridge_first_video_logged;
    bool media_bridge_first_audio_to_cloud_logged;
    bool media_bridge_first_audio_to_d2d_logged;
    bool media_bridge_local_playback_muted_logged;
    bool media_bridge_video_last_ts_valid;
    bool media_bridge_video_current_ts_send;
    uint32_t media_bridge_video_last_ts;
    uint64_t media_bridge_video_last_send_ms;
    uint64_t media_bridge_video_stats_start_ms;
    uint64_t media_bridge_video_stats_last_log_ms;
    uint64_t media_bridge_video_last_idr_ms;
    unsigned int media_bridge_video_input_count;
    unsigned int media_bridge_video_sent_count;
    unsigned int media_bridge_video_throttled_count;
    unsigned int media_bridge_video_failed_count;
    unsigned int media_bridge_video_idr_count;
    unsigned int media_bridge_video_sps_count;
    unsigned int media_bridge_video_pps_count;
    size_t media_bridge_video_bytes;
    size_t media_bridge_video_sent_bytes;
    size_t media_bridge_video_max_len;
    resize_buf_t media_bridge_h264_sps;
    resize_buf_t media_bridge_h264_pps;
    resize_buf_t media_bridge_h264_send_buf;
    char media_bridge_h264_dump_tag[SIMPLE_MEDIA_BRIDGE_H264_DUMP_TAG_MAX];
    FILE *media_bridge_h264_input_file;
    char media_bridge_h264_input_path[SIMPLE_MEDIA_BRIDGE_H264_DUMP_PATH_MAX];
    unsigned int media_bridge_h264_input_frames;
    size_t media_bridge_h264_input_bytes;
    unsigned int media_bridge_h264_input_idr_count;
    unsigned int media_bridge_h264_input_sps_count;
    unsigned int media_bridge_h264_input_pps_count;
    bool media_bridge_h264_dump_failed;
    void *event_ctx;
} simple_indoor_codec_ctx_t;

typedef struct
{
    const char *target_device_id;
    const char *d2d_room_id;
    const char *d2d_peer_id;
    const char *registry_peer_device_id;
    const char *spe_if;
    const char *spe_peer_ip_arg;
    const char *spe_local_ip_arg;
    bool target_device_id_set;
    uint32_t indoor_plc_addr;
    uint32_t converter_plc_addr;
    uint32_t d2d_peer_plc_addr;
    uint32_t direct_plc_cco_addr;
    uint32_t spe_peer_ip;
    uint32_t spe_local_ip;
    simple_indoor_link_plan_t link_plan;
} simple_indoor_start_config_t;

typedef struct
{
    simple_d2d_business_runtime_t d2d_business;
    simple_d2d_villa_heartbeat_t villa_heartbeat;
    simple_topology_audio_t topology_audio;
    simple_media_ops_t media_ops;
    simple_codec_ops_t codec_ops;
    simple_indoor_codec_ctx_t codec_ctx;
    simple_call_app_t *app;
    simple_control_link_t links[2];
    size_t link_count;
    size_t route_count;
    bool plc_inited;
    bool engine_inited;
    bool plc_registered;
    bool udp_registered;
    bool tcp_registered;
    bool topology_audio_inited;
    bool media_open;
    bool call_app_started;
    bool villa_heartbeat_started;
    bool d2d_business_started;
    bool codec_ctx_inited;
} simple_indoor_runtime_ctx_t;

struct kr_indoor_handle
{
    kr_indoor_config_t cfg;
    char interface_buf[64];
    char target_device_id_buf[64];
    char room_id_buf[32];
    char unlock_peer_device_id_buf[64];
    char registry_peer_device_id_buf[64];
    simple_indoor_start_config_t start_cfg;
    simple_indoor_runtime_ctx_t runtime;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    kr_indoor_event_t queue[KR_INDOOR_QUEUE_CAPACITY];
    size_t queue_head;
    size_t queue_len;
    pthread_t media_thread;
    bool locks_ready;
    bool running;
    bool started;
    bool media_thread_started;
    bool enabled;
    bool external_call_busy;
    int playback_volume;
    int preview_brightness;
};

static void kr_indoor_inbound_observer(void *user_ctx,
                                       const call_flow_runtime_inbound_event_t *event);
static bool kr_indoor_external_call_busy(void *user_ctx);

static void simple_indoor_request_video_recovery(simple_indoor_codec_ctx_t *indoor,
                                                 bool use_fir,
                                                 const char *reason);
static void simple_indoor_request_monitor_local_media_stop(simple_indoor_codec_ctx_t *indoor);

static bool simple_parse_u32_auto(const char *s, uint32_t *out)
{
    unsigned long value;
    char *end = NULL;

    if (!s || !out)
    {
        return false;
    }
    errno = 0;
    value = strtoul(s, &end, 0);
    if (errno != 0 || end == s || (end && *end != '\0') || value > 0xffffffffUL)
    {
        return false;
    }
    *out = (uint32_t)value;
    return true;
}

static bool simple_device_id_equal(const char *a, const char *b)
{
    return a && b && a[0] != '\0' && b[0] != '\0' && strcmp(a, b) == 0;
}

static void *kr_indoor_plc_bmalloc(size_t size)
{
    return bmalloc(size);
}

static void *kr_indoor_plc_brealloc(void *ptr, size_t size)
{
    return brealloc(ptr, size);
}

static void kr_indoor_plc_bfree(void *ptr)
{
    bfree(ptr);
}

static const char *kr_indoor_nonempty(const char *value, const char *fallback)
{
    return value && value[0] != '\0' ? value : fallback;
}

static void kr_indoor_push_event(kr_indoor_handle_t *handle, const kr_indoor_event_t *event)
{
    size_t index;

    if (!handle || !event || !handle->locks_ready)
    {
        return;
    }

    pthread_mutex_lock(&handle->queue_mutex);
    if (handle->queue_len == KR_INDOOR_QUEUE_CAPACITY)
    {
        handle->queue_head = (handle->queue_head + 1U) % KR_INDOOR_QUEUE_CAPACITY;
        handle->queue_len--;
    }
    index = (handle->queue_head + handle->queue_len) % KR_INDOOR_QUEUE_CAPACITY;
    handle->queue[index] = *event;
    handle->queue_len++;
    pthread_cond_signal(&handle->queue_cond);
    pthread_mutex_unlock(&handle->queue_mutex);
}

static void kr_indoor_push_simple_event(kr_indoor_handle_t *handle,
                                        kr_indoor_event_kind_t kind,
                                        int state,
                                        int result,
                                        bool video,
                                        bool audio,
                                        const char *text)
{
    kr_indoor_event_t event;

    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.session_state = state;
    event.result_code = result;
    event.video_active = video;
    event.audio_active = audio;
    if (text)
    {
        snprintf(event.text, sizeof(event.text), "%s", text);
    }
    kr_indoor_push_event(handle, &event);
}

static void kr_indoor_push_secondary_confirm_event(kr_indoor_handle_t *handle,
                                                   kr_indoor_event_kind_t kind,
                                                   uint32_t peer_id,
                                                   int result,
                                                   const char *device_id,
                                                   const char *dev_model)
{
    kr_indoor_event_t event;

    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.peer_id = peer_id;
    event.result_code = result;
    if (device_id)
    {
        snprintf(event.text, sizeof(event.text), "%s", device_id);
    }
    if (dev_model)
    {
        snprintf(event.text2, sizeof(event.text2), "%s", dev_model);
    }
    kr_indoor_push_event(handle, &event);
}

static int kr_indoor_normalize_mac(const char *raw, char *out, size_t out_len)
{
    unsigned int bytes[6];
    char sep1;
    char sep2;
    char sep3;
    char sep4;
    char sep5;
    int consumed = 0;
    int matched;

    if (!raw || !out || out_len < TOPOLOGY_MAC_STR_MAX)
    {
        return -EINVAL;
    }

    matched = sscanf(raw,
                     " %2x%c%2x%c%2x%c%2x%c%2x%c%2x %n",
                     &bytes[0],
                     &sep1,
                     &bytes[1],
                     &sep2,
                     &bytes[2],
                     &sep3,
                     &bytes[3],
                     &sep4,
                     &bytes[4],
                     &sep5,
                     &bytes[5],
                     &consumed);
    if (matched != 11 ||
        (sep1 != ':' && sep1 != '-') ||
        sep2 != sep1 ||
        sep3 != sep1 ||
        sep4 != sep1 ||
        sep5 != sep1 ||
        raw[consumed] != '\0')
    {
        return -EINVAL;
    }

    for (size_t i = 0; i < sizeof(bytes) / sizeof(bytes[0]); ++i)
    {
        if (bytes[i] > 0xffU)
        {
            return -EINVAL;
        }
    }

    snprintf(out,
             out_len,
             "%02X-%02X-%02X-%02X-%02X-%02X",
             bytes[0],
             bytes[1],
             bytes[2],
             bytes[3],
             bytes[4],
             bytes[5]);
    if (strcmp(out, "00-00-00-00-00-00") == 0)
    {
        return -EINVAL;
    }
    return 0;
}

static int kr_indoor_lookup_arp_mac(uint32_t addr, char *out, size_t out_len)
{
    FILE *fp;
    struct in_addr in_addr_value;
    char target_ip[INET_ADDRSTRLEN];
    char line[256];

    if (addr == 0 || !out || out_len < TOPOLOGY_MAC_STR_MAX)
    {
        return -EINVAL;
    }

    in_addr_value.s_addr = htonl(addr);
    if (inet_ntop(AF_INET, &in_addr_value, target_ip, sizeof(target_ip)) == NULL)
    {
        return -errno;
    }

    fp = fopen("/proc/net/arp", "r");
    if (!fp)
    {
        return -errno;
    }

    while (fgets(line, sizeof(line), fp))
    {
        char ip[64] = {0};
        char hw_type[16] = {0};
        char flags[16] = {0};
        char mac[64] = {0};
        char mask[64] = {0};
        char dev[64] = {0};

        if (sscanf(line,
                   "%63s %15s %15s %63s %63s %63s",
                   ip,
                   hw_type,
                   flags,
                   mac,
                   mask,
                   dev) != 6)
        {
            continue;
        }
        if (strcmp(ip, target_ip) != 0)
        {
            continue;
        }
        fclose(fp);
        return kr_indoor_normalize_mac(mac, out, out_len);
    }

    fclose(fp);
    return -ENOENT;
}

static int kr_indoor_resolve_secondary_proxy_sn(simple_indoor_codec_ctx_t *indoor,
                                                const simple_d2d_villa_heartbeat_status_t *status,
                                                char *out,
                                                size_t out_len)
{
    int ret;

    if (!indoor || !status || !out || out_len < TOPOLOGY_MAC_STR_MAX)
    {
        return -EINVAL;
    }

    ret = kr_indoor_normalize_mac(status->mac, out, out_len);
    if (ret == 0)
    {
        safe_strncpy(indoor->secondary_proxy_sn,
                     out,
                     sizeof(indoor->secondary_proxy_sn));
        return 0;
    }

    ret = kr_indoor_lookup_arp_mac(status->endpoint.addr, out, out_len);
    if (ret == 0)
    {
        safe_strncpy(indoor->secondary_proxy_sn,
                     out,
                     sizeof(indoor->secondary_proxy_sn));
        return 0;
    }

    if (indoor->secondary_proxy_sn[0] != '\0')
    {
        safe_strncpy(out, indoor->secondary_proxy_sn, out_len);
        return 0;
    }

    return ret;
}

static int kr_indoor_abs_deadline(struct timespec *deadline, uint32_t timeout_ms)
{
    if (clock_gettime(CLOCK_REALTIME, deadline) != 0)
    {
        return -errno;
    }
    deadline->tv_sec += timeout_ms / 1000U;
    deadline->tv_nsec += (long)(timeout_ms % 1000U) * 1000000L;
    if (deadline->tv_nsec >= 1000000000L)
    {
        deadline->tv_sec++;
        deadline->tv_nsec -= 1000000000L;
    }
    return 0;
}

static uint64_t kr_indoor_monotonic_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return 0;
    }
    return ((uint64_t)ts.tv_sec * 1000ULL) + ((uint64_t)ts.tv_nsec / 1000000ULL);
}

static bool simple_indoor_bridge_h264_dump_enabled(void)
{
    const char *value = getenv(SIMPLE_MEDIA_BRIDGE_H264_DUMP_ENV);

    return value != NULL &&
           value[0] != '\0' &&
           strcmp(value, "0") != 0 &&
           strcmp(value, "false") != 0 &&
           strcmp(value, "FALSE") != 0 &&
           strcmp(value, "off") != 0 &&
           strcmp(value, "OFF") != 0;
}

static int simple_indoor_mkdir_p(const char *path)
{
    char tmp[SIMPLE_MEDIA_BRIDGE_H264_DUMP_PATH_MAX];
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

static void simple_indoor_bridge_h264_dump_close_locked(simple_indoor_codec_ctx_t *indoor)
{
    FILE *file;
    unsigned int frames;
    size_t bytes;
    unsigned int idr_count;
    unsigned int sps_count;
    unsigned int pps_count;
    char path[SIMPLE_MEDIA_BRIDGE_H264_DUMP_PATH_MAX];

    if (indoor == NULL || indoor->media_bridge_h264_input_file == NULL)
    {
        return;
    }

    file = indoor->media_bridge_h264_input_file;
    frames = indoor->media_bridge_h264_input_frames;
    bytes = indoor->media_bridge_h264_input_bytes;
    idr_count = indoor->media_bridge_h264_input_idr_count;
    sps_count = indoor->media_bridge_h264_input_sps_count;
    pps_count = indoor->media_bridge_h264_input_pps_count;
    snprintf(path, sizeof(path), "%s", indoor->media_bridge_h264_input_path);
    indoor->media_bridge_h264_input_file = NULL;
    indoor->media_bridge_h264_input_path[0] = '\0';
    indoor->media_bridge_h264_input_frames = 0;
    indoor->media_bridge_h264_input_bytes = 0;
    indoor->media_bridge_h264_input_idr_count = 0;
    indoor->media_bridge_h264_input_sps_count = 0;
    indoor->media_bridge_h264_input_pps_count = 0;
    fclose(file);
    SIMPLE_LOGI("bridge h264 dump closed stream=input path=%s frames=%u bytes=%zu "
                "idr=%u sps=%u pps=%u\n",
                path,
                frames,
                bytes,
                idr_count,
                sps_count,
                pps_count);
}

static void simple_indoor_bridge_h264_dump_open_locked(simple_indoor_codec_ctx_t *indoor)
{
    const char *dir;
    int ret;

    if (indoor == NULL || indoor->media_bridge_h264_input_file != NULL)
    {
        return;
    }
    if (!simple_indoor_bridge_h264_dump_enabled())
    {
        indoor->media_bridge_h264_dump_tag[0] = '\0';
        return;
    }

    snprintf(indoor->media_bridge_h264_dump_tag,
             sizeof(indoor->media_bridge_h264_dump_tag),
             "bridge_%ld_%llu",
             (long)getpid(),
             (unsigned long long)kr_indoor_monotonic_ms());

    dir = getenv(SIMPLE_MEDIA_BRIDGE_H264_DUMP_DIR_ENV);
    if (dir == NULL || dir[0] == '\0')
    {
        dir = SIMPLE_MEDIA_BRIDGE_H264_DUMP_DEFAULT_DIR;
    }

    ret = simple_indoor_mkdir_p(dir);
    if (ret < 0)
    {
        SIMPLE_LOGE("bridge h264 dump mkdir failed stream=input dir=%s ret=%d\n", dir, ret);
        indoor->media_bridge_h264_dump_failed = true;
        return;
    }

    snprintf(indoor->media_bridge_h264_input_path,
             sizeof(indoor->media_bridge_h264_input_path),
             "%s/%s_input.h264",
             dir,
             indoor->media_bridge_h264_dump_tag);
    indoor->media_bridge_h264_input_file = fopen(indoor->media_bridge_h264_input_path, "wb");
    if (indoor->media_bridge_h264_input_file == NULL)
    {
        SIMPLE_LOGE("bridge h264 dump open failed stream=input path=%s errno=%d\n",
                    indoor->media_bridge_h264_input_path,
                    errno);
        indoor->media_bridge_h264_dump_failed = true;
        indoor->media_bridge_h264_input_path[0] = '\0';
        return;
    }

    indoor->media_bridge_h264_input_frames = 0;
    indoor->media_bridge_h264_input_bytes = 0;
    indoor->media_bridge_h264_input_idr_count = 0;
    indoor->media_bridge_h264_input_sps_count = 0;
    indoor->media_bridge_h264_input_pps_count = 0;
    indoor->media_bridge_h264_dump_failed = false;
    SIMPLE_LOGI("bridge h264 dump open stream=input path=%s tag=%s\n",
                indoor->media_bridge_h264_input_path,
                indoor->media_bridge_h264_dump_tag);
}

static void simple_indoor_bridge_h264_dump_write(simple_indoor_codec_ctx_t *indoor,
                                                 const uint8_t *data,
                                                 size_t len,
                                                 bool has_idr,
                                                 bool has_sps,
                                                 bool has_pps)
{
    if (indoor == NULL || data == NULL || len == 0)
    {
        return;
    }

    pthread_mutex_lock(&indoor->dec_mutex);
    if (indoor->media_bridge_h264_input_file != NULL)
    {
        if (fwrite(data, 1, len, indoor->media_bridge_h264_input_file) != len)
        {
            SIMPLE_LOGE("bridge h264 dump write failed stream=input path=%s errno=%d\n",
                        indoor->media_bridge_h264_input_path,
                        errno);
            indoor->media_bridge_h264_dump_failed = true;
            simple_indoor_bridge_h264_dump_close_locked(indoor);
        }
        else
        {
            indoor->media_bridge_h264_input_frames++;
            indoor->media_bridge_h264_input_bytes += len;
            if (has_idr)
            {
                indoor->media_bridge_h264_input_idr_count++;
            }
            if (has_sps)
            {
                indoor->media_bridge_h264_input_sps_count++;
            }
            if (has_pps)
            {
                indoor->media_bridge_h264_input_pps_count++;
            }
        }
    }
    pthread_mutex_unlock(&indoor->dec_mutex);
}

static bool simple_indoor_primary_uses_plc(
    const simple_indoor_link_plan_t *plan)
{
    return plan && plan->primary_path != SIMPLE_INDOOR_PRIMARY_NONE;
}

static bool simple_indoor_primary_uses_unit_door_plc(
    const simple_indoor_link_plan_t *plan)
{
    return plan &&
           plan->primary_path == SIMPLE_INDOOR_PRIMARY_VIA_UNIT_DOOR_PLC;
}

static bool simple_indoor_secondary_requested(
    const simple_indoor_link_plan_t *plan)
{
    return plan && plan->secondary_policy != SIMPLE_INDOOR_SECONDARY_OFF;
}

static bool simple_indoor_secondary_required(
    const simple_indoor_link_plan_t *plan)
{
    return plan &&
           plan->secondary_policy == SIMPLE_INDOOR_SECONDARY_REQUIRED;
}

static const char *simple_indoor_primary_path_name(
    simple_indoor_primary_path_t path)
{
    switch (path)
    {
        case SIMPLE_INDOOR_PRIMARY_VIA_CONVERTER:
            return "converter-plc";
        case SIMPLE_INDOOR_PRIMARY_VIA_UNIT_DOOR_PLC:
            return "unit-door-plc";
        case SIMPLE_INDOOR_PRIMARY_NONE:
            return "none";
        default:
            return "unknown";
    }
}

static const char *simple_indoor_secondary_policy_name(
    simple_indoor_secondary_policy_t policy)
{
    switch (policy)
    {
        case SIMPLE_INDOOR_SECONDARY_OFF:
            return "off";
        case SIMPLE_INDOOR_SECONDARY_OPTIONAL:
            return "optional";
        case SIMPLE_INDOOR_SECONDARY_REQUIRED:
            return "required";
        default:
            return "unknown";
    }
}

static bool simple_indoor_request_uses_secondary_confirm(
    const simple_indoor_codec_ctx_t *indoor,
    const simple_media_endpoint_request_t *request)
{
    return indoor && indoor->secondary_active && request &&
           (simple_device_id_equal(request->caller_device_id,
                                   SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID) ||
            simple_device_id_equal(request->callee_device_id,
                                   SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID));
}

static int simple_indoor_apply_bootstrap_route(simple_call_app_t *app,
                                               size_t *route_count,
                                               const char *callee_device_id,
                                               call_link_kind_t link_kind,
                                               uint32_t addr)
{
    simple_control_route_fact_t fact;
    int ret;

    if (!app || !route_count)
    {
        return -EINVAL;
    }
    if (link_kind == CALL_LINK_PLC)
    {
        ret = simple_control_route_provider_build_terminal_plc_bootstrap(
            &fact,
            callee_device_id,
            addr);
    }
    else if (link_kind == CALL_LINK_UDP)
    {
        ret = simple_control_route_provider_build_terminal_udp_bootstrap(
            &fact,
            callee_device_id,
            addr);
    }
    else
    {
        return -ENOTSUP;
    }
    if (ret == 0)
    {
        ret = simple_call_apply_route_fact(app, &fact);
    }
    if (ret == 0)
    {
        (*route_count)++;
    }
    return ret;
}

static void simple_indoor_secondary_heartbeat_update(
    void *user_ctx,
    const simple_d2d_villa_heartbeat_status_t *status)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)user_ctx;
    kr_indoor_handle_t *handle = indoor ? (kr_indoor_handle_t *)indoor->event_ctx : NULL;
    simple_control_route_fact_t route_fact;
    char proxy_sn[TOPOLOGY_MAC_STR_MAX] = {0};
    int ret;

    if (!indoor || !status || !indoor->call_app ||
        !simple_device_id_equal(status->device_id,
                                SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID))
    {
        return;
    }

    if (kr_indoor_resolve_secondary_proxy_sn(indoor,
                                             status,
                                             proxy_sn,
                                             sizeof(proxy_sn)) != 0)
    {
        SIMPLE_LOGW("secondary confirm proxy mac unresolved: device_id=%s "
                    "addr=0x%08x heartbeat_mac=%s\n",
                    status->device_id,
                    status->endpoint.addr,
                    status->mac);
    }

    if (!status->online)
    {
        ret = simple_control_route_provider_build_secondary_confirm_learned_offline(
            &route_fact,
            status->device_id);
        if (ret == 0)
        {
            ret = simple_call_apply_route_fact(indoor->call_app, &route_fact);
        }
        if (simple_control_route_provider_build_observed_secondary_confirm_offline(
                &route_fact,
                status->device_id) == 0)
        {
            (void)simple_call_apply_route_fact(indoor->call_app, &route_fact);
        }
        if (simple_device_id_equal(indoor->fallback_peer_device_id, status->device_id))
        {
            indoor->fallback_peer_udp_addr = 0;
        }
        (void)simple_d2d_business_runtime_update_secondary_unlock_peer(
            indoor->d2d_business,
            0);
        SIMPLE_LOGI("secondary confirm source offline: source=25000 device_id=%s "
                    "ret=%d\n",
                    status->device_id,
                    ret);
        kr_indoor_push_secondary_confirm_event(
            handle,
            KR_INDOOR_EVENT_SECONDARY_CONFIRM_OFFLINE,
            0,
            ret,
            proxy_sn,
            status->dev_model);
        return;
    }

    ret = simple_control_route_provider_build_secondary_confirm_learned_online(
        &route_fact,
        status->device_id,
        status->endpoint.addr,
        status->observed_ms,
        status->ttl_ms);
    if (ret == 0)
    {
        ret = simple_call_apply_route_fact(indoor->call_app, &route_fact);
    }
    indoor->fallback_peer_udp_addr = status->endpoint.addr;
    (void)simple_d2d_business_runtime_update_secondary_unlock_peer(
        indoor->d2d_business,
        status->endpoint.addr);
    snprintf(indoor->fallback_peer_device_id,
             sizeof(indoor->fallback_peer_device_id),
             "%s",
             status->device_id);
    SIMPLE_LOGI("secondary confirm source online: source=25000 device_id=%s "
                "proxy_sn=%s addr=0x%08x ttl=%u ret=%d\n",
                status->device_id,
                proxy_sn,
                status->endpoint.addr,
                status->ttl_ms,
                ret);
    kr_indoor_push_secondary_confirm_event(
        handle,
        KR_INDOOR_EVENT_SECONDARY_CONFIRM_ONLINE,
        status->endpoint.addr,
        ret,
        proxy_sn,
        status->dev_model);
}

static bool kr_indoor_external_call_busy(void *user_ctx)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)user_ctx;
    kr_indoor_handle_t *handle = indoor ? (kr_indoor_handle_t *)indoor->event_ctx : NULL;
    bool busy = false;

    if (!handle || !handle->locks_ready)
    {
        return false;
    }

    pthread_mutex_lock(&handle->queue_mutex);
    busy = handle->external_call_busy;
    pthread_mutex_unlock(&handle->queue_mutex);
    return busy;
}

static void simple_indoor_secondary_inbound_observer(
    void *user_ctx,
    const call_flow_runtime_inbound_event_t *event)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)user_ctx;
    simple_control_route_fact_t route_fact;
    const call_payload_t *payload;
    int ret;

    if (!indoor || !indoor->secondary_active || !indoor->call_app || !event ||
        event->src_ep.link_kind != CALL_LINK_UDP ||
        event->src_ep.addr == 0 || event->src_ep.port == 0)
    {
        return;
    }
    if (event->header.msg_type != CALL_MSG_CALL_INVITE &&
        event->header.msg_type != CALL_MSG_MONITOR_START)
    {
        return;
    }

    payload = &event->payload;
    if (!payload->has_caller_device_id ||
        !payload->has_callee_device_id ||
        !simple_device_id_equal(payload->caller_device_id,
                                SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID) ||
        !simple_device_id_equal(payload->callee_device_id,
                                SIMPLE_INDOOR_DEVICE_ID))
    {
        return;
    }

    ret = simple_control_route_provider_build_observed_secondary_confirm_peer(
        &route_fact,
        payload->caller_device_id,
        &event->src_ep);
    if (ret == 0)
    {
        ret = simple_call_apply_route_fact(indoor->call_app, &route_fact);
    }
    if (ret == 0)
    {
        indoor->fallback_peer_udp_addr = event->src_ep.addr;
        (void)simple_d2d_business_runtime_update_secondary_unlock_peer(
            indoor->d2d_business,
            event->src_ep.addr);
        snprintf(indoor->fallback_peer_device_id,
                 sizeof(indoor->fallback_peer_device_id),
                 "%s",
                 payload->caller_device_id);
    }
    SIMPLE_LOGI("secondary confirm observed route: source=inbound msg=%s "
                "device_id=%s addr=0x%08x ret=%d\n",
                call_proto_msg_type_name(event->header.msg_type),
                payload->caller_device_id,
                event->src_ep.addr,
                ret);
}


static void kr_indoor_inbound_observer(void *user_ctx,
                                       const call_flow_runtime_inbound_event_t *event)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)user_ctx;
    kr_indoor_handle_t *handle = indoor ? (kr_indoor_handle_t *)indoor->event_ctx : NULL;
    const call_payload_t *payload = event ? &event->payload : NULL;
    const char *caller = NULL;

    simple_indoor_secondary_inbound_observer(user_ctx, event);

    if (!handle || !event)
    {
        return;
    }

    if (event->header.msg_type == CALL_MSG_CALL_INVITE ||
        event->header.msg_type == CALL_MSG_MONITOR_START)
    {
        if (payload && payload->has_caller_device_id)
        {
            caller = payload->caller_device_id;
        }
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_INCOMING,
                                    SIMPLE_SESSION_CALL_RINGING,
                                    0,
                                    false,
                                    false,
                                    caller ? caller : "");
    }
    else if (event->header.msg_type == CALL_MSG_CALL_BYE ||
             event->header.msg_type == CALL_MSG_MONITOR_STOP ||
             event->header.msg_type == CALL_MSG_MONITOR_REJECT)
    {
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_REMOTE_HANGUP,
                                    SIMPLE_SESSION_IDLE,
                                    0,
                                    false,
                                    false,
                                    NULL);
    }
}

static void simple_indoor_enable_aec_if_ready(simple_indoor_codec_ctx_t *indoor)
{
    if (!indoor || indoor->aec_enabled || indoor->pb_stream == 0 || indoor->ca_stream == 0)
    {
        return;
    }

    AudioCore_SetAecMode(true);
    indoor->aec_enabled = true;
    SIMPLE_LOGI("indoor audio aec enable\n");
}

static void simple_indoor_disable_aec_if_enabled(simple_indoor_codec_ctx_t *indoor)
{
    if (!indoor || !indoor->aec_enabled)
    {
        return;
    }

    AudioCore_SetAecMode(false);
    indoor->aec_enabled = false;
    SIMPLE_LOGI("indoor audio aec disable\n");
}

static int codec_log_start(void *ctx)
{
    const char *name = (const char *)ctx;

    SIMPLE_LOGI("%s start\n", name);
    return 0;
}

static void codec_log_stop(void *ctx)
{
    const char *name = (const char *)ctx;

    SIMPLE_LOGI("%s stop\n", name);
}

static const uint8_t *ff_avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *a = p + 4 - ((intptr_t)p & 3);

	for (end -= 3; p < a && p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	for (end -= 3; p < end; p += 4) {
		uint32_t x = *(const uint32_t *)p;

		if ((x - 0x01010101) & (~x) & 0x80808080) {
			if (p[1] == 0) {
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p + 1;
			}

			if (p[3] == 0) {
				if (p[2] == 0 && p[4] == 1)
					return p + 2;
				if (p[4] == 0 && p[5] == 1)
					return p + 3;
			}
		}
	}

	for (end += 3; p < end; p++) {
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}

	return end + 3;
}

static const uint8_t *nal_find_startcode(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *out = ff_avc_find_startcode_internal(p, end);
	if (p < out && out < end && !out[-1])
		out--;
	return out;
}

static void simple_indoor_h264_gate_init(simple_indoor_h264_gate_t *gate)
{
    if (!gate)
    {
        return;
    }

    memset(gate, 0, sizeof(*gate));
    resize_buf_init(&gate->sps, 128);
    resize_buf_init(&gate->pps, 64);
}

static void simple_indoor_h264_gate_deinit(simple_indoor_h264_gate_t *gate)
{
    if (!gate)
    {
        return;
    }

    resize_buf_free(&gate->sps);
    resize_buf_free(&gate->pps);
    memset(gate, 0, sizeof(*gate));
}

static void simple_indoor_h264_gate_lock(simple_indoor_h264_gate_t *gate)
{
    if (!gate)
    {
        return;
    }

    gate->unlocked = false;
    gate->wait_idr_logged = false;
    gate->missing_param_logged = false;
    gate->pli_requested = false;
}

static void simple_indoor_h264_gate_request_pli(simple_indoor_codec_ctx_t *indoor,
                                                simple_indoor_h264_gate_t *gate,
                                                const char *reason)
{
    if (!gate || gate->pli_requested)
    {
        return;
    }

    gate->pli_requested = true;
    simple_indoor_request_video_recovery(indoor, false, reason);
}

static int simple_indoor_h264_gate_decode(simple_indoor_video_decoder_t *dec,
                                          const uint8_t *data,
                                          size_t size)
{
    if (!dec || !data || size == 0 || size > (size_t)INT32_MAX)
    {
        return -EINVAL;
    }

    if (f1_video_decoder_do_decode(&dec->codec_data, (uint8_t *)data, (int)size) < 0)
    {
        SIMPLE_LOGE("simple indoor video decode enqueue failed\n");
        return -1;
    }

    return 0;
}

static int simple_indoor_h264_gate_send_parameter_sets(simple_indoor_video_decoder_t *dec)
{
    simple_indoor_h264_gate_t *gate = dec ? &dec->h264_gate : NULL;

    if (!gate || gate->sps.size == 0 || gate->pps.size == 0)
    {
        return -EINVAL;
    }
    if (simple_indoor_h264_gate_decode(dec, gate->sps.buf, gate->sps.size) < 0)
    {
        return -1;
    }
    if (simple_indoor_h264_gate_decode(dec, gate->pps.buf, gate->pps.size) < 0)
    {
        return -1;
    }
    return 0;
}

static bool simple_indoor_h264_next_nal(const uint8_t *data,
                                        size_t size,
                                        const uint8_t **nal,
                                        size_t *nal_size,
                                        int *nal_type,
                                        const uint8_t **next)
{
    const uint8_t *end;
    const uint8_t *start_code;
    const uint8_t *payload;
    const uint8_t *nal_end;

    if (!data || size == 0 || !nal || !nal_size || !nal_type || !next)
    {
        return false;
    }

    end = data + size;
    start_code = nal_find_startcode(data, end);
    if (start_code >= end)
    {
        return false;
    }

    payload = start_code;
    while (payload < end && !*(payload++))
    {
        ;
    }
    if (payload >= end)
    {
        return false;
    }

    nal_end = nal_find_startcode(payload, end);
    if (nal_end > end)
    {
        nal_end = end;
    }
    if (nal_end <= start_code)
    {
        return false;
    }

    *nal = start_code;
    *nal_size = (size_t)(nal_end - start_code);
    *nal_type = H264_NAL_TYPE(payload[0]);
    *next = nal_end;
    return true;
}

typedef struct
{
    bool has_idr;
    bool has_sps;
    bool has_pps;
} simple_indoor_h264_bridge_info_t;

static simple_indoor_h264_bridge_info_t simple_indoor_h264_bridge_analyze(
    const uint8_t *data,
    size_t size
)
{
    simple_indoor_h264_bridge_info_t info;
    const uint8_t *pos = data;
    const uint8_t *end = data ? data + size : NULL;

    memset(&info, 0, sizeof(info));
    while (pos && pos < end)
    {
        const uint8_t *nal = NULL;
        const uint8_t *next = NULL;
        const uint8_t *payload;
        const uint8_t *nal_end;
        size_t nal_size = 0;
        int nal_type = -1;

        if (!simple_indoor_h264_next_nal(pos,
                                         (size_t)(end - pos),
                                         &nal,
                                         &nal_size,
                                         &nal_type,
                                         &next))
        {
            break;
        }

        payload = nal;
        nal_end = nal + nal_size;
        while (payload < nal_end && !*(payload++))
        {
            ;
        }
        if (payload >= nal_end)
        {
            pos = next;
            continue;
        }

        if (nal_type == H264_NAL_FU_A && payload + 1 < nal_end && (payload[1] & 0x80) != 0)
        {
            nal_type = H264_NAL_TYPE(payload[1]);
        }

        if (nal_type == H264_NAL_IDR_SLICE)
        {
            info.has_idr = true;
        }
        else if (nal_type == H264_NAL_SPS)
        {
            info.has_sps = true;
        }
        else if (nal_type == H264_NAL_PPS)
        {
            info.has_pps = true;
        }
        pos = next;
    }
    return info;
}

static void simple_indoor_media_bridge_h264_cache_parameter_sets(
    simple_indoor_codec_ctx_t *indoor,
    const uint8_t *data,
    size_t size)
{
    const uint8_t *pos = data;
    const uint8_t *end = data ? data + size : NULL;

    if (indoor == NULL || data == NULL || size == 0)
    {
        return;
    }

    while (pos && pos < end)
    {
        const uint8_t *nal = NULL;
        const uint8_t *next = NULL;
        size_t nal_size = 0;
        int nal_type = -1;

        if (!simple_indoor_h264_next_nal(pos,
                                         (size_t)(end - pos),
                                         &nal,
                                         &nal_size,
                                         &nal_type,
                                         &next))
        {
            break;
        }

        if (nal_type == H264_NAL_SPS)
        {
            resize_buf_copy(&indoor->media_bridge_h264_sps, nal, nal_size);
        }
        else if (nal_type == H264_NAL_PPS)
        {
            resize_buf_copy(&indoor->media_bridge_h264_pps, nal, nal_size);
        }

        if (!next || next <= pos)
        {
            break;
        }
        pos = next;
    }
}

static const uint8_t *simple_indoor_media_bridge_h264_build_send_frame(
    simple_indoor_codec_ctx_t *indoor,
    const uint8_t *data,
    size_t size,
    const simple_indoor_h264_bridge_info_t *info,
    size_t *out_size)
{
    bool prepend_sps;
    bool prepend_pps;

    if (out_size != NULL)
    {
        *out_size = size;
    }
    if (indoor == NULL || data == NULL || size == 0 || info == NULL || !info->has_idr)
    {
        return data;
    }

    prepend_sps = !info->has_sps && indoor->media_bridge_h264_sps.size > 0;
    prepend_pps = !info->has_pps && indoor->media_bridge_h264_pps.size > 0;
    if (!prepend_sps && !prepend_pps)
    {
        return data;
    }

    resize_buf_copy(&indoor->media_bridge_h264_send_buf, NULL, 0);
    if (prepend_sps)
    {
        resize_buf_append(&indoor->media_bridge_h264_send_buf,
                          indoor->media_bridge_h264_sps.buf,
                          indoor->media_bridge_h264_sps.size);
    }
    if (prepend_pps)
    {
        resize_buf_append(&indoor->media_bridge_h264_send_buf,
                          indoor->media_bridge_h264_pps.buf,
                          indoor->media_bridge_h264_pps.size);
    }
    resize_buf_append(&indoor->media_bridge_h264_send_buf, data, size);

    if (out_size != NULL)
    {
        *out_size = indoor->media_bridge_h264_send_buf.size;
    }
    return indoor->media_bridge_h264_send_buf.buf;
}

static bool simple_indoor_media_bridge_video_should_send(
    simple_indoor_codec_ctx_t *indoor,
    uint32_t ts,
    uint64_t now_ms,
    const simple_indoor_h264_bridge_info_t *info
)
{
    bool important;
    bool new_ts;
    bool send;

    if (!indoor || !info || now_ms == 0)
    {
        return true;
    }

    important = info->has_idr || info->has_sps || info->has_pps;
    new_ts = !indoor->media_bridge_video_last_ts_valid ||
             indoor->media_bridge_video_last_ts != ts;

    if (!new_ts)
    {
        return indoor->media_bridge_video_current_ts_send;
    }

    send = important ||
           indoor->media_bridge_video_last_send_ms == 0 ||
           now_ms - indoor->media_bridge_video_last_send_ms >=
               SIMPLE_MEDIA_BRIDGE_VIDEO_MIN_INTERVAL_MS;

    indoor->media_bridge_video_last_ts_valid = true;
    indoor->media_bridge_video_last_ts = ts;
    indoor->media_bridge_video_current_ts_send = send;
    if (send)
    {
        indoor->media_bridge_video_last_send_ms = now_ms;
    }

    return send;
}

static void simple_indoor_media_bridge_video_log_stats(
    simple_indoor_codec_ctx_t *indoor,
    uint64_t now_ms
)
{
    if (!indoor || now_ms == 0)
    {
        return;
    }

    if (indoor->media_bridge_video_stats_start_ms == 0)
    {
        indoor->media_bridge_video_stats_start_ms = now_ms;
        indoor->media_bridge_video_stats_last_log_ms = now_ms;
        return;
    }

    if (now_ms - indoor->media_bridge_video_stats_last_log_ms <
        SIMPLE_MEDIA_BRIDGE_VIDEO_STATS_INTERVAL_MS)
    {
        return;
    }

    indoor->media_bridge_video_stats_last_log_ms = now_ms;
}

static int simple_indoor_h264_gate_process_nal(simple_indoor_codec_ctx_t *indoor,
                                               const uint8_t *nal,
                                               size_t nal_size,
                                               int nal_type,
                                               uint32_t ts,
                                               int flags)
{
    simple_indoor_video_decoder_t *dec = indoor ? &indoor->video : NULL;
    simple_indoor_h264_gate_t *gate = dec ? &dec->h264_gate : NULL;

    if (!gate || !nal || nal_size == 0)
    {
        return 0;
    }

    if (nal_type == H264_NAL_SPS)
    {
        resize_buf_copy(&gate->sps, nal, nal_size);
        return 0;
    }
    if (nal_type == H264_NAL_PPS)
    {
        resize_buf_copy(&gate->pps, nal, nal_size);
        return 0;
    }

    if (nal_type == H264_NAL_IDR_SLICE)
    {
        if (gate->sps.size == 0 || gate->pps.size == 0)
        {
            if (!gate->missing_param_logged)
            {
                gate->missing_param_logged = true;
                SIMPLE_LOGW("indoor H264 gate drop IDR without SPS/PPS: ts=%u flags=0x%x\n",
                            ts, flags);
            }
            simple_indoor_h264_gate_request_pli(indoor, gate, "wait_sps_pps");
            return 0;
        }

        if (simple_indoor_h264_gate_send_parameter_sets(dec) < 0 ||
            simple_indoor_h264_gate_decode(dec, nal, nal_size) < 0)
        {
            return -1;
        }

        gate->unlocked = true;
        gate->wait_idr_logged = false;
        gate->missing_param_logged = false;
        gate->corrupt_logged = false;
        gate->pli_requested = false;

        if (!dec->first_frame_logged)
        {
            dec->first_frame_logged = true;
            SIMPLE_LOGI("indoor first video frame ready: size=%zu ts=%u flags=0x%x\n",
                        nal_size, ts, flags);
        }
        return 0;
    }

    if (!gate->unlocked)
    {
        if (nal_type >= H264_NAL_SLICE && nal_type <= 4)
        {
            if (!gate->wait_idr_logged)
            {
                gate->wait_idr_logged = true;
                SIMPLE_LOGW("indoor H264 gate waiting IDR, drop slice: nal=%d ts=%u flags=0x%x\n",
                            nal_type, ts, flags);
            }
            simple_indoor_h264_gate_request_pli(indoor, gate, "wait_idr");
        }
        return 0;
    }

    return simple_indoor_h264_gate_decode(dec, nal, nal_size);
}

static int simple_indoor_h264_gate_process(simple_indoor_codec_ctx_t *indoor,
                                           const uint8_t *data,
                                           size_t size,
                                           uint32_t ts,
                                           int flags)
{
    simple_indoor_video_decoder_t *dec = indoor ? &indoor->video : NULL;
    simple_indoor_h264_gate_t *gate = dec ? &dec->h264_gate : NULL;
    const uint8_t *pos = data;
    size_t remain = size;
    bool is_corrupted;

    if (!gate || !data || size == 0)
    {
        return 0;
    }

    is_corrupted = (flags & MTRANS_RX_FLAG_PACKET_CORRUPT) != 0 ||
                   (flags & MTRANS_RX_FLAG_PACKET_LOST) != 0;
    if (is_corrupted)
    {
        simple_indoor_h264_gate_lock(gate);
        if (!gate->corrupt_logged)
        {
            gate->corrupt_logged = true;
            SIMPLE_LOGW("indoor H264 gate drop corrupted input: size=%zu ts=%u flags=0x%x\n",
                        size, ts, flags);
        }
        simple_indoor_h264_gate_request_pli(indoor, gate, "corrupt_h264");
        return 0;
    }

    while (remain > 0)
    {
        const uint8_t *nal = NULL;
        const uint8_t *next = NULL;
        size_t nal_size = 0;
        int nal_type = -1;

        if (!simple_indoor_h264_next_nal(pos, remain, &nal, &nal_size, &nal_type, &next))
        {
            break;
        }
        if (simple_indoor_h264_gate_process_nal(indoor,
                                                nal,
                                                nal_size,
                                                nal_type,
                                                ts,
                                                flags) < 0)
        {
            return -1;
        }
        if (!next || next <= pos)
        {
            break;
        }
        remain = (size_t)((data + size) - next);
        pos = next;
    }

    return 0;
}

static int simple_indoor_start_video_decoder(void *ctx)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;
    simple_indoor_video_decoder_t *dec = indoor ? &indoor->video : NULL;
    int ret;

    if (!dec)
    {
        return -1;
    }
    if (dec->running)
    {
        return 0;
    }

    memset(&dec->codec_data, 0, sizeof(dec->codec_data));
    dec->codec_data.decoder_chan = -1;
    (void)f1_video_decoder_set_brightness(&dec->codec_data,
                                           indoor->preview_brightness);

    ret = f1_video_decoder_open(&dec->codec_data,
                             0,
                             0,
                             SIMPLE_INDOOR_VIDEO_W,
                             SIMPLE_INDOOR_VIDEO_H,
                             VDEC_PAYLOAD_TYPE_H264);

    if (ret < 0)
    {
        SIMPLE_LOGE("simple indoor video decoder open failed\n");
        return -1;
    }

    resize_buf_init(&dec->rb, 128 * 1024);
    simple_indoor_h264_gate_init(&dec->h264_gate);

    dec->running = true;
    dec->first_packet_logged = false;
    dec->first_frame_logged = false;
    SIMPLE_LOGI("indoor video decoder start\n");
    return 0;
}

static void simple_indoor_stop_video_decoder(void *ctx)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;
    simple_indoor_video_decoder_t *dec = indoor ? &indoor->video : NULL;

    if (!dec || !dec->running)
    {
        return;
    }

    dec->running = false;
    f1_video_decoder_close(&dec->codec_data);
    resize_buf_free(&dec->rb);
    simple_indoor_h264_gate_deinit(&dec->h264_gate);
    memset(&dec->codec_data, 0, sizeof(dec->codec_data));
    dec->codec_data.decoder_chan = -1;
    dec->first_packet_logged = false;
    dec->first_frame_logged = false;
    SIMPLE_LOGI("indoor video decoder stop\n");
}

static int simple_indoor_input_video_frame(void *ctx,
                                           const uint8_t *frame_data,
                                           size_t len,
                                           uint32_t ts,
                                           int flags)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;
    simple_indoor_video_decoder_t *dec = indoor ? &indoor->video : NULL;
    uint8_t start_code[] = {0x00, 0x00, 0x00, 0x01};
    uint64_t now_ms = 0;

    if (!dec || !dec->running || !frame_data || len == 0)
    {
        return 0;
    }

    if (!dec->first_packet_logged)
    {
        dec->first_packet_logged = true;
        SIMPLE_LOGI("indoor first video packet received: len=%zu ts=%u flags=0x%x\n",
               len, ts, flags);
    }

    resize_buf_copy(&dec->rb, start_code, sizeof(start_code));
    resize_buf_append(&dec->rb, frame_data, len);

    if (__atomic_load_n(&indoor->media_bridge_enabled, __ATOMIC_RELAXED) != 0 &&
        indoor->webrtc_bridge != NULL)
    {
        simple_indoor_h264_bridge_info_t info =
            simple_indoor_h264_bridge_analyze(dec->rb.buf, dec->rb.size);
        const uint8_t *send_data;
        size_t send_size;
        int ret;

        now_ms = kr_indoor_monotonic_ms();
        indoor->media_bridge_video_input_count++;
        indoor->media_bridge_video_bytes += dec->rb.size;
        if (dec->rb.size > indoor->media_bridge_video_max_len)
        {
            indoor->media_bridge_video_max_len = dec->rb.size;
        }
        if (info.has_idr)
        {
            indoor->media_bridge_video_idr_count++;
            indoor->media_bridge_video_last_idr_ms = now_ms;
        }
        if (info.has_sps)
        {
            indoor->media_bridge_video_sps_count++;
        }
        if (info.has_pps)
        {
            indoor->media_bridge_video_pps_count++;
        }
        simple_indoor_bridge_h264_dump_write(indoor,
                                             dec->rb.buf,
                                             dec->rb.size,
                                             info.has_idr,
                                             info.has_sps,
                                             info.has_pps);
        simple_indoor_media_bridge_h264_cache_parameter_sets(indoor, dec->rb.buf, dec->rb.size);
        send_data = simple_indoor_media_bridge_h264_build_send_frame(indoor,
                                                                     dec->rb.buf,
                                                                     dec->rb.size,
                                                                     &info,
                                                                     &send_size);

        ret = kr_webrtc_send_video_frame(
            indoor->webrtc_bridge,
            0,
            0,
            send_data,
            send_size
        );
        if (ret < 0)
        {
            indoor->media_bridge_video_failed_count++;
            SIMPLE_LOGE("media bridge send video to webrtc failed ret=%d len=%zu ts=%u\n",
                        ret,
                        send_size,
                        ts);
        }
        else
        {
            indoor->media_bridge_video_sent_count++;
            indoor->media_bridge_video_sent_bytes += send_size;
            if (!indoor->media_bridge_first_video_logged)
            {
                indoor->media_bridge_first_video_logged = true;
                SIMPLE_LOGI("media bridge first video to webrtc len=%zu ts=%u "
                            "idr=%d sps=%d pps=%d mode=full send_len=%zu\n",
                             dec->rb.size,
                             ts,
                             info.has_idr ? 1 : 0,
                             info.has_sps ? 1 : 0,
                             info.has_pps ? 1 : 0,
                             send_size);
            }
        }

        simple_indoor_media_bridge_video_log_stats(indoor, now_ms);
    }

    if (simple_indoor_h264_gate_process(indoor, dec->rb.buf, dec->rb.size, ts, flags) < 0)
    {
        return -1;
    }

    return 0;
}

static void simple_indoor_request_video_recovery(simple_indoor_codec_ctx_t *indoor,
                                                 bool use_fir,
                                                 const char *reason)
{
    simple_media_runtime_video_feedback_t feedback;
    int ret;

    if (!indoor)
    {
        return;
    }

    feedback = use_fir ? SIMPLE_MEDIA_RUNTIME_VIDEO_FEEDBACK_FIR :
                         SIMPLE_MEDIA_RUNTIME_VIDEO_FEEDBACK_PLI;
    ret = simple_media_runtime_request_video_feedback(&indoor->media_runtime,
                                                      feedback,
                                                      reason);
    if (ret == 0)
    {
        SIMPLE_LOGI("indoor active rtcp %s sent (%s)\n",
               use_fir ? "FIR" : "PLI",
               reason ? reason : "-");
    }
    else if (ret == -EAGAIN || ret == -ENOENT)
    {
        return;
    }
    else
    {
        SIMPLE_LOGI("indoor active rtcp %s failed (%s), ret=%d\n",
               use_fir ? "FIR" : "PLI",
               reason ? reason : "-",
               ret);
    }
}

static void simple_indoor_on_stream_event(void *ctx, mtrans_stream_event_t event)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;

    if (!indoor)
    {
        return;
    }

    if (event == MTRANS_STREAM_EVENT_JITTER_TIMEOUT)
    {
        SIMPLE_LOGI("indoor stream event: jitter_timeout\n");
        simple_indoor_request_video_recovery(indoor, false, "jitter_timeout");
    }
    else if (event == MTRANS_STREAM_EVENT_NETWORK_POOR)
    {
        SIMPLE_LOGI("indoor stream event: network_poor\n");
        simple_indoor_request_video_recovery(indoor, true, "network_poor");
    }
    else if (event == MTRANS_STREAM_EVENT_PLI_REQUESTED)
    {
        SIMPLE_LOGI("indoor stream event: pli_requested\n");
    }
}

static int simple_indoor_playback_cb(char *data_buffer, int data_size, void *ctx)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;

    if (!indoor || !data_buffer || data_size <= 0)
    {
        return 0;
    }

    memset(data_buffer, 0, (size_t)data_size);
    if (__atomic_load_n(&indoor->media_bridge_enabled, __ATOMIC_RELAXED) != 0)
    {
        if (!indoor->media_bridge_local_playback_muted_logged)
        {
            indoor->media_bridge_local_playback_muted_logged = true;
            SIMPLE_LOGI("media bridge local playback muted\n");
        }
        return data_size;
    }
    pthread_mutex_lock(&indoor->pb_mutex);
    if (indoor->pb_packet.size >= (size_t)data_size)
    {
        circlebuf_pop_front(&indoor->pb_packet, data_buffer, (size_t)data_size);
    }
    else if (indoor->pb_packet.size > 0)
    {
        size_t copy_len = indoor->pb_packet.size;

        circlebuf_pop_front(&indoor->pb_packet, data_buffer, copy_len);
    }
    pthread_mutex_unlock(&indoor->pb_mutex);
    return data_size;
}

static void simple_indoor_send_pcm_to_d2d(simple_indoor_codec_ctx_t *indoor,
                                          const uint8_t *pcm,
                                          size_t pcm_len,
                                          const char *source)
{
    size_t offset = 0;

    if (!indoor || !pcm || pcm_len == 0 ||
        !simple_media_runtime_audio_tx_ready(&indoor->media_runtime) ||
        !indoor->opus_encoder)
    {
        return;
    }

    while (offset + SIMPLE_AUDIO_BLOCK_SIZE <= pcm_len)
    {
        uint8_t out_buf[1024];
        size_t out_len = sizeof(out_buf);

        if (libopus_encoder_encode(indoor->opus_encoder,
                                   (uint8_t *)pcm + offset,
                                   SIMPLE_AUDIO_BLOCK_SIZE,
                                   out_buf,
                                   &out_len) == 0 &&
            out_len > 0)
        {
            uint32_t rtp_ts = indoor->audio_next_rtp_ts;

            if (simple_media_runtime_input_audio(&indoor->media_runtime,
                                                 out_buf,
                                                 out_len,
                                                 rtp_ts) < 0)
            {
                SIMPLE_LOGE("simple indoor audio tx failed source=%s\n",
                            source ? source : "-");
            }
            else if (!indoor->audio_first_tx_logged)
            {
                simple_media_runtime_endpoint_snapshot_t snapshot;
                uint32_t audio_tx_remote_addr = 0;
                uint16_t audio_tx_remote_port = 0;

                indoor->audio_first_tx_logged = true;
                memset(&snapshot, 0, sizeof(snapshot));
                if (simple_media_runtime_get_endpoint_snapshot(
                        &indoor->media_runtime,
                        SIMPLE_MEDIA_RUNTIME_ENDPOINT_SNAPSHOT_CURRENT,
                        &snapshot))
                {
                    audio_tx_remote_addr = snapshot.audio_tx_remote_ep_enabled ?
                                           snapshot.audio_tx_remote_addr :
                                           snapshot.plc_remote_addr;
                    audio_tx_remote_port = snapshot.audio_tx_remote_ep_enabled ?
                                           snapshot.audio_tx_remote_port :
                                           snapshot.plc_remote_port;
                }
                SIMPLE_LOGI("indoor first audio frame sent: source=%s len=%zu rtp_ts=%u "
                       "plc_local=0x%x:0x%x plc_audio_tx_remote=0x%x:0x%x "
                       "plc_default_remote=0x%x:0x%x "
                       "plc_rx_match=%s 0x%x:0x%x\n",
                       source ? source : "-",
                       out_len,
                       rtp_ts,
                       snapshot.plc_local_addr,
                       SIMPLE_PLC_MEDIA_PORT,
                       audio_tx_remote_addr,
                       audio_tx_remote_port,
                       snapshot.plc_remote_addr,
                       snapshot.plc_remote_port,
                       snapshot.plc_rx_match_ep_enabled ? "on" : "off",
                       snapshot.plc_rx_match_addr,
                       snapshot.plc_rx_match_port);
            }
            indoor->audio_next_rtp_ts += SIMPLE_AUDIO_BLOCK_TIME * 48U;
        }
        offset += SIMPLE_AUDIO_BLOCK_SIZE;
    }
}

static size_t simple_indoor_resample_s16_mono_16k_to_8k(const uint8_t *src,
                                                        size_t src_len,
                                                        uint8_t *dst,
                                                        size_t dst_cap)
{
    const int16_t *src_samples = (const int16_t *)src;
    int16_t *dst_samples = (int16_t *)dst;
    size_t src_count;
    size_t dst_count;
    size_t i;

    if (!src || !dst || src_len < 4 ||
        (SIMPLE_AUDIO_SAMPLE_RATE != SIMPLE_WEBRTC_BRIDGE_SAMPLE_RATE * 2))
    {
        return 0;
    }

    src_count = src_len / sizeof(int16_t);
    dst_count = src_count / 2;
    if (dst_cap < dst_count * sizeof(int16_t))
    {
        return 0;
    }

    for (i = 0; i < dst_count; ++i)
    {
        int32_t mixed = (int32_t)src_samples[i * 2] + (int32_t)src_samples[i * 2 + 1];
        dst_samples[i] = (int16_t)(mixed / 2);
    }
    return dst_count * sizeof(int16_t);
}

static size_t simple_indoor_resample_s16_mono_8k_to_16k(const uint8_t *src,
                                                        size_t src_len,
                                                        uint8_t *dst,
                                                        size_t dst_cap)
{
    const int16_t *src_samples = (const int16_t *)src;
    int16_t *dst_samples = (int16_t *)dst;
    size_t src_count;
    size_t dst_count;
    size_t i;

    if (!src || !dst || src_len < sizeof(int16_t) ||
        (SIMPLE_AUDIO_SAMPLE_RATE != SIMPLE_WEBRTC_BRIDGE_SAMPLE_RATE * 2))
    {
        return 0;
    }

    src_count = src_len / sizeof(int16_t);
    dst_count = src_count * 2;
    if (dst_cap < dst_count * sizeof(int16_t))
    {
        return 0;
    }

    for (i = 0; i < src_count; ++i)
    {
        int16_t current = src_samples[i];
        int16_t next = (i + 1 < src_count) ? src_samples[i + 1] : current;
        dst_samples[i * 2] = current;
        dst_samples[i * 2 + 1] = (int16_t)(((int32_t)current + (int32_t)next) / 2);
    }
    return dst_count * sizeof(int16_t);
}

static void simple_indoor_webrtc_remote_audio_sink(const unsigned char *data,
                                                   size_t len,
                                                   void *user)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)user;
    uint8_t pcm_16k[SIMPLE_AUDIO_BLOCK_SIZE * 2];
    size_t pcm_16k_len;

    if (!indoor || !data || len == 0 ||
        __atomic_load_n(&indoor->media_bridge_enabled, __ATOMIC_RELAXED) == 0)
    {
        return;
    }
    pcm_16k_len = simple_indoor_resample_s16_mono_8k_to_16k(
        data,
        len,
        pcm_16k,
        sizeof(pcm_16k)
    );
    if (pcm_16k_len == 0)
    {
        SIMPLE_LOGE("media bridge resample webrtc audio 8k->16k failed len=%zu\n", len);
        return;
    }
    if (!indoor->media_bridge_first_audio_to_d2d_logged)
    {
        indoor->media_bridge_first_audio_to_d2d_logged = true;
        SIMPLE_LOGI("media bridge first webrtc audio to d2d len=%zu resampled=%zu\n",
                    len,
                    pcm_16k_len);
    }
    simple_indoor_send_pcm_to_d2d(indoor, pcm_16k, pcm_16k_len, "webrtc");
}

static int simple_indoor_capture_cb(char *data_buffer, int data_size, void *ctx)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;

    if (!indoor || !data_buffer || data_size <= 0)
    {
        return 0;
    }
    if (__atomic_load_n(&indoor->media_bridge_enabled, __ATOMIC_RELAXED) != 0)
    {
        return data_size;
    }
    if (!simple_media_runtime_audio_tx_ready(&indoor->media_runtime) ||
        !indoor->opus_encoder)
    {
        return data_size;
    }

    circlebuf_push_back(&indoor->capture_buf, data_buffer, (size_t)data_size);
    while (indoor->capture_buf.size >= SIMPLE_AUDIO_BLOCK_SIZE)
    {
        uint8_t pcm[SIMPLE_AUDIO_BLOCK_SIZE];

        circlebuf_pop_front(&indoor->capture_buf, pcm, sizeof(pcm));
        simple_indoor_send_pcm_to_d2d(indoor, pcm, sizeof(pcm), "capture");
    }

    return data_size;
}

static int simple_indoor_open_playback(simple_indoor_codec_ctx_t *indoor)
{
    int stream_id;
    opus_codec_config_t opus_cfg;
    bool bridge_mode;

    if (!indoor)
    {
        return -1;
    }
    bridge_mode = __atomic_load_n(&indoor->media_bridge_enabled, __ATOMIC_RELAXED) != 0;
    if (indoor->opus_decoder && (bridge_mode || indoor->pb_stream != 0))
    {
        return 0;
    }

    stream_id = 0;
    if (!bridge_mode)
    {
        stream_id = Audio_Player_Open();
        if (stream_id == 0)
        {
            SIMPLE_LOGE("indoor audio playback open failed bridge=%d\n", bridge_mode ? 1 : 0);
            return -1;
        }
        if (!Audio_Player_SetDevice(stream_id, "default") ||
            !Audio_Player_SetFormat(stream_id,
                                    SIMPLE_AUDIO_CHANNELS,
                                    SIMPLE_AUDIO_SAMPLE_BITS,
                                    SIMPLE_AUDIO_SAMPLE_RATE) ||
            !Audio_Player_SetCache(stream_id,
                                   SIMPLE_AUDIO_BLOCK_COUNT,
                                   SIMPLE_AUDIO_SAMPLE_RATE * SIMPLE_AUDIO_BLOCK_TIME / 1000) ||
            !Audio_Player_SetMode(stream_id, false, simple_indoor_playback_cb, indoor) ||
            !Audio_Player_SetVolume(stream_id, SIMPLE_PLAYBACK_VOLUME) ||
            !Audio_Player_Start(stream_id))
        {
            bool close_ok = Audio_Player_Close(stream_id);
            SIMPLE_LOGE("indoor audio playback setup failed stream=%d close_ok=%d\n",
                        stream_id,
                        close_ok ? 1 : 0);
            return -1;
        }
    }

    if (!indoor->opus_decoder)
    {
        memset(&opus_cfg, 0, sizeof(opus_cfg));
        opus_cfg.sample_rate = SIMPLE_AUDIO_SAMPLE_RATE;
        opus_cfg.channel_cnt = SIMPLE_AUDIO_CHANNELS;
        opus_cfg.frm_ptime = SIMPLE_AUDIO_BLOCK_TIME;
        opus_cfg.bit_rate = 24000;
        opus_cfg.packet_loss = 5;
        opus_cfg.plc = 1;
        indoor->opus_decoder = libopus_decoder_create(&opus_cfg);
        if (!indoor->opus_decoder)
        {
            if (stream_id != 0)
            {
                Audio_Player_Stop(stream_id);
                Audio_Player_Close(stream_id);
            }
            return -1;
        }
    }

    if (bridge_mode)
    {
        SIMPLE_LOGI("indoor audio playback bridge mode decoder only\n");
        return 0;
    }

    indoor->pb_stream = stream_id;
    simple_indoor_enable_aec_if_ready(indoor);
    SIMPLE_LOGI("indoor audio playback start\n");
    return 0;
}

static void simple_indoor_close_playback(simple_indoor_codec_ctx_t *indoor)
{
    if (!indoor)
    {
        return;
    }

    simple_indoor_disable_aec_if_enabled(indoor);
    if (indoor->pb_stream != 0)
    {
        int stream_id = indoor->pb_stream;
        bool stop_ok = Audio_Player_Stop(stream_id);
        bool close_ok = Audio_Player_Close(stream_id);
        SIMPLE_LOGI("indoor audio playback closed stream=%d stop_ok=%d close_ok=%d\n",
                    stream_id,
                    stop_ok ? 1 : 0,
                    close_ok ? 1 : 0);
        indoor->pb_stream = 0;
    }
    if (indoor->opus_decoder)
    {
        libopus_decoder_destroy(indoor->opus_decoder);
        indoor->opus_decoder = NULL;
    }
    pthread_mutex_lock(&indoor->pb_mutex);
    circlebuf_free(&indoor->pb_packet);
    circlebuf_init(&indoor->pb_packet);
    pthread_mutex_unlock(&indoor->pb_mutex);
    SIMPLE_LOGI("indoor audio playback stop\n");
}

static int simple_indoor_open_capture(simple_indoor_codec_ctx_t *indoor)
{
    int stream_id;
    opus_codec_config_t opus_cfg;
    bool bridge_mode;

    if (!indoor)
    {
        return -1;
    }
    bridge_mode = __atomic_load_n(&indoor->media_bridge_enabled, __ATOMIC_RELAXED) != 0;
    if (indoor->opus_encoder && (bridge_mode || indoor->ca_stream != 0))
    {
        return 0;
    }

    stream_id = 0;
    if (!bridge_mode)
    {
        stream_id = Audio_Recorder_Open();
        if (stream_id == 0)
        {
            return -1;
        }
        if (!Audio_Recorder_SetDevice(stream_id, "default") ||
            !Audio_Recorder_SetFormat(stream_id,
                                      SIMPLE_AUDIO_CHANNELS,
                                      SIMPLE_AUDIO_SAMPLE_BITS,
                                      SIMPLE_AUDIO_SAMPLE_RATE) ||
            !Audio_Recorder_SetCache(stream_id,
                                     SIMPLE_AUDIO_BLOCK_COUNT,
                                     SIMPLE_AUDIO_SAMPLE_RATE * SIMPLE_AUDIO_BLOCK_TIME / 1000) ||
            !Audio_Recorder_SetMode(stream_id, false, simple_indoor_capture_cb, indoor) ||
            !Audio_Recorder_SetVolume(stream_id, SIMPLE_AUDIO_VOLUME) ||
            !Audio_Recorder_Start(stream_id))
        {
            Audio_Recorder_Close(stream_id);
            return -1;
        }
    }

    if (!indoor->opus_encoder)
    {
        memset(&opus_cfg, 0, sizeof(opus_cfg));
        opus_cfg.sample_rate = SIMPLE_AUDIO_SAMPLE_RATE;
        opus_cfg.channel_cnt = SIMPLE_AUDIO_CHANNELS;
        opus_cfg.frm_ptime = SIMPLE_AUDIO_BLOCK_TIME;
        opus_cfg.bit_rate = 24000;
        opus_cfg.plc = 1;
        opus_cfg.complexity = 5;
        opus_cfg.packet_loss = 5;
        indoor->opus_encoder = libopus_encoder_create(&opus_cfg);
        if (!indoor->opus_encoder)
        {
            if (stream_id != 0)
            {
                Audio_Recorder_Stop(stream_id);
                Audio_Recorder_Close(stream_id);
            }
            return -1;
        }
    }

    indoor->audio_next_rtp_ts = 0;
    indoor->audio_first_tx_logged = false;
    if (bridge_mode)
    {
        SIMPLE_LOGI("indoor audio capture bridge mode encoder only\n");
        return 0;
    }

    indoor->ca_stream = stream_id;
    simple_indoor_enable_aec_if_ready(indoor);
    SIMPLE_LOGI("indoor audio capture start\n");
    return 0;
}

static void simple_indoor_close_capture(simple_indoor_codec_ctx_t *indoor)
{
    if (!indoor)
    {
        return;
    }

    simple_indoor_disable_aec_if_enabled(indoor);
    if (indoor->ca_stream != 0)
    {
        Audio_Recorder_Stop(indoor->ca_stream);
        Audio_Recorder_Close(indoor->ca_stream);
        indoor->ca_stream = 0;
    }
    if (indoor->opus_encoder)
    {
        libopus_encoder_destroy(indoor->opus_encoder);
        indoor->opus_encoder = NULL;
    }
    circlebuf_free(&indoor->capture_buf);
    circlebuf_init(&indoor->capture_buf);
    SIMPLE_LOGI("indoor audio capture stop\n");
}

static int simple_indoor_start_audio_encoder(void *ctx)
{
    return simple_indoor_open_capture((simple_indoor_codec_ctx_t *)ctx);
}

static int simple_indoor_query_audio(simple_indoor_codec_ctx_t *indoor,
                                     const simple_media_endpoint_request_t *request,
                                     topology_audio_endpoint_result_t *audio,
                                     topology_endpoint_t *local_endpoint)
{
    uint32_t indoor_plc_addr;
    uint32_t peer_plc_addr;
    const char *peer_device_id;
    const char *domain_id;
    bool bridge_required;

    if (!indoor || !request || !audio || !local_endpoint)
    {
        return -EINVAL;
    }

    if (simple_indoor_request_uses_secondary_confirm(indoor, request))
    {
        if (indoor->local_udp_addr == 0 || indoor->fallback_peer_udp_addr == 0)
        {
            return -ENOENT;
        }

        memset(audio, 0, sizeof(*audio));
        snprintf(audio->tx_endpoint.device_id, sizeof(audio->tx_endpoint.device_id),
                 "%s", SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID);
        snprintf(audio->tx_endpoint.domain_id, sizeof(audio->tx_endpoint.domain_id),
                 "%s", "spe-udp");
        audio->tx_endpoint.link_kind = TOPOLOGY_LINK_LAN;
        audio->tx_endpoint.addr = indoor->fallback_peer_udp_addr;
        audio->tx_endpoint.port = SIMPLE_UDP_MEDIA_PORT;
        audio->rx_match_endpoint = audio->tx_endpoint;
        audio->bridge_required = false;
        audio->hop_count = 1U;
        snprintf(audio->explain, sizeof(audio->explain),
                 "%s", "simple_indoor fallback secondary confirm spe udp audio endpoint");

        memset(local_endpoint, 0, sizeof(*local_endpoint));
        snprintf(local_endpoint->device_id, sizeof(local_endpoint->device_id),
                 "%s", request->self_device_id);
        snprintf(local_endpoint->domain_id, sizeof(local_endpoint->domain_id),
                 "%s", "spe-udp");
        local_endpoint->link_kind = TOPOLOGY_LINK_LAN;
        local_endpoint->addr = indoor->local_udp_addr;
        local_endpoint->port = SIMPLE_UDP_MEDIA_PORT;
        return 0;
    }

    if (!indoor->topology_audio || !indoor->topology_audio->enabled)
    {
        peer_plc_addr = indoor->fallback_peer_plc_addr;
        peer_device_id = indoor->fallback_peer_device_id[0] ?
                         indoor->fallback_peer_device_id :
                         SIMPLE_DEFAULT_CONVERTER_DEVICE_ID;
        if (simple_plc_addr_from_device_id(request->self_device_id, &indoor_plc_addr) != 0 ||
            peer_plc_addr == 0)
        {
            return -ENOENT;
        }
        domain_id =
            (indoor->primary_path == SIMPLE_INDOOR_PRIMARY_VIA_UNIT_DOOR_PLC) ?
            "unit-door-plc" : "converter-2-plc";
        bridge_required =
            indoor->primary_path == SIMPLE_INDOOR_PRIMARY_VIA_CONVERTER;

        memset(audio, 0, sizeof(*audio));
        snprintf(audio->tx_endpoint.device_id, sizeof(audio->tx_endpoint.device_id),
                 "%s", peer_device_id);
        snprintf(audio->tx_endpoint.domain_id, sizeof(audio->tx_endpoint.domain_id),
                 "%s", domain_id);
        audio->tx_endpoint.link_kind = TOPOLOGY_LINK_PLC;
        audio->tx_endpoint.addr = peer_plc_addr;
        audio->tx_endpoint.port = SIMPLE_PLC_MEDIA_PORT;
        audio->rx_match_endpoint = audio->tx_endpoint;
        snprintf(audio->rx_match_endpoint.device_id,
                 sizeof(audio->rx_match_endpoint.device_id),
                 "%s", peer_device_id);
        snprintf(audio->gateway_device_id, sizeof(audio->gateway_device_id),
                 "%s", bridge_required ? peer_device_id : "");
        audio->bridge_required = bridge_required;
        audio->hop_count = bridge_required ? 2U : 1U;
        snprintf(audio->explain, sizeof(audio->explain),
                 "%s", bridge_required ?
                 "simple_indoor fallback converter plc audio endpoint" :
                 "simple_indoor fallback unit door plc audio endpoint");

        memset(local_endpoint, 0, sizeof(*local_endpoint));
        snprintf(local_endpoint->device_id, sizeof(local_endpoint->device_id),
                 "%s", request->self_device_id);
        snprintf(local_endpoint->domain_id, sizeof(local_endpoint->domain_id),
                 "%s", domain_id);
        local_endpoint->link_kind = TOPOLOGY_LINK_PLC;
        local_endpoint->addr = indoor_plc_addr;
        local_endpoint->port = SIMPLE_PLC_MEDIA_PORT;
        return 0;
    }

    return simple_topology_audio_resolve_binding(indoor->topology_audio,
                                                 request,
                                                 TOPOLOGY_LINK_PLC,
                                                 SIMPLE_PLC_MEDIA_PORT,
                                                 audio,
                                                 local_endpoint);
}

static int simple_indoor_endpoint_audio_resolver(
    void *user_ctx,
    const simple_media_endpoint_request_t *request,
    topology_audio_endpoint_result_t *audio_out,
    topology_endpoint_t *local_endpoint_out)
{
    return simple_indoor_query_audio((simple_indoor_codec_ctx_t *)user_ctx,
                                     request,
                                     audio_out,
                                     local_endpoint_out);
}

static int simple_indoor_endpoint_provider(void *user_ctx,
                                           const simple_media_endpoint_request_t *request,
                                           simple_media_endpoint_result_t *result)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)user_ctx;
    simple_endpoint_provider_config_t cfg;
    simple_media_runtime_endpoint_snapshot_t snapshot;
    int video_desired_active;

    if (!indoor)
    {
        return -EINVAL;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.role = SIMPLE_ENDPOINT_PROVIDER_ROLE_INDOOR;
    cfg.node_name = "simple_indoor";
    cfg.local_media_addr = indoor->local_udp_addr;
    cfg.static_udp_enabled = simple_indoor_request_uses_secondary_confirm(indoor, request);
    cfg.static_udp_domain_id = "spe-udp";
    cfg.video_enabled = true;
    cfg.audio_enabled = true;
    video_desired_active =
        __atomic_load_n(&indoor->video_desired_active, __ATOMIC_RELAXED) != 0;
    cfg.video_desired_active = video_desired_active != 0;
    if (video_desired_active &&
        simple_media_runtime_get_endpoint_snapshot(
            &indoor->media_runtime,
            SIMPLE_MEDIA_RUNTIME_ENDPOINT_SNAPSHOT_CURRENT,
            &snapshot) &&
        snapshot.type == SIMPLE_MEDIA_ENDPOINT_PLC_RX)
    {
        cfg.existing_plc_video_endpoint_enabled = true;
        cfg.existing_plc_video_addr = snapshot.plc_remote_addr;
        cfg.existing_plc_video_port = snapshot.plc_remote_port;
    }
    cfg.audio_resolver = simple_indoor_endpoint_audio_resolver;
    cfg.audio_resolver_ctx = indoor;
    return simple_endpoint_provider_resolve(&indoor->endpoint_provider,
                                            &cfg,
                                            request,
                                            result);
}

static void simple_indoor_endpoint_release(void *user_ctx,
                                           uint64_t session_id,
                                           const char *reason)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)user_ctx;
    kr_indoor_handle_t *handle = indoor ? (kr_indoor_handle_t *)indoor->event_ctx : NULL;

    if (!indoor || session_id == 0)
    {
        return;
    }
    simple_endpoint_provider_release_session(&indoor->endpoint_provider,
                                             session_id);
    SIMPLE_LOGI("indoor endpoint released: session=0x%" PRIx64 " reason=%s\n",
                session_id, reason ? reason : "-");
    if (reason && strcmp(reason, "CALL_INVITE_PREEMPT_MONITOR") == 0)
    {
        simple_indoor_request_monitor_local_media_stop(indoor);
    }
    if (handle && reason &&
        (strcmp(reason, "session_error") == 0 ||
         strcmp(reason, "force_idle") == 0))
    {
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_REMOTE_HANGUP,
                                    SIMPLE_SESSION_IDLE,
                                    0,
                                    false,
                                    false,
                                    NULL);
    }
}

static void simple_indoor_stop_audio_encoder(void *ctx)
{
    simple_indoor_close_capture((simple_indoor_codec_ctx_t *)ctx);
}

static int simple_indoor_request_video_start(void *ctx)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;

    if (!indoor)
    {
        return -1;
    }

    __atomic_store_n(&indoor->video_desired_active, 1, __ATOMIC_RELAXED);
    return 0;
}

static void simple_indoor_request_video_stop(void *ctx)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;

    if (!indoor)
    {
        return;
    }

    __atomic_store_n(&indoor->video_desired_active, 0, __ATOMIC_RELAXED);
}

static int simple_indoor_request_audio_start(void *ctx)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;

    if (!indoor)
    {
        return -1;
    }

    __atomic_store_n(&indoor->audio_desired_active, 1, __ATOMIC_RELAXED);
    return 0;
}

static void simple_indoor_request_audio_stop(void *ctx)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;

    if (!indoor)
    {
        return;
    }

    __atomic_store_n(&indoor->audio_desired_active, 0, __ATOMIC_RELAXED);
}

static void simple_indoor_drive_media(simple_indoor_codec_ctx_t *indoor)
{
    int video_desired;
    int audio_desired;

    if (!indoor)
    {
        return;
    }

    video_desired = __atomic_load_n(&indoor->video_desired_active, __ATOMIC_RELAXED);
    audio_desired = __atomic_load_n(&indoor->audio_desired_active, __ATOMIC_RELAXED);

    if (video_desired && !indoor->video_active)
    {
        if (simple_media_runtime_start_video(&indoor->media_runtime) == 0)
        {
            indoor->video_active = 1;
        }
    }
    else if (!video_desired && indoor->video_active)
    {
        simple_media_runtime_stop_video(&indoor->media_runtime);
        indoor->video_active = 0;
    }

    if (audio_desired && !indoor->audio_active)
    {
        if (simple_media_runtime_start_audio(&indoor->media_runtime) == 0)
        {
            indoor->audio_active = 1;
        }
    }
    else if (!audio_desired && indoor->audio_active)
    {
        simple_media_runtime_stop_audio(&indoor->media_runtime);
        indoor->audio_active = 0;
    }
}

static void simple_indoor_request_monitor_local_media_stop(simple_indoor_codec_ctx_t *indoor)
{
    if (!indoor)
    {
        return;
    }

    __atomic_store_n(&indoor->video_desired_active, 0, __ATOMIC_RELAXED);
    __atomic_store_n(&indoor->audio_desired_active, 0, __ATOMIC_RELAXED);

    SIMPLE_LOGI("monitor local media stop requested: video_active=%d audio_active=%d\n",
                indoor->video_active,
                indoor->audio_active);
}

static int simple_indoor_start_audio_decoder(void *ctx)
{
    return simple_indoor_open_playback((simple_indoor_codec_ctx_t *)ctx);
}

static void simple_indoor_stop_audio_decoder(void *ctx)
{
    simple_indoor_close_playback((simple_indoor_codec_ctx_t *)ctx);
}

static int simple_indoor_input_audio_frame(void *ctx,
                                           const uint8_t *frame,
                                           size_t len,
                                           uint32_t ts,
                                           int flags)
{
    simple_indoor_codec_ctx_t *indoor = (simple_indoor_codec_ctx_t *)ctx;
    uint8_t pcm[SIMPLE_AUDIO_BLOCK_SIZE * 8];
    size_t pcm_len = sizeof(pcm);

    (void)ts;
    (void)flags;

    if (!indoor || !indoor->opus_decoder || !frame || len == 0)
    {
        return 0;
    }

    pthread_mutex_lock(&indoor->dec_mutex);
    if (libopus_decoder_decode(indoor->opus_decoder, frame, len, pcm, &pcm_len) == 0 &&
        pcm_len > 0)
    {
        if (__atomic_load_n(&indoor->media_bridge_enabled, __ATOMIC_RELAXED) != 0 &&
            indoor->webrtc_bridge != NULL)
        {
            uint8_t pcm_8k[SIMPLE_AUDIO_BLOCK_SIZE];
            size_t pcm_8k_len = simple_indoor_resample_s16_mono_16k_to_8k(
                pcm,
                pcm_len,
                pcm_8k,
                sizeof(pcm_8k)
            );
            int ret;
            if (pcm_8k_len == 0)
            {
                SIMPLE_LOGE("media bridge resample d2d audio 16k->8k failed len=%zu\n",
                            pcm_len);
                pthread_mutex_unlock(&indoor->dec_mutex);
                return 0;
            }
            ret = kr_webrtc_send_audio_frame(indoor->webrtc_bridge, pcm_8k, pcm_8k_len);
            if (ret < 0)
            {
                SIMPLE_LOGE("media bridge send audio to webrtc failed ret=%d len=%zu\n",
                            ret,
                            pcm_8k_len);
            }
            else if (!indoor->media_bridge_first_audio_to_cloud_logged)
            {
                indoor->media_bridge_first_audio_to_cloud_logged = true;
                SIMPLE_LOGI("media bridge first d2d audio to webrtc len=%zu resampled=%zu\n",
                             pcm_len,
                             pcm_8k_len);
            }
            pthread_mutex_unlock(&indoor->dec_mutex);
            return 0;
        }
        pthread_mutex_lock(&indoor->pb_mutex);
        circlebuf_push_back(&indoor->pb_packet, pcm, pcm_len);
        pthread_mutex_unlock(&indoor->pb_mutex);
    }
    pthread_mutex_unlock(&indoor->dec_mutex);
    return 0;
}

static void simple_indoor_start_config_init(simple_indoor_start_config_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
    cfg->target_device_id = SIMPLE_DEFAULT_DOOR_DEVICE_ID;
    cfg->d2d_room_id = "101";
    cfg->d2d_peer_id = SIMPLE_DEFAULT_DOOR_DEVICE_ID;
    cfg->registry_peer_device_id = SIMPLE_DEFAULT_CONVERTER_DEVICE_ID;
    cfg->spe_if = "eth0";
    cfg->direct_plc_cco_addr = 0x00020000U;
    cfg->link_plan.primary_path = SIMPLE_INDOOR_PRIMARY_VIA_CONVERTER;
    cfg->link_plan.secondary_policy = SIMPLE_INDOOR_SECONDARY_OFF;
}

static int simple_indoor_finalize_start_config(simple_indoor_start_config_t *cfg)
{
    if (simple_plc_addr_from_device_id(SIMPLE_INDOOR_DEVICE_ID,
                                       &cfg->indoor_plc_addr) != 0 ||
        simple_plc_addr_from_device_id(SIMPLE_DEFAULT_CONVERTER_DEVICE_ID,
                                       &cfg->converter_plc_addr) != 0)
    {
        SIMPLE_LOGE("derive PLC addr failed\n");
        return -EINVAL;
    }

    cfg->d2d_peer_plc_addr =
        simple_indoor_primary_uses_unit_door_plc(&cfg->link_plan) ?
        cfg->direct_plc_cco_addr : cfg->converter_plc_addr;
    cfg->registry_peer_device_id =
        simple_indoor_primary_uses_unit_door_plc(&cfg->link_plan) ?
        SIMPLE_DEFAULT_DOOR_DEVICE_ID : SIMPLE_DEFAULT_CONVERTER_DEVICE_ID;

    if (simple_indoor_secondary_requested(&cfg->link_plan))
    {
        if (cfg->spe_peer_ip_arg &&
            !simple_ipv4_parse_host_order(cfg->spe_peer_ip_arg, &cfg->spe_peer_ip))
        {
            SIMPLE_LOGE("invalid --secondary-confirm-ip: %s\n", cfg->spe_peer_ip_arg);
            return -EINVAL;
        }
        if (cfg->spe_local_ip_arg)
        {
            if (!simple_ipv4_parse_host_order(cfg->spe_local_ip_arg, &cfg->spe_local_ip))
            {
                if (simple_indoor_secondary_required(&cfg->link_plan))
                {
                    SIMPLE_LOGE("invalid --spe-local-ip: %s\n", cfg->spe_local_ip_arg);
                    return -EINVAL;
                }
                SIMPLE_LOGW("optional secondary disabled: invalid --spe-local-ip=%s\n",
                            cfg->spe_local_ip_arg);
                cfg->link_plan.secondary_policy = SIMPLE_INDOOR_SECONDARY_OFF;
            }
        }
        else if (!simple_ipv4_get_if_host_order(cfg->spe_if, &cfg->spe_local_ip))
        {
            if (simple_indoor_secondary_required(&cfg->link_plan))
            {
                SIMPLE_LOGE("resolve SPE ip on %s failed; pass --spe-local-ip\n",
                            cfg->spe_if);
                return -EINVAL;
            }
            SIMPLE_LOGW("optional secondary disabled: resolve SPE ip on %s failed\n",
                        cfg->spe_if);
            cfg->link_plan.secondary_policy = SIMPLE_INDOOR_SECONDARY_OFF;
        }
        if (simple_indoor_secondary_requested(&cfg->link_plan) &&
            !cfg->target_device_id_set)
        {
            cfg->target_device_id = SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID;
        }
        if (simple_indoor_secondary_required(&cfg->link_plan))
        {
            cfg->d2d_peer_id = SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID;
            cfg->registry_peer_device_id = SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID;
        }
    }
    return 0;
}

static void simple_indoor_runtime_init(simple_indoor_runtime_ctx_t *rt)
{
    memset(rt, 0, sizeof(*rt));
    rt->codec_ctx.topology_audio = &rt->topology_audio;
    rt->codec_ctx.video.codec_data.decoder_chan = -1;
    simple_endpoint_provider_init(&rt->codec_ctx.endpoint_provider);
    resize_buf_init(&rt->codec_ctx.media_bridge_h264_sps, 128);
    resize_buf_init(&rt->codec_ctx.media_bridge_h264_pps, 64);
    resize_buf_init(&rt->codec_ctx.media_bridge_h264_send_buf, 128 * 1024);
    pthread_mutex_init(&rt->codec_ctx.pb_mutex, NULL);
    pthread_mutex_init(&rt->codec_ctx.dec_mutex, NULL);
    circlebuf_init(&rt->codec_ctx.pb_packet);
    circlebuf_init(&rt->codec_ctx.capture_buf);
    rt->codec_ctx_inited = true;
}

static void simple_indoor_configure_codec_context(simple_indoor_runtime_ctx_t *rt,
                                                  const simple_indoor_start_config_t *cfg)
{
    simple_indoor_codec_ctx_t *codec_ctx = &rt->codec_ctx;

    codec_ctx->primary_path = cfg->link_plan.primary_path;
    codec_ctx->secondary_active = cfg->link_plan.secondary_active;
    codec_ctx->call_app = rt->app;
    codec_ctx->fallback_peer_plc_addr = cfg->d2d_peer_plc_addr;
    codec_ctx->fallback_peer_udp_addr = cfg->spe_peer_ip;
    codec_ctx->local_udp_addr = cfg->spe_local_ip;
    codec_ctx->d2d_business = &rt->d2d_business;
    snprintf(codec_ctx->fallback_peer_device_id,
             sizeof(codec_ctx->fallback_peer_device_id),
             "%s",
             cfg->registry_peer_device_id);
}

static void simple_indoor_runtime_cleanup(simple_indoor_runtime_ctx_t *rt)
{
    if (rt->call_app_started && rt->app)
    {
        simple_call_hangup(rt->app);
    }
    if (rt->villa_heartbeat_started)
    {
        simple_d2d_villa_heartbeat_stop(&rt->villa_heartbeat);
        rt->villa_heartbeat_started = false;
    }
    if (rt->d2d_business_started)
    {
        simple_d2d_business_runtime_stop(&rt->d2d_business);
        rt->d2d_business_started = false;
    }
    if (rt->app)
    {
        simple_call_app_deinit(rt->app);
        rt->app = NULL;
    }
    if (rt->media_open)
    {
        simple_media_runtime_close(&rt->codec_ctx.media_runtime);
        rt->media_open = false;
    }
    if (rt->topology_audio_inited)
    {
        simple_topology_audio_deinit(&rt->topology_audio);
        rt->topology_audio_inited = false;
    }
    if (rt->codec_ctx_inited)
    {
        simple_indoor_close_capture(&rt->codec_ctx);
        simple_indoor_close_playback(&rt->codec_ctx);
        pthread_mutex_lock(&rt->codec_ctx.dec_mutex);
        simple_indoor_bridge_h264_dump_close_locked(&rt->codec_ctx);
        pthread_mutex_unlock(&rt->codec_ctx.dec_mutex);
        resize_buf_free(&rt->codec_ctx.media_bridge_h264_sps);
        resize_buf_free(&rt->codec_ctx.media_bridge_h264_pps);
        resize_buf_free(&rt->codec_ctx.media_bridge_h264_send_buf);
        pthread_mutex_destroy(&rt->codec_ctx.pb_mutex);
        pthread_mutex_destroy(&rt->codec_ctx.dec_mutex);
        rt->codec_ctx_inited = false;
    }
    if (rt->engine_inited)
    {
        mtrans_engine_deinit();
        rt->engine_inited = false;
    }
    if (rt->udp_registered)
    {
        transport_unregister(TRANSPORT_PROTO_UDP);
        rt->udp_registered = false;
    }
    if (rt->tcp_registered)
    {
        transport_unregister(TRANSPORT_PROTO_TCP);
        rt->tcp_registered = false;
    }
    if (rt->plc_registered)
    {
        transport_unregister(TRANSPORT_PROTO_PLC);
        rt->plc_registered = false;
    }
    if (rt->plc_inited)
    {
        goblin_plc_trans_deinit();
        rt->plc_inited = false;
    }
}

static int simple_indoor_start_transports(simple_indoor_runtime_ctx_t *rt,
                                          simple_indoor_start_config_t *cfg)
{
    goblin_plc_initinfo_t plc_info = {0};
    mtrans_engine_config_t engine_cfg;

    plc_info.plcdevid = cfg->indoor_plc_addr;
    goblin_plc_trans_init(&plc_info);
    rt->plc_inited = true;

    memset(&engine_cfg, 0, sizeof(engine_cfg));
    engine_cfg.log_level = MTRANS_INFO;
    engine_cfg.log_handler = simple_example_mtrans_log_handler;
    engine_cfg.mem_hooks = simple_example_mtrans_mem_hooks();
    if (mtrans_engine_init(&engine_cfg) != 0)
    {
        SIMPLE_LOGE("mtrans_engine_init failed\n");
        return -EIO;
    }
    rt->engine_inited = true;
    transport_mem_register_hooks(simple_example_transport_mem_hooks());
    transport_log_set_handler(simple_example_transport_log_handler);

    if (transport_register(TRANSPORT_PROTO_PLC, &g_plc_transport_ops) != 0)
    {
        SIMPLE_LOGE("transport_register plc failed\n");
        return -EIO;
    }
    rt->plc_registered = true;

    if (simple_indoor_secondary_requested(&cfg->link_plan))
    {
        if (transport_register(TRANSPORT_PROTO_UDP, &g_udp_transport_ops) != 0)
        {
            if (simple_indoor_secondary_required(&cfg->link_plan))
            {
                SIMPLE_LOGE("transport_register udp failed\n");
                return -EIO;
            }
            SIMPLE_LOGW("optional secondary disabled: transport_register udp failed\n");
            cfg->link_plan.secondary_policy = SIMPLE_INDOOR_SECONDARY_OFF;
        }
        else
        {
            rt->udp_registered = true;
            if (transport_register(TRANSPORT_PROTO_TCP, &g_tcp_transport_ops) != 0)
            {
                if (simple_indoor_secondary_required(&cfg->link_plan))
                {
                    SIMPLE_LOGE("transport_register tcp failed\n");
                    return -EIO;
                }
                SIMPLE_LOGW("optional secondary disabled: transport_register tcp failed\n");
                transport_unregister(TRANSPORT_PROTO_UDP);
                rt->udp_registered = false;
                cfg->link_plan.secondary_policy = SIMPLE_INDOOR_SECONDARY_OFF;
            }
            else
            {
                rt->tcp_registered = true;
            }
        }
    }

    cfg->link_plan.secondary_active = simple_indoor_secondary_requested(&cfg->link_plan);
    rt->codec_ctx.secondary_active = cfg->link_plan.secondary_active;
    return 0;
}

static int simple_indoor_start_media(simple_indoor_runtime_ctx_t *rt,
                                     const simple_indoor_start_config_t *cfg)
{
    simple_media_config_t media_cfg;
    simple_media_runtime_config_t media_runtime_cfg;
    int ret;

    ret = simple_topology_audio_init(&rt->topology_audio,
                                     "simple_indoor",
                                     SIMPLE_INDOOR_DEVICE_ID);

    if (ret < 0)
    {
        SIMPLE_LOGE("simple_topology_audio_init failed: %d\n", ret);
        return ret;
    }
    rt->topology_audio_inited = true;

    simple_media_default_config(&media_cfg,
                                simple_indoor_primary_uses_plc(&cfg->link_plan) ?
                                SIMPLE_MEDIA_ENDPOINT_PLC_RX :
                                SIMPLE_MEDIA_ENDPOINT_UDP_RX);
    if (simple_indoor_primary_uses_plc(&cfg->link_plan))
    {
        media_cfg.plc_local_addr = cfg->indoor_plc_addr;
        media_cfg.audio_tx_remote_ep_enabled = true;
        media_cfg.audio_tx_remote_addr = cfg->d2d_peer_plc_addr;
        media_cfg.audio_tx_remote_port = SIMPLE_PLC_MEDIA_PORT;
    }
    if (cfg->link_plan.secondary_active)
    {
        media_cfg.udp_interface = cfg->spe_if;
        media_cfg.udp_local_addr = cfg->spe_local_ip;
        media_cfg.udp_remote_addr = cfg->spe_peer_ip;
        media_cfg.udp_remote_port = SIMPLE_UDP_MEDIA_PORT;
        if (!simple_indoor_primary_uses_plc(&cfg->link_plan))
        {
            media_cfg.audio_tx_remote_ep_enabled = true;
            media_cfg.audio_tx_remote_addr = cfg->spe_peer_ip;
            media_cfg.audio_tx_remote_port = SIMPLE_UDP_MEDIA_PORT;
        }
    }
    media_cfg.video_ssrc = 0;
    media_cfg.video_rx_ssrc = 0;
    media_cfg.audio_ssrc = 0;
    media_cfg.audio_rx_ssrc = 0;
    media_cfg.video_tx_enabled = false;
    media_cfg.video_rx_enabled = true;
    media_cfg.audio_tx_enabled = true;
    media_cfg.audio_rx_enabled = true;

    rt->codec_ops.start_video_decoder = simple_indoor_start_video_decoder;
    rt->codec_ops.stop_video_decoder = simple_indoor_stop_video_decoder;
    rt->codec_ops.input_video_frame = simple_indoor_input_video_frame;
    rt->codec_ops.on_stream_event = simple_indoor_on_stream_event;
    rt->codec_ops.start_audio_encoder = simple_indoor_start_audio_encoder;
    rt->codec_ops.stop_audio_encoder = simple_indoor_stop_audio_encoder;
    rt->codec_ops.start_audio_decoder = simple_indoor_start_audio_decoder;
    rt->codec_ops.stop_audio_decoder = simple_indoor_stop_audio_decoder;
    rt->codec_ops.input_audio_frame = simple_indoor_input_audio_frame;
    rt->codec_ops.user_ctx = &rt->codec_ctx;
    media_cfg.codec_ops = &rt->codec_ops;

    simple_media_runtime_config_init(&media_runtime_cfg);
    media_runtime_cfg.node_name = "simple_indoor";
    media_runtime_cfg.delegate_ctx = &rt->codec_ctx;
    media_runtime_cfg.start_video = simple_indoor_request_video_start;
    media_runtime_cfg.stop_video = simple_indoor_request_video_stop;
    media_runtime_cfg.start_audio = simple_indoor_request_audio_start;
    media_runtime_cfg.stop_audio = simple_indoor_request_audio_stop;
    ret = simple_media_runtime_open(&rt->codec_ctx.media_runtime,
                                    &media_runtime_cfg,
                                    &media_cfg);
    if (ret < 0 ||
        simple_media_runtime_fill_ops(&rt->codec_ctx.media_runtime, &rt->media_ops) < 0)
    {
        SIMPLE_LOGE("simple_media_runtime open failed: %d\n", ret);
        simple_media_runtime_close(&rt->codec_ctx.media_runtime);
        return ret < 0 ? ret : -EIO;
    }
    rt->media_open = true;
    return 0;
}

static int simple_indoor_start_call(simple_indoor_runtime_ctx_t *rt,
                                    const simple_indoor_start_config_t *cfg)
{
    simple_call_app_config_t app_cfg;
    int ret;

    memset(rt->links, 0, sizeof(rt->links));
    if (simple_indoor_primary_uses_plc(&cfg->link_plan))
    {
        rt->links[rt->link_count].link_kind = CALL_LINK_PLC;
        rt->links[rt->link_count].proto = TRANSPORT_PROTO_PLC;
        rt->links[rt->link_count].local_ep.addr = cfg->indoor_plc_addr;
        rt->links[rt->link_count].local_ep.port = SIMPLE_CALL_CTRL_PLC_PORT;
        rt->link_count++;
    }
    if (cfg->link_plan.secondary_active)
    {
        rt->links[rt->link_count].link_kind = CALL_LINK_UDP;
        rt->links[rt->link_count].proto = TRANSPORT_PROTO_UDP;
        rt->links[rt->link_count].local_ep.addr = ADDR_ANY;
        rt->links[rt->link_count].local_ep.port = SIMPLE_CALL_CTRL_UDP_PORT;
        rt->link_count++;
    }

    memset(&app_cfg, 0, sizeof(app_cfg));
    app_cfg.role = SIMPLE_DEVICE_INDOOR;
    app_cfg.self_device_id = SIMPLE_INDOOR_DEVICE_ID;
    app_cfg.disable_route_fallback =
        cfg->link_plan.primary_path == SIMPLE_INDOOR_PRIMARY_VIA_UNIT_DOOR_PLC ||
        simple_indoor_secondary_requested(&cfg->link_plan);
    app_cfg.links = rt->links;
    app_cfg.link_count = rt->link_count;
    app_cfg.media_ops = &rt->media_ops;
    app_cfg.endpoint_provider = simple_indoor_endpoint_provider;
    app_cfg.endpoint_provider_ctx = &rt->codec_ctx;
    app_cfg.endpoint_release = simple_indoor_endpoint_release;
    app_cfg.endpoint_release_ctx = &rt->codec_ctx;
    app_cfg.inbound_observer = kr_indoor_inbound_observer;
    app_cfg.external_call_busy = kr_indoor_external_call_busy;
    app_cfg.user_ctx = &rt->codec_ctx;

    ret = simple_call_app_init(&rt->app, &app_cfg);
    if (ret == 0 &&
        cfg->link_plan.primary_path == SIMPLE_INDOOR_PRIMARY_NONE &&
        cfg->link_plan.secondary_active &&
        cfg->spe_peer_ip != 0)
    {
        ret = simple_indoor_apply_bootstrap_route(rt->app,
                                                  &rt->route_count,
                                                  cfg->target_device_id,
                                                  CALL_LINK_UDP,
                                                  cfg->spe_peer_ip);
    }
    if (ret == 0 && simple_indoor_primary_uses_plc(&cfg->link_plan))
    {
        ret = simple_indoor_apply_bootstrap_route(rt->app,
                                                  &rt->route_count,
                                                  SIMPLE_DEFAULT_DOOR_DEVICE_ID,
                                                  CALL_LINK_PLC,
                                                  cfg->d2d_peer_plc_addr);
    }
    if (ret == 0 && simple_indoor_primary_uses_plc(&cfg->link_plan))
    {
        ret = simple_indoor_apply_bootstrap_route(rt->app,
                                                  &rt->route_count,
                                                  SIMPLE_DEFAULT_MANAGE_DEVICE_ID,
                                                  CALL_LINK_PLC,
                                                  cfg->d2d_peer_plc_addr);
    }
    if (ret == 0 && simple_indoor_primary_uses_plc(&cfg->link_plan) &&
        !(cfg->link_plan.secondary_active &&
          simple_device_id_equal(cfg->target_device_id,
                                 SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID)) &&
        !simple_device_id_equal(cfg->target_device_id, SIMPLE_DEFAULT_DOOR_DEVICE_ID) &&
        !simple_device_id_equal(cfg->target_device_id, SIMPLE_DEFAULT_MANAGE_DEVICE_ID))
    {
        ret = simple_indoor_apply_bootstrap_route(rt->app,
                                                  &rt->route_count,
                                                  cfg->target_device_id,
                                                  CALL_LINK_PLC,
                                                  cfg->d2d_peer_plc_addr);
    }
    if (ret == 0)
    {
        ret = simple_call_app_start(rt->app);
    }
    if (ret < 0)
    {
        SIMPLE_LOGE("simple_call_app start failed: %d\n", ret);
        return ret;
    }
    rt->call_app_started = true;
    rt->codec_ctx.call_app = rt->app;
    return 0;
}

static int simple_indoor_start_secondary_heartbeat(simple_indoor_runtime_ctx_t *rt,
                                                   simple_indoor_start_config_t *cfg)
{
    simple_d2d_villa_heartbeat_config_t heartbeat_cfg;
    int ret;

    if (!cfg->link_plan.secondary_active)
    {
        return 0;
    }

    simple_d2d_villa_heartbeat_config_init(&heartbeat_cfg);
    heartbeat_cfg.role = SIMPLE_D2D_VILLA_HEARTBEAT_ROLE_SERVER;
    heartbeat_cfg.eth.if_name = cfg->spe_if;
    heartbeat_cfg.eth.local_addr = cfg->spe_local_ip;
    heartbeat_cfg.eth.peer_addr = cfg->spe_peer_ip;
    heartbeat_cfg.identity.local_device_id = SIMPLE_INDOOR_DEVICE_ID;
    heartbeat_cfg.identity.peer_device_id = SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID;
    heartbeat_cfg.callbacks.observer.observer = simple_d2d_bus_debug_observer;
    heartbeat_cfg.callbacks.update_cb = simple_indoor_secondary_heartbeat_update;
    heartbeat_cfg.callbacks.user_ctx = &rt->codec_ctx;
    ret = simple_d2d_villa_heartbeat_start(&rt->villa_heartbeat, &heartbeat_cfg);
    if (ret != 0)
    {
        if (simple_indoor_secondary_required(&cfg->link_plan))
        {
            SIMPLE_LOGE("secondary confirm villa heartbeat server start failed: %d\n",
                        ret);
            return ret;
        }
        SIMPLE_LOGW("optional secondary disabled: villa heartbeat start failed: %d\n",
                    ret);
        rt->codec_ctx.secondary_active = false;
        cfg->link_plan.secondary_active = false;
        return 0;
    }
    rt->villa_heartbeat_started = true;
    return 0;
}

static int simple_indoor_apply_secondary_bootstrap(simple_indoor_runtime_ctx_t *rt,
                                                  const simple_indoor_start_config_t *cfg)
{
    int ret;

    if (!(cfg->link_plan.secondary_active &&
          simple_indoor_primary_uses_plc(&cfg->link_plan) &&
          cfg->spe_peer_ip != 0))
    {
        return 0;
    }

    ret = simple_indoor_apply_bootstrap_route(rt->app,
                                              &rt->route_count,
                                              SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID,
                                              CALL_LINK_UDP,
                                              cfg->spe_peer_ip);
    if (ret != 0)
    {
        SIMPLE_LOGE("secondary confirm bootstrap route failed: %d\n", ret);
    }
    return ret;
}

static int simple_indoor_start_business(simple_indoor_runtime_ctx_t *rt,
                                        const simple_indoor_start_config_t *cfg)
{
    simple_d2d_business_runtime_config_t business_cfg;
    int ret;

    simple_d2d_business_runtime_config_init(&business_cfg);
    business_cfg.role = SIMPLE_D2D_BUSINESS_ROLE_INDOOR_CLIENT;
    business_cfg.identity.local_device_id = SIMPLE_INDOOR_DEVICE_ID;
    business_cfg.primary.unlock_peer_device_id = cfg->d2d_peer_id;
    business_cfg.primary_unlock_enabled = simple_indoor_primary_uses_plc(&cfg->link_plan);
    business_cfg.secondary.enabled = cfg->link_plan.secondary_active;
    business_cfg.secondary.unlock_peer_device_id =
        SIMPLE_DEFAULT_SECONDARY_CONFIRM_DEVICE_ID;
    business_cfg.secondary.endpoint.peer_addr = cfg->spe_peer_ip;
    if (cfg->link_plan.secondary_active)
    {
        business_cfg.secondary.shared_service =
            simple_d2d_villa_heartbeat_bus_service(&rt->villa_heartbeat);
    }
    business_cfg.eth.if_name = cfg->spe_if;
    business_cfg.eth.local_addr = cfg->spe_local_ip;
    business_cfg.secondary.endpoint.if_name = cfg->spe_if;
    business_cfg.secondary.endpoint.local_addr = cfg->spe_local_ip;
    business_cfg.primary.registry_peer_device_id = cfg->registry_peer_device_id;
    business_cfg.primary.room_id = cfg->d2d_room_id;
    business_cfg.plc.local_addr = cfg->indoor_plc_addr;
    business_cfg.plc.peer_addr = cfg->d2d_peer_plc_addr;
    business_cfg.callbacks.observer = simple_d2d_bus_debug_observer;
    ret = simple_d2d_business_runtime_start(&rt->d2d_business, &business_cfg);
    if (ret != 0)
    {
        SIMPLE_LOGE("simple_d2d_business indoor start failed: %d\n", ret);
        return ret;
    }
    rt->d2d_business_started = true;

    if (cfg->link_plan.secondary_active)
    {
        simple_d2d_villa_heartbeat_status_t secondary_status;

        if (simple_d2d_villa_heartbeat_snapshot(&rt->villa_heartbeat,
                                                &secondary_status,
                                                1) == 1 &&
            secondary_status.online)
        {
            (void)simple_d2d_business_runtime_update_secondary_unlock_peer(
                &rt->d2d_business,
                secondary_status.endpoint.addr);
        }
    }
    return 0;
}


static void *kr_indoor_media_thread(void *arg)
{
    kr_indoor_handle_t *handle = (kr_indoor_handle_t *)arg;

    while (handle && handle->running)
    {
        simple_indoor_drive_media(&handle->runtime.codec_ctx);
        usleep(20000);
    }
    return NULL;
}

static void kr_indoor_copy_string(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0)
    {
        return;
    }
    snprintf(dst, dst_size, "%s", src ? src : "");
}

static void kr_indoor_apply_config(kr_indoor_handle_t *handle, const kr_indoor_config_t *cfg)
{
    kr_indoor_config_t *stored = &handle->cfg;

    memset(stored, 0, sizeof(*stored));
    stored->enabled = cfg ? cfg->enabled : true;
    stored->direct_plc_cco_addr = cfg && cfg->direct_plc_cco_addr != 0 ?
        cfg->direct_plc_cco_addr : 0x00020000U;
    stored->playback_volume_percent = cfg && cfg->playback_volume_percent > 0 ?
        cfg->playback_volume_percent : SIMPLE_PLAYBACK_VOLUME;

    kr_indoor_copy_string(handle->interface_buf,
                          sizeof(handle->interface_buf),
                          kr_indoor_nonempty(cfg ? cfg->interface : NULL, "eth0"));
    kr_indoor_copy_string(handle->target_device_id_buf,
                          sizeof(handle->target_device_id_buf),
                          kr_indoor_nonempty(cfg ? cfg->target_device_id : NULL,
                                             SIMPLE_DEFAULT_DOOR_DEVICE_ID));
    kr_indoor_copy_string(handle->room_id_buf,
                          sizeof(handle->room_id_buf),
                          kr_indoor_nonempty(cfg ? cfg->room_id : NULL, "101"));
    kr_indoor_copy_string(handle->unlock_peer_device_id_buf,
                          sizeof(handle->unlock_peer_device_id_buf),
                          kr_indoor_nonempty(cfg ? cfg->unlock_peer_device_id : NULL,
                                             SIMPLE_DEFAULT_DOOR_DEVICE_ID));
    kr_indoor_copy_string(handle->registry_peer_device_id_buf,
                          sizeof(handle->registry_peer_device_id_buf),
                          kr_indoor_nonempty(cfg ? cfg->registry_peer_device_id : NULL,
                                             SIMPLE_DEFAULT_CONVERTER_DEVICE_ID));

    stored->interface = handle->interface_buf;
    stored->target_device_id = handle->target_device_id_buf;
    stored->room_id = handle->room_id_buf;
    stored->unlock_peer_device_id = handle->unlock_peer_device_id_buf;
    stored->registry_peer_device_id = handle->registry_peer_device_id_buf;
}

static void kr_indoor_prepare_start_config(kr_indoor_handle_t *handle)
{
    simple_indoor_start_config_init(&handle->start_cfg);
    handle->start_cfg.target_device_id = handle->cfg.target_device_id;
    handle->start_cfg.d2d_room_id = handle->cfg.room_id;
    handle->start_cfg.d2d_peer_id = handle->cfg.unlock_peer_device_id;
    handle->start_cfg.registry_peer_device_id = handle->cfg.registry_peer_device_id;
    handle->start_cfg.spe_if = kr_indoor_nonempty(handle->cfg.interface, "eth0");
    handle->start_cfg.spe_peer_ip_arg = SIMPLE_DEFAULT_SECONDARY_CONFIRM_IP;
    handle->start_cfg.direct_plc_cco_addr = handle->cfg.direct_plc_cco_addr;
    handle->start_cfg.link_plan.secondary_policy = SIMPLE_INDOOR_SECONDARY_OPTIONAL;
    SIMPLE_LOGI("simple_indoor secondary SPE requested policy=%s spe_if=%s peer_ip=%s\n",
                simple_indoor_secondary_policy_name(
                    handle->start_cfg.link_plan.secondary_policy),
                handle->start_cfg.spe_if,
                handle->start_cfg.spe_peer_ip_arg);
}

static int kr_indoor_start_runtime(kr_indoor_handle_t *handle)
{
    int ret;
    char spe_local_ip[INET_ADDRSTRLEN];
    char spe_peer_ip[INET_ADDRSTRLEN];

    kr_indoor_prepare_start_config(handle);
    simple_indoor_runtime_init(&handle->runtime);
    handle->runtime.codec_ctx.event_ctx = handle;
    handle->runtime.codec_ctx.preview_brightness = handle->preview_brightness;

    ret = simple_indoor_finalize_start_config(&handle->start_cfg);
    if (ret != 0)
    {
        return ret;
    }
    simple_indoor_configure_codec_context(&handle->runtime, &handle->start_cfg);
    handle->runtime.codec_ctx.event_ctx = handle;
    handle->runtime.codec_ctx.preview_brightness = handle->preview_brightness;

    goblin_plc_memory_InitHooks(kr_indoor_plc_bmalloc, kr_indoor_plc_brealloc, kr_indoor_plc_bfree);

    ret = simple_indoor_start_transports(&handle->runtime, &handle->start_cfg);
    if (ret != 0)
    {
        return ret;
    }
    simple_indoor_configure_codec_context(&handle->runtime, &handle->start_cfg);
    handle->runtime.codec_ctx.event_ctx = handle;
    handle->runtime.codec_ctx.preview_brightness = handle->preview_brightness;

    ret = simple_indoor_start_media(&handle->runtime, &handle->start_cfg);
    if (ret != 0)
    {
        return ret;
    }
    ret = simple_indoor_start_call(&handle->runtime, &handle->start_cfg);
    if (ret != 0)
    {
        return ret;
    }
    ret = simple_indoor_start_secondary_heartbeat(&handle->runtime, &handle->start_cfg);
    if (ret != 0)
    {
        return ret;
    }
    ret = simple_indoor_apply_secondary_bootstrap(&handle->runtime, &handle->start_cfg);
    if (ret != 0)
    {
        return ret;
    }
    ret = simple_indoor_start_business(&handle->runtime, &handle->start_cfg);
    if (ret != 0)
    {
        return ret;
    }

    SIMPLE_LOGI("simple_indoor bridge running target=%s primary=%s secondary=%s active=%d "
                "spe_if=%s spe_local_ip=%s spe_peer_ip=%s routes=%zu links=%zu\n",
                handle->start_cfg.target_device_id,
                simple_indoor_primary_path_name(handle->start_cfg.link_plan.primary_path),
                simple_indoor_secondary_policy_name(
                    handle->start_cfg.link_plan.secondary_policy),
                handle->start_cfg.link_plan.secondary_active ? 1 : 0,
                handle->start_cfg.spe_if,
                simple_ipv4_to_str(handle->start_cfg.spe_local_ip,
                                   spe_local_ip,
                                   sizeof(spe_local_ip)),
                simple_ipv4_to_str(handle->start_cfg.spe_peer_ip,
                                   spe_peer_ip,
                                   sizeof(spe_peer_ip)),
                handle->runtime.route_count,
                handle->runtime.link_count);
    return 0;
}

int kr_indoor_create(kr_indoor_handle_t **out, const kr_indoor_config_t *cfg)
{
    kr_indoor_handle_t *handle;

    if (!out)
    {
        return -EINVAL;
    }
    *out = NULL;

    simple_utils_log_backend_register("simple_indoor");
    simple_bmem_backend_register();

    handle = (kr_indoor_handle_t *)bzalloc(sizeof(*handle));
    if (!handle)
    {
        return -ENOMEM;
    }
    kr_indoor_apply_config(handle, cfg);
    handle->enabled = handle->cfg.enabled;
    handle->playback_volume = handle->cfg.playback_volume_percent;
    handle->preview_brightness = 50;

    if (pthread_mutex_init(&handle->queue_mutex, NULL) != 0)
    {
        bfree(handle);
        return -errno;
    }
    if (pthread_cond_init(&handle->queue_cond, NULL) != 0)
    {
        pthread_mutex_destroy(&handle->queue_mutex);
        bfree(handle);
        return -errno;
    }
    handle->locks_ready = true;
    *out = handle;
    return 0;
}

int kr_indoor_start(kr_indoor_handle_t *handle)
{
    int ret;

    if (!handle)
    {
        return -EINVAL;
    }
    if (!handle->enabled)
    {
        return 0;
    }
    if (handle->started)
    {
        return 0;
    }

    ret = kr_indoor_start_runtime(handle);
    if (ret != 0)
    {
        simple_indoor_runtime_cleanup(&handle->runtime);
        return ret;
    }

    handle->running = true;
    ret = pthread_create(&handle->media_thread, NULL, kr_indoor_media_thread, handle);
    if (ret != 0)
    {
        handle->running = false;
        simple_indoor_runtime_cleanup(&handle->runtime);
        return -ret;
    }
    handle->media_thread_started = true;
    handle->started = true;
    kr_indoor_push_simple_event(handle,
                                KR_INDOOR_EVENT_CALL_STATE_CHANGED,
                                SIMPLE_SESSION_IDLE,
                                0,
                                false,
                                false,
                                NULL);
    return 0;
}

void kr_indoor_stop(kr_indoor_handle_t *handle)
{
    if (!handle || !handle->started)
    {
        return;
    }

    handle->running = false;
    if (handle->media_thread_started)
    {
        pthread_join(handle->media_thread, NULL);
        handle->media_thread_started = false;
    }
    if (handle->locks_ready)
    {
        pthread_mutex_lock(&handle->queue_mutex);
        handle->external_call_busy = false;
        pthread_mutex_unlock(&handle->queue_mutex);
    }
    (void)kr_indoor_set_media_bridge(handle, false);
    (void)kr_indoor_set_webrtc_bridge(handle, NULL);
    simple_indoor_runtime_cleanup(&handle->runtime);
    handle->started = false;

    pthread_mutex_lock(&handle->queue_mutex);
    pthread_cond_signal(&handle->queue_cond);
    pthread_mutex_unlock(&handle->queue_mutex);
}

void kr_indoor_destroy(kr_indoor_handle_t *handle)
{
    if (!handle)
    {
        return;
    }
    kr_indoor_stop(handle);
    if (handle->locks_ready)
    {
        pthread_cond_destroy(&handle->queue_cond);
        pthread_mutex_destroy(&handle->queue_mutex);
    }
    bfree(handle);
}

int kr_indoor_accept(kr_indoor_handle_t *handle)
{
    int ret;

    if (!handle || !handle->runtime.app)
    {
        return -EINVAL;
    }
    ret = simple_call_accept(handle->runtime.app);
    if (ret == 0)
    {
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_STATE_CHANGED,
                                    SIMPLE_SESSION_CALL_ACTIVE,
                                    0,
                                    true,
                                    true,
                                    NULL);
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_MEDIA_CHANGED,
                                    SIMPLE_SESSION_CALL_ACTIVE,
                                    0,
                                    true,
                                    true,
                                    NULL);
    }
    return ret;
}

int kr_indoor_reject(kr_indoor_handle_t *handle)
{
    int ret;

    if (!handle || !handle->runtime.app)
    {
        return -EINVAL;
    }
    ret = simple_call_reject(handle->runtime.app);
    if (ret == 0)
    {
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_STATE_CHANGED,
                                    SIMPLE_SESSION_IDLE,
                                    0,
                                    false,
                                    false,
                                    NULL);
    }
    return ret;
}

int kr_indoor_hangup(kr_indoor_handle_t *handle)
{
    int ret;

    if (!handle || !handle->runtime.app)
    {
        return -EINVAL;
    }
    ret = simple_call_hangup(handle->runtime.app);
    if (ret == 0)
    {
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_STATE_CHANGED,
                                    SIMPLE_SESSION_IDLE,
                                    0,
                                    false,
                                    false,
                                    NULL);
    }
    return ret;
}

int kr_indoor_call(kr_indoor_handle_t *handle, const char *target_device_id)
{
    const char *target;
    int ret;

    if (!handle || !handle->runtime.app)
    {
        return -EINVAL;
    }
    target = kr_indoor_nonempty(target_device_id, handle->start_cfg.target_device_id);
    ret = simple_call_start(handle->runtime.app, target);
    if (ret == 0)
    {
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_STATE_CHANGED,
                                    SIMPLE_SESSION_CALL_RINGING,
                                    0,
                                    false,
                                    false,
                                    target);
    }
    return ret;
}

int kr_indoor_monitor(kr_indoor_handle_t *handle, const char *target_device_id)
{
    const char *target;
    int ret;

    if (!handle || !handle->runtime.app)
    {
        return -EINVAL;
    }
    target = kr_indoor_nonempty(target_device_id, handle->start_cfg.target_device_id);
    ret = simple_monitor_start(handle->runtime.app, target);
    if (ret == 0)
    {
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_STATE_CHANGED,
                                    SIMPLE_SESSION_MONITOR_ONLY,
                                    0,
                                    true,
                                    false,
                                    target);
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_MEDIA_CHANGED,
                                    SIMPLE_SESSION_MONITOR_ONLY,
                                    0,
                                    true,
                                    false,
                                    target);
    }
    return ret;
}

int kr_indoor_monitor_stop(kr_indoor_handle_t *handle)
{
    int ret;

    if (!handle || !handle->runtime.app)
    {
        return -EINVAL;
    }
    ret = simple_monitor_stop(handle->runtime.app);
    if (ret == 0)
    {
        simple_indoor_request_monitor_local_media_stop(&handle->runtime.codec_ctx);
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_STATE_CHANGED,
                                    SIMPLE_SESSION_IDLE,
                                    0,
                                    false,
                                    false,
                                    NULL);
        kr_indoor_push_simple_event(handle,
                                    KR_INDOOR_EVENT_CALL_MEDIA_CHANGED,
                                    SIMPLE_SESSION_IDLE,
                                    0,
                                    false,
                                    false,
                                    NULL);
    }
    return ret;
}

int kr_indoor_set_webrtc_bridge(kr_indoor_handle_t *handle, void *webrtc_handle)
{
    if (!handle)
    {
        return -EINVAL;
    }

    handle->runtime.codec_ctx.webrtc_bridge = (kr_webrtc_handle_t *)webrtc_handle;
    SIMPLE_LOGI("indoor webrtc bridge set handle=%p\n", webrtc_handle);
    return 0;
}

int kr_indoor_set_media_bridge(kr_indoor_handle_t *handle, bool enabled)
{
    simple_indoor_codec_ctx_t *indoor;
    kr_webrtc_handle_t *webrtc;
    int ret = 0;

    if (!handle)
    {
        return -EINVAL;
    }

    indoor = &handle->runtime.codec_ctx;
    webrtc = indoor->webrtc_bridge;
    if (enabled && webrtc == NULL)
    {
        return -ENODEV;
    }

    __atomic_store_n(&indoor->media_bridge_enabled, enabled ? 1 : 0, __ATOMIC_RELAXED);
    indoor->media_bridge_first_video_logged = false;
    indoor->media_bridge_first_audio_to_cloud_logged = false;
    indoor->media_bridge_first_audio_to_d2d_logged = false;
    indoor->media_bridge_local_playback_muted_logged = false;
    indoor->media_bridge_video_last_ts_valid = false;
    indoor->media_bridge_video_current_ts_send = false;
    indoor->media_bridge_video_last_ts = 0;
    indoor->media_bridge_video_last_send_ms = 0;
    indoor->media_bridge_video_stats_start_ms = 0;
    indoor->media_bridge_video_stats_last_log_ms = 0;
    indoor->media_bridge_video_last_idr_ms = 0;
    indoor->media_bridge_video_input_count = 0;
    indoor->media_bridge_video_sent_count = 0;
    indoor->media_bridge_video_throttled_count = 0;
    indoor->media_bridge_video_failed_count = 0;
    indoor->media_bridge_video_idr_count = 0;
    indoor->media_bridge_video_sps_count = 0;
    indoor->media_bridge_video_pps_count = 0;
    indoor->media_bridge_video_bytes = 0;
    indoor->media_bridge_video_sent_bytes = 0;
    indoor->media_bridge_video_max_len = 0;
    resize_buf_copy(&indoor->media_bridge_h264_sps, NULL, 0);
    resize_buf_copy(&indoor->media_bridge_h264_pps, NULL, 0);
    resize_buf_copy(&indoor->media_bridge_h264_send_buf, NULL, 0);
    pthread_mutex_lock(&indoor->dec_mutex);
    simple_indoor_bridge_h264_dump_close_locked(indoor);
    if (enabled)
    {
        simple_indoor_bridge_h264_dump_open_locked(indoor);
    }
    else
    {
        indoor->media_bridge_h264_dump_tag[0] = '\0';
    }
    pthread_mutex_unlock(&indoor->dec_mutex);
    if (enabled)
    {
        pthread_mutex_lock(&indoor->pb_mutex);
        circlebuf_free(&indoor->pb_packet);
        circlebuf_init(&indoor->pb_packet);
        pthread_mutex_unlock(&indoor->pb_mutex);
    }

    if (webrtc != NULL)
    {
        if (enabled)
        {
            (void)kr_webrtc_set_bridge_h264_dump_tag(webrtc, indoor->media_bridge_h264_dump_tag);
            ret = kr_webrtc_set_remote_audio_sink(
                webrtc,
                simple_indoor_webrtc_remote_audio_sink,
                indoor
            );
            if (ret == 0)
            {
                ret = kr_webrtc_set_media_bridge_mode(webrtc, true);
            }
            if (ret < 0)
            {
                (void)kr_webrtc_set_remote_audio_sink(webrtc, NULL, NULL);
                __atomic_store_n(&indoor->media_bridge_enabled, 0, __ATOMIC_RELAXED);
                pthread_mutex_lock(&indoor->dec_mutex);
                simple_indoor_bridge_h264_dump_close_locked(indoor);
                indoor->media_bridge_h264_dump_tag[0] = '\0';
                pthread_mutex_unlock(&indoor->dec_mutex);
            }
        }
        else
        {
            (void)kr_webrtc_set_media_bridge_mode(webrtc, false);
            (void)kr_webrtc_set_remote_audio_sink(webrtc, NULL, NULL);
            (void)kr_webrtc_set_bridge_h264_dump_tag(webrtc, NULL);
        }
    }

    SIMPLE_LOGI("indoor media bridge set enabled=%d ret=%d\n", enabled ? 1 : 0, ret);
    return ret;
}

int kr_indoor_request_video_recovery(kr_indoor_handle_t *handle, bool use_fir)
{
    if (!handle)
    {
        return -EINVAL;
    }

    simple_indoor_request_video_recovery(&handle->runtime.codec_ctx,
                                         use_fir,
                                         use_fir ? "webrtc_ask_iframe" : "webrtc_pli");
    return 0;
}

int kr_indoor_unlock(kr_indoor_handle_t *handle, const char *target_device_id, const char *room_id, int place)
{
    const char *target;
    const char *room;
    int ret;

    if (!handle || !handle->runtime.d2d_business_started)
    {
        return -EINVAL;
    }
    target = kr_indoor_nonempty(target_device_id, handle->start_cfg.d2d_peer_id);
    room = kr_indoor_nonempty(room_id, handle->start_cfg.d2d_room_id);
    ret = simple_d2d_business_runtime_send_unlock(&handle->runtime.d2d_business,
                                                  target,
                                                  room,
                                                  place > 0 ? place : 1);
    kr_indoor_push_simple_event(handle,
                                ret == 0 ? KR_INDOOR_EVENT_UNLOCK_DONE : KR_INDOOR_EVENT_UNLOCK_FAIL,
                                SIMPLE_SESSION_IDLE,
                                ret,
                                false,
                                false,
                                target);
    return ret;
}

int kr_indoor_set_playback_volume(kr_indoor_handle_t *handle, int volume_percent)
{
    int volume;

    if (!handle)
    {
        return -EINVAL;
    }
    volume = volume_percent;
    if (volume < 0)
    {
        volume = 0;
    }
    if (volume > 100)
    {
        volume = 100;
    }
    handle->playback_volume = volume;
    if (handle->runtime.codec_ctx.pb_stream > 0 &&
        !Audio_Player_SetVolume(handle->runtime.codec_ctx.pb_stream, volume))
    {
        return -EIO;
    }
    return 0;
}

int kr_indoor_set_preview_brightness(kr_indoor_handle_t *handle, int level)
{
    int brightness;

    if (!handle)
    {
        return -EINVAL;
    }

    brightness = level;
    if (brightness < 0)
    {
        brightness = 0;
    }
    if (brightness > 100)
    {
        brightness = 100;
    }

    handle->preview_brightness = brightness;
    handle->runtime.codec_ctx.preview_brightness = brightness;
    return f1_video_decoder_set_brightness(&handle->runtime.codec_ctx.video.codec_data,
                                           brightness);
}

int kr_indoor_set_external_call_busy(kr_indoor_handle_t *handle, bool busy)
{
    if (!handle || !handle->locks_ready)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->queue_mutex);
    handle->external_call_busy = busy;
    pthread_mutex_unlock(&handle->queue_mutex);
    SIMPLE_LOGI("external call busy set: busy=%d\n", busy ? 1 : 0);
    return 0;
}

int kr_indoor_wait_event(kr_indoor_handle_t *handle, kr_indoor_event_t *out, uint32_t timeout_ms)
{
    struct timespec deadline;
    int ret = 0;

    if (!handle || !out)
    {
        return -EINVAL;
    }
    memset(out, 0, sizeof(*out));

    pthread_mutex_lock(&handle->queue_mutex);
    while (handle->queue_len == 0 && handle->running)
    {
        if (timeout_ms == 0)
        {
            pthread_mutex_unlock(&handle->queue_mutex);
            return 0;
        }
        ret = kr_indoor_abs_deadline(&deadline, timeout_ms);
        if (ret < 0)
        {
            pthread_mutex_unlock(&handle->queue_mutex);
            return ret;
        }
        ret = pthread_cond_timedwait(&handle->queue_cond, &handle->queue_mutex, &deadline);
        if (ret == ETIMEDOUT)
        {
            pthread_mutex_unlock(&handle->queue_mutex);
            return 0;
        }
        if (ret != 0)
        {
            pthread_mutex_unlock(&handle->queue_mutex);
            return -ret;
        }
    }

    if (handle->queue_len == 0 && !handle->running)
    {
        pthread_mutex_unlock(&handle->queue_mutex);
        return -ECANCELED;
    }

    *out = handle->queue[handle->queue_head];
    handle->queue_head = (handle->queue_head + 1U) % KR_INDOOR_QUEUE_CAPACITY;
    handle->queue_len--;
    pthread_mutex_unlock(&handle->queue_mutex);
    return 1;
}
