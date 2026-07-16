#include "kr_ipc_bridge.h"

#define KR_IPC_ONVIF_MAIN_WIDTH 1920
#define KR_IPC_ONVIF_MAIN_HEIGHT 1080

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "IpCammer.h"
#include <video/vd/VDecApi.h>
#include <video/vd/VoApi.h>
#define LOG_TAG "ipc-bridge"
#include <utils/log.h>
#include <utils/bmem.h>
#include <utils/util.h>

#ifndef VDEC_PAYLOAD_TYPE_H265
#define VDEC_PAYLOAD_TYPE_H265 ((VDEC_PAYLOAD_TYPE_E)4)
#endif

typedef struct kr_ipc_event_node
{
    kr_ipc_event_t event;
    struct kr_ipc_event_node *next;
} kr_ipc_event_node_t;

struct kr_ipc_handle
{
    pthread_mutex_t lock;
    pthread_cond_t cond;
    bool cancel_wait;
    bool monitoring;
    bool preview_layer_enabled;
    bool stream_ready;
    bool capture_pending;
    bool faulted;
    int preview_brightness;
    int codec_type;
    int video_width;
    int video_height;
    int decoder_channel;
    void *monitor_handle;
    kr_ipc_config_t cfg;
    kr_ipc_event_node_t *head;
    kr_ipc_event_node_t *tail;
    char capture_path[512];
    char camera_name[128];
    char camera_ip[64];
    char camera_rtsp[256];
};

static pthread_mutex_t g_kr_ipc_registry_lock = PTHREAD_MUTEX_INITIALIZER;
static kr_ipc_handle_t *g_kr_ipc_registry_handle = NULL;

static void kr_ipc_guess_decoder_size(
    const kr_ipc_handle_t *handle,
    bool use_substream,
    int *out_width,
    int *out_height)
{
    int width;
    int height;

    if (out_width == NULL || out_height == NULL)
    {
        return;
    }

    width = handle != NULL && handle->cfg.preview_width > 0 ? handle->cfg.preview_width : 800;
    height = handle != NULL && handle->cfg.preview_height > 0 ? handle->cfg.preview_height : 480;

    if (!use_substream)
    {
        width *= 2;
        height *= 2;
    }

    *out_width = width > 0 ? ALIGN_UP(width, 32) : 0;
    *out_height = height > 0 ? ALIGN_UP(height, 32) : 0;
}

static void kr_ipc_push_event(kr_ipc_handle_t *handle, const kr_ipc_event_t *event)
{
    kr_ipc_event_node_t *node;

    if (handle == NULL || event == NULL)
    {
        return;
    }

    node = (kr_ipc_event_node_t *)bzalloc(sizeof(*node));
    if (node == NULL)
    {
        return;
    }
    node->event = *event;

    pthread_mutex_lock(&handle->lock);
    if (handle->tail != NULL)
    {
        handle->tail->next = node;
    }
    else
    {
        handle->head = node;
    }
    handle->tail = node;
    pthread_cond_signal(&handle->cond);
    pthread_mutex_unlock(&handle->lock);
}

static kr_ipc_handle_t *kr_ipc_find_handle_by_monitor(void *monitor_handle)
{
    kr_ipc_handle_t *handle = NULL;

    pthread_mutex_lock(&g_kr_ipc_registry_lock);
    if (g_kr_ipc_registry_handle != NULL &&
        g_kr_ipc_registry_handle->monitor_handle == monitor_handle)
    {
        handle = g_kr_ipc_registry_handle;
    }
    pthread_mutex_unlock(&g_kr_ipc_registry_lock);
    return handle;
}

static void kr_ipc_apply_preview_cm_locked(kr_ipc_handle_t *handle)
{
    VO_LAYER_CM cm;

    if (handle == NULL || !handle->preview_layer_enabled)
    {
        return;
    }

    memset(&cm, 0, sizeof(cm));
    cm.Brightness = handle->preview_brightness;
    cm.Saturation = 50;
    cm.Hue = 50;
    cm.Contrast = 50;
    (void)VO_ChnSetCM(0, 0, &cm);
}

