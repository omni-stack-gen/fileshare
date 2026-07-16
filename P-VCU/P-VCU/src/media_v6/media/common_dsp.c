#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "config.h"
#include "dsp_ext/FH_typedef.h"
#include "fh_vb_mpipara.h"
#include "sample_common.h"
#include "dsp/fh_system_mpi.h"
#include "dsp/fh_vb_mpi.h"
#include "dsp/fh_vpu_mpi.h"
#include "dsp/fh_venc_mpi.h"
#include "dsp/fh_jpege_mpi.h"
#include "libdmc.h"
#include "libdmc_rtsp.h"
#include "time_helper.h"
#include "log.h"
#include "npu_com.h"

#define H265_BREATH_IP_QPDELTA 3
#define H265_BREATH_I_BITPROP  10
#define H265_BREATH_P_BITPROP  1

// #define  ENC_FRAME_WIDTH    1920//CH0_WIDTH_G0
// #define  ENC_FRAME_HEIGHT   1088//CH0_HEIGHT_G0

unsigned int ENC_FRAME_WIDTH = 1920;
unsigned int ENC_FRAME_HEIGHT = 1088;
unsigned int ENC_BIT_RATE_G0 = CH0_BIT_RATE_G0;
unsigned int ENC_FRAME_COUNT_G0 = CH0_FRAME_COUNT_G0;

#define  ENC1_FRAME_WIDTH    1024//1024//CH0_WIDTH_G0
#define  ENC1_FRAME_HEIGHT   600//600//CH0_HEIGHT_G0

struct enc_channel_info
{
    FH_SINT32 channel;
    FH_SINT32 enable;
    FH_UINT32 width;
    FH_UINT32 height;
    FH_UINT32 max_width;
    FH_UINT32 max_height;
    FH_UINT8 frame_count;
    FH_UINT8 frame_time;
    FH_UINT32 bps;
    FH_UINT32 enc_type;
    FH_UINT32 rc_type;
    FH_UINT16 breath_on;
};

extern FH_SINT32 fh_vpu_init(FH_VOID);
extern FH_SINT32 fh_vpu_close(FH_VOID);
extern FH_SINT32 sample_dmc_init(FH_CHAR *dst_ip, FH_UINT32 port);
extern FH_SINT32 sample_dmc_deinit(FH_VOID);

static FH_SINT32 g_get_stream_stop = 0;
static FH_SINT32 g_get_stream_running = 0;


void ReLoadEncParam(unsigned int width, unsigned int height, unsigned int bit_rate, unsigned int frame_count)
{
    ENC_FRAME_WIDTH = width;
    ENC_FRAME_HEIGHT = height;
    ENC_BIT_RATE_G0 = bit_rate;
    ENC_FRAME_COUNT_G0 = frame_count;
}

FH_SINT32 sample_common_media_sys_init(FH_VOID)
{
    VB_CONF_S stVbConf;
    FH_SINT32 ret;
    int i;
	FH_UINT32 size;
    FH_SYS_Exit();

    for(i=0;i<VB_MAX_USER;i++)
    {
         FH_VB_ExitModCommPool(i);
    }
    for(i=0; i<VB_MAX_POOLS; i++)
    {
         FH_VB_DestroyPool(i);
    }

    FH_VB_Exit();

    memset(&stVbConf, 0, sizeof(VB_CONF_S));

	ret = FH_VPSS_GetFrameBlkSize(CH0_WIDTH_G0, CH0_HEIGHT_G0, 1<<VPU_VOMODE_TILE224, &size);
	if (ret)
    {
        printf("[vpu] FH_VPSS_GetFrameBlkSize failed with:%x\n", ret);
        return -1;
    }
    stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkSize = size;
    stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkCnt = 3;

    printf("===========================size:%d ===========================\n", size);

    stVbConf.u32MaxPoolCnt += 1;
#if (ENC_CHANNEL_NUM>1) //VPU_VOMODE_RRGGBB
    ret = FH_VPSS_GetFrameBlkSize(CH1_WIDTH_G0, CH1_HEIGHT_G0, 1<<VPU_VOMODE_TILE224, &size);
	if (ret)
    {
        printf("[vpu] FH_VPSS_GetFrameBlkSize failed with:%x\n", ret);
        return -1;
    }

    printf("===========================222 size:%d ===========================\n", size);
    stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkSize = size;
    stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkCnt = 3;
    // #if defined(CH1_MJPEG_G0)
    // stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkCnt += 3;
    // #endif
    stVbConf.u32MaxPoolCnt += 1;
#else
    stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkSize = 1024 * 600 * 2;
    stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkCnt = 4;
    stVbConf.u32MaxPoolCnt += 1;
#endif

    ret = FH_VB_SetConf(&stVbConf);
    if (ret)
    {
        printf("[FH_VB_SetConf] failed with:%x\n", ret);
        return -1;
    }

    ret = FH_VB_Init();
    if (ret)
    {
        printf("[FH_VB_Init] failed with:%x\n", ret);
        return -1;
    }

    WR_PROC_DEV(JPEG_PROC, "frmsize_30_3000000_3000000");
    WR_PROC_DEV(JPEG_PROC, "jpgstm_12000000_32");
    WR_PROC_DEV(JPEG_PROC, "mjpgstm_12000000_32");
    WR_PROC_DEV(ENC_PROC, "stm_12000000_128");

    ret =  FH_SYS_Init();
    if(ret) {
        LOG_PRT("FH_SYS_Init failed! ret = 0x%x\n", ret);
        return ret;
    }
    FH_VENC_DeInit();
    fh_vpu_close();

    ret = fh_vpu_init();
    if(ret) {
        LOG_PRT("vpu init failed! ret = 0x%x\n", ret);
        return ret;
    }
    FH_VENC_DTS_CLK_ATTR stClk_attr={0};

	stClk_attr.workMode = 0;

    ret = FH_VENC_Init(&stClk_attr);
	if(ret){
		LOG_PRT("FH_VENC_Init failed,result[0x%x]\n",ret);
		return ret;
	}

	FH_VENC_SET_DRV_CONFIG DrvCfg;
	memset(&DrvCfg,0,sizeof(FH_VENC_SET_DRV_CONFIG));
	DrvCfg.lvd = 1;
	DrvCfg.syncMode = FH_AUTO_MODE;
	DrvCfg.startFrmType = FH_START_FRAME_AUTO;
	ret = FH_VENC_SetDrvCfg(&DrvCfg);
	if(ret){
		LOG_PRT("VENC_SetDrvCfg failed,result[0x%x]\n",ret);
		return ret;
	}

    return ret;
}

FH_SINT32 sample_common_media_sys_exit(FH_VOID)
{
    int ret32;
    int i;

    fh_vpu_close();
    FH_VENC_DeInit();

    ret32 = FH_SYS_Exit();
    if (ret32)
    {
        printf("[FH_SYS_Exit] failed with:%x\n", ret32);
        return -1;
    }

    for(i=0;i<VB_MAX_USER;i++)
    {
         FH_VB_ExitModCommPool(i);
    }
    for(i=0; i<VB_MAX_POOLS; i++)
    {
         FH_VB_DestroyPool(i);
    }

    return FH_VB_Exit();
}

