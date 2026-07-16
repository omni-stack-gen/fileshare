#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"
#include "libdmc.h"
#include "libdmc_rtsp.h"
#include "librtsp.h"
#include "../../transport/trans.h"

#define MAX_RTSP_CHANNEL (32) /*it should be enough*/
static RTSP_HANDLE g_rtsp_server[MAX_RTSP_CHANNEL];

static unsigned int g_rtsp_port[MAX_RTSP_CHANNEL];
static unsigned int g_printed[MAX_RTSP_CHANNEL];
static media_frame * ppacket = NULL;

static int _rtsp_input_fn(int media_chn,
        int media_type,
        int media_subtype,
        unsigned long long frame_pts,
        unsigned char *frame_data,
        int frame_len,
        int frame_end_flag)
{
    if ((unsigned int)media_chn >= MAX_RTSP_CHANNEL)
        return 0;
	//printf("%s %d len %d type %x\n",__func__,__LINE__,frame_len,media_type);
    // printf("RTSP: input data, channel %d, type %x media_subtype: %x, pts %llu, len %d, end_flag %d\n",
    //        media_chn, media_type, media_subtype, frame_pts, frame_len, frame_end_flag);
    if (media_type == DMC_MEDIA_TYPE_H264)
    {
        librtsp_push_data(g_rtsp_server[media_chn], frame_data,frame_len, frame_pts, 0/*FIXME, not used yet*/, FH_VENC_H264);
    }
    else if (media_type == DMC_MEDIA_TYPE_H265)
    {
        librtsp_push_data(g_rtsp_server[media_chn], frame_data,frame_len, frame_pts, 0/*FIXME, not used yet*/, FH_VENC_H265);

        if(1)
        {
            static unsigned char seq = 0;
            static unsigned int frame_cache_len = 0;
            if (ppacket == NULL)
            {
                int maxframelen = 512*1024;
                ppacket = (media_frame*)malloc(sizeof(media_frame) + maxframelen);
            }
            int len = 0;
            int frame_type = -1;
            unsigned char * ptr = (unsigned char *)ppacket->data;

            memcpy(ptr+frame_cache_len, frame_data, frame_len);

            if(frame_end_flag==1)
            {
                len = frame_len+frame_cache_len;
                frame_type = media_subtype==1 ? 1 : 0;

                ppacket->check = 0xabcd1234;
                ppacket->bkey = frame_type;
                ppacket->timestamp = (frame_pts + 500)/1000;
                ppacket->seq = seq++;
                ppacket->streamid = 0;

                mediatrans_send(media_chn, ppacket, len + sizeof(media_frame));

                frame_cache_len = 0;
            }
            else
                frame_cache_len += frame_len;
        }

    }
    else if (media_type == DMC_MEDIA_TYPE_AUDIO)
    {
        librtsp_push_data(g_rtsp_server[media_chn], frame_data,frame_len, frame_pts, 0/*FIXME, not used yet*/, FH_SDP_LPCM_L16_R16/*FIXME*/);
    }

    if (!g_printed[media_chn])
    {
        printf("RTSP: listen on port %d ... \n", g_rtsp_port[media_chn]);
        g_printed[media_chn] = 1;
    }

    return 0;
}

int dmc_rtsp_subscribe(int rtspgrp, int port)
{
    int i;
    int curgrp;
    int code = 0x1;

    curgrp = 0;
    while(curgrp <= MAX_GRP_NUM) {
        if(rtspgrp & code) {
            for(i = curgrp * MAX_VPU_CHN_NUM; i < (curgrp + 1) * MAX_VPU_CHN_NUM; i++) {
                g_rtsp_server[i] = librtsp_start_server(RTP_TRANSPORT_TCP, port + i * 2 + MAX_VPU_CHN_NUM * MAX_GRP_NUM);
                g_printed[i] = 0;
                g_rtsp_port[i] = port + i * 2 + MAX_VPU_CHN_NUM * MAX_GRP_NUM;
            }
        }
        code <<= 1;
        curgrp++;
    }
	//LOG_PRT("%s %d \n",__func__,__LINE__);
    dmc_subscribe("RTSP", rtspgrp, DMC_MEDIA_TYPE_H264 | DMC_MEDIA_TYPE_H265 | DMC_MEDIA_TYPE_AUDIO, _rtsp_input_fn);

    return 0;
}

int dmc_rtsp_unsubscribe(void)
{
    int i;

    dmc_unsubscribe("RTSP", DMC_MEDIA_TYPE_H264 | DMC_MEDIA_TYPE_H265 | DMC_MEDIA_TYPE_AUDIO);

    for (i=0; i<MAX_RTSP_CHANNEL; i++)
    {
        if (g_rtsp_server[i])
        {
            librtsp_stop_server(g_rtsp_server[i]);
            g_rtsp_server[i] = NULL;
        }
    }

    return 0;
}