static int kr_ipc_open_preview_locked(kr_ipc_handle_t *handle)
{
    VO_LAYER_INFO layer_info;
    VDEC_CHN_ATTR_S dec_attr;
    int decoder_channel;
    VDEC_PAYLOAD_TYPE_E payload_type;

    if (handle->decoder_channel >= 0)
    {
        return 0;
    }
    if (handle->video_width <= 0 || handle->video_height <= 0)
    {
        LOGI(
                "open_preview pending size width=%d height=%d codec=%d\n",
                handle->video_width,
                handle->video_height,
                handle->codec_type);
        return -EAGAIN;
    }

    payload_type = handle->codec_type == VIDEO_H265 ? VDEC_PAYLOAD_TYPE_H265 : VDEC_PAYLOAD_TYPE_H264;

    if (VO_Enable() != 1)
    {
        LOGE("VO_Enable failed\n");
        return -EIO;
    }

    memset(&layer_info, 0, sizeof(layer_info));
    layer_info.Rect.X = handle->cfg.preview_x;
    layer_info.Rect.Y = handle->cfg.preview_y;
    layer_info.Rect.W = (unsigned int)handle->cfg.preview_width;
    layer_info.Rect.H = (unsigned int)handle->cfg.preview_height;
    layer_info.AlphaValue = 0xFF;
    layer_info.Rotate = VO_ROTATE_NONE;

    if (VO_EnableVideoLayer(0, 0, &layer_info) != 1)
    {
        LOGI(
                "VO_EnableVideoLayer failed rect=%d,%d %dx%d\n",
                handle->cfg.preview_x,
                handle->cfg.preview_y,
                handle->cfg.preview_width,
                handle->cfg.preview_height);
        VO_Disable();
        return -EIO;
    }

    memset(&dec_attr, 0, sizeof(dec_attr));
    dec_attr.deType = payload_type;
    dec_attr.FormatType = FORMAT_NV12;
    dec_attr.BufSize = 0x100000;
    dec_attr.u32PicWidth = (unsigned int)handle->video_width;
    dec_attr.u32PicHight = (unsigned int)handle->video_height;

    if (VDEC_Init() != 1)
    {
        LOGE("VDEC_Init failed\n");
        (void)VO_DisableVideoLayer(0, 0);
        (void)VO_Disable();
        return -EIO;
    }

    decoder_channel = VDEC_CreateChn(&dec_attr);
    if (decoder_channel < 0)
    {
        LOGI(
                "VDEC_CreateChn failed codec=%d size=%dx%d\n",
                payload_type,
                handle->video_width,
                handle->video_height);
        (void)VDEC_DeInit();
        (void)VO_DisableVideoLayer(0, 0);
        (void)VO_Disable();
        return -EIO;
    }

    handle->decoder_channel = decoder_channel;
    handle->preview_layer_enabled = true;
    handle->stream_ready = false;
    kr_ipc_apply_preview_cm_locked(handle);
    LOGI(
            "open_preview ok channel=%d codec=%d rect=%d,%d %dx%d size=%dx%d\n",
            decoder_channel,
            payload_type,
            handle->cfg.preview_x,
            handle->cfg.preview_y,
            handle->cfg.preview_width,
            handle->cfg.preview_height,
            handle->video_width,
            handle->video_height);
    return 0;
}

static void kr_ipc_close_preview_locked(kr_ipc_handle_t *handle)
{
    if (handle == NULL)
    {
        return;
    }

    if (handle->decoder_channel >= 0)
    {
        (void)VDEC_DestroyChn(handle->decoder_channel);
        handle->decoder_channel = -1;
        (void)VDEC_DeInit();
    }
    if (handle->preview_layer_enabled)
    {
        (void)VO_DisableVideoLayer(0, 0);
        handle->preview_layer_enabled = false;
        (void)VO_Disable();
    }
    handle->stream_ready = false;
}

static int kr_ipc_mkdirs_for_file(const char *path)
{
    char buffer[512];
    size_t len;
    size_t i;

    if (path == NULL || path[0] == '\0')
    {
        return -EINVAL;
    }

    safe_strncpy(buffer, path, sizeof(buffer));
    len = strlen(buffer);
    if (len == 0)
    {
        return -EINVAL;
    }

    for (i = 1; i < len; ++i)
    {
        if (buffer[i] != '/')
        {
            continue;
        }
        buffer[i] = '\0';
        if (mkdir(buffer, 0775) != 0 && errno != EEXIST)
        {
            return -errno;
        }
        buffer[i] = '/';
    }

    return 0;
}

static int kr_ipc_save_jpeg(const char *path, const unsigned char *data, size_t len)
{
    FILE *fp;

    if (kr_ipc_mkdirs_for_file(path) < 0)
    {
        return -EIO;
    }

    fp = fopen(path, "wb");
    if (fp == NULL)
    {
        return -errno;
    }

    if (fwrite(data, 1, len, fp) != len)
    {
        fclose(fp);
        return -EIO;
    }

    fclose(fp);
    return 0;
}

