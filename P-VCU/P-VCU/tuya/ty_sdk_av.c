#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <signal.h>
#include <sys/prctl.h>

#include "plog.h"
#include "tuya_iot_config.h"
#include "tuya_ipc_api.h"
#include "tuya_ipc_media.h"
#include "tuya_ring_buffer.h"
#include "tuya_ipc_p2p.h"
#include "tuya_ipc_stream_storage.h"
#include "tuya_ipc_media_stream_event.h"
#include "ty_sdk_common.h"

#include "tuyamain.h"
// #include "tal_audio.h"
// #include "tal_video_enc.h"

RING_BUFFER_USER_HANDLE_T s_ring_buffer_handles[E_IPC_STREAM_MAX] = {
    NULL,
};

// void * __video_main(void *args)
// {
//     UCHAR_T *frame_buff = NULL;
//     int bufSize = MAX_MEDIA_FRAME_SIZE;
//     int naluSeiLen = 0;
//     TAL_VENC_FRAME_T frame = {0};

//     // 视频主码流获取
//     frame_buff = (UCHAR_T *)malloc(bufSize);
//     if (NULL == frame_buff) 
//     {
//         error("malloc failed type = %d\n", E_IPC_STREAM_VIDEO_MAIN);
//         return NULL;
//     }
//     frame.pbuf = (char *)frame_buff;
//     frame.buf_size = bufSize;

//     while(1)
//     {
//         if (0 == tal_venc_get_frame(0, 0, &frame)) 
//         {

//             if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN] == NULL) 
//             {
//                 s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN] = tuya_ipc_ring_buffer_open(0, 0, E_IPC_STREAM_VIDEO_MAIN, E_RBUF_WRITE);
//             }

//             if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN]) 
//             {
//                 tuya_ipc_ring_buffer_append_data(s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN], frame_buff,
//                                                  frame.used_size + naluSeiLen, frame.frametype, frame.pts);

//             } 
//             else 
//             {
//                 error("tuya_ipc_ring_buffer_open failed,channle:%d\n", E_IPC_STREAM_VIDEO_MAIN);
//             }
//         } 
//         else 
//         {
//             usleep(10 * 1000);
//         }
//     }
//     free(frame_buff);
//     if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN] != NULL) 
//     {
//         tuya_ipc_ring_buffer_close(s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN]);
//     }
//     return NULL;
// }
 
static bool StartVideoMain(void)
{
    bool ret = false;

    if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN] == NULL) 
    {
        s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN] = tuya_ipc_ring_buffer_open(0, 0, E_IPC_STREAM_VIDEO_MAIN, E_RBUF_WRITE);
        if(s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN])
        {
            info("Start Video Main success!\n");
            ret = true;
        }
        else
        {
            info("Start Video Main fail!\n");
        }
    }
    else
        warn("Video Main is open!\n");

    return ret;
}

static void SendVideoMain(char *buf, int bufsize, int type, unsigned long long int pts)
{
    if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN]) 
    {
        MEDIA_FRAME_TYPE_E frametype = E_VIDEO_PB_FRAME;
        if(type)
            frametype = E_VIDEO_I_FRAME;
        else
            frametype = E_VIDEO_PB_FRAME;
        tuya_ipc_ring_buffer_append_data(s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN], buf,
                                            bufsize, frametype, pts);

    } 
    else 
    {
        error("tuya_ipc_ring_buffer_open failed,channle:%d\n", E_IPC_STREAM_VIDEO_MAIN);
    }
}

static void StopVideoMain(void)
{
    if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN] != NULL) 
    {
        tuya_ipc_ring_buffer_close(s_ring_buffer_handles[E_IPC_STREAM_VIDEO_MAIN]);
        info("Stop Video Main success!\n");
    }
}

// void * __video_sub(void *args)
// {
//     UCHAR_T *frame_buff = NULL;
//     int bufSize = MAX_MEDIA_FRAME_SIZE;
//     int naluSeiLen = 0;
//     TAL_VENC_FRAME_T frame = {0};

//     // 视频子码流获取
//     frame_buff = (UCHAR_T *)malloc(bufSize);
//     if (NULL == frame_buff) 
//     {
//         error("malloc failed type = %d\n", E_IPC_STREAM_VIDEO_SUB);
//         return NULL;
//     }
//     frame.pbuf = (char *)frame_buff;
//     frame.buf_size = bufSize;

//     while(1)
//     {
//         if (0 == tal_venc_get_frame(0, 1, &frame)) 
//         {

//             if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB] == NULL) 
//             {
//                 s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB] = tuya_ipc_ring_buffer_open(0, 0, E_IPC_STREAM_VIDEO_SUB, E_RBUF_WRITE);
//             }

//             if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB]) 
//             {
//                 tuya_ipc_ring_buffer_append_data(s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB], frame_buff,
//                                                  frame.used_size + naluSeiLen, frame.frametype, frame.pts);

//             } 
//             else 
//             {
//                 error("tuya_ipc_ring_buffer_open failed,channle:%d\n", E_IPC_STREAM_VIDEO_SUB);
//             }
//         } 
//         else 
//         {
//             usleep(10 * 1000);
//         }
//     }
//     free(frame_buff);
//     if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB] != NULL) 
//     {
//         tuya_ipc_ring_buffer_close(s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB]);
//     }
//     return NULL;
// }

