#ifndef _TRAN_VIDEO_H_
#define _TRAN_VIDEO_H_
#include <stdint.h>
#include "trans.h"

enum {
    VIDEO_TRAN_SEND = 1,
    VIDEO_TRAN_RECV,
    VIDEO_TRAN_SEND_AND_RECV,
    VIDEO_TRAN_BUTT
};

typedef struct tran_video_show
{
    int display_x;
    int display_y;
    int display_w;
    int display_h;
}v_show_rct_t;

typedef struct tran_video_info_
{
    /* data */
    uint32_t rdev;              //对端设备号
    uint32_t rport;             //对端端口号
    uint32_t rport_sub;         //对端子码流端口号
    uint32_t srcdev;            //本地设备号
    uint8_t trandirection;      //传输方向
    v_show_rct_t *prect;          //显示范围
    uint8_t isdisablevideocache;    //是否禁止图像缓存
    uint8_t isdisablevdecshow;      //是否禁止解码显示
    uint8_t videoType;           //视频类型 0:主码流 1:子码流

    tran_drop_cb dropcb;	//丢包回调
	void *pdropuserdata;	//自定义数据
}v_transmit_param_t;//传输参数设置


void *plc_video_trans_start(v_transmit_param_t *pvparam);

int plc_video_trans_send(void*param, int channel, void *pdata,int dlen);

int plc_video_trans_set_displayarea(void*param, v_show_rct_t *prect);

int plc_video_enc_jpg(void*param, uint32_t width, uint32_t height, char **pjpgdata, uint32_t timeout);

void plc_video_trans_stop(void*param);

#endif//_TRAN_VIDEO_H_