static void kr_ipc_finish_capture_locked(
    kr_ipc_handle_t *handle,
    VDEC_FRAME_S *frame
)
{
    unsigned char *jpeg_data = NULL;
    int jpeg_len;
    int rc;
    kr_ipc_event_t event;

    if (handle == NULL || frame == NULL || !handle->capture_pending || handle->capture_path[0] == '\0')
    {
        return;
    }

    jpeg_len = VDEC_Convert_Jpeg(handle->decoder_channel, frame, 90, &jpeg_data);
    if (jpeg_len <= 0 || jpeg_data == NULL)
    {
        memset(&event, 0, sizeof(event));
        event.kind = KR_IPC_EVENT_ERROR;
        event.value0 = jpeg_len;
        safe_strncpy(event.text1, "capture_convert_failed", sizeof(event.text1));
        kr_ipc_push_event(handle, &event);
        handle->capture_pending = false;
        handle->capture_path[0] = '\0';
        return;
    }

    rc = kr_ipc_save_jpeg(handle->capture_path, jpeg_data, (size_t)jpeg_len);
    (void)VDEC_ReleaseJpegData(jpeg_data);
    if (rc < 0)
    {
        memset(&event, 0, sizeof(event));
        event.kind = KR_IPC_EVENT_ERROR;
        event.value0 = rc;
        safe_strncpy(event.text1, "capture_save_failed", sizeof(event.text1));
        safe_strncpy(event.text2, handle->capture_path, sizeof(event.text2));
        kr_ipc_push_event(handle, &event);
        handle->capture_pending = false;
        handle->capture_path[0] = '\0';
        return;
    }

    memset(&event, 0, sizeof(event));
    event.kind = KR_IPC_EVENT_CAPTURE_SAVED;
    safe_strncpy(event.text1, handle->camera_name, sizeof(event.text1));
    safe_strncpy(event.text2, handle->camera_ip, sizeof(event.text2));
    safe_strncpy(event.text3, handle->capture_path, sizeof(event.text3));
    kr_ipc_push_event(handle, &event);

    handle->capture_pending = false;
    handle->capture_path[0] = '\0';
}

typedef struct kr_ipc_h264_split_keyframe
{
    unsigned char *sps;
    unsigned int sps_len;
    unsigned char *pps;
    unsigned int pps_len;
    unsigned char *idr;
    unsigned int idr_len;
} kr_ipc_h264_split_keyframe_t;

static int kr_ipc_h264_start_code_len(const unsigned char *data, unsigned int len)
{
    if (len >= 4 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01)
    {
        return 4;
    }
    if (len >= 3 && data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01)
    {
        return 3;
    }
    return 0;
}

static unsigned int kr_ipc_find_h264_start_code(
    const unsigned char *data,
    unsigned int len,
    unsigned int offset,
    unsigned int *out_start_code_len
)
{
    unsigned int i;

    if (out_start_code_len != NULL)
    {
        *out_start_code_len = 0;
    }
    if (data == NULL || offset >= len)
    {
        return len;
    }

    for (i = offset; i + 3 <= len; ++i)
    {
        int start_code_len = kr_ipc_h264_start_code_len(data + i, len - i);
        if (start_code_len > 0)
        {
            if (out_start_code_len != NULL)
            {
                *out_start_code_len = (unsigned int)start_code_len;
            }
            return i;
        }
    }

    return len;
}

static bool kr_ipc_split_h264_keyframe(
    unsigned char *data,
    unsigned int len,
    kr_ipc_h264_split_keyframe_t *out_split
)
{
    unsigned int offset = 0;

    if (out_split == NULL)
    {
        return false;
    }
    memset(out_split, 0, sizeof(*out_split));
    if (data == NULL || len < 5)
    {
        return false;
    }

    while (offset < len)
    {
        unsigned int start_code_len = 0;
        unsigned int nal_start = kr_ipc_find_h264_start_code(data, len, offset, &start_code_len);
        unsigned int nal_header;
        unsigned int next_start;
        unsigned int nal_type;

        if (nal_start >= len || start_code_len == 0)
        {
            break;
        }
        nal_header = nal_start + start_code_len;
        if (nal_header >= len)
        {
            break;
        }
        next_start = kr_ipc_find_h264_start_code(data, len, nal_header + 1, NULL);
        nal_type = data[nal_header] & 0x1F;

        if (nal_type == 7)
        {
            out_split->sps = data + nal_start;
            out_split->sps_len = (next_start >= len ? len : next_start) - nal_start;
        }
        else if (nal_type == 8)
        {
            out_split->pps = data + nal_start;
            out_split->pps_len = (next_start >= len ? len : next_start) - nal_start;
        }
        else if (nal_type == 5)
        {
            out_split->idr = data + nal_start;
            out_split->idr_len = len - nal_start;
            break;
        }

        offset = next_start >= len ? len : next_start;
    }

    return out_split->sps != NULL &&
           out_split->sps_len > 0 &&
           out_split->pps != NULL &&
           out_split->pps_len > 0 &&
           out_split->idr != NULL &&
           out_split->idr_len > 0 &&
           out_split->sps < out_split->pps &&
           out_split->pps < out_split->idr;
}