static bool StartVideoSub(void)
{
    bool ret = false;

    if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB] == NULL) 
    {
        s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB] = tuya_ipc_ring_buffer_open(0, 0, E_IPC_STREAM_VIDEO_SUB, E_RBUF_WRITE);
        if(s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB])
        {
            info("Start Video Sub success!\n");
            ret = true;
        }
        else
        {
            info("Start Video Sub fail!\n");
        }
    }
    else
        warn("Video Sub is open!\n");

    return ret;
}

static void SendVideoSub(char *buf, int bufsize, int type, unsigned long long int pts)
{
    if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB]) 
    {
        MEDIA_FRAME_TYPE_E frametype = E_VIDEO_PB_FRAME;
        if(type)
            frametype = E_VIDEO_I_FRAME;
        else
            frametype = E_VIDEO_PB_FRAME;
        tuya_ipc_ring_buffer_append_data(s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB], buf,
                                            bufsize, frametype, pts);

    } 
    else 
    {
        error("tuya_ipc_ring_buffer_open failed,channle:%d\n", E_IPC_STREAM_VIDEO_SUB);
    }
    
}

static void StopVideoSub(void)
{
    if (s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB] != NULL) 
    {
        tuya_ipc_ring_buffer_close(s_ring_buffer_handles[E_IPC_STREAM_VIDEO_SUB]);
        info("Stop Video Sub fail!\n");
    }
}

// void * __audio_main(void *args)
// {
//     UCHAR_T *frame_buff = NULL;
//     int bufSize = 1280;
//     TAL_AUDIO_FRAME_INFO_T frame = {0};

//     // 音频获取
//     frame_buff = (UCHAR_T *)malloc(bufSize);
//     if (NULL == frame_buff) 
//     {
//         error("malloc failed type = %d\n", E_IPC_STREAM_AUDIO_MAIN);
//         return NULL;
//     }
//     frame.pbuf = (char *)frame_buff;
//     frame.buf_size = bufSize;

//     while(1)
//     {
//         if (0 ==  tal_ai_get_frame(0, 0, &frame)) 
//         {
//             if (s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN] == NULL) 
//             {
//                 s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN] = tuya_ipc_ring_buffer_open(0, 0, E_IPC_STREAM_AUDIO_MAIN, E_RBUF_WRITE);
//             }

//             if (s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN]) 
//             {
//                 tuya_ipc_ring_buffer_append_data(s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN], frame_buff, frame.used_size,
//                                                      frame.type, frame.pts);

//             }
//             else 
//             {
//                 error("tuya_ipc_ring_buffer_open failed,channle:%d\n", E_IPC_STREAM_AUDIO_MAIN);
//             }
//         } 
//         else 
//         {
//             usleep(10 * 1000);
//         }
//     }
//     free(frame_buff);
//     if (s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN] != NULL) 
//     {
//         tuya_ipc_ring_buffer_close(s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN]);
//     }
//     return NULL;
// }

bool StartTuyaVideo(void)
{
    bool main = StartVideoMain();
    bool sub = StartVideoSub();
    if(main || sub)
        return true;
    else
        return false;
}

void SendTuyVideo(char *buf, int bufsize, int type, unsigned long long int pts)
{
    SendVideoMain(buf, bufsize, type, pts);
    SendVideoSub(buf, bufsize, type, pts);
}

void StopTuyaVideo(void)
{
    StopVideoMain();
    StopVideoSub();   
}

bool StartTuyaAudio(void)
{
    bool ret = false;
    if (s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN] == NULL) 
    {
        s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN] = tuya_ipc_ring_buffer_open(0, 0, E_IPC_STREAM_AUDIO_MAIN, E_RBUF_WRITE);
        if(s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN])
        {
            ret = true;
            info("Start Tuya Audio success!\n");
        }
        else
            info("Start Tuya Audio fail!\n");
    }
    else
        warn("Tuya Audio is open!\n");
}

void SendTuyaAudio(char *buf, int bufsize, unsigned long long int pts)
{
    if (s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN]) 
    {
        tuya_ipc_ring_buffer_append_data(s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN], buf, bufsize,
                                                E_AUDIO_FRAME, pts);

    }
    else 
    {
        error("tuya_ipc_ring_buffer_open failed,channle:%d\n", E_IPC_STREAM_AUDIO_MAIN);
    }
}

void StopTuyaAudio(void)
{
    if (s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN] != NULL) 
    {
        tuya_ipc_ring_buffer_close(s_ring_buffer_handles[E_IPC_STREAM_AUDIO_MAIN]);
        info("Stop Tuya Audio success!\n");
    }
}

// VOID TUYA_IPC_av_start()
// {
//     info("create av task!!!\n"); 
//     int ret = 0;
//     ret = tal_vi_init(NULL, 1);
//     if (0 != ret) {
//         error("ty_dsp_init failed,%d\n", ret);
//         return ;
//     }

//     ret = tal_venc_init(0, NULL, 4);
//     if (0 != ret) {
//         error("ty_dsp_init failed,%d\n", ret);
//         return ;
//     }

//     ret = tal_ai_init(-1, 1);
//     if (0 != ret) {
//         error("tal_ai_init failed,%d\n", ret);
//         return ;
//     }
//     pthread_t main_pid = -1;
//     pthread_t sub_pid = -1;
//     pthread_t audio_pid = -1;
//     pthread_create(&main_pid, NULL, __video_main,NULL);   
//     pthread_create(&sub_pid, NULL, __video_sub,NULL);   
//     pthread_create(&audio_pid, NULL, __audio_main,NULL);  

//     return ; 
// }