FH_SINT32 sample_common_dsp_init(FH_UINT32 grpid)
{
    FH_SINT32 ret;
    FH_VPU_SIZE vi_pic;

    FH_VPU_SET_GRP_INFO grp_info = {0};
    grp_info.vi_max_size.u32Width = CH0_WIDTH_G0;
    grp_info.vi_max_size.u32Height = CH0_HEIGHT_G0;

    //创建视频输入
    ret = FH_VPSS_CreateGrp(grpid, &grp_info);
    if (ret)
    {
        printf("Error(%d - %x): FH_VPSS_CreateGrp (grp):(%d)!\n", ret, ret, grpid);
        return -1;
    }

    vi_pic.vi_size.u32Width = CH0_WIDTH_G0;//CH0_WIDTH_G0;
    vi_pic.vi_size.u32Height = CH0_HEIGHT_G0;//CH0_HEIGHT_G0
    vi_pic.crop_area.crop_en = 0;
    vi_pic.crop_area.vpu_crop_area.u32X =  0;
    vi_pic.crop_area.vpu_crop_area.u32Y = 0;
    vi_pic.crop_area.vpu_crop_area.u32Width = 2560;
    vi_pic.crop_area.vpu_crop_area.u32Height = 1440;

    //设置视频输入属性
    ret = FH_VPSS_SetViAttr(grpid, &vi_pic);
    if (ret != 0)
    {
        printf("Error(%d - %x): FH_VPSS_SetViAttr (grp):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    //使能视频处理模块 (更换工作模式时，需先调用 FH_VPSS_Disable, 然后再调用此函数。)
    return FH_VPSS_Enable(grpid, VPU_MODE_ISP);
}

FH_SINT32 sample_common_vpu_create_chan(FH_UINT32 grpid, FH_UINT32 chan_vpu, FH_UINT32 width, FH_UINT32 height, FH_UINT32 yuv_type)
{
    FH_SINT32 ret;
    FH_VPU_CHN_INFO chn_info;

	FH_VPU_CHN_CONFIG chn_attr;

    // if (chan_vpu == 0)
    // {
    //     chn_info.bgm_enable = g_vpu_grp_infos[grpid].bgm_enable;
    //     chn_info.cpy_enable = g_vpu_grp_infos[grpid].cpy_enable;
    //     chn_info.sad_enable = g_vpu_grp_infos[grpid].sad_enable;
    //     chn_info.bgm_ds = g_vpu_grp_infos[grpid].bgm_ds;
    // }
    // else
    {
        chn_info.bgm_enable = 0;
        chn_info.cpy_enable = 0;
        chn_info.sad_enable = 0;
        chn_info.bgm_ds = 0;
    }

    chn_info.chn_max_size.u32Width = width;

    chn_info.chn_max_size.u32Height = height;
    chn_info.out_mode = yuv_type;
    chn_info.support_mode = 1 << yuv_type;
    chn_info.bufnum = yuv_type==VPU_VOMODE_TILE224 ? CH0_BUFNUM_G0 : 3;//g_vpu_chn_infos[grpid][chan_vpu].bufnum;
    chn_info.max_stride = 0;

    //创建通道
    ret = FH_VPSS_CreateChn(grpid, chan_vpu, &chn_info);
    if (ret != 0)
    {
        printf("Error(%d - %x): FH_VPSS_CreateChn (grp-chn):(%d-%d)!\n", ret, ret, grpid, chan_vpu);
        return ret;
    }

	chn_attr.vpu_chn_size.u32Width = width;
    chn_attr.vpu_chn_size.u32Height = height;
    chn_attr.crop_area.crop_en = 0;
    chn_attr.crop_area.vpu_crop_area.u32X = 0;
    chn_attr.crop_area.vpu_crop_area.u32Y = 0;
    chn_attr.crop_area.vpu_crop_area.u32Width = 0;
    chn_attr.crop_area.vpu_crop_area.u32Height = 0;
    chn_attr.stride = 0;
    chn_attr.offset = 0;
    chn_attr.depth = 1;

    //设置视频处理模块通道属性。修改通道属性时不需要停止视频处理模块。但建议先停止该通道的使能。(在设置通道属性后, 包含帧率控制, 视频冻结等通道配置会失效, 需要重新设置。)
    ret = FH_VPSS_SetChnAttr(grpid, chan_vpu, &chn_attr);

    if (ret != 0)
    {
        printf("Error(%d - %x): FH_VPSS_SetChnAttr (grp-chn):(%d-%d)!\n", ret, ret, grpid, chan_vpu);
        return ret;
    }

    //设置视频处理模块通道输出组织格式
    ret = FH_VPSS_SetVOMode(grpid, chan_vpu, yuv_type);
    if (ret != 0)
    {
        printf("Error(%d - %x): FH_VPSS_SetVOMode (grp-chn):(%d-%d)!\n", ret, ret, grpid, chan_vpu);
        return ret;
    }

    //开启通道使能
    return FH_VPSS_OpenChn(grpid, chan_vpu);
}

FH_SINT32 sample_common_start_vpu(FH_VOID)
{
    int ret;
    int grploop, vpuloop;

    grploop = 0;
    vpuloop = 0;

    ret = sample_common_dsp_init(grploop);
    if (ret != 0)
    {
        printf("Error(%d - %x): sample_common_dsp_init (grp):(%d)!\n", ret, ret, grploop);
        return -1;
    }

    for (vpuloop = 0; vpuloop < ENC_CHANNEL_NUM; vpuloop++)
    {
        //ret = sample_common_vpu_create_chan(grploop, vpuloop, CH0_WIDTH_G0, CH0_HEIGHT_G0, VPU_VOMODE_TILE224);
        if(vpuloop==0)
            ret = sample_common_vpu_create_chan(grploop, vpuloop, ENC_FRAME_WIDTH, ENC_FRAME_HEIGHT, VPU_VOMODE_TILE224);
        else
             ret = sample_common_vpu_create_chan(grploop, vpuloop, ENC1_FRAME_WIDTH, ENC1_FRAME_HEIGHT, VPU_VOMODE_TILE224); //VPU_VOMODE_RRGGBB
        if (ret != 0)
        {
            printf("Error(%d - %x): sample_common_vpu_create_chan (grp-chn):(%d-%d)!\n", ret, ret, grploop, vpuloop);
            return -1;
        }
    }


    return 0;
}



FH_SINT32 sample_common_enc_create_chan(FH_UINT32 grpid, FH_UINT32 chan_enc, struct enc_channel_info *enc_info)
{
    if(!enc_info)
        return -1;
    FH_VENC_CHN_CAP cfg_vencmem;
    JPEGE_CHN_ATTR_S stJpegAttr;
	FH_UINT32 bufSize;

    cfg_vencmem.support_type = enc_info->enc_type;
    cfg_vencmem.max_size.u32Width = enc_info->max_width;
    cfg_vencmem.max_size.u32Height = enc_info->max_height;
	bufSize = cfg_vencmem.max_size.u32Width * cfg_vencmem.max_size.u32Height;

    // printf("sample_common_enc_create_chan----enc_info->enc_type:%d max_width:%d max_height:%d enc_info->width[%d] enc_info->height[%d]\n",
    //     enc_info->enc_type, enc_info->max_width, enc_info->max_height, enc_info->width, enc_info->height);

    memset(&stJpegAttr, 0, sizeof(stJpegAttr));
    stJpegAttr.u32MaxPicWidth  = enc_info->max_width;
    stJpegAttr.u32MaxPicHeight = enc_info->max_height;
    stJpegAttr.u32PicWidth     = enc_info->width;
    stJpegAttr.u32PicHeight    = enc_info->height;
    stJpegAttr.u32BufSize      =  stJpegAttr.u32PicWidth * stJpegAttr.u32PicHeight * 1;

    if(cfg_vencmem.support_type!=FH_MJPEG)
        return FH_VENC_CreateChn(grpid * MAX_VPU_CHN_NUM + chan_enc, &cfg_vencmem) && FH_VENC_SetStreamBufSize(chan_enc, bufSize);
    else
        return FH_JPEGE_CreateChn(/*grpid * MAX_VPU_CHN_NUM +*/ chan_enc, &stJpegAttr);
}

FH_VOID sample_common_set_h264_rc_vbr(struct enc_channel_info *info, FH_VENC_CHN_CONFIG *cfg)
{
    cfg->rc_attr.rc_type                          = FH_RC_H264_VBR;
    cfg->rc_attr.h264_vbr.bitrate                 = info->bps;
    cfg->rc_attr.h264_vbr.init_qp                 = 35;
    cfg->rc_attr.h264_vbr.ImaxQP                  = 48;
    cfg->rc_attr.h264_vbr.IminQP                  = 28;
    cfg->rc_attr.h264_vbr.PmaxQP                  = 48;
    cfg->rc_attr.h264_vbr.PminQP                  = 28;
    cfg->rc_attr.h264_vbr.FrameRate.frame_count   = info->frame_count;
    cfg->rc_attr.h264_vbr.FrameRate.frame_time    = info->frame_time;
    cfg->rc_attr.h264_vbr.maxrate_percent         = 200;
    cfg->rc_attr.h264_vbr.IFrmMaxBits             = 0;
    if (info->breath_on)
    {
        cfg->rc_attr.h264_vbr.IP_QPDelta          = 5;
        cfg->rc_attr.h264_vbr.I_BitProp           = 15;
        cfg->rc_attr.h264_vbr.P_BitProp           = 1;
    }
    else
    {
       /*
        [2024-12-25 by pang]
        On the XC01 chip, the default values marked are -20, -1, -1,0, but they can also be set to 0.
        The encoder will still convert them back to the default values of -20, -1, -1.
        It is important to note that the default values will only be used if all values are set to 0.
        If you want to modify one of the values, you need to set it like this:
        IP_QPDelta = 3; --->change
        I_BitProp = -1;
        P_BitProp = -1;
        fluctuate_level = 0;
       */
        cfg->rc_attr.h264_vbr.IP_QPDelta          = 0;
        cfg->rc_attr.h264_vbr.I_BitProp           = 0;
        cfg->rc_attr.h264_vbr.P_BitProp           = 0;
    }
    cfg->rc_attr.h264_vbr.fluctuate_level         = 0;
}

FH_VOID sample_common_set_h265_rc_cbr(struct enc_channel_info *info, FH_VENC_CHN_CONFIG *cfg)
{
    cfg->rc_attr.rc_type                          = FH_RC_H265_CBR;
    cfg->rc_attr.h265_cbr.bitrate                 = info->bps;
    cfg->rc_attr.h265_cbr.init_qp                 = 35;
    cfg->rc_attr.h265_cbr.FrameRate.frame_count   = info->frame_count;
    cfg->rc_attr.h265_cbr.FrameRate.frame_time    = info->frame_time;
    cfg->rc_attr.h265_cbr.maxrate_percent         = 200;
    cfg->rc_attr.h265_cbr.IFrmMaxBits             = 0;
    if (info->breath_on)
    {
        cfg->rc_attr.h265_cbr.IP_QPDelta          = H265_BREATH_IP_QPDELTA;
        cfg->rc_attr.h265_cbr.I_BitProp           = H265_BREATH_I_BITPROP;
        cfg->rc_attr.h265_cbr.P_BitProp           = H265_BREATH_P_BITPROP;
    }
    else
    {
       /*
        [2024-12-25 by pang]
        On the XC01 chip, the default values marked are -20, -1, -1,0, but they can also be set to 0.
        The encoder will still convert them back to the default values of -20, -1, -1.
        It is important to note that the default values will only be used if all values are set to 0.
        If you want to modify one of the values, you need to set it like this:
        IP_QPDelta = 3; --->change
        I_BitProp = -1;
        P_BitProp = -1;
        fluctuate_level = 0;
       */
        cfg->rc_attr.h265_cbr.IP_QPDelta          = 0;
        cfg->rc_attr.h265_cbr.I_BitProp           = 0;
        cfg->rc_attr.h265_cbr.P_BitProp           = 0;
    }
    cfg->rc_attr.h265_cbr.fluctuate_level         = 0;
}

FH_VOID sample_common_set_h265_rc_vbr(struct enc_channel_info *info, FH_VENC_CHN_CONFIG *cfg)
{
    cfg->rc_attr.rc_type                          = FH_RC_H265_VBR;
    cfg->rc_attr.h265_vbr.bitrate                 = info->bps;
    cfg->rc_attr.h265_vbr.init_qp                 = 35;
    // cfg->rc_attr.h265_vbr.ImaxQP                  = 50;
    // cfg->rc_attr.h265_vbr.IminQP                  = 28;
    // cfg->rc_attr.h265_vbr.PmaxQP                  = 50;
    // cfg->rc_attr.h265_vbr.PminQP                  = 28;
    cfg->rc_attr.h265_vbr.ImaxQP                  = 42;
    cfg->rc_attr.h265_vbr.IminQP                  = 28;
    cfg->rc_attr.h265_vbr.PmaxQP                  = 42;
    cfg->rc_attr.h265_vbr.PminQP                  = 28;
    cfg->rc_attr.h265_vbr.FrameRate.frame_count   = info->frame_count;
    cfg->rc_attr.h265_vbr.FrameRate.frame_time    = info->frame_time;
    cfg->rc_attr.h265_vbr.maxrate_percent         = 200;
    cfg->rc_attr.h265_vbr.IFrmMaxBits             = 0;
    if (info->breath_on)
    {
        // cfg->rc_attr.h265_vbr.IP_QPDelta          = H265_BREATH_IP_QPDELTA;
        // cfg->rc_attr.h265_vbr.I_BitProp           = H265_BREATH_I_BITPROP;
        // cfg->rc_attr.h265_vbr.P_BitProp           = H265_BREATH_P_BITPROP;
    }
    else
    {
       /*
        [2024-12-25 by pang]
        On the XC01 chip, the default values marked are -20, -1, -1,0, but they can also be set to 0.
        The encoder will still convert them back to the default values of -20, -1, -1.
        It is important to note that the default values will only be used if all values are set to 0.
        If you want to modify one of the values, you need to set it like this:
        IP_QPDelta = 3; --->change
        I_BitProp = -1;
        P_BitProp = -1;
        fluctuate_level = 0;
       */
        cfg->rc_attr.h265_vbr.IP_QPDelta          = 0;
        cfg->rc_attr.h265_vbr.I_BitProp           = 0;
        cfg->rc_attr.h265_vbr.P_BitProp           = 0;
    }
    cfg->rc_attr.h265_vbr.fluctuate_level         = 0;
}

FH_VOID sample_common_set_h265_rc_avbr(struct enc_channel_info *info, FH_VENC_CHN_CONFIG *cfg)
{
    cfg->rc_attr.rc_type                          = FH_RC_H265_AVBR;
    cfg->rc_attr.h265_avbr.bitrate                = info->bps;
    cfg->rc_attr.h265_avbr.init_qp                = 35;
    cfg->rc_attr.h265_avbr.ImaxQP                 = 51;
    cfg->rc_attr.h265_avbr.IminQP                 = 28;
    cfg->rc_attr.h265_avbr.PmaxQP                 = 51;
    cfg->rc_attr.h265_avbr.PminQP                 = 28;
    cfg->rc_attr.h265_avbr.FrameRate.frame_count  = info->frame_count;
    cfg->rc_attr.h265_avbr.FrameRate.frame_time   = info->frame_time;
    cfg->rc_attr.h265_avbr.maxrate_percent        = 200;
    cfg->rc_attr.h265_avbr.IFrmMaxBits            = 0;
    if (info->breath_on)
    {
        cfg->rc_attr.h265_avbr.IP_QPDelta         = H265_BREATH_IP_QPDELTA;
        cfg->rc_attr.h265_avbr.I_BitProp          = H265_BREATH_I_BITPROP;
        cfg->rc_attr.h265_avbr.P_BitProp          = H265_BREATH_P_BITPROP;
    }
    else
    {
       /*
        [2024-12-25 by pang]
        On the XC01 chip, the default values marked are -20, -1, -1,0, but they can also be set to 0.
        The encoder will still convert them back to the default values of -20, -1, -1.
        It is important to note that the default values will only be used if all values are set to 0.
        If you want to modify one of the values, you need to set it like this:
        IP_QPDelta = 3; --->change
        I_BitProp = -1;
        P_BitProp = -1;
        fluctuate_level = 0;
       */
        cfg->rc_attr.h265_avbr.IP_QPDelta         = 0;
        cfg->rc_attr.h265_avbr.I_BitProp          = 0;
        cfg->rc_attr.h265_avbr.P_BitProp          = 0;
    }
    cfg->rc_attr.h265_avbr.fluctuate_level        = 0;
    cfg->rc_attr.h265_avbr.stillrate_percent      = 30;
    cfg->rc_attr.h265_avbr.maxstillqp             = 34;
}


FH_SINT32 sample_common_vpu_bind_to_enc(FH_UINT32 grpid, FH_UINT32 chan_vpu, FH_UINT32 chan_enc)
{
    FH_SINT32 ret;
    FH_BIND_INFO src, dst;
#ifndef ENABLE_AOV //AOV的旋转在arc端跑
    /* 设置编码旋转角度 */
    FH_ROTATE_OPS rotate_info = FH_RO_OPS_180;

    ret = FH_VPSS_SetVORotate(grpid , grpid * MAX_VPU_CHN_NUM + chan_enc, rotate_info); /* yuv vofmt为tile时，需要该设置 */
    if (ret != 0)
    {
        printf("Error(%d - %x): FH_VPSS_SetVORotate\n", ret, ret);
    }

    ret = FH_VENC_SetRotate(grpid * MAX_VPU_CHN_NUM + chan_enc, rotate_info);
    if (ret != 0)
    {
        printf("Error(%d - %x): FH_VENC_SetRotate\n", ret, ret);
    }
#endif
    ret = FH_VENC_StartRecvPic(grpid * MAX_VPU_CHN_NUM + chan_enc);
    if (ret != 0)
    {
        printf("Error(%d-%x): FH_VENC_StartRecvPic!\n", ret, ret);
        return ret;
    }

    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = grpid;
    src.chn_id = chan_vpu;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = grpid * MAX_VPU_CHN_NUM + chan_enc;
	printf("----VPU to ENC vpuloop[%d] veuloop[%d]\n", src.chn_id, dst.chn_id);

    FH_SYS_UnBindbyDst(dst);
    return FH_SYS_Bind(src, dst);
}


FH_SINT32 sample_common_vpu_unbind_to_enc(FH_UINT32 grpid, FH_UINT32 chan_vpu, FH_UINT32 chan_enc)
{
    FH_BIND_INFO src, dst;

//    ret = FH_VENC_StartRecvPic(/*grpid * MAX_VPU_CHN_NUM +*/ chan_enc);
//    if (ret != 0)
//    {
//        printf("Error(%d-%x): FH_VENC_StartRecvPic!\n", ret, ret);
//        return ret;
//    }

    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = grpid;
    src.chn_id = chan_vpu;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = grpid * MAX_VPU_CHN_NUM + chan_enc;

    return FH_SYS_UnBind(src, dst);

}

/* 设置码控类型和参数 */

static FH_SINT32 sample_common_set_rc_type(struct enc_channel_info *info, FH_VENC_CHN_CONFIG *cfg)
{
    FH_SINT32 ret = 0;
    FH_UINT32 rc_type = info->rc_type;

    switch (rc_type)
    {
    case FH_RC_H264_FIXQP:
    {
        //sample_common_set_h264_rc_fixqp(info, cfg);
        break;
    }
    case FH_RC_H264_VBR:
    {
        sample_common_set_h264_rc_vbr(info, cfg);
        break;
    }
    case FH_RC_H264_CBR:
    {
        //sample_common_set_h264_rc_cbr(info, cfg);
        break;
    }
    case FH_RC_H264_AVBR:
    {
        //sample_common_set_h264_rc_avbr(info, cfg);
        break;
    }
    case FH_RC_H264_CVBR:
    {
        //sample_common_set_h264_rc_cvbr(info, cfg);
        break;
    }

    case FH_RC_H265_FIXQP:
    {
        //sample_common_set_h265_rc_fixqp(info, cfg);
        break;
    }
    case FH_RC_H265_VBR:
    {
        sample_common_set_h265_rc_vbr(info, cfg);
        break;
    }
    case FH_RC_H265_CBR:
    {
        sample_common_set_h265_rc_cbr(info, cfg);
        break;
    }
    case FH_RC_H265_CVBR:
    {
        //sample_common_set_h265_rc_cvbr(info, cfg);
        break;
    }
    case FH_RC_H265_AVBR:
    {
        sample_common_set_h265_rc_avbr(info, cfg);
        break;
    }

    case FH_RC_H264_QVBR:
    {
        //sample_common_set_h264_rc_qvbr(info, cfg);
        break;
    }
    case FH_RC_H265_QVBR:
    {
        //sample_common_set_h265_rc_qvbr(info, cfg);
        break;
    }

    case FH_RC_MJPEG_FIXQP:
    {
        //sample_common_set_mjpeg_rc_fixqp(info, cfg);
        break;
    }
    case FH_RC_MJPEG_VBR:
    {
        //sample_common_set_mjpeg_rc_vbr(info, cfg);
        break;
    }
    default:
    {
        ret = -1;
        break;
    }
    }

    return ret;
}

/* 设置H264编码通道 */
static FH_SINT32 sample_common_set_h264_chan(FH_UINT32 grpid, FH_UINT32 chan, struct enc_channel_info *info)
{
    FH_VENC_CHN_CONFIG cfg_param;

    cfg_param.chn_attr.enc_type = FH_NORMAL_H264;
    cfg_param.chn_attr.h264_attr.profile = H264_PROFILE_MAIN;
    cfg_param.chn_attr.h264_attr.i_frame_intterval = 50;
    cfg_param.chn_attr.h264_attr.size.u32Width = info->width;
    cfg_param.chn_attr.h264_attr.size.u32Height = info->height;

    if (sample_common_set_rc_type(info, &cfg_param))
    {
        sample_common_set_h264_rc_vbr(info, &cfg_param);
    }

    return FH_VENC_SetChnAttr(grpid * MAX_VPU_CHN_NUM + chan, &cfg_param);
}

/* 设置H265编码通道 */
static FH_SINT32 sample_common_set_h265_chan(FH_UINT32 grpid, FH_UINT32 chan, struct enc_channel_info *info)
{
	FH_SINT32 s32Ret = 0;
    FH_VENC_CHN_CONFIG cfg_param;

    cfg_param.chn_attr.enc_type = FH_NORMAL_H265;
    cfg_param.chn_attr.h265_attr.profile = H265_PROFILE_MAIN;
    cfg_param.chn_attr.h265_attr.i_frame_intterval = info->frame_count*2;
    cfg_param.chn_attr.h265_attr.size.u32Width = info->width;
    cfg_param.chn_attr.h265_attr.size.u32Height = info->height;

    if (sample_common_set_rc_type(info, &cfg_param))
    {
        sample_common_set_h265_rc_vbr(info, &cfg_param);
    }

    s32Ret =  FH_VENC_SetChnAttr(grpid * MAX_VPU_CHN_NUM + chan, &cfg_param);

	return s32Ret;
}

FH_SINT32 sample_common_enc_set_chan(FH_UINT32 grpid, FH_UINT32 chan_enc, struct enc_channel_info *info)
{
    FH_SINT32 ret = 0;

    switch (info->enc_type)
    {
    case FH_NORMAL_H264:
        ret = sample_common_set_h264_chan(grpid, chan_enc, info);
        break;

    case FH_SMART_H264:
        //ret = sample_common_set_s264_chan(grpid, chan_enc, info);
        break;

    case FH_NORMAL_H265:
        ret = sample_common_set_h265_chan(grpid, chan_enc, info);
        break;

    case FH_SMART_H265:
        //ret = sample_common_set_s265_chan(grpid, chan_enc, info);
        break;

    case FH_MJPEG:
        //ret = sample_common_set_mjpeg_chan(grpid, chan_enc, info);
        break;

    case FH_JPEG:
        //ret = sample_common_set_jpeg_chan(grpid, chan_enc, info);
        break;

    default:
        ret = 0;
        break;
    }

    if (ret)
    {
        printf("Error: set encode chan failed %d(%x)\n", ret, ret);
        return ret;
    }



    return ret;
}

FH_SINT32 sample_common_start_enc(FH_VOID)
{
    int ret;
    int grploop=0, encloop=0;
    // struct enc_channel_info *penc;
    //struct grp_vpu_info *pgrp;

    printf("################### sample_common_start_enc ###############\n");


    struct enc_channel_info enc_info_array[ENC_CHANNEL_NUM]  = {
        {
            .enc_type = FH_NORMAL_H265,

            .enable = 1,
            .channel = 0,
            .width = ENC_FRAME_WIDTH,//CH0_WIDTH_G0,
            .height = ENC_FRAME_HEIGHT,//CH0_HEIGHT_G0,

            .max_width = CH0_MAX_WIDTH_G0,
            .max_height = CH0_MAX_HEIGHT_G0,

            .frame_count = ENC_FRAME_COUNT_G0,
            .frame_time = CH0_FRAME_TIME_G0,
            .bps = ENC_BIT_RATE_G0,
            .enc_type = FH_NORMAL_H265,
            .rc_type = FH_RC_H265_VBR,
            .breath_on = 0,
        },
#if (ENC_CHANNEL_NUM>1)
        {
            .enc_type = FH_NORMAL_H265,

            .enable = 1,
            .channel = 0,
            .width = ENC1_FRAME_WIDTH,//CH0_WIDTH_G0,
            .height = ENC1_FRAME_HEIGHT,//CH0_HEIGHT_G0,

            .max_width = CH1_MAX_WIDTH_G0,
            .max_height = CH1_MAX_HEIGHT_G0,

            .frame_count = ENC_FRAME_COUNT_G0,
            .frame_time = CH0_FRAME_TIME_G0,
            .bps = CH1_BIT_RATE_G0,
            .enc_type = FH_NORMAL_H265,
            .rc_type = FH_RC_H265_VBR,
            .breath_on = 0,
        },
#endif
    };


    struct enc_channel_info *penc={0};
    for (int i = 0; i < ENC_CHANNEL_NUM; i++)
    {
        penc = &enc_info_array[i];
        encloop = i;
        ret = sample_common_enc_create_chan(grploop, encloop, penc);
        if (ret != 0)
        {
            printf("Error(%d - %x): sample_common_enc_create_chan (grp-chn):(%d-%d)!\n", ret, ret, grploop, encloop);
            return -1;
        }

        ret = sample_common_enc_set_chan(grploop, encloop, penc);
        if (ret != 0)
        {
            printf("Error(%d - %x): sample_common_enc_set_chan (grp-chn):(%d-%d)!\n", ret, ret, grploop, encloop);
            return -1;
        }
    }


    return 0;
}

FH_SINT32 sample_common_stop_enc(FH_VOID)
{
    int ret;
    int grploop=0, encloop=0;
    //struct enc_channel_info *penc;
    //struct grp_vpu_info *pgrp;

    //for (grploop = 0; grploop < MAX_GRP_NUM; grploop++)
    {
        //pgrp = &g_vpu_grp_infos[grploop];
        //if (pgrp->enable)
        {
            for (encloop = 0; encloop < ENC_CHANNEL_NUM; encloop++)
            {
                //penc = &g_enc_chn_infos[grploop][encloop];
                //if (penc->enable)
                {
                    ret = FH_VENC_StopRecvPic(grploop * MAX_VPU_CHN_NUM + encloop);
                    if (ret != 0)
                    {
                        printf("Error(%d - %x): FH_VENC_StopRecvPic chn:(%d)!\n", ret, ret, grploop * MAX_VPU_CHN_NUM + encloop);
                        return -1;
                    }

                    ret = FH_VENC_DestroyChn(grploop * MAX_VPU_CHN_NUM + encloop);
                    if (ret != 0)
                    {
                        printf("Error(%d - %x): FH_VENC_DestroyChn chn:(%d)!\n", ret, ret, grploop * MAX_VPU_CHN_NUM + encloop);
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}


static FH_SINT32 g_jpeg_chn = JPEGE_MAX_CHN_NUM-1;
FH_SINT32 sample_common_start_mjpeg(FH_VOID)
{
    int ret;
    int grploop=0, jpegloop=0;
    struct enc_channel_info myjpeg = {0};

    FH_BIND_INFO src, dst;
   // int bjpeg_start = 0;

    {
        myjpeg.enc_type = FH_MJPEG;
        myjpeg.enable = 1;
        myjpeg.channel = g_jpeg_chn;

        myjpeg.width = 640;
        myjpeg.height = 480;
        myjpeg.max_width = CH0_MAX_WIDTH_G0;
        myjpeg.max_height = CH0_MAX_HEIGHT_G0;
        myjpeg.frame_count = ENC_FRAME_COUNT_G0;
        myjpeg.frame_time = CH0_FRAME_TIME_G0;
        myjpeg.bps = CH0_BIT_RATE_G0;
        myjpeg.rc_type = FH_RC_MJPEG_CBR;

        {

            ret = sample_common_enc_create_chan(0, g_jpeg_chn, &myjpeg);
            if (ret != 0)
            {
                printf("Error(%d - %x): jpeg sample_common_enc_create_chan (chn):(%d)!\n", ret, ret, g_jpeg_chn);
                return -1;
            }
            ret = FH_JPEGE_StartRecvPic(g_jpeg_chn);
            if (ret != 0)
            {
                printf("Error(%d - %x): jpeg FH_VENC_StartRecvPic (chn):(%d)!\n", ret, ret, g_jpeg_chn);
                return -1;
            }


            {
                src.obj_id = FH_OBJ_VPU_VO;
                src.dev_id = grploop;
                src.chn_id = jpegloop;
                dst.obj_id = FH_OBJ_JPEG;
                dst.dev_id = 0;
                dst.chn_id = g_jpeg_chn;
                ret = FH_SYS_Bind(src, dst);
                if (ret != 0)
                {
                    printf("Error(%d - %x): FH_SYS_Bind VPU to JPEG(%d to %d)\n", ret, ret, grploop * MAX_VPU_CHN_NUM + jpegloop, g_jpeg_chn);

                    FH_JPEGE_StopRecvPic(g_jpeg_chn);
                    FH_JPEGE_DestroyChn(g_jpeg_chn);
                    return -1;
                }
            }
        }
    }

    return 0;
}


FH_SINT32 sample_common_stop_mjpeg_gend_stream(FH_VOID)
{
    FH_BIND_INFO src, dst;
    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = 0;
    src.chn_id = 0;
    dst.obj_id = FH_OBJ_JPEG;
    dst.dev_id = 0;
    dst.chn_id = g_jpeg_chn;

    FH_SYS_UnBind(src, dst);
    FH_JPEGE_StopRecvPic(g_jpeg_chn);
    FH_JPEGE_DestroyChn(g_jpeg_chn);

    return 0;
}

FH_SINT32 sample_common_start_mjpeg_gend_stream(FH_CHAR *pFileName)
{
    FH_SINT32 s32Ret = 0;

    JPEGE_STREAM_S stStream;

    memset(&stStream, 0, sizeof(stStream));
    stStream.s32MilliSec = 100;
    s32Ret = FH_JPEGE_GetStream(g_jpeg_chn, &stStream);

    if (s32Ret == 0) {
        printf("====================== 1 len:%d \n", stStream.u32Len);
        if(pFileName)
        {
            FILE *fp = fopen(pFileName, "wb");
            if(fp)
            {
                fwrite(stStream.pu8Addr, 1, stStream.u32Len, fp);
                fclose(fp);
            }
        }
        FH_JPEGE_ReleaseStream(g_jpeg_chn, &stStream);
    }
    else{
        printf("Error(%d - %x): FH_JPEGE_GetStream failed!\n", s32Ret, s32Ret);
    }

    return s32Ret;
}

//=-=====================================
void AovVideoMutexLock(bool block);
FH_VOID *sample_common_get_stream_proc(FH_VOID *arg)
{
    FH_SINT32 ret, i;
    FH_SINT32 end_flag;
    FH_SINT32 subtype;
    FH_VENC_STREAM stream;
    FH_SINT32 *stop = (FH_SINT32 *)arg;
//    FH_SINT32 mediaType = 0;

    FH_UINT8 *pVirAddrNalu[MAX_NALU_CNT];

    prctl(PR_SET_NAME, "demo_get_stream");

    FH_FRAMERATE frameRate = {0};
    frameRate.frame_count = ENC_FRAME_COUNT_G0;
    frameRate.frame_time = 1; // frameRate.frame_count;
    FH_VPSS_SetFramectrl(0, 0, &frameRate);
#if (ENC_CHANNEL_NUM>1)
    FH_VPSS_SetFramectrl(0, 1, &frameRate);
#endif
    FH_VENC_CHN_CONFIG venc_cfg = {0};
    FH_VENC_GetChnAttr(0, &venc_cfg);
//    do{
//    	ret = FH_VENC_GetChnStream_Timeout(0, &stream, 0);
//	}while(ret==0);
#ifndef NDEBUG
    static int eindex = 0;
    char filename[128] = {0};
    sprintf(filename, "./stream/stream0_%d.h265", eindex);

    FILE *fp = fopen(filename, "wb");
    FILE *fp1 = NULL;//fopen("./stream1.h265", "w+");
#if (ENC_CHANNEL_NUM>1)
    sprintf(filename, "./stream/stream1_%d.h265", eindex);
    fp1 = fopen(filename, "wb");
#endif //  (ENC_CHANNEL_NUM>1)
    printf("Readly to recvice frame fp:%p\n", fp);
    eindex++;
#endif
    unsigned int ltick = time(NULL);
    int isFirst = 1;
    while (*stop == 0)
    {
        //WR_PROC_DEV(TRACE_PROC, "timing_GetStream_START");

    	/*阻塞模式下,获取一帧H264或者H265数据*/
        //ret = FH_VENC_GetStream_Block(FH_STREAM_ALL & (~(FH_STREAM_JPEG)), &stream);
//        ret = FH_VENC_GetChnStream_Timeout(0, &stream, 1000);
       // WR_PROC_DEV(TRACE_PROC, "timing_EncBlkFinish_xxx");
        AovVideoMutexLock(true);
        ret = FH_VENC_GetStream_Block(FH_STREAM_ALL&(~(FH_STREAM_JPEG | FH_STREAM_MJPEG)),&stream);
        AovVideoMutexLock(false);
        if (ret != 0)
        {
            LOG_PRT("Error(%d - %x) %llu: FH_VENC_GetStream_Block failed!\n", ret, ret, time(NULL));
            usleep(1000);
            continue;
        }
        if(isFirst)
        {
            isFirst = 0;
            unsigned int dtick = time(NULL);
            printf("[%u] Get stream first frame, dtick:%d\n", time_relative_ms(), dtick - ltick);
            ltick = dtick;
        }

         switch(stream.stmtype) {
            case FH_STREAM_H265:
//                mediaType = DMC_MEDIA_TYPE_H265;
                break;
            case FH_STREAM_H264:
//                mediaType = DMC_MEDIA_TYPE_H264;
                break;
            case FH_STREAM_JPEG:
            case FH_STREAM_MJPEG:
//                mediaType = DMC_MEDIA_TYPE_H265;
                break;
            default:
//                mediaType = DMC_MEDIA_TYPE_H264;
                printf("media type error!\r\n");
        }
	    //printf("stream.chan:%d stream.stmtype %x,timestamp %lld\n",stream.chan,stream.stmtype,stream.h265_stream.time_stamp);
		/*获取到一帧H264数据,按照下面的方式处理*/
        if ((stream.stmtype == FH_STREAM_H264) || (0 == stream.stmtype))
        {
            subtype = stream.h264_stream.frame_type == FH_FRAME_I ? DMC_MEDIA_SUBTYPE_IFRAME : DMC_MEDIA_SUBTYPE_PFRAME;
            for (i = 0; i < stream.h264_stream.nalu_cnt; i++)
            {
                pVirAddrNalu[i] = stream.h264_stream.nalu[i].start;


            	end_flag = (i == (stream.h264_stream.nalu_cnt - 1)) ? 1 : 0;
                dmc_input(stream.chan/5,
                          DMC_MEDIA_TYPE_H264,
                          subtype,
                          stream.h264_stream.time_stamp,
                          pVirAddrNalu[i],
                          stream.h264_stream.nalu[i].length,
                          end_flag);
                //printf("end_flag:%d\n", end_flag);
            }
        }
        /*获取到一帧H265数据,按照下面的方式处理*/
        else if (stream.stmtype == FH_STREAM_H265)
        {
            //printf("===============chan:%d subtype:%d nalu_cnt:%d\n", stream.chan, subtype, stream.h265_stream.nalu_cnt);
            static unsigned int findex = 0;
            findex++;
            subtype = stream.h265_stream.frame_type == FH_FRAME_I ? DMC_MEDIA_SUBTYPE_IFRAME : DMC_MEDIA_SUBTYPE_PFRAME;
            //if(stream.h265_stream.frame_type == FH_FRAME_I)
            //LOG_PRT("stream.stmtype %x,timestamp %lld\n", stream.stmtype, stream.h265_stream.time_stamp);
            // memset(&frameRate, 0, sizeof(FH_FRAMERATE));
            if(findex%150 ==0 && 0)
            {
                FH_VENC_GetChnAttr(0, &venc_cfg);
                printf("findex:%d, FrameRate:%d  frame_count:%d\n", findex, venc_cfg.rc_attr.h265_vbr.bitrate, venc_cfg.rc_attr.h265_vbr.FrameRate.frame_count);
                // venc_cfg.rc_attr.h265_vbr.bitrate+= 1024*128;
                // FH_VENC_SetChnAttr(0, &venc_cfg);
                static int icrop = 0;
                //icrop = 1 - icrop;
                icrop++;
                if(icrop > 2)
                {
                    icrop = 0;
                }

                int w_h_array[][4] = {
                    {320, 180, 1920, 1080},
                    {640, 360, 1280, 720},
                    {0, 0, 2560, 1440},
                };
                FH_VPU_CROP stVpucropinfo = {0};
                stVpucropinfo.crop_en = 1;
                stVpucropinfo.vpu_crop_area.u32X = w_h_array[icrop][0];
                stVpucropinfo.vpu_crop_area.u32Y = w_h_array[icrop][1];
                stVpucropinfo.vpu_crop_area.u32Width = w_h_array[icrop][2];
                stVpucropinfo.vpu_crop_area.u32Height = w_h_array[icrop][3];
                FH_VPSS_SetChnCrop(0, 0, VPU_CROP_SCALER, &stVpucropinfo);

                printf("icrop:%d, x:%d y:%d  w:%d h:%d\n", icrop, w_h_array[icrop][0], w_h_array[icrop][1], w_h_array[icrop][2], w_h_array[icrop][3]);
                FH_VPSS_GetFrameRate(0, 0, &frameRate);
                printf("<%06llu: %03d>=============== subtype:%d frame_count:%d frame_time:%d\n", time(NULL), findex, subtype, frameRate.frame_count, frameRate.frame_time);
            }
            // FH_VPSS_GetFrameRate(0, 0, &frameRate);
            // printf("<%06llu: %03d>=============== subtype:%d frame_count:%d frame_time:%d\n", time(NULL), findex, subtype, frameRate.frame_count, frameRate.frame_time);
            for (i = 0; i < stream.h265_stream.nalu_cnt; i++)
			{
                pVirAddrNalu[i] = stream.h265_stream.nalu[i].start;
            	end_flag = (i == (stream.h265_stream.nalu_cnt - 1)) ? 1 : 0;

                dmc_input(stream.chan,
			    		  DMC_MEDIA_TYPE_H265,
			    		  subtype,
			    		  stream.h265_stream.time_stamp,
		    		      pVirAddrNalu[i],
		    		      stream.h265_stream.nalu[i].length,
		    		      end_flag);
#ifndef NDEBUG
                if(fp && stream.chan==0)
                {
                    fwrite(pVirAddrNalu[i], 1, stream.h265_stream.nalu[i].length, fp);
                }

                if(fp1 && stream.chan==1)
                {
                    fwrite(pVirAddrNalu[i], 1, stream.h265_stream.nalu[i].length, fp1);
                }
#endif
			}
        }



		/*必须和FH_VENC_GetStream配套调用,以释放码流资源*/
        ret = FH_VENC_ReleaseStream(&stream);
        if(ret)
        {
            LOG_PRT("Error(%d - %x): FH_VENC_ReleaseStream failed for chan(%d)!\n", ret, ret, stream.chan);
        }

        //WR_PROC_DEV(TRACE_PROC, "timing_GetStream_END");
    }

    LOG_PRT("sample_common_get_stream_proc exit %d: %llu\n", *stop, time(NULL));
    *stop = 0;
#ifndef NDEBUG
    if(fp)
    {
        fclose(fp);
        fp = NULL;
    }
    if(fp1)
    {
        fclose(fp1);
        fp1 = NULL;
    }
#endif
    return NULL;
}


FH_SINT32 sample_common_start_get_stream(FH_VOID)
{
//    FH_SINT32 grploop;
//    struct grp_vpu_info grp_info;
    pthread_attr_t attr;
    pthread_t thread_stream;
    struct sched_param param;

	g_get_stream_running = 1;
	g_get_stream_stop = 0;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, 3 * 1024);

	// pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); /* 子线程默认继承父线程调度策略，此处配为分离式，即不继承父线程 */
	pthread_attr_setschedpolicy(&attr, SCHED_RR);

//				  param.sched_priority = 55;
#ifdef __RTTHREAD_OS__
	param.sched_priority = 130;
#endif
	pthread_attr_setschedparam(&attr, &param);
	pthread_create(&thread_stream, &attr, sample_common_get_stream_proc, &g_get_stream_stop);
	pthread_attr_destroy(&attr);

    return 0;
}

FH_SINT32 sample_common_stop_get_stream(FH_VOID)
{
    if (g_get_stream_running)
    {
        /*让获取码流的线程退出,置退出标记*/
        g_get_stream_stop = 1;

        /*等待获取码流的线程退出*/
        while (g_get_stream_stop != 0)
            usleep(10 * 1000);

        g_get_stream_running = 0;
    }

    return 0;
}

FH_SINT32 sample_common_dmc_init(FH_CHAR *dst_ip, FH_UINT32 port)
{
    return sample_dmc_init(dst_ip, port);
}

FH_SINT32 sample_common_dmc_deinit(FH_VOID)
{
    return sample_dmc_deinit();
}

FH_SINT32 sample_dmc_init(FH_CHAR *dst_ip, FH_UINT32 port)
{

    FH_SINT32 rtspgrp= 0x01;


    dmc_init();

    dmc_rtsp_subscribe(rtspgrp, port);

    return 0;
}

FH_SINT32 sample_dmc_deinit(FH_VOID)
{
    dmc_rtsp_unsubscribe();

    // dmc_pes_unsubscribe();

    // dmc_record_unsubscribe();

    // dmc_http_mjpeg_unsubscribe();

    dmc_deinit();

    return 0;
}

void RequestIFrame(void)
{
    static uint32_t last_tick = 0;
    uint32_t ucur_tick = time_relative_ms();
    //请求I帧间隔要大于0.5秒，防止快速重传I帧导致数据量过大，影响传输质量
    if(ucur_tick - last_tick > 500)
    {
        FH_VENC_RequestIDR(0);
#if (ENC_CHANNEL_NUM>1)
        FH_VENC_RequestIDR(1);
#endif
        last_tick = ucur_tick;
    }
    else{
        LOGW("RequestIFrame too fast, last_tick:%u, ucur_tick:%u\n", last_tick, ucur_tick);
    }
}

void RequestCropFrame(unsigned int iscrop)
{

    FH_VPU_CROP stVpucropinfo = {0};
    stVpucropinfo.crop_en = iscrop;
    stVpucropinfo.vpu_crop_area.u32X = 320;
    stVpucropinfo.vpu_crop_area.u32Y = 180;
    stVpucropinfo.vpu_crop_area.u32Width = 1920;
    stVpucropinfo.vpu_crop_area.u32Height = 1080;
    FH_VPSS_SetChnCrop(0, 0, VPU_CROP_SCALER, &stVpucropinfo);

    return;
}

#if defined(FH_NN_ENABLE)
FH_SINT32 sample_nn_chan_create(FH_VOID)
{
    FH_SINT32 ret;
    FH_VPU_CHN_INFO chn_info;
    FH_VPU_CHN_CONFIG chn_attr;
    int grploop;

    for (grploop = 0; grploop < 1; grploop++)
    {
        memset(&chn_info, 0, sizeof(FH_VPU_CHN_INFO));
        memset(&chn_attr, 0, sizeof(FH_VPU_CHN_CONFIG));

        chn_info.bgm_enable = 0;
        chn_info.cpy_enable = 0;
        chn_info.sad_enable = 0;
        chn_info.bgm_ds = 0;

        chn_info.chn_max_size.u32Width = NN_CHN_WIDTH;
        chn_info.chn_max_size.u32Height = NN_CHN_HEIGHT;
        chn_info.out_mode = CH3_YUV_TYPE_G0;//VPU_VOMODE_SCAN;
        chn_info.support_mode = 1 << CH3_YUV_TYPE_G0;//VPU_VOMODE_SCAN;
        chn_info.bufnum = 3;
        chn_info.max_stride = 0;

        ret = FH_VPSS_CreateChn(grploop, 3, &chn_info);
        if (ret != 0)
        {
            printf("Error(%d - %x): FH_VPSS_CreateChn\n", ret, ret);
            return ret;
        }

        chn_attr.vpu_chn_size.u32Width = NN_CHN_WIDTH;
        chn_attr.vpu_chn_size.u32Height = NN_CHN_HEIGHT;
        chn_attr.crop_area.crop_en = 0;
        chn_attr.crop_area.vpu_crop_area.u32X = 0;
        chn_attr.crop_area.vpu_crop_area.u32Y = 0;
        chn_attr.crop_area.vpu_crop_area.u32Width = 0;
        chn_attr.crop_area.vpu_crop_area.u32Height = 0;
        chn_attr.stride = 0;
        chn_attr.offset = 0;
        chn_attr.depth = 1;
        ret = FH_VPSS_SetChnAttr(grploop, 3, &chn_attr);
        if (ret)
        {
            printf("Error(%d - %x): FH_VPSS_SetChnAttr\n", ret, ret);
            return ret;
        }

        ret = FH_VPSS_SetVOMode(grploop, 3, CH3_YUV_TYPE_G0);//VPU_VOMODE_SCAN);
        if (ret)
        {
            printf("Error(%d - %x): FH_VPSS_SetVOMode\n", ret, ret);
            return ret;
        }

        ret = FH_VPSS_OpenChn(grploop, 3);
        if (ret)
        {
            printf("Error(%d - %x): FH_VPSS_SetVOMode\n", ret, ret);
            return ret;
        }
    }
    return ret;
}
#endif