static void kr_ipc_decode_stream_and_show(
    kr_ipc_handle_t *handle,
    unsigned char *data,
    unsigned int len
)
{
    VDEC_STREAM_S stream;
    VDEC_CHN_STAT_S stat;
    VDEC_FRAME_S frame;
    VDEC_STATUS status;
    kr_ipc_event_t event;

    if (handle == NULL || handle->decoder_channel < 0 || data == NULL || len == 0)
    {
        return;
    }

    memset(&stream, 0, sizeof(stream));
    stream.pu8Addr = data;
    stream.u32Len = len;
    stream.u64PTS = (unsigned long long)-1;

    if (VDEC_SendStream(handle->decoder_channel, &stream) != 1)
    {
        LOGI(
                "VDEC_SendStream failed channel=%d len=%u\n",
                handle->decoder_channel,
                len);
        return;
    }
    memset(&stat, 0, sizeof(stat));
    if (VDEC_Query(handle->decoder_channel, &stat) != 1)
    {
        LOGI(
                "VDEC_Query failed channel=%d\n",
                handle->decoder_channel);
        return;
    }
    if (stat.u32FrameNum == 0)
    {
        return;
    }

    status = VDEC_StartRecvStream(handle->decoder_channel, 0, 0, 0);
    if (status != VDEC_FRAME_DECODED && status != VDEC_KEYFRAME_DECODED)
    {
        LOGI(
                "VDEC_StartRecvStream status=%d channel=%d\n",
                status,
                handle->decoder_channel);
        return;
    }

    memset(&frame, 0, sizeof(frame));
    if (VDEC_GetImage(handle->decoder_channel, &frame) != 1)
    {
        LOGI(
                "VDEC_GetImage failed channel=%d status=%d\n",
                handle->decoder_channel,
                status);
        return;
    }

    if (handle->preview_layer_enabled)
    {
        if (VO_SetZoomInWindow(0, 0, &frame.SrcInfo) != 1)
        {
            LOGI(
                    "VO_SetZoomInWindow failed src=%ux%u channel=%d\n",
                    frame.SrcInfo.W,
                    frame.SrcInfo.H,
                    handle->decoder_channel);
        }
        if (VO_ChnShow(0, 0, &frame) != 1)
        {
            LOGI(
                    "VO_ChnShow failed channel=%d\n",
                    handle->decoder_channel);
        }
        else if (!handle->stream_ready)
        {
            LOGI(
                    "preview frame shown channel=%d src=%ux%u\n",
                    handle->decoder_channel,
                    frame.SrcInfo.W,
                    frame.SrcInfo.H);
        }
    }

    if (!handle->stream_ready)
    {
        handle->stream_ready = true;
        memset(&event, 0, sizeof(event));
        event.kind = KR_IPC_EVENT_STREAM_READY;
        event.value0 = handle->codec_type;
        event.value1 = handle->video_width;
        safe_strncpy(event.text1, handle->camera_name, sizeof(event.text1));
        safe_strncpy(event.text2, handle->camera_ip, sizeof(event.text2));
        kr_ipc_push_event(handle, &event);
    }

    kr_ipc_finish_capture_locked(handle, &frame);
    (void)VDEC_ReleaseImage(handle->decoder_channel, &frame);
}

static void kr_ipc_decode_and_show(
    kr_ipc_handle_t *handle,
    unsigned char *data,
    unsigned int len
)
{
    kr_ipc_h264_split_keyframe_t split;

    if (handle == NULL || handle->decoder_channel < 0 || data == NULL || len == 0)
    {
        return;
    }

    if (handle->codec_type == VIDEO_H264 &&
        kr_ipc_split_h264_keyframe(data, len, &split))
    {
        if (!handle->stream_ready)
        {
            LOGI(
                    "split h264 keyframe sps=%u pps=%u idr=%u total=%u\n",
                    split.sps_len,
                    split.pps_len,
                    split.idr_len,
                    len);
        }
        kr_ipc_decode_stream_and_show(handle, split.sps, split.sps_len);
        kr_ipc_decode_stream_and_show(handle, split.pps, split.pps_len);
        kr_ipc_decode_stream_and_show(handle, split.idr, split.idr_len);
        return;
    }

    kr_ipc_decode_stream_and_show(handle, data, len);
}

static void *kr_ipc_detach_monitor_locked(kr_ipc_handle_t *handle)
{
    void *monitor_handle;

    if (handle == NULL)
    {
        return NULL;
    }

    monitor_handle = handle->monitor_handle;
    handle->monitor_handle = NULL;
    handle->monitoring = false;
    handle->faulted = false;
    handle->video_width = 0;
    handle->video_height = 0;
    handle->codec_type = VIDEO_H264;
    handle->capture_pending = false;
    handle->capture_path[0] = '\0';

    kr_ipc_close_preview_locked(handle);
    return monitor_handle;
}

static void kr_ipc_status_callback(void *monitor_handle, int type)
{
    kr_ipc_handle_t *handle = kr_ipc_find_handle_by_monitor(monitor_handle);
    kr_ipc_event_t event;

    if (handle == NULL)
    {
        return;
    }

    memset(&event, 0, sizeof(event));
    event.kind = type == ERR_REMOTE_OFFLINE ? KR_IPC_EVENT_OFFLINE : KR_IPC_EVENT_ERROR;
    event.value0 = type;
    switch (type)
    {
    case ERR_AUTHENTICATE:
        safe_strncpy(event.text1, "authenticate_failed", sizeof(event.text1));
        break;
    case ERR_REMOTE_OFFLINE:
        safe_strncpy(event.text1, "remote_offline", sizeof(event.text1));
        break;
    case ERR_ONVIF_SEARCH:
        safe_strncpy(event.text1, "onvif_search_failed", sizeof(event.text1));
        break;
    default:
        safe_strncpy(event.text1, "unknown_ipc_error", sizeof(event.text1));
        break;
    }
    pthread_mutex_lock(&handle->lock);
    handle->faulted = true;
    pthread_mutex_unlock(&handle->lock);

    kr_ipc_push_event(handle, &event);
}

static void kr_ipc_info_callback(void *monitor_handle, ipc_info_t *info)
{
    kr_ipc_handle_t *handle = kr_ipc_find_handle_by_monitor(monitor_handle);
    kr_ipc_event_t event;

    if (handle == NULL || info == NULL)
    {
        return;
    }

    pthread_mutex_lock(&handle->lock);
    handle->video_width = info->video_width;
    handle->video_height = info->video_height;
    if (info->rtsp_url != NULL)
    {
        safe_strncpy(handle->camera_rtsp, info->rtsp_url, sizeof(handle->camera_rtsp));
    }
    pthread_mutex_unlock(&handle->lock);
    LOGI(
            "info ip=%s width=%d height=%d rtsp=%s\n",
            SAFE_STRING(info->ip),
            info->video_width,
            info->video_height,
            SAFE_STRING(info->rtsp_url));

    memset(&event, 0, sizeof(event));
    event.kind = KR_IPC_EVENT_INFO;
    event.value0 = info->video_width;
    event.value1 = info->video_height;
    safe_strncpy(event.text1, handle->camera_name, sizeof(event.text1));
    safe_strncpy(event.text2, info->ip, sizeof(event.text2));
    safe_strncpy(event.text3, info->rtsp_url, sizeof(event.text3));
    kr_ipc_push_event(handle, &event);
}

static int kr_ipc_try_onvif_search_rtsp(
    const kr_ipc_monitor_config_t *cfg,
    int width,
    int height,
    char *out_rtsp,
    size_t out_rtsp_len)
{
    onvif_setting setting;
    onvif_result result;

    if (cfg == NULL || out_rtsp == NULL || out_rtsp_len == 0 ||
        cfg->ip == NULL || cfg->ip[0] == '\0')
    {
        return -EINVAL;
    }

    memset(&setting, 0, sizeof(setting));
    memset(&result, 0, sizeof(result));
    setting.timeout = 5 * 1000;
    safe_strncpy(setting.ip, cfg->ip, sizeof(setting.ip));
    safe_strncpy(setting.user, cfg->user, sizeof(setting.user));
    safe_strncpy(setting.passwd, cfg->password, sizeof(setting.passwd));
    ipc_onvif_set_select_video_resolution(width, height);
    LOGI(
            "onvif_search ip=%s user=%s passwd=%s width=%d height=%d\n",
            setting.ip,
            setting.user,
            setting.passwd,
            width,
            height);

    if (ipc_onvif_search(&setting, &result) < 1 || result.rtsp_url[0] == '\0')
    {
        LOGI(
                "onvif_search failed ip=%s width=%d height=%d\n",
                setting.ip,
                width,
                height);
        out_rtsp[0] = '\0';
        return -ENOENT;
    }

    safe_strncpy(out_rtsp, result.rtsp_url, out_rtsp_len);
    LOGI(
            "onvif_search ok ip=%s rtsp=%s width=%d height=%d\n",
            setting.ip,
            out_rtsp,
            width,
            height);
    return 0;
}

static void kr_ipc_frame_callback(
    void *monitor_handle,
    unsigned char *buf,
    unsigned int len,
    bool keyframe
)
{
    kr_ipc_handle_t *handle = kr_ipc_find_handle_by_monitor(monitor_handle);

    if (handle == NULL || buf == NULL || len == 0)
    {
        return;
    }

    pthread_mutex_lock(&handle->lock);
    if (!handle->monitoring)
    {
        pthread_mutex_unlock(&handle->lock);
        return;
    }

    if (handle->decoder_channel < 0)
    {
        if (!keyframe || kr_ipc_open_preview_locked(handle) < 0)
        {
            pthread_mutex_unlock(&handle->lock);
            return;
        }
    }

    kr_ipc_decode_and_show(handle, buf, len);
    pthread_mutex_unlock(&handle->lock);
}

static int kr_ipc_pop_event_locked(
    kr_ipc_handle_t *handle,
    kr_ipc_event_t *out_event,
    int timeout_ms
)
{
    if (handle->cancel_wait)
    {
        return -ECANCELED;
    }

    if (handle->head == NULL)
    {
        if (timeout_ms < 0)
        {
            while (handle->head == NULL && !handle->cancel_wait)
            {
                pthread_cond_wait(&handle->cond, &handle->lock);
            }
        }
        else if (timeout_ms > 0)
        {
            struct timespec ts;
            int rc;

            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += timeout_ms / 1000;
            ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
            if (ts.tv_nsec >= 1000000000L)
            {
                ts.tv_sec += 1;
                ts.tv_nsec -= 1000000000L;
            }

            while (handle->head == NULL && !handle->cancel_wait)
            {
                rc = pthread_cond_timedwait(&handle->cond, &handle->lock, &ts);
                if (rc == ETIMEDOUT)
                {
                    return 0;
                }
            }
        }
        else
        {
            return 0;
        }
    }

    if (handle->cancel_wait)
    {
        return -ECANCELED;
    }

    if (handle->head != NULL)
    {
        kr_ipc_event_node_t *node = handle->head;
        handle->head = node->next;
        if (handle->head == NULL)
        {
            handle->tail = NULL;
        }
        if (out_event != NULL)
        {
            *out_event = node->event;
        }
        bfree(node);
        return 1;
    }

    return 0;
}

int kr_ipc_create(kr_ipc_handle_t **out_handle, const kr_ipc_config_t *cfg)
{
    kr_ipc_handle_t *handle;
    pthread_mutexattr_t attr;

    if (out_handle == NULL || cfg == NULL)
    {
        return -EINVAL;
    }

    handle = (kr_ipc_handle_t *)bzalloc(sizeof(*handle));
    if (handle == NULL)
    {
        return -ENOMEM;
    }

    if (pthread_mutexattr_init(&attr) != 0)
    {
        bfree(handle);
        return -ENOMEM;
    }
    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
    {
        pthread_mutexattr_destroy(&attr);
        bfree(handle);
        return -ENOMEM;
    }
    if (pthread_mutex_init(&handle->lock, &attr) != 0 ||
        pthread_cond_init(&handle->cond, NULL) != 0)
    {
        pthread_mutexattr_destroy(&attr);
        bfree(handle);
        return -ENOMEM;
    }
    pthread_mutexattr_destroy(&attr);

    handle->cfg.interface = bstrdup(cfg->interface != NULL ? cfg->interface : "wlan0");
    handle->cfg.preview_x = cfg->preview_x;
    handle->cfg.preview_y = cfg->preview_y;
    handle->cfg.preview_width = cfg->preview_width;
    handle->cfg.preview_height = cfg->preview_height;
    handle->preview_brightness = 50;
    handle->decoder_channel = -1;
    handle->codec_type = VIDEO_H264;

    ipc_set_network_interface((char *)(handle->cfg.interface != NULL ? handle->cfg.interface : "wlan0"));
    pthread_mutex_lock(&g_kr_ipc_registry_lock);
    g_kr_ipc_registry_handle = handle;
    pthread_mutex_unlock(&g_kr_ipc_registry_lock);
    *out_handle = handle;
    return 0;
}

int kr_ipc_destroy(kr_ipc_handle_t *handle)
{
    kr_ipc_event_node_t *node;

    if (handle == NULL)
    {
        return 0;
    }

    pthread_mutex_lock(&handle->lock);
    handle->cancel_wait = true;
    pthread_cond_broadcast(&handle->cond);
    {
        void *monitor_handle = kr_ipc_detach_monitor_locked(handle);
        pthread_mutex_unlock(&handle->lock);
        if (monitor_handle != NULL)
        {
            ipc_stop_monitor(monitor_handle);
        }
    }

    node = handle->head;
    while (node != NULL)
    {
        kr_ipc_event_node_t *next = node->next;
        bfree(node);
        node = next;
    }

    bfree((char *)handle->cfg.interface);
    pthread_mutex_lock(&g_kr_ipc_registry_lock);
    if (g_kr_ipc_registry_handle == handle)
    {
        g_kr_ipc_registry_handle = NULL;
    }
    pthread_mutex_unlock(&g_kr_ipc_registry_lock);
    pthread_cond_destroy(&handle->cond);
    pthread_mutex_destroy(&handle->lock);
    bfree(handle);
    return 0;
}

int kr_ipc_wait_event(kr_ipc_handle_t *handle, kr_ipc_event_t *out_event, int timeout_ms)
{
    int rc;
    int cleanup = 0;

    if (handle == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    rc = kr_ipc_pop_event_locked(handle, out_event, timeout_ms);
    {
        void *monitor_handle = NULL;
        if (rc > 0 && out_event != NULL)
        {
            cleanup = out_event->kind == KR_IPC_EVENT_OFFLINE || out_event->kind == KR_IPC_EVENT_ERROR;
            if (cleanup && handle->faulted)
            {
                monitor_handle = kr_ipc_detach_monitor_locked(handle);
            }
        }
        pthread_mutex_unlock(&handle->lock);
        if (monitor_handle != NULL)
        {
            ipc_stop_monitor(monitor_handle);
        }
        return rc;
    }
}

int kr_ipc_start_monitor(kr_ipc_handle_t *handle, const kr_ipc_monitor_config_t *cfg)
{
    ipc_setting_s setting;
    void *monitor_handle;
    bool try_substream;
    bool selected_is_substream;
    const char *selected_rtsp;
    char resolved_rtsp[256];
    kr_ipc_event_t event;
    int onvif_rc;

    if (handle == NULL || cfg == NULL || cfg->ip == NULL || cfg->ip[0] == '\0')
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    {
        void *monitor_handle = kr_ipc_detach_monitor_locked(handle);
        pthread_mutex_unlock(&handle->lock);
        if (monitor_handle != NULL)
        {
            ipc_stop_monitor(monitor_handle);
        }
    }
    memset(&setting, 0, sizeof(setting));
    safe_strncpy(handle->camera_name, cfg->name, sizeof(handle->camera_name));
    safe_strncpy(handle->camera_ip, cfg->ip, sizeof(handle->camera_ip));
    handle->camera_rtsp[0] = '\0';
    resolved_rtsp[0] = '\0';

    try_substream = cfg->prefer_substream;
    selected_is_substream = try_substream &&
        cfg->sub_rtsp_url != NULL &&
        cfg->sub_rtsp_url[0] != '\0';
    selected_rtsp = selected_is_substream ? cfg->sub_rtsp_url : cfg->rtsp_url;

    kr_ipc_guess_decoder_size(
        handle,
        selected_is_substream,
        &handle->video_width,
        &handle->video_height);
    LOGI(
            "guess_decoder_size name=%s substream=%d size=%dx%d\n",
            SAFE_STRING(cfg->name),
            selected_is_substream ? 1 : 0,
            handle->video_width,
            handle->video_height);

    safe_strncpy(setting.ip, cfg->ip, sizeof(setting.ip));
    safe_strncpy(setting.user, cfg->user, sizeof(setting.user));
    safe_strncpy(setting.passwd, cfg->password, sizeof(setting.passwd));
    safe_strncpy(setting.rtsp_url, selected_rtsp, sizeof(setting.rtsp_url));
    setting.use_onvif = false;
    setting.frame_cb = kr_ipc_frame_callback;
    setting.status_cb = kr_ipc_status_callback;
    setting.info_cb = kr_ipc_info_callback;

    if (setting.rtsp_url[0] == '\0')
    {
        onvif_rc = kr_ipc_try_onvif_search_rtsp(
            cfg,
            try_substream ? handle->cfg.preview_width : KR_IPC_ONVIF_MAIN_WIDTH,
            try_substream ? handle->cfg.preview_height : KR_IPC_ONVIF_MAIN_HEIGHT,
            resolved_rtsp,
            sizeof(resolved_rtsp));
        if (onvif_rc == 0)
        {
            safe_strncpy(setting.rtsp_url, resolved_rtsp, sizeof(setting.rtsp_url));
        }
    }

    monitor_handle = setting.rtsp_url[0] == '\0' ? NULL : ipc_start_monitor(&setting);
    if (monitor_handle == NULL && try_substream)
    {
        memset(&setting.rtsp_url, 0, sizeof(setting.rtsp_url));
        safe_strncpy(setting.rtsp_url, cfg->rtsp_url, sizeof(setting.rtsp_url));
        if (setting.rtsp_url[0] == '\0')
        {
            onvif_rc = kr_ipc_try_onvif_search_rtsp(
                cfg,
                KR_IPC_ONVIF_MAIN_WIDTH,
                KR_IPC_ONVIF_MAIN_HEIGHT,
                resolved_rtsp,
                sizeof(resolved_rtsp));
            if (onvif_rc == 0)
            {
                safe_strncpy(setting.rtsp_url, resolved_rtsp, sizeof(setting.rtsp_url));
            }
        }
        monitor_handle = setting.rtsp_url[0] == '\0' ? NULL : ipc_start_monitor(&setting);
    }
    if (monitor_handle == NULL)
    {
        if (setting.rtsp_url[0] == '\0')
        {
            memset(&event, 0, sizeof(event));
            event.kind = KR_IPC_EVENT_ERROR;
            event.value0 = ERR_ONVIF_SEARCH;
            safe_strncpy(event.text1, "onvif_search_failed", sizeof(event.text1));
            safe_strncpy(event.text2, cfg->ip, sizeof(event.text2));
            kr_ipc_push_event(handle, &event);
        }
        return -EIO;
    }

    pthread_mutex_lock(&handle->lock);
    handle->monitor_handle = monitor_handle;
    handle->monitoring = true;
    handle->faulted = false;
    handle->stream_ready = false;
    handle->codec_type = ipc_get_codec_type(monitor_handle);
    safe_strncpy(handle->camera_rtsp, setting.rtsp_url, sizeof(handle->camera_rtsp));
    LOGI(
            "monitor_started codec=%d fallback_size=%dx%d rtsp=%s\n",
            handle->codec_type,
            handle->video_width,
            handle->video_height,
            setting.rtsp_url);
    pthread_mutex_unlock(&handle->lock);

    memset(&event, 0, sizeof(event));
    event.kind = KR_IPC_EVENT_MONITOR_STARTED;
    event.value0 = handle->codec_type;
    safe_strncpy(event.text1, cfg->name, sizeof(event.text1));
    safe_strncpy(event.text2, cfg->ip, sizeof(event.text2));
    safe_strncpy(event.text3, setting.rtsp_url, sizeof(event.text3));
    kr_ipc_push_event(handle, &event);
    return 0;
}

int kr_ipc_stop_monitor(kr_ipc_handle_t *handle)
{
    kr_ipc_event_t event;

    if (handle == NULL)
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    if (!handle->monitoring && handle->monitor_handle == NULL)
    {
        pthread_mutex_unlock(&handle->lock);
        return 0;
    }

    memset(&event, 0, sizeof(event));
    event.kind = KR_IPC_EVENT_MONITOR_STOPPED;
    safe_strncpy(event.text1, handle->camera_name, sizeof(event.text1));
    safe_strncpy(event.text2, handle->camera_ip, sizeof(event.text2));
    {
        void *monitor_handle = kr_ipc_detach_monitor_locked(handle);
        pthread_mutex_unlock(&handle->lock);
        if (monitor_handle != NULL)
        {
            ipc_stop_monitor(monitor_handle);
        }
    }

    kr_ipc_push_event(handle, &event);
    return 0;
}

int kr_ipc_capture_jpeg(kr_ipc_handle_t *handle, const char *output_path)
{
    if (handle == NULL || output_path == NULL || output_path[0] == '\0')
    {
        return -EINVAL;
    }

    pthread_mutex_lock(&handle->lock);
    if (!handle->monitoring || handle->decoder_channel < 0)
    {
        pthread_mutex_unlock(&handle->lock);
        return -EAGAIN;
    }

    safe_strncpy(handle->capture_path, output_path, sizeof(handle->capture_path));
    handle->capture_pending = true;
    pthread_mutex_unlock(&handle->lock);
    return 0;
}

int kr_ipc_is_monitoring(kr_ipc_handle_t *handle)
{
    int active;

    if (handle == NULL)
    {
        return 0;
    }

    pthread_mutex_lock(&handle->lock);
    active = handle->monitoring ? 1 : 0;
    pthread_mutex_unlock(&handle->lock);
    return active;
}

int kr_ipc_set_preview_brightness(kr_ipc_handle_t *handle, int level)
{
    if (handle == NULL)
    {
        return -EINVAL;
    }

    if (level < 0)
    {
        level = 0;
    }
    else if (level > 100)
    {
        level = 100;
    }

    pthread_mutex_lock(&handle->lock);
    handle->preview_brightness = level;
    kr_ipc_apply_preview_cm_locked(handle);
    pthread_mutex_unlock(&handle->lock);
    return 0;
}
