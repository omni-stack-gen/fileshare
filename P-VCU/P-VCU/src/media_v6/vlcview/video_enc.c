#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <dsp_ext/FH_typedef.h>
#include "vlcview.h"

#include "config.h"
#include "sample_common_isp.h"
#include "npu_com.h"
#include "fh_vb_mpipara.h"
#include "sample_common.h"
#include "dsp/fh_system_mpi.h"
#include "dsp/fh_venc_mpi.h"
#include "dsp/fh_vb_mpi.h"
#include "dsp/fh_vpu_mpi.h"
#include "dsp/fh_jpege_mpi.h"
#include "dsp/fh_vgs_mpipara.h"
#include "dsp/fh_vgs_mpi.h"
#include "dsp_ext/FHAdv_SmartIR_mpi.h"

#include "vicap/fh_vicap_mpi.h"
#include "dsp/fh_vpu_mpi.h"
#include "isp/isp_common.h"
#include "isp/isp_api.h"
#include "isp/isp_enum.h"
#include "dsp_ext/FHAdv_Isp_mpi_v3.h"
#include "utils/time_helper.h"
#include "types/vmm_api.h"
#include <atomic.h>
#include <dpbase.h>
#include <service/hard_node_service.h>

#include "libdmc.h"
#include "libdmc_rtsp.h"
#include <VideoAI.h>
//#ifndef ENABLE_AOV
#if 1
extern FH_SINT32 fh_vpu_init(FH_VOID);
extern FH_SINT32 fh_vpu_close(FH_VOID);


extern FH_SINT32 sample_common_dmc_init(FH_CHAR *dst_ip, FH_UINT32 port);
extern FH_SINT32 sample_common_dmc_deinit(FH_VOID);
extern FH_SINT32 sample_nn_chan_create(FH_VOID);
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

#define ENC_BIND_VI_ENABLE      0
#define VI_YUV_FRAME_ENABLE     1
#define VI_YUV_TYPE             VPU_VOMODE_SCAN  //VPU_VOMODE_TILE224 VPU_VOMODE_SCAN VPU_VOMODE_RGB888
#define VI_CHANNEL              0

static unsigned int ENC_FRAME_WIDTH = 960;
static unsigned int ENC_FRAME_HEIGHT = 544;
static unsigned int ENC_BIT_RATE_G0 = CH0_BIT_RATE_G0;
static unsigned int ENC_FRAME_COUNT_G0 = CH0_FRAME_COUNT_G0;

#define  ENC1_FRAME_WIDTH    960//1024//CH0_WIDTH_G0
#define  ENC1_FRAME_HEIGHT   544//600//CH0_HEIGHT_G0

FH_SINT32 media_sys_init(FH_VOID)
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

	ret = FH_VPSS_GetFrameBlkSize(ENC_FRAME_WIDTH, ENC_FRAME_HEIGHT, 1<<VI_YUV_TYPE, &size);
	if (ret)
    {
        printf("[vpu] FH_VPSS_GetFrameBlkSize failed with:%x\n", ret);
        return -1;
    }
    stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkSize = size;
    stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkCnt = 3;

    printf("===========================size:%d ===========================\n", size);

    stVbConf.u32MaxPoolCnt += 1;

    // stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkSize = 1920 * 1080 * 2;
    // stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkCnt = 4;
    // stVbConf.u32MaxPoolCnt += 1;
#if (ENC_CHANNEL_NUM>1) //VPU_VOMODE_RRGGBB
    ret = FH_VPSS_GetFrameBlkSize(ENC1_FRAME_WIDTH, ENC1_FRAME_WIDTH, 1<<VI_YUV_TYPE, &size);
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

FH_SINT32 media_sys_exit(FH_VOID)
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

//2.=========

FH_SINT32 dsp_init(FH_UINT32 grpid)
{
    FH_SINT32 ret;
    FH_VPU_SIZE vi_pic;

    FH_VPU_SET_GRP_INFO grp_info = {0};
    grp_info.vi_max_size.u32Width = CH0_MAX_WIDTH_G0;
    grp_info.vi_max_size.u32Height = CH0_MAX_HEIGHT_G0;

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
    vi_pic.crop_area.vpu_crop_area.u32Width = CH0_WIDTH_G0;
    vi_pic.crop_area.vpu_crop_area.u32Height = CH0_HEIGHT_G0;

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

FH_SINT32 vpu_create_chan(FH_UINT32 grpid, FH_UINT32 chan_vpu, FH_UINT32 width, FH_UINT32 height, FH_UINT32 yuv_type)
{
    FH_SINT32 ret;
    FH_VPU_CHN_INFO chn_info;

	FH_VPU_CHN_CONFIG chn_attr;

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
    //设置为0 可以异步编码； 设置为3即可获取对应YUV数据； 否则编码不了、获取不到YUV数据
    chn_info.bufnum = yuv_type==VPU_VOMODE_RGB888 ? 3 : CH0_BUFNUM_G0;//g_vpu_chn_infos[grpid][chan_vpu].bufnum; CH0_BUFNUM_G0
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

FH_SINT32 start_vpu(FH_VOID)
{
    int ret;
    int grploop, vpuloop;

    grploop = 0;
    vpuloop = 0;

    ret = dsp_init(grploop);
    if (ret != 0)
    {
        printf("Error(%d - %x): dsp_init (grp):(%d)!\n", ret, ret, grploop);
        return -1;
    }

    for (vpuloop = 0; vpuloop < ENC_CHANNEL_NUM; vpuloop++)
    {
        if(vpuloop==0)
            ret = vpu_create_chan(grploop, vpuloop, ENC_FRAME_WIDTH, ENC_FRAME_HEIGHT, VI_YUV_TYPE);
        else if(vpuloop==1)
            ret = vpu_create_chan(grploop, vpuloop, ENC1_FRAME_WIDTH, ENC1_FRAME_HEIGHT, VI_YUV_TYPE);


        if (ret != 0)
        {
            printf("Error(%d - %x): vpu_create_chan (grp-chn):(%d-%d)!\n", ret, ret, grploop, vpuloop);
            return -1;
        }
    }

    return 0;
}

//3. start encode
FH_SINT32 enc_create_chan(FH_UINT32 grpid, FH_UINT32 chan_enc, struct enc_channel_info *enc_info)
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

FH_VOID set_h265_rc_cbr(struct enc_channel_info *info, FH_VENC_CHN_CONFIG *cfg)
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
        cfg->rc_attr.h265_cbr.IP_QPDelta          = 3;
        cfg->rc_attr.h265_cbr.I_BitProp           = 10;
        cfg->rc_attr.h265_cbr.P_BitProp           = 1;
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

FH_VOID set_h265_rc_vbr(struct enc_channel_info *info, FH_VENC_CHN_CONFIG *cfg)
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

static FH_SINT32 set_h265_chan(FH_UINT32 grpid, FH_UINT32 chan, struct enc_channel_info *info)
{
	FH_SINT32 s32Ret = 0;
    FH_VENC_CHN_CONFIG cfg_param;

    cfg_param.chn_attr.enc_type = FH_NORMAL_H265;
    cfg_param.chn_attr.h265_attr.profile = H265_PROFILE_MAIN;
    cfg_param.chn_attr.h265_attr.i_frame_intterval = info->frame_count*2;
    cfg_param.chn_attr.h265_attr.size.u32Width = info->width;
    cfg_param.chn_attr.h265_attr.size.u32Height = info->height;


    {
#if 1
        set_h265_rc_cbr(info, &cfg_param);
#else
        set_h265_rc_vbr(info, &cfg_param);
#endif
    }

    s32Ret =  FH_VENC_SetChnAttr(grpid * MAX_VPU_CHN_NUM + chan, &cfg_param);

	return s32Ret;
}

FH_SINT32 start_enc(FH_VOID)
{
    int ret;
    int grploop=0, encloop=0;


    printf("################### sample_common_start_enc ###############\n");


    struct enc_channel_info enc_info_array[ENC_CHANNEL_NUM]  = {
        {
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

    //struct enc_channel_info *penc= &enc_info;
    struct enc_channel_info *penc={0};
    for (int i = 0; i < ENC_CHANNEL_NUM; i++)
    {
        encloop = i;
        penc = &enc_info_array[i];
        ret = enc_create_chan(grploop, encloop, penc);
        if (ret != 0)
        {
            printf("Error(%d - %x): enc_create_chan (grp-chn):(%d-%d)!\n", ret, ret, grploop, encloop);
            return -1;
        }

        ret = set_h265_chan(grploop, encloop, penc);
        if (ret != 0)
        {
            printf("Error(%d - %x): set_h265_chan (grp-chn):(%d-%d)!\n", ret, ret, grploop, encloop);
            return -1;
        }
    }


    return 0;
}

FH_SINT32 stop_enc(FH_VOID)
{
    int ret;
    int grploop=0, encloop=0;

    for (encloop = 0; encloop < ENC_CHANNEL_NUM; encloop++)
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

    return 0;
}

//========================4. isp===================
static int isp_type = 0;
#define ISP_FORMAT_G0      (isp_type == 1 ? FORMAT_WDR_400WP25 : FORMAT_400WP25)
static volatile FH_SINT32 g_isp_start = 0;

void ReLoadDayNightParamByMyEnc(unsigned int isDay)
{
    isp_type = isDay == 1 ? 0 : 1;
}

void LoadFileParam(int u32Id, unsigned int isDay)
{
    char *fileName = "./hex/gc4653_mipi_attr.hex";//"./hex/ovos02k_mipi_wdr_attr.hex"
    ISP_PARAM_CONFIG param;


    if (access(fileName, F_OK))
    {
        fileName = "./gc4653_mipi_attr.hex";
        if (access(fileName, F_OK))
        {
            fileName = "/system/hex/gc4653_mipi_attr.hex";
        }
    }

    FH_SINT32 isp_get_wdr_mode(FH_SINT32 format);
    if(isp_get_wdr_mode(ISP_FORMAT_G0))
    {
        printf("isp_get_wdr_mode = %d\n", isp_get_wdr_mode(ISP_FORMAT_G0));
        fileName = "/system/hex/gc4653_mipi_wdr_attr.hex";
        if (access(fileName, F_OK))
            fileName = "./gc4653_mipi_wdr_attr.hex";
    }

    FILE *param_file;
    API_ISP_GetBinAddr(u32Id, &param);
    param_file = fopen(fileName,"rb");
    if(param_file == NULL)
    {
        LOG_PRT("open file failed! %s\n", fileName);
        return ;
    }
    LOG_PRT("[irc] open file! %s\n", fileName);
    LOG_PRT("API_ISP_GetBinAddr size = %d isp_grp = %d\n", param.u32BinSize, u32Id);

    char *isp_param_buff = malloc(sizeof(unsigned int)*param.u32BinSize);
    fread(isp_param_buff,1,param.u32BinSize,param_file);
    fclose(param_file);
    API_ISP_LoadIspParam(u32Id, isp_param_buff);
    free(isp_param_buff);
    isp_param_buff = NULL;
}


static FH_SINT32 isp_init(FH_UINT32 grpid)
{
    FH_SINT32 ret;
    ISP_MEM_INIT stMemInit = {0};
    Sensor_Init_t initConf = {0};
    ISP_VI_ATTR_S sensor_vi_attr = {0};

    LOG_PRT("grpid = %d\n",grpid);

	FH_SINT32 csi = 0;//choose_csi(grpid);

    void vicap_mipi_init(int grpid, int mipi);
	vicap_mipi_init(grpid, csi);
    //printf("start API_ISP_MemInit, isp_max_width = %d, isp_max_height = %d\n", g_isp_info[grpid].isp_max_width, g_isp_info[grpid].isp_max_height);

    stMemInit.enOfflineWorkMode = ISP_OFFLINE_MODE_DISABLE;

    stMemInit.stPicConf.u32Width = CH0_WIDTH_G0;
    stMemInit.stPicConf.u32Height = CH0_HEIGHT_G0;
    stMemInit.enLut2dWorkMode = ISP_LUT2D_BYPASS;

    printf("enOfflineWorkMode:%d u32Width:%d u32Height:%d enLut2dWorkMode:%d\n", stMemInit.enOfflineWorkMode, stMemInit.stPicConf.u32Width, stMemInit.stPicConf.u32Height, stMemInit.enLut2dWorkMode);

    {
        stMemInit.enIspOutMode = ISP_OUT_TO_VPU;
    }

    ret = API_ISP_MemInit(grpid, &stMemInit);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_MemInit (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    static struct isp_sensor_if *psensor = NULL;

    struct isp_sensor_if *start_sensor(char *SensorName, int grpid);
    if(!psensor)
    {
#ifdef ENABLE_AOV
    //create sensor 需要再arc端实现该函数
    #define SENSOR_NAME "gc4653_mipi"
    psensor = Sensor_Create(SENSOR_NAME);
#else
    psensor = start_sensor("gc4653_mipi", grpid);
#endif
    }
   // psensor = start_sensor("gc4653_mipi", grpid);

    if (!psensor)
    {
        LOG_PRT("Error(%d - %x): start_sensor (grpid):(%d)!\n", ret, ret, grpid);
        return -1;
    }

    ret = API_ISP_SensorRegCb(grpid, 0, psensor);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_SensorRegCb (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    initConf.u8CsiDeviceId = csi;
    initConf.u8CciDeviceId = 0;//choose_i2c(grpid);
    initConf.bGrpSync = FH_FALSE;
// #endif
	LOG_PRT("start API_ISP_SensorInit. i2c %d-%d\r\n", initConf.u8CsiDeviceId, initConf.u8CciDeviceId);
    ret = API_ISP_SensorInit(grpid, &initConf);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_SensorInit (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    // 下载配置到sensor
	//LOG_PRT("start API_ISP_SetSensorFmt: 0x%x\n", g_isp_info[grpid].isp_format);

    ret = API_ISP_SetSensorFmt(grpid, ISP_FORMAT_G0);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_SetSensorFmt (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }


    // 启动sensor输出
     LOG_PRT("start API_ISP_SensorKick\r\n");
    ret = API_ISP_SensorKick(grpid);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_SensorKick (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    // 初始化ISP硬件寄存器
    LOG_PRT("start API_ISP_Init\r\n");
    ret = API_ISP_Init(grpid);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_Init (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    // 3 根据实际使用，配置vicap是离线模式还是在线模式
     LOG_PRT("start API_ISP_GetViAttr\r\n");
    ret = API_ISP_GetViAttr(grpid, &sensor_vi_attr);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): API_ISP_GetViAttr (grpid):(%d)!\n", ret, ret, grpid);
        return ret;
    }

    LoadFileParam(grpid, 1);


    return 0;
}

FH_SINT32 start_isp(FH_VOID)
{
    FH_UINT32 ret;
    FH_UINT32 grpid = 0;
    if (g_isp_start)
    {
        printf("isp_init is already running\n");
        return 0;
    }
    FH_VICAP_STITCH_GRP_ATTR_S stStitchConf = {0};

    memset(&stStitchConf, 0, sizeof(stStitchConf));

    FH_VICAP_SetStitchGrpAttr(&stStitchConf);

    ret = isp_init(grpid);
    if (ret)
    {
        LOG_PRT("Error(%d - %x): isp_init (grp):(%d)!\n", ret, ret, grpid);
        return -1;
    }

    g_isp_start = 1;
    return 0;
}

//==========5. bind===================
FH_SINT32 vpu_bind_to_enc(FH_UINT32 grpid, FH_UINT32 chan_vpu, FH_UINT32 chan_enc)
{
    FH_SINT32 ret;
    FH_BIND_INFO src, dst;
#if 0
    /* 设置编码旋转角度 */
    FH_ROTATE_OPS rotate_info = FH_RO_OPS_180;

    // ret = FH_VPSS_SetVORotate(grpid * MAX_VPU_CHN_NUM + chan_enc, 0, rotate_info); /* yuv vofmt为tile时，需要该设置 */
    // if (ret != 0)
    // {
    //     printf("Error(%d - %x): FH_VPSS_SetVORotate\n", ret, ret);
    // }

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
#if ENC_BIND_VI_ENABLE
    return FH_SYS_Bind(src, dst);
#else
    return 0;
#endif
}


FH_SINT32 start_bind(FH_VOID)
{
    int ret;
    int grploop=0, chnloop=0;

    FH_BIND_INFO src, dst;

    if(1)
    {
        src.obj_id = FH_OBJ_ISP;
        src.dev_id = grploop;
        {
            src.chn_id = 0;
        }

        dst.obj_id = FH_OBJ_VPU_VI;
        dst.dev_id = 0;//grp_info.channel;
        dst.chn_id = 0;
        ret = FH_SYS_Bind(src, dst);
        if (ret != 0)
        {
            printf("Error(%d - %x): FH_SYS_Bind ISP[%d] to VPU[%d]-chn[%d]\n", ret, ret, 0, dst.dev_id, dst.chn_id);
            return -1;
        }
    }

    for (chnloop = 0; chnloop < ENC_CHANNEL_NUM; chnloop++)
    {
        ret = vpu_bind_to_enc(0, chnloop, chnloop);
        if (ret != 0)
        {
            //printf("Error(%d - %x): FH_SYS_Bind VPU to ENC([%d][%d] to %d)\n", ret, ret, grp_info.channel, vpu_info.channel, enc_info.channel);
            return -1;
        }
    }

    return 0;
}

FH_SINT32 vpu_unbind_to_enc(FH_UINT32 grpid, FH_UINT32 chan_vpu, FH_UINT32 chan_enc)
{
    FH_BIND_INFO src, dst;

    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = grpid;
    src.chn_id = chan_vpu;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = grpid * MAX_VPU_CHN_NUM + chan_enc;

    return FH_SYS_UnBind(src, dst);

}

FH_SINT32 stop_bind(FH_VOID)
{
    int ret;
    int grploop=0, chnloop=0;

    ret = vpu_unbind_to_enc(grploop, chnloop, chnloop);//sample_common_vpu_unbind_to_enc(grp_info.channel, vpu_info.channel, enc_info.channel);
    if (ret != 0)
    {
        //printf("Error(%d - %x): FH_SYS_Bind VPU to ENC([%d][%d] to %d)\n", ret, ret, grp_info.channel, vpu_info.channel, enc_info.channel);
        return -1;
    }

    return 0;
}

//============== 6. get media stream =======
static FH_SINT32 g_get_stream_stop = 0;
static FH_SINT32 g_get_stream_running = 0;
void AovVideoMutexLock(bool block);

static pthread_mutex_t g_drop_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile  int g_drop_frame_cnt = 0;
FH_VOID *get_stream_proc(FH_VOID *arg)
{
    FH_SINT32 ret, i;
    FH_SINT32 end_flag;
    FH_SINT32 subtype;
    FH_VENC_STREAM stream;
    FH_SINT32 *stop = (FH_SINT32 *)arg;

    FH_UINT8 *pVirAddrNalu[MAX_NALU_CNT];

    prctl(PR_SET_NAME, "demo_get_stream");

    FH_FRAMERATE frameRate = {0};
    frameRate.frame_count = ENC_FRAME_COUNT_G0;
    frameRate.frame_time = 1; // frameRate.frame_count;
    FH_VPSS_SetFramectrl(0, 0, &frameRate);

    // FH_VENC_CHN_CONFIG venc_cfg = {0};
    // FH_VENC_GetChnAttr(0, &venc_cfg);
//    do{
//    	ret = FH_VENC_GetChnStream_Timeout(0, &stream, 0);
//	}while(ret==0);
#ifndef NDEBUG
    static int eindex = 0;
    char filename[128] = {0};
    sprintf(filename, "./stream/stream0_%d.h265", eindex);
    eindex++;
    FILE *fp = fopen(filename, "wb");
    FILE *fp1 = NULL;//fopen("./stream1.h265", "w+");
    printf("Readly to recvice frame fp:%p\n", fp);
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
            LOG_PRT("Error(%d - %x) %llu: FH_VENC_GetChnStream_Timeout failed!\n", ret, ret, time(NULL));
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
        if (stream.stmtype == FH_STREAM_H265)
        {
            static unsigned int findex = 0;
            subtype = stream.h265_stream.frame_type == FH_FRAME_I ? DMC_MEDIA_SUBTYPE_IFRAME : DMC_MEDIA_SUBTYPE_PFRAME;
            //printf("<%u:%u>===============chan:%d subtype:%d nalu_cnt:%d len:%d\n",time_relative_ms(), findex, stream.chan, subtype, stream.h265_stream.nalu_cnt, stream.h265_stream.nalu[0].length);
            findex++;
#if 0
            static int is_maxframe_count = 0;
            if (subtype == DMC_MEDIA_SUBTYPE_PFRAME && stream.h265_stream.nalu[0].length > 20*1024)
            {
                is_maxframe_count++;

                if(is_maxframe_count > 1)
                {
                    is_maxframe_count=0;
                    pthread_mutex_lock(&g_drop_mutex);
                    //if(g_drop_frame_cnt==0)
                    {
                        g_drop_frame_cnt++;
                        printf("Framerate too high, drop frame! size:%d\n", stream.h265_stream.nalu[0].length);
                    }
                    pthread_mutex_unlock(&g_drop_mutex);
                }
            }
            else if(subtype == DMC_MEDIA_SUBTYPE_PFRAME)
            {
                is_maxframe_count=0;
            }
#endif
            //if(stream.h265_stream.frame_type == FH_FRAME_I)
            //LOG_PRT("stream.stmtype %x,timestamp %lld\n", stream.stmtype, stream.h265_stream.time_stamp);
            // memset(&frameRate, 0, sizeof(FH_FRAMERATE));
            if(findex%150 ==0 && 0)
            {
                //FH_VENC_GetChnAttr(0, &venc_cfg);
                //printf("findex:%d, FrameRate:%d  frame_count:%d\n", findex, venc_cfg.rc_attr.h265_vbr.bitrate, venc_cfg.rc_attr.h265_vbr.FrameRate.frame_count);
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
            //printf("<%06llu: %03d>=============== subtype:%d frame_count:%d frame_time:%d\n", time(NULL), findex, subtype, frameRate.frame_count, frameRate.frame_time);
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
#if 0
            static int index1 = 0;
                    index1++;
                    if(index1%15 == 0){
                        FH_VENC_RC_ATTR Vencrcattr = {0};
                        FH_CHN_STATUS VencStatus = {0};
                        FH_VENC_GetRCAttr(stream.chan, &Vencrcattr);
                        FH_VENC_GetChnStatus(stream.chan, &VencStatus);
                        printf("<%u>index11111:%d, bitrate:%d 上帧QP:%d 上一I帧QP:%d 实际码率:%d  待编码帧数：%d 已编码帧数：%d 输出队列中的帧数:%d 通道的累计丢帧数:%d\n", time_relative_ms(), index1,
                                Vencrcattr.h265_vbr.bitrate, VencStatus.lastqp, VencStatus.lastiqp, VencStatus.bps, VencStatus.FrameToEnc, VencStatus.framecnt, VencStatus.streamcnt, VencStatus.lostcnt);
                    }
#endif
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

FH_SINT32 start_get_stream(FH_VOID)
{
    pthread_attr_t attr;
    pthread_t thread_stream;
    struct sched_param param;

	g_get_stream_running = 1;
	g_get_stream_stop = 0;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_attr_setstacksize(&attr, 64 * 1024);

	// pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); /* 子线程默认继承父线程调度策略，此处配为分离式，即不继承父线程 */
	pthread_attr_setschedpolicy(&attr, SCHED_RR);

//				  param.sched_priority = 55;
#ifdef __RTTHREAD_OS__
	param.sched_priority = 130;
#endif
	pthread_attr_setschedparam(&attr, &param);
	pthread_create(&thread_stream, &attr, get_stream_proc, &g_get_stream_stop);
	pthread_attr_destroy(&attr);

    return 0;
}

FH_SINT32 stop_get_stream(FH_VOID)
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

//============== 7. isp run =======
static FH_BOOL bStop;
static FH_BOOL running;

static FH_SINT32 g_smartir_inited = 0;
FH_SINT32 sample_SmartIR_init(FH_CHAR *sensor_name, FH_SINT32 grpidx)
{
    FH_SINT32 ret = -1;
    //FH_SINT32 hex_file_len;
    FHADV_SMARTIR_CFG_t smtir_cfg;

    // if (g_smartir_inited[grpidx])
    // {
    //     printf("SmartIR: already inited!\n");
    // 	return 0;
    // }

    // sensor_param_day[grpidx]   = get_isp_sensor_param((char *)sensor_name, SAMPLE_SENSOR_FLAG_NORMAL, &hex_file_len);
    // if (!sensor_param_day[grpidx])
    // {
    //     printf("Error: Cann't load sensor hex Day file!\n");
    //     goto Exit;
    // }

    // sensor_param_night[grpidx] = get_isp_sensor_param((char *)sensor_name, SAMPLE_SENSOR_FLAG_NIGHT, &hex_file_len);
    // if (!sensor_param_night[grpidx])
    // {
    //     printf("Error: Cann't load sensor hex Night file!\n");
    //     goto Exit;
    // }

    ret = FHAdv_SmartIR_Init(grpidx);
    if ( ret != 0 )
    {
        printf("Error: FHAdv_SmartIR_Init failed with %d\n", ret);
        goto Exit;
    }

    ret = FHAdv_SmartIR_GetCfg(grpidx, &smtir_cfg);
    if ( ret != 0 )
    {
        printf("Error: FHAdv_SmartIR_GetCfg failed with %d\n", ret);
        goto Exit;
    }

    smtir_cfg.d2n_thre  = 0x1000;
    smtir_cfg.n2d_value = 0x500;
    smtir_cfg.n2d_gain  = 0x140;
    ret = FHAdv_SmartIR_SetCfg(grpidx, &smtir_cfg);
    if ( ret != 0 )
    {
        printf("Error: FHAdv_SmartIR_SetCfg failed with %d\n", ret);
        goto Exit;
    }

    FHAdv_SmartIR_SetDebugLevel(grpidx, SMARTIR_DEBUG_OFF);

    g_smartir_inited = 1;

    return 0;

Exit:
	// if (sensor_param_day[grpidx])
    // {
    // 	free_isp_sensor_param(sensor_param_day[grpidx]);
    // 	sensor_param_day[grpidx] = NULL;
    // }

 	// if (sensor_param_night[grpidx])
    // {
    // 	free_isp_sensor_param(sensor_param_night[grpidx]);
    // 	sensor_param_night[grpidx] = NULL;
    // }

    return ret;
}

FH_SINT32 sample_SmartIR_deinit(FH_SINT32 grpidx)
{
    FH_SINT32 ret = -1;

 	// if (sensor_param_day[grpidx])
    // {
    // 	free_isp_sensor_param(sensor_param_day[grpidx]);
    // 	sensor_param_day[grpidx] = NULL;
    // }

 	// if (sensor_param_night[grpidx])
    // {
    // 	free_isp_sensor_param(sensor_param_night[grpidx]);
    // 	sensor_param_night[grpidx] = NULL;
    // }

    ret = FHAdv_SmartIR_UnInit(grpidx);
    if ( ret != 0 )
    {
        printf("Error: FHAdv_SmartIR_UnInit failed with %d\n", ret);
        return -1;
    }

	g_smartir_inited = 0;

	return 0;
}

FH_VOID sample_SmartIR_Ctrl(FH_SINT32 grpidx)
{
    static FHADV_SMARTIR_STATUS_e sirStaPrev = SMARTIR_STATUS_DAY; /*the initial value must be FHADV_IR_DAY*/
    FH_SINT32 irStaCurr;
    //FH_SINT32 ret;

    // if (!g_smartir_inited[grpidx])
    // 	return;

    irStaCurr = FHAdv_SmartIR_GetDayNightStatus(grpidx, sirStaPrev);
    //printf("[SmartIR]: irStaCurr:%x sirStaPrev:%x\n", irStaCurr, sirStaPrev);
    if (irStaCurr == sirStaPrev)
        return;

    if (irStaCurr == SMARTIR_STATUS_DAY)
    {
        printf("[SmartIR]: Switch to Day\n");
        // ret = API_ISP_LoadIspParam(grpidx, sensor_param_day[grpidx]);
        // if (ret)
        // {
        //     printf("Error(%d): API_ISP_LoadIspParam!\n", ret);
        // }
    }
    else if(irStaCurr == SMARTIR_STATUS_NIGHT) /*night*/
    {
        printf("[SmartIR]: switch to Night\n");
        // ret = API_ISP_LoadIspParam(grpidx, sensor_param_night[grpidx]);
        // if (ret)
        // {
        //     printf("Error(%d): API_ISP_LoadIspParam!\n", ret);
        // }
    }
    else
        printf("[SmartIR]: FHAdv_SmartIR_GetDayNightStatus failed with:%x\n", irStaCurr);

    sirStaPrev = irStaCurr;
}

FH_VOID *isp_proc(FH_VOID *arg)
{
    //struct dev_isp_info *isp_info = (struct dev_isp_info *)arg;
    char name[20];

    sprintf(name, "demo_isp%d", 0);
    prctl(PR_SET_NAME, name);
    running = 1;

    while (!bStop)
    {
        API_ISP_Run(0);

        //获取白天、黑夜状态
        sample_SmartIR_Ctrl(0);
        usleep(10000);
    }

    bStop = 0;
    running = 0;
    return NULL;
}
FH_SINT32 isp_run(FH_VOID)
{
    FH_UINT32 ret;
    pthread_t isp_thread={0};
    pthread_attr_t attr={0};
    struct sched_param param={0};
    FH_UINT32 grpid = 0;

    //初始化智能白天、黑夜检测
    sample_SmartIR_init(NULL, 0);
    //for (grpid = 0; grpid < MAX_GRP_NUM; grpid++)
    {
        //if (g_isp_info[grpid].enable)
        {
            //g_isp_info[grpid].bStop = 0;
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
            pthread_attr_setstacksize(&attr, 8 * 1024);
#ifdef __RTTHREAD_OS__
            param.sched_priority = 130;
#endif
            pthread_attr_setschedparam(&attr, &param);
            ret = pthread_create(&isp_thread, &attr, isp_proc, NULL);
            if (ret != 0)
            {
                LOG_PRT("Error: create ISP%d thread failed!\n", grpid);
                return -1;
            }

            pthread_attr_destroy(&attr);
        }
    }

    return ret;
}

FH_SINT32 isp_stop(FH_VOID)
{
    FH_UINT32 grpid = 0;

    if (bStop == 0)
    {
        bStop = 1;
        while (running)
        {
            usleep(20 * 1000);
        }

        sample_SmartIR_deinit(grpid);
#ifdef CONFIG_VICAP_OFFLINE_MODE
        sample_common_unbind_isp(grpid);
#endif
        FH_VICAP_Exit(grpid);
        API_ISP_Exit(grpid);
    }
// #ifndef ENABLE_AOV
    g_isp_start = 0;
// #endif
    return 0;
}

//============== 8. isp start =======

static volatile FH_SINT32 g_stop_isp = 0;
static volatile FH_SINT32 g_isp_runnig = 0;
#ifndef ENABLE_AOV
#if 1
static FH_VOID* sample_isp_thread(FH_VOID *args)
{
    int cmd = 0;
    int k;

    prctl(PR_SET_NAME, "demo_adv_isp");
    while (!g_stop_isp)
    {
// #ifdef FH_APP_ISP_MIRROR_FLIP
//         if (cmd == 0 && 0)
//         {
//             SetMirrorAndFlip();
//         }
// #endif
        //如果是绑定状态才有效，非绑定状态无效
        if(FHAdv_Isp_SetMirrorAndflip(0, 0, 1, 1))
            printf("FHAdv_Isp_SetMirrorAndflip error\n");
        if (++cmd > 4)
            cmd = 0;

        for (k=0; k<20 && !g_stop_isp; k++)
        {
            usleep(100*1000);
        }
    }

    g_isp_runnig = 0;

    printf("*******************  sample_isp_thread exit\n");
    return FH_NULL;
}
#endif
pthread_t g_isp_thread;
FH_SINT32 isp_start(FH_VOID)
{
    FH_SINT32 ret;


    uint32_t tick = time_relative_ms();
    printf("sample_isp_start\n");
    if (g_isp_runnig)
    {
        printf("Isp strategy is already running\n");
        return 0;
    }

    ret = FHAdv_Isp_Init(FH_APP_GRP_ID);
    if (ret)
    {
        printf("Error: isp_strategy init failed!\n");
        return ret;
    }
    printf("FHAdv_Isp_Init success 11111111 lost time: %u ms\n", time_relative_ms() - tick);
#if 0
    FHAdv_Isp_SetMirrorAndflip(FH_APP_GRP_ID, FH_APP_GRP_ID, 1, 1);
    printf("FHAdv_Isp_SetMirrorAndflip success 22222222, lost time: %u ms\n", time_relative_ms() - tick);
    // for (size_t i = 0; i < 10; i++)
    // {
    //     FHAdv_Isp_SetMirrorAndflip(FH_APP_GRP_ID, FH_APP_GRP_ID, 1, 1);
    //     usleep(1000*100);
    // }
#else

    pthread_attr_t attr;
    g_isp_runnig = 1;
    g_stop_isp   = 0;

    pthread_attr_init(&attr);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 10 * 1024);
    printf("sample_isp_start: pthread_attr_setstacksize %d\n", 10 * 1024);
    ret = pthread_create(&g_isp_thread, &attr, sample_isp_thread, FH_NULL);
    if (ret)
    {
        printf("Error: Create isp_strategy thread failed!\n");
        g_isp_runnig = 0;
    }
    pthread_attr_destroy(&attr);
#endif
    return ret;
}

FH_SINT32 isp_stoprun(FH_VOID)
{
#if 1
    uint32_t tick = time_relative_ms();
    g_stop_isp = 1;
    pthread_join(g_isp_thread, FH_NULL);
    g_isp_runnig = 0;

    printf("isp_stop lost time: %u ms\n", time_relative_ms() - tick);
#endif
    return  0;
}

#endif //#ifndef ENABLE_AOV
//=============== get blaock =======================
#define DEFAULT_YC_TIMEOUT 2000
#define CONVERT_0_WIDTH 1920
#define CONVERT_0_HEIGHT 1088
#define CONVERT_1_WIDTH 1920
#define CONVERT_1_HEIGHT 1088
FH_SINT32 npu_crop_resize(FH_RECT_FLOAT_T *pstCrop, FH_UINT32 phyAddr,  FH_UINT32 phyAddrOut, FH_SINT32 picWidSrc, FH_SINT32 picHgtSrc, FH_SINT32 picWidDst, FH_SINT32 picHgtDst)
{
    FH_SINT32 ret = 0;
    VGS_HANDLE VgsHandle;
    VGS_TASK_ATTR_S stVgsTask;
    FH_SINT32 l=0,t=0,r=0,b=0;

    // FH_SINT32 picWidSrc = CONVERT_0_WIDTH;
    // FH_SINT32 picHgtSrc = CONVERT_0_HEIGHT;
    // FH_SINT32 picWidDst = CONVERT_1_WIDTH;
    // FH_SINT32 picHgtDst = CONVERT_1_HEIGHT;


//    printf("\n--------VGS add scale task test begin--------\n");

    // l = ALIGNDOWN(pstCrop->x1*NPU_DETECT_WIDTH, 2);
    // t = ALIGNDOWN(pstCrop->y1*NPU_DETECT_HEIGHT,2);
    // r = ALIGNDOWN((1.0 - pstCrop->x2)*NPU_DETECT_WIDTH, 2);
    // b =  ALIGNDOWN((1.0 - pstCrop->y2)*NPU_DETECT_HEIGHT, 2);

    if(picWidSrc - l - r < 32 || picHgtSrc - t - b < 32)
    {
        printf("src crop l-t-r-b: (%d x %d,  %d x %d)\n",
                l, t, r, b);
        return FH_FAILURE;
    }

    ret = FH_VGS_BeginJob(&VgsHandle);

    memset(&stVgsTask, 0 ,sizeof(stVgsTask));
    stVgsTask.stImgIn.stVFrame.u32Width = picWidSrc;
    stVgsTask.stImgIn.stVFrame.u32Height = picHgtSrc;
    stVgsTask.stImgIn.stVFrame.enPixelFormat = PIXEL_FMT_YUV_SEMIPLANAR_420;
    stVgsTask.stImgIn.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stVgsTask.stImgIn.stVFrame.u32PhyAddr[0] = phyAddr;
    stVgsTask.stImgIn.stVFrame.u32PhyAddr[1] = phyAddr + picWidSrc * picHgtSrc;
    stVgsTask.stImgIn.stVFrame.u32Stride[0] = picWidSrc;
    stVgsTask.stImgIn.stVFrame.enCompressMode = COMPRESS_MODE_NONE;


	stVgsTask.stImgIn.stVFrame.s16OffsetLeft = l;
	stVgsTask.stImgIn.stVFrame.s16OffsetTop = t;
	stVgsTask.stImgIn.stVFrame.s16OffsetRight = r;
	stVgsTask.stImgIn.stVFrame.s16OffsetBottom = b;

    stVgsTask.stImgOut.stVFrame.u32Width = picWidDst;
    stVgsTask.stImgOut.stVFrame.u32Height = picHgtDst;
    stVgsTask.stImgOut.stVFrame.enPixelFormat = PIXEL_FMT_YUV_SEMIPLANAR_420;
    stVgsTask.stImgOut.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stVgsTask.stImgOut.stVFrame.u32PhyAddr[0] = phyAddrOut;
    stVgsTask.stImgOut.stVFrame.u32PhyAddr[1] = phyAddrOut + picWidDst * picHgtDst;
    stVgsTask.stImgOut.stVFrame.u32Stride[0] = picWidDst;
    stVgsTask.stImgOut.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
	stVgsTask.stImgOut.stVFrame.s16OffsetLeft = 0;
	stVgsTask.stImgOut.stVFrame.s16OffsetRight = 0;
	stVgsTask.stImgOut.stVFrame.s16OffsetTop = 0;
	stVgsTask.stImgOut.stVFrame.s16OffsetBottom = 0;
//    printf("src(w*h: %d x %d)  dst(w*h: %d x %d)\n",
//            stVgsTask.stImgIn.stVFrame.u32Width, stVgsTask.stImgIn.stVFrame.u32Height,
//            stVgsTask.stImgOut.stVFrame.u32Width, stVgsTask.stImgOut.stVFrame.u32Height);
//    printf("src crop l-t-r-b: (%d x %d,  %d x %d)\n",
//            stVgsTask.stImgIn.stVFrame.s16OffsetLeft, stVgsTask.stImgIn.stVFrame.s16OffsetTop,
//            stVgsTask.stImgIn.stVFrame.s16OffsetRight, stVgsTask.stImgIn.stVFrame.s16OffsetBottom);


    ret = FH_VGS_AddScaleTask(VgsHandle, &stVgsTask);
    if(ret != FH_SUCCESS)
    {
        FH_VGS_CancelJob(VgsHandle);
        printf("FH_VGS_AddScaleTask error, s32Ret:0x%x.\n", ret);
        goto exit;
    }

    ret = FH_VGS_EndJob(VgsHandle);
    if(ret != FH_SUCCESS)
    {
        printf("FH_VGS_EndJob error, s32Ret:0x%x.\n", ret);
        goto exit;
    }

exit:
    return ret;
}

FH_SINT32 npu_convert_yuvt2argb(FH_UINT32 width, FH_UINT32 height, FH_UINT32 phyAddr, FH_UINT32 phyAddrOut)
{
    FH_SINT32 ret = 0;
    VGS_HANDLE VgsHandle;

    ret = FH_VGS_BeginJob(&VgsHandle);

    VIDEO_FRAME_S stFrmIn={0};
    VIDEO_FRAME_S stFrmOut={0};

    stFrmIn.u32Width = width;
    stFrmIn.u32Height = height;
    stFrmIn.enPixelFormat = PIXEL_FMT_YUV_SEMIPLANAR_420;
    stFrmIn.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stFrmIn.u32PhyAddr[0] = phyAddr;
    stFrmIn.u32PhyAddr[1] = phyAddr + width * height;
    stFrmIn.u32Stride[0] = width;
    stFrmIn.enCompressMode = COMPRESS_MODE_NONE;
    stFrmIn.s16OffsetLeft = 0;
    stFrmIn.s16OffsetRight = 0;
    stFrmIn.s16OffsetTop = 0;
    stFrmIn.s16OffsetBottom = 0;

    stFrmOut.u32Width = width;
    stFrmOut.u32Height = height;
    stFrmOut.enPixelFormat = PIXEL_FMT_RGB_8888;
    stFrmOut.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stFrmOut.u32PhyAddr[0] = phyAddrOut;
    stFrmOut.u32Stride[0] = width*4;
    stFrmOut.enCompressMode = COMPRESS_MODE_NONE;
    stFrmOut.s16OffsetLeft = 0;
    stFrmOut.s16OffsetRight = 0;
    stFrmOut.s16OffsetTop = 0;
    stFrmOut.s16OffsetBottom = 0;

    ret = FH_VGS_AddFmtConvertTask(VgsHandle, &stFrmIn, &stFrmOut);
    if(ret != FH_SUCCESS)
    {
        FH_VGS_CancelJob(VgsHandle);
        printf("FH_VGS_AddFmtConvertTask error, s32Ret:0x%x.\n", ret);
        goto exit;
    }

    ret = FH_VGS_EndJob(VgsHandle);
    if(ret != FH_SUCCESS)
    {
        printf("FH_VGS_EndJob error, s32Ret:0x%x.\n", ret);
        goto exit;
    }

exit:
    return ret;
}


FH_SINT32 npu_convert_argb2yuv(FH_UINT32 width, FH_UINT32 height, FH_UINT32 phyAddrIn_argb, FH_UINT32 phyAddrOut_yuv)
{
    FH_SINT32 ret = 0;
    VGS_HANDLE VgsHandle;

    ret = FH_VGS_BeginJob(&VgsHandle);

    VIDEO_FRAME_S stFrmIn={0};
    VIDEO_FRAME_S stFrmOut={0};

    stFrmIn.u32Width = width;
    stFrmIn.u32Height = height;
    stFrmIn.enPixelFormat = PIXEL_FMT_RGB_8888;
    stFrmIn.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stFrmIn.u32PhyAddr[0] = phyAddrIn_argb;
    //stFrmIn.u32PhyAddr[1] = phyAddr + width * height;
    stFrmIn.u32Stride[0] = width*4;
    stFrmIn.enCompressMode = COMPRESS_MODE_NONE;
    stFrmIn.s16OffsetLeft = 0;
    stFrmIn.s16OffsetRight = 0;
    stFrmIn.s16OffsetTop = 0;
    stFrmIn.s16OffsetBottom = 0;

    stFrmOut.u32Width = width;
    stFrmOut.u32Height = height;
    stFrmOut.enPixelFormat = PIXEL_FMT_YUV_SEMIPLANAR_420;
    stFrmOut.enVideoFormat = VIDEO_FORMAT_LINEAR;
    stFrmOut.u32PhyAddr[0] = phyAddrOut_yuv;
    stFrmOut.u32PhyAddr[1] = phyAddrOut_yuv + width * height;
    stFrmOut.u32Stride[0] = width;
    stFrmOut.enCompressMode = COMPRESS_MODE_NONE;
    stFrmOut.s16OffsetLeft = 0;
    stFrmOut.s16OffsetRight = 0;
    stFrmOut.s16OffsetTop = 0;
    stFrmOut.s16OffsetBottom = 0;

    ret = FH_VGS_AddFmtConvertTask(VgsHandle, &stFrmIn, &stFrmOut);
    if(ret != FH_SUCCESS)
    {
        FH_VGS_CancelJob(VgsHandle);
        printf("FH_VGS_AddFmtConvertTask error, s32Ret:0x%x.\n", ret);
        goto exit;
    }

    ret = FH_VGS_EndJob(VgsHandle);
    if(ret != FH_SUCCESS)
    {
        printf("FH_VGS_EndJob error, s32Ret:0x%x.\n", ret);
        goto exit;
    }

exit:
    return ret;
}

/*​
 * @brief 将ARGB缓冲区转换为RGB缓冲区 (按行批量处理，效率可能更高)
 * @param argb_input 指向输入ARGB数据块的指针
 * @param rgb_output 指向输出RGB数据块的指针
 * @param width 图像宽度
 * @param height 图像高度
*/
void argb_to_rgb_bulk(const uint8_t* argb_input, uint8_t* rgb_output, int width, int height) {
#if 1
    const uint8_t* argb_row_ptr = argb_input;
    uint8_t* rgb_row_ptr = rgb_output;
    size_t row_stride_argb = width * 4;
    size_t row_stride_rgb = width * 3;

    for (int y = 0; y < height; ++y) {
        const uint8_t* argb_pixel_ptr = argb_row_ptr;
        uint8_t* rgb_pixel_ptr = rgb_row_ptr;

        for (int x = 0; x < width; ++x) {
            // 拷贝 B, G, R 三个通道数据，跳过 A
            rgb_pixel_ptr[0] = argb_pixel_ptr[0];
            rgb_pixel_ptr[1] = argb_pixel_ptr[1];
            rgb_pixel_ptr[2] = argb_pixel_ptr[2];

            argb_pixel_ptr += 4;
            rgb_pixel_ptr += 3;
        }
        argb_row_ptr += row_stride_argb; // 移动到下一行起始
        rgb_row_ptr += row_stride_rgb;
    }
#else
    size_t total_pixels = width * height;
    const uint8_t* argb_ptr1 = argb_input;
    uint8_t* rgb_ptr1 = rgb_output;

    // // 一次性处理所有像素
    for (size_t i = 0; i < total_pixels; ++i) {
        // 假设内存布局为 [B, G, R, A]（即BGRA），我们想要RGB就是 [B, G, R]
        // 如果你的ARGB布局是 [A, R, G, B]，请改为：
        // rgb_ptr[0] = argb_ptr[1]; // R
        // rgb_ptr[1] = argb_ptr[2]; // G
        // rgb_ptr[2] = argb_ptr[3]; // B
        rgb_ptr1[0] = argb_ptr1[0]; // 复制 B
        rgb_ptr1[1] = argb_ptr1[1]; // 复制 G
        rgb_ptr1[2] = argb_ptr1[2]; // 复制 R

        argb_ptr1 += 4;
        rgb_ptr1 += 3;
    }
    printf("argb_to_rgb_bulk done, total_pixels:%d, %p  %p\n", total_pixels, argb_ptr1, rgb_ptr1);
#endif
}

#include <arm_neon.h>

/**
 * @brief 使用 NEON 指令将 ARGB 图像数据转换为 RGB 图像数据
 *
 * @param argb 输入缓冲区，指向 ARGB 格式的图像数据
 * @param rgb 输出缓冲区，指向存储 RGB 结果的预分配内存
 * @param pixelCount 需要转换的像素总数
*/
void argb_to_rgb_neon(uint8_t *argb, uint8_t *rgb, int pixelCount) {
    // 确保像素数量是 16 的倍数，以便用 NEON 进行完整处理。
    // 对于非16倍数的部分，需要在主循环后用C语言处理剩余像素。
    int numPixels16 = pixelCount / 16;

    for (int i = 0; i < numPixels16; i++) {
        // 1. 加载数据：一次性加载 16 个像素（64字节）的 ARGB 数据。
        //    使用 vld4q_u8 函数将交织的 ARGB 数据分离到四个 128 位寄存器中。
        uint8x16x4_t argb_vector = vld4q_u8(argb);

        // 2. 移除Alpha通道：我们只需要 val[1] (R), val[2] (G), val[3] (B)。
        uint8x16x3_t rgb_vector;
        rgb_vector.val[0] = argb_vector.val[0]; // Red 分量
        rgb_vector.val[1] = argb_vector.val[1]; // Green 分量
        rgb_vector.val[2] = argb_vector.val[2]; // Blue 分量

        // 3. 存储数据：使用 vst3q_u8 将 R, G, B 三个分量重新交织并存储到 RGB 缓冲区。
        vst3q_u8(rgb, rgb_vector);

        // 4. 移动指针：更新输入和输出指针的位置。
        argb += 16 * 4; // 每个像素4字节（A,R,G,B），处理了16个像素，所以前进64字节。
        rgb += 16 * 3;  // 每个像素3字节（R,G,B），处理了16个像素，所以前进48字节。
    }

    // 5. 处理剩余像素（如果 pixelCount 不是 16 的倍数）
    int remainingPixels = pixelCount % 16;
    for (int i = 0; i < remainingPixels; i++) {
        // 使用标准的 C 代码逐像素处理剩余部分
        rgb[0] = argb[0]; // R
        rgb[1] = argb[1]; // G
        rgb[2] = argb[2]; // B
        argb += 4;
        rgb += 3;
    }
}

#include <arm_neon.h>
#include <stdint.h>

/** @brief 使用 NEON 指令将 RGB 图像数据转换为 ARGB 图像数据
 *
 * @param rgb 输入缓冲区，指向 RGB 格式的图像数据（每个像素3字节）
 * @param argb 输出缓冲区，指向存储 ARGB 结果的预分配内存（每个像素4字节）
 * @param width 图像宽度（像素数）
 * @param height 图像高度（像素数）
 * @param alpha 要设置的 Alpha 值（通常为 0xFF 表示不透明）
*/
void rgb_to_argb_neon(uint8_t *rgb, uint8_t *argb, int width, int height, uint8_t alpha) {
    int pixelCount = width * height;
    // 计算每次迭代处理16个像素的循环次数
    int numChunks = pixelCount / 16;
    // 计算剩余不足以用NEON一次处理的像素数
    int remainingPixels = pixelCount % 16;

    // 使用 NEON 指令创建一个所有元素都为指定 Alpha 值的向量
    uint8x16_t alpha_vec = vdupq_n_u8(alpha);

    for (int i = 0; i < numChunks; ++i) {
        // 1. 加载数据：一次性加载 16 个像素（48字节）的 RGB 数据。
        //    使用 vld3q_u8 函数将交织的 RGB 数据分离到三个 128 位寄存器中[7](@ref)。
        uint8x16x3_t rgb_vec = vld3q_u8(rgb);

        // 2. 准备一个四通道向量结构用于存储 ARGB
        uint8x16x4_t argb_vec;

        // 3. 组合ARGB数据：
        argb_vec.val[0] = rgb_vec.val[0];   // R 通道：来自原始RGB数据的R分量
        argb_vec.val[1] = rgb_vec.val[1];   // G 通道：来自原始RGB数据的G分量
        argb_vec.val[2] = rgb_vec.val[2];   // B 通道：来自原始RGB数据的B分量
        argb_vec.val[3] = alpha_vec;        // A 通道：全部填充为指定的Alpha值

        // 4. 存储数据：使用 vst4q_u8 将 A, R, G, B 四个分量重新交织并存储到 ARGB 缓冲区[6](@ref)。
        vst4q_u8(argb, argb_vec);

        // 5. 移动指针：更新输入和输出指针的位置。
        rgb += 16 * 3;  // 每个像素3字节，处理了16个像素，所以前进48字节。
        argb += 16 * 4; // 每个像素4字节，处理了16个像素，所以前进64字节。
    }

    // 6. 处理剩余像素（如果总像素数不是 16 的倍数）
    for (int i = 0; i < remainingPixels; ++i) {
        // 使用标准的 C 代码逐像素处理剩余部分
        argb[0] = rgb[0];  // R
        argb[1] = rgb[1];  // G
        argb[2] = rgb[2];  // B
        argb[3] = alpha;  // A
        rgb += 3;
        argb += 4;
    }
}

void rgb_to_argb_bulk_alt(const uint8_t* rgb_input, uint8_t* argb_output, int width, int height, uint8_t default_alpha)
 {
    size_t total_pixels = width * height;
    const uint8_t* rgb_ptr = rgb_input;
    uint8_t* argb_ptr = argb_output;

    for (size_t i = 0; i < total_pixels; ++i) {
        argb_ptr[0] = rgb_ptr[0];    // Red
        argb_ptr[1] = rgb_ptr[1];    // Green
        argb_ptr[2] = rgb_ptr[2];    // Blue
        argb_ptr[3] = default_alpha; // Alpha
        rgb_ptr += 3;
        argb_ptr += 4;
    }
}

#include <arm_neon.h>

void *neon_memcpy(void *dest, const void *src, size_t n) {
    if (dest == NULL || src == NULL || n == 0) {
        return dest;
    }

    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    // 处理小数据量情况
    if (n < 64) {
        // 小数据直接使用逐字节拷贝
        for (; n > 0; n--) {
            *d++ = *s++;
        }
        return dest;
    }

    // 对齐预处理：处理目标地址不对齐的情况
    uintptr_t align_mask = 15; // 16字节对齐掩码
    uintptr_t misalign = (uintptr_t)d & align_mask;
    if (misalign > 0) {
        size_t align_bytes = 16 - misalign;
        for (size_t i = 0; i < align_bytes; i++) {
            *d++ = *s++;
            n--;
        }
    }

    // 主循环：每次处理64字节
    size_t chunks = n / 64;
    for (size_t i = 0; i < chunks; i++) {
        // 一次性加载64字节数据到4个128位NEON寄存器
        uint8x16_t data0 = vld1q_u8(s);
        uint8x16_t data1 = vld1q_u8(s + 16);
        uint8x16_t data2 = vld1q_u8(s + 32);
        uint8x16_t data3 = vld1q_u8(s + 48);

        // 存储到目标地址
        vst1q_u8(d, data0);
        vst1q_u8(d + 16, data1);
        vst1q_u8(d + 32, data2);
        vst1q_u8(d + 48, data3);

        s += 64;
        d += 64;
        n -= 64;
    }

    // 处理剩余数据（16字节块）
    size_t vec16_count = n / 16;
    for (size_t i = 0; i < vec16_count; i++) {
        uint8x16_t data = vld1q_u8(s);
        vst1q_u8(d, data);
        s += 16;
        d += 16;
        n -= 16;
    }

    // 尾部字节处理（小于16字节）
    for (; n > 0; n--) {
        *d++ = *s++;
    }

    return dest;
}

void *neon_memcpy_optimized(void *dest, const void *src, size_t n) {
    if (dest == NULL || src == NULL || n == 0) {
        return dest;
    }

    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;

    // 小数据优化
    if (n < 128) {
        return neon_memcpy(dest, src, n);
    }

    // 数据预取（提前加载到缓存）
    size_t chunks = n / 64;

    for (size_t i = 0; i < chunks; i++) {
        // 预取下一块数据
        __builtin_prefetch(s + 64);

        // 使用NEON指令批量拷贝
        uint8x16_t v0 = vld1q_u8(s);
        uint8x16_t v1 = vld1q_u8(s + 16);
        uint8x16_t v2 = vld1q_u8(s + 32);
        uint8x16_t v3 = vld1q_u8(s + 48);

        vst1q_u8(d, v0);
        vst1q_u8(d + 16, v1);
        vst1q_u8(d + 32, v2);
        vst1q_u8(d + 48, v3);

        s += 64;
        d += 64;
    }

    // 处理剩余数据
    size_t remaining = n % 64;
    if (remaining >= 16) {
        uint8x16_t v0 = vld1q_u8(s);
        vst1q_u8(d, v0);
        s += 16;
        d += 16;
        remaining -= 16;
    }

    // 处理最后几个字节
    for (size_t i = 0; i < remaining; i++) {
        *d++ = *s++;
    }

    return dest;
}

#define  ENABLE_SAVE_FILE 0

//是否启用图像增强算法
#define  VIDEO_AI_WORK  1
#if VIDEO_AI_WORK
static void *g_VAIHandle = NULL;
#endif //#if VIDEO_AI_WORK
static volatile bool g_video_ai_work_flag = false;
//是否保存当前帧
static volatile bool g_videa_frame_save_flag = false;

//是否停止获取图像线程
static volatile bool g_stop_get_block_thread = false;

void set_video_ai_work_flag(bool flag)
{
    atomic_exchange_bool(&g_video_ai_work_flag, flag);
}

void set_video_frame_save_flag(bool flag)
{
    atomic_exchange_bool(&g_videa_frame_save_flag, flag);
}

void * sample_get_block_thread(void *arg)
{
    FH_VPU_STREAM_ADV frameinfo;
    FH_ENC_FRAME encStr;
    FH_SINT32 ret;
    FH_UINT32 handle_lock;

    FH_UINT8 isfirstframe = 1;
    FH_UINT32 index = 0;
#if VI_YUV_FRAME_ENABLE
    FH_UINT8 *pVirAddr[3] = {NULL};//, *pGaddr = NULL, *pBaddr = NULL;
#endif //#if VI_YUV_FRAME_ENABLE
    FH_UINT32 u32PhyAddr_Y=0;
    //FH_UINT8 *pVirAddr_Y=NULL;
    FH_UINT32 u32PhyAddr_UV=0;
    //FH_UINT8 *pVirAddr_UV=NULL;

    //缩放后的YUV缓冲区
    FH_UINT32 u32PhyAddr_Resize=0;
    //FH_UINT8 *pVirAddr_Resize=NULL;

    //ARGB缓冲区
    FH_UINT32 u32PhyAddr_Convert=0;
    FH_UINT8 *pVirAddr_Convert=NULL;
#if 0
    int size_Y = 1920 * 1088;
    int size_UV = 1920 * 1088/2;
    int size_Resize = CONVERT_1_WIDTH * CONVERT_1_HEIGHT * 4;
    int size_Convert = CONVERT_0_WIDTH * CONVERT_0_HEIGHT *4;


	ret = FH_SYS_VmmAlloc(&u32PhyAddr_Y,(FH_VOID **)&pVirAddr_Y, "yuv_test_Y", NULL, size_Y);
	ret = FH_SYS_VmmAlloc(&u32PhyAddr_UV,(FH_VOID **)&pVirAddr_UV, "yuv_test_UV", NULL, size_UV);

    ret = FH_SYS_VmmAlloc(&u32PhyAddr_Resize,(FH_VOID **)&pVirAddr_Resize, "yuv_resize", NULL, size_Resize);
    ret = FH_SYS_VmmAlloc(&u32PhyAddr_Convert,(FH_VOID **)&pVirAddr_Convert, "yuv_convert", NULL, size_Convert);


    int BReadFile(char *filename, char **buf);
    char * pbuf = NULL;
    int yuvlen = BReadFile("vi2.yuv", (char**)&pbuf);
    if (yuvlen  && pbuf)
    {
        /* code */
        printf("########### yuvlen:%d size_Y:%d size_UV:%d u32PhyAddr_Y: 0x%x u32PhyAddr_UV: 0x%x\n", yuvlen, size_Y, size_UV, u32PhyAddr_Y, u32PhyAddr_UV);
        memcpy(pVirAddr_Y, pbuf, size_Y);
        memcpy(pVirAddr_UV, pbuf+size_Y, size_UV);
        free(pbuf);
    }
#endif //0

#if VIDEO_AI_WORK
    //AI处理的缓存
    FH_UINT32 u32PhyAddr_AI=0;
    FH_UINT8 *pVirAddr_AI=NULL;

    ret = FH_SYS_VmmAlloc(&u32PhyAddr_AI,(FH_VOID **)&pVirAddr_AI, "yuv_AI_buf", NULL, 1920 * 1088*3/2);
    char *pyuvbuf = (char*)malloc(1920*1088*3/2);
    char strModelFileName[128] = "./res/zerodce_960x544_12_3_scale_factor_4.bin";
    int   ImageWidth    = ENC_FRAME_WIDTH;
    int   ImageHeight   = ENC_FRAME_HEIGHT;
    int   ImageFormat   = 3;

    if(!g_VAIHandle)
        g_VAIHandle = VideoAI_LLE_Create (strModelFileName, ImageWidth, ImageHeight, ImageFormat);

    //if (g_VAIHandle)
	{
        printf("~~~~~~~~~~ create Low_Light_Enhance model %s ~~~~~~~~~~~\n", g_VAIHandle ? "success" : "fail");
    }

    uint32_t starttick = time_relative_ms();
#endif
    while (!g_stop_get_block_thread)
    {
        memset(&frameinfo,0,sizeof(FH_VPU_STREAM_ADV));
        memset(&encStr, 0, sizeof(FH_ENC_FRAME));

        if ((ret = FH_VPSS_LockChnFrameAdv(0, VI_CHANNEL, &frameinfo, DEFAULT_YC_TIMEOUT, &handle_lock)) != FH_SUCCESS)
        {
            printf("[nna]FH_VPSS_LockChnFrameAdv error: 0x%x ,grp %d -- chn %d\n", ret,0,0);
            usleep(500);
            continue;
        }

        //  printf("<%05u:%u> width:%d height:%d  stride:%d size:%d\n",index, time_relative_ms(), frameinfo.size.u32Width, frameinfo.size.u32Height,
        //         frameinfo.frm_rgb888.stride, frameinfo.frm_rgb888.data.size);
        index++;
        if(isfirstframe&&index>500&&0)
        //if(isfirstframe)
        {
            isfirstframe = 0;
            printf("first frame\n");
#if ENABLE_SAVE_FILE
#if VI_YUV_FRAME_ENABLE
            FILE *pfd = fopen("./vi.yuv", "wb");
#else
            FILE *pfd = fopen("vi.rgb", "wb");
#endif
            if (pfd)
#endif //ENABLE_SAVE_FILE
            {
#if VI_YUV_FRAME_ENABLE
                printf("width:%d height:%d  size:%d  Y:%u UV:%u\n", frameinfo.size.u32Width, frameinfo.size.u32Height, frameinfo.size.u32Width*frameinfo.size.u32Height*3/2,
                    frameinfo.frm_scan.luma.data.size, frameinfo.frm_scan.chroma.data.size);

                uint32_t ltick = time_relative_ms();
                uint32_t lltick = ltick;
                int u32ComponentSize = frameinfo.size.u32Width*frameinfo.size.u32Height;
                pVirAddr[0] = FH_SYS_Mmap(frameinfo.frm_scan.luma.data.base, u32ComponentSize);
                pVirAddr[1] = FH_SYS_Mmap(frameinfo.frm_scan.chroma.data.base, u32ComponentSize/2);

#if ENABLE_SAVE_FILE
                size_t wlen1 = fwrite(pVirAddr[0], 1, frameinfo.size.u32Width*frameinfo.size.u32Height, pfd);
                if(wlen1 != frameinfo.size.u32Width*frameinfo.size.u32Height)
                {
                    perror("fwrite1");
                }
                size_t wlen2 = fwrite(pVirAddr[1], 1, frameinfo.size.u32Width*frameinfo.size.u32Height/2, pfd);
                if(wlen2 != frameinfo.size.u32Width*frameinfo.size.u32Height/2)
                {
                    perror("fwrite2");
                }
                printf("wlen1:%d wlen2:%d\n", wlen1, wlen2);
#endif //ENABLE_SAVE_FILE
#if 0

                if(npu_crop_resize(NULL, frameinfo.frm_scan.luma.data.base, u32PhyAddr_Resize,CONVERT_0_WIDTH,CONVERT_0_HEIGHT, CONVERT_1_WIDTH, CONVERT_1_HEIGHT)==0){
                    printf("npu_crop_resize success, lost time:%u\n", time_relative_ms() - ltick);
                    FILE *pfd_Y = fopen("./vi_crop_1.yuv", "wb");
                    if (pfd_Y)
                    {
                        fwrite(pVirAddr_Resize, 1, CONVERT_1_WIDTH*CONVERT_1_HEIGHT*3/2, pfd_Y);
                        fclose(pfd_Y);
                    }
                    printf("npu_crop_resize success 2222, lost time:%u\n", time_relative_ms() - ltick);
                }
                else{
                    printf("npu_crop_resize failed\n");
                }
#else
                //1，YUV缩小
#if 0            //同样大小不需要缩放了
                if(npu_crop_resize(NULL, frameinfo.frm_scan.luma.data.base, u32PhyAddr_Resize, CONVERT_0_WIDTH, CONVERT_0_HEIGHT, CONVERT_1_WIDTH, CONVERT_1_HEIGHT)==0){
#if ENABLE_SAVE_FILE
                    printf("npu_crop_resize success, lost time:%u\n", time_relative_ms() - ltick);
                    FILE *pfd_resize = fopen("./vi_resize.yuv", "wb");
                    if (pfd_resize)
                    {
                        fwrite(pVirAddr_Resize, 1, CONVERT_1_WIDTH*CONVERT_1_HEIGHT*3/2, pfd_resize);
                        fclose(pfd_resize);
                    }
                    printf("npu_crop_resize success 2222, lost time:%u\n", time_relative_ms() - ltick);
#endif //ENABLE_SAVE_FILE
                }
                else{
                    printf("npu_crop_resize failed\n");
                }
#endif //

                int dstwidth = CONVERT_1_WIDTH;
                int dstheight = CONVERT_1_HEIGHT;
                ltick = time_relative_ms();
                //2，缩小后的YUV转ARGB 如果1是缩小的 用物理内存u32PhyAddr_Resize
                if(npu_convert_yuvt2argb(CONVERT_1_WIDTH, CONVERT_1_HEIGHT, frameinfo.frm_scan.luma.data.base, u32PhyAddr_Convert)==0){
#if ENABLE_SAVE_FILE
                    printf("npu_convert_yuvt2argb success, lost time:%u\n", time_relative_ms() - ltick);
                    FILE *pfd_rgb = fopen("./vi_convert.rgb", "wb");
                    if (pfd_rgb)
#endif //ENABLE_SAVE_FILE
                    {
                        printf("npu_convert_yuvt2argb success 000000000 , lost time:%u\n", time_relative_ms() - ltick);
                        uint8_t *pbuf_rgb = (uint8_t*)malloc(dstwidth*dstheight*3);
                        uint8_t *pbuf_argb = (uint8_t*)malloc(dstwidth*dstheight*4);
                        memset(pbuf_rgb, 0, dstwidth*dstheight*3);
                        memset(pbuf_argb, 0, dstwidth*dstheight*4);
                        printf("npu_convert_yuvt2argb success aaaaaaaa , lost time:%u\n", time_relative_ms() - ltick);
                        //3，ARGB转RGB
                        //argb_to_rgb_bulk(pVirAddr_Convert, pbuf_rgb, dstwidth, dstheight);
                        argb_to_rgb_neon(pVirAddr_Convert, pbuf_rgb, dstwidth*dstheight);
                        printf("npu_convert_yuvt2argb success bbbbbbbbbb , lost time:%u\n", time_relative_ms() - ltick);
#if ENABLE_SAVE_FILE
                        fwrite(pbuf_rgb, 1, dstwidth*dstheight*3, pfd_rgb);
#endif //ENABLE_SAVE_FILE
                        //3.1，RGB转ARGB
                        //rgb_to_argb_bulk_alt(pbuf_rgb, pbuf_argb, dstwidth, dstheight, 0xff);
                        rgb_to_argb_neon(pbuf_rgb, pbuf_argb, dstwidth, dstheight, 0xff);
                        printf("npu_convert_yuvt2argb success cccccccccc , lost time:%u\n", time_relative_ms() - ltick);
#if ENABLE_SAVE_FILE
                        FILE *pfd_argb = fopen("./vi_convert_2.argb", "wb");
                        if(pfd_argb)
                        {
                            fwrite(pbuf_argb, 1, dstwidth*dstheight*4, pfd_argb);
                            fclose(pfd_argb);
                        }
#endif //ENABLE_SAVE_FILE
                        free(pbuf_rgb);
                        free(pbuf_argb);


                        //for
                        //fwrite(pVirAddr_Convert, 1, dstwidth*dstheight*4, pfd_argb);
#if ENABLE_SAVE_FILE
                        fclose(pfd_rgb);
#endif //ENABLE_SAVE_FILE
                    }
                    //printf("npu_convert_yuvt2argb success 2222, lost time:%u\n", time_relative_ms() - ltick);
                }
                else{
                    printf("npu_convert_yuvt2argb failed\n");
                }

                //4，RGB转YUV
                ltick = time_relative_ms();
                if(npu_convert_argb2yuv(CONVERT_1_WIDTH, CONVERT_1_HEIGHT, u32PhyAddr_Convert, u32PhyAddr_Resize)==0){
#if ENABLE_SAVE_FILE
                    printf("npu_convert_argb2yuv success, lost time:%u\n", time_relative_ms() - ltick);
                    FILE *pfd_yuv = fopen("./vi_convert_yuv.yuv", "wb");
                    if (pfd_yuv)
                    {
                        fwrite(pVirAddr_Resize, 1, CONVERT_1_WIDTH*CONVERT_1_HEIGHT, pfd_yuv);
                        fwrite(pVirAddr_Resize+CONVERT_1_WIDTH*CONVERT_1_HEIGHT, 1, CONVERT_1_WIDTH*CONVERT_1_HEIGHT/2, pfd_yuv);
                        fclose(pfd_yuv);
                    }
#endif //ENABLE_SAVE_FILE
                }

                //5，YUV放大
                ltick = time_relative_ms();
#if  0          //1的时候不缩小， 这边旧不需要放大
                if(npu_crop_resize(NULL, u32PhyAddr_Resize, u32PhyAddr_Convert, CONVERT_1_WIDTH, CONVERT_1_HEIGHT, CONVERT_0_WIDTH, CONVERT_0_HEIGHT)==0){
#if ENABLE_SAVE_FILE
                    printf("npu_crop_resize success, lost time:%u\n", time_relative_ms() - ltick);
                    FILE *pfd_yuv = fopen("./vi_resize_2.yuv", "w+b");
                    if (pfd_yuv)
                    {
                        fwrite(pVirAddr_Convert, 1, CONVERT_0_WIDTH*CONVERT_0_HEIGHT, pfd_yuv);
                        fwrite(pVirAddr_Convert+CONVERT_0_WIDTH*CONVERT_0_HEIGHT, 1, CONVERT_0_WIDTH*CONVERT_0_HEIGHT/2, pfd_yuv);
                        fclose(pfd_yuv);
                    }
#endif //ENABLE_SAVE_FILE
                }
                else{
                    printf("npu_crop_resize failed\n");
                }
#endif //0
#endif


#else
#if ENABLE_SAVE_FILE
                //fwrite(frameinfo.frm_rgb888.data.vbase, 1, frameinfo.frm_rgb888.stride*frameinfo.size.u32Height, pfd);
                fwrite(frameinfo.frm_rgb888.data.vbase, 1, frameinfo.size.u32Width*frameinfo.size.u32Height*3, pfd);
                //fwrite(frameinfo.frm_rgb888.data.vbase, 1, frameinfo.size.u32Width*frameinfo.size.u32Height*3, pfd);
#endif //#if ENABLE_SAVE_FILE
#endif
#if ENABLE_SAVE_FILE
                fflush(pfd);
                fclose(pfd);
                //exit(0);
#endif //ENABLE_SAVE_FILE
#if VI_YUV_FRAME_ENABLE
                // memcpy(pVirAddr[0], pVirAddr_Convert, size_Y);
                // memcpy(pVirAddr[1], pVirAddr_Convert+size_Y, size_UV);
                FH_SYS_Munmap((FH_VOID *)pVirAddr[0], u32ComponentSize);
                FH_SYS_Munmap((FH_VOID *)pVirAddr[1], u32ComponentSize/2);
                printf("Convert one frame lost time:%u\n", time_relative_ms() - lltick);
#endif //#if VI_YUV_FRAME_ENABLE
            }
        }

        static int iindex = 0;
        iindex++;
        //printf("^^^^ %u --------index:%d \n", time_relative_ms(), iindex);
#if VIDEO_AI_WORK
        if(iindex>1 || 1)
#else
        if(iindex>6)
#endif
        {
#if VIDEO_AI_WORK//VI_YUV_FRAME_ENABLE
            //替换帧数据
            //if (yuvlen  && pbuf)
            static int index_yuv = 0;
            index_yuv++;
            //if(index_yuv==50)
            {

                //uint32_t ltick = time_relative_ms();
                int u32ComponentSize = frameinfo.size.u32Width*frameinfo.size.u32Height;
                pVirAddr[0] = FH_SYS_Mmap(frameinfo.frm_scan.luma.data.base, u32ComponentSize*3/2);
                //pVirAddr[1] = FH_SYS_Mmap(frameinfo.frm_scan.chroma.data.base, u32ComponentSize/2);

                // FILE *pfd123 = fopen("./vi_yuv222_123.yuv", "wb");
                // if(pfd123)
                // {
                //     fwrite(pVirAddr[0], 1, u32ComponentSize*3/2, pfd123);
                //     fclose(pfd123);
                // }

                // memcpy(pVirAddr[0], pVirAddr_Y, size_Y);
                // memcpy(pVirAddr[1], pVirAddr_UV, size_UV);
                // memcpy(pVirAddr[0], pVirAddr_Convert, size_Y);
                // memcpy(pVirAddr[1], pVirAddr_Convert+size_Y, size_UV);
                // memcpy(pVirAddr_AI, pVirAddr[0],  1920*1088);
                // memcpy(pVirAddr_AI+1920*1088, pVirAddr[1],  1920*1088/2);

                //memset(pyuvbuf, 0, 1920*1088*3/2);
                memcpy(pyuvbuf, pVirAddr[0], u32ComponentSize*3/2);
                //memcpy(pyuvbuf+u32ComponentSize, pVirAddr[1], u32ComponentSize/2);
                if(atomic_load_bool(&g_videa_frame_save_flag) )
                {
                    atomic_exchange_bool(&g_videa_frame_save_flag, false);
                    struct sdcard_info tfinfo = {0};
                    memset(&tfinfo, 0, sizeof(tfinfo));
                    sdcard_get_info(&tfinfo);
                    printf("+++++++++ tf card insert:%d, mounted:%d, mount_point:%s\n", tfinfo.is_insert, tfinfo.is_mounted, tfinfo.mount_point);
#if 0
                    char filename_yuv[256]={0};
                    char filename_rgb[256]={0};
                    uint32_t ltick = time_relative_ms();
                    sprintf(filename_yuv, "./yuv/vi_yuv_%u.yuv", ltick);
                    sprintf(filename_rgb, "./yuv/vi_rgb_%u.rgb", ltick);
                    FILE *pfd = fopen(filename_yuv, "wb");
                    FILE *pfd_rgb = fopen(filename_rgb, "wb");
                    if(pfd)
                    {
                        fwrite((char*)pVirAddr[0], 1, u32ComponentSize*3/2, pfd);
                        fclose(pfd);
                    }
                    if(pfd_rgb)
                    {
                        char *pbuf_rgb = (char*)malloc(u32ComponentSize*3);
                        VideoAI_NV12_To_RGB3((char*)pVirAddr[0], pbuf_rgb, frameinfo.size.u32Width, frameinfo.size.u32Height);
                        fwrite(pbuf_rgb, 1, u32ComponentSize*3, pfd_rgb);
                        fclose(pfd_rgb);
                        free(pbuf_rgb);
                    }
                    printf("save yuv frame %d, lost time:%u\n", index_yuv, time_relative_ms() - ltick);
#else
                    if(tfinfo.is_insert && tfinfo.is_mounted)
                    {
                        char filename_yuv[256]={0};
                        char filename_rgb[256]={0};
                        uint32_t ltick = time_relative_ms();
                        sprintf(filename_yuv, "%s/yuv/vi_yuv_%u.yuv", tfinfo.mount_point, ltick);
                        sprintf(filename_rgb, "%s/rgb/vi_rgb_%u.rgb", tfinfo.mount_point, ltick);
                        FILE *pfd = fopen(filename_yuv, "wb");
                        FILE *pfd_rgb = fopen(filename_rgb, "wb");
                        if(pfd)
                        {
                            fwrite(pVirAddr[0], 1, u32ComponentSize*3/2, pfd);
                            fclose(pfd);
                        }
                        if(pfd_rgb)
                        {
                            char *pbuf_rgb = (char*)malloc(u32ComponentSize*3);
                            VideoAI_NV12_To_RGB3((char*)pVirAddr[0], pbuf_rgb, frameinfo.size.u32Width, frameinfo.size.u32Height);
                            fwrite(pbuf_rgb, 1, u32ComponentSize*3, pfd_rgb);
                            fflush(pfd_rgb);
                            fclose(pfd_rgb);
                            free(pbuf_rgb);
                            system("sync");

                            hard_node_mgnt_control(NODE_LOCK_LED, NODE_STATE_ON);
                        }
                        printf("save yuv frame %d, lost time:%u\n", index_yuv, time_relative_ms() - ltick);


                    }
#endif
                }

                //uint32_t ltick1 = time_relative_ms();
                if(atomic_load_bool(&g_video_ai_work_flag) && g_VAIHandle)
                {
                    //printf("1111111111 u32ComponentSize:%d\n", u32ComponentSize);
                    //uint32_t ltick11 = time_relative_ms();
                    // process image
                    VideoAI_LLE_Process (g_VAIHandle, (char*)pyuvbuf, u32ComponentSize*3/2);
                    //printf("2222222222 lost time:%u\n", time_relative_ms()-ltick11);
                }
                else
                {
                    //上下memcpy拷贝耗费了 20多毫秒， 要 15帧/秒， 大概66秒每帧 - 20ms 大概是40毫秒的延时
                    usleep(43*1000);
                }

                memcpy(pVirAddr[0], pyuvbuf, u32ComponentSize*3/2);
                //memcpy(pVirAddr[1], pyuvbuf+1920*1088, 1920*1088/2);
                //uint32_t ltick2 = time_relative_ms();

                FH_SYS_Munmap((FH_VOID *)pVirAddr[0], u32ComponentSize*3/2);
                //FH_SYS_Munmap((FH_VOID *)pVirAddr[1], u32ComponentSize/2);
                //uint32_t ltt = time_relative_ms();
                //printf("one frame process lost time:%u  %u  %u\n",  ltt - ltick, ltt - ltick1, ltt - ltick2);
            }
#endif //VI_YUV_FRAME_ENABLE
            iindex = 0;
            do{
                static bool drop_break = false;
                pthread_mutex_lock(&g_drop_mutex);
                if(g_drop_frame_cnt > 0)
                {
                    g_drop_frame_cnt--;
                    drop_break = true;
                }
                pthread_mutex_unlock(&g_drop_mutex);
                if(drop_break)
                {
                    drop_break = false;
                    printf("Frame drop cnt \n");
                    break;
                }
                // encStr.lumma_addr = frameinfo.frm_tile224.data.base;
                // encStr.chroma_addr = frameinfo.frm_tile224.data.base + frameinfo.size.u32Width*frameinfo.size.u32Height;
                encStr.lumma_addr = frameinfo.frm_scan.luma.data.base;//frameinfo.frm_scan.luma.data.base; u32PhyAddr_Y
                encStr.chroma_addr = frameinfo.frm_scan.chroma.data.base;//frameinfo.frm_scan.chroma.data.base u32PhyAddr_UV;
                encStr.time_stamp = frameinfo.time_stamp;

                encStr.size.u32Width = frameinfo.size.u32Width;
                encStr.size.u32Height = frameinfo.size.u32Height;
                //encStr.frame_id = frameinfo.frame_id;
                static int insert_index = 0;
                insert_index++;
                //printf("insert_index:%d\n", insert_index);
                ret = FH_VENC_Submit_ENC_Ex(VI_CHANNEL, &encStr, VPU_VOMODE_SCAN);
                if (ret)
                {
                    printf("FH_VENC_Submit_ENC_Ex failed, ret = %x\n", ret);
                    usleep(500*3);
                    //continue;
                }
                usleep(500);
            }while(0);
        }
        else
            usleep(200);

        if ((ret = FH_VPSS_UnlockChnFrameAdv(0, VI_CHANNEL, &frameinfo, handle_lock))!= FH_SUCCESS)
        {
            SAMPLE_NNA_PRT("FH_VPSS_UnlockChnFrameAdv error: 0x%x ,grp %d -- chn %d\n", ret, 0, 0);
        }
    }

    if(u32PhyAddr_Y)
    FH_SYS_VmmFree(u32PhyAddr_Y);
    if(u32PhyAddr_UV)
    FH_SYS_VmmFree(u32PhyAddr_UV);
    if(u32PhyAddr_Resize)
    FH_SYS_VmmFree(u32PhyAddr_Resize);
    if(u32PhyAddr_Convert)
    FH_SYS_VmmFree(u32PhyAddr_Convert);

#if VIDEO_AI_WORK
    // destroy AI model
    // if(VAIHandle)
	//     VideoAI_LLE_Destroy (VAIHandle);
    if(u32PhyAddr_AI)
        FH_SYS_VmmFree(u32PhyAddr_AI);
    if(pyuvbuf)
    {
        free(pyuvbuf);
        pyuvbuf=NULL;
    }
    printf("sample_get_block_thread exit, lost time:%u\n", time_relative_ms() - starttick);
#endif
    return NULL;
}

pthread_t g_get_block_thread;
int Get_block_start(void)
{
    int ret = 0;

    pthread_attr_t attr;
    //g_isp_runnig = 1;

    pthread_attr_init(&attr);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 8 * 1024 * 1024);
    printf("sample_get_block_thread: pthread_attr_setstacksize %d\n", 10 * 1024);
    g_stop_get_block_thread = false;
    ret = pthread_create(&g_get_block_thread, &attr, sample_get_block_thread, FH_NULL);
    if (ret)
    {
        g_stop_get_block_thread = true;
        printf("Error: Create isp_strategy thread failed!\n");
        g_isp_runnig = 0;
    }
    pthread_attr_destroy(&attr);
    return ret;
}

int stop_get_block_thread(void)
{
    uint32_t starttick = time_relative_ms();
    if(!g_stop_get_block_thread)
    {
        g_stop_get_block_thread = true;
        pthread_join(g_get_block_thread, FH_NULL);
        printf("sample_get_block_thread exit!, lost time:%u\n", time_relative_ms() - starttick);
    }
    else
    {
        printf("sample_get_block_thread already exit!\n");
    }
    return 0;
}

static bool g_bsys_init = false;
static bool g_bvideo_enc_run = false;
//###########################
int Encode_video_stop(void)
{
    if(!g_bvideo_enc_run){
        printf("Encode_video already not exit!\n");
        return 0;
    }
    uint32_t starttick = time_relative_ms();
    printf("Encode_video_stop enter!\n");
    //1. stop get media stream
#ifdef ENABLE_AOV
    int aov_video_exit(void);
    aov_video_exit();
#endif //#ifdef ENABLE_AOV

    //1. stop get media stream
    stop_get_stream();

    //2. stop get block thread
    stop_get_block_thread();
    isp_stoprun();
    g_stop_isp = 1;
    int ret = 0;

    //2.stop_bind
    stop_bind();

    //3.stop_ENC
    stop_enc();
    sample_common_dmc_deinit();
    //4.isp_stop
    isp_stop();

    //5. stop media system
    media_sys_exit();
    printf("Encode_video_stop exit! lost time:%u\n", time_relative_ms() - starttick);
    g_bvideo_enc_run = false;
    return ret;
}

FH_SINT32 Encode_video_start(void)
{
    if(g_bvideo_enc_run){
        printf("Encode_video already run!\n");
        return 0;
    }
    g_bvideo_enc_run = true;
    uint32_t starttick = time_relative_ms();
    printf("Encode_video_start enter!\n");
    FH_SINT32 ret = -1;

    if(!g_bsys_init){
    //1. init media system -
    ret = media_sys_init();
    if (ret != 0)
    {
        printf("Error(%d - %x): media_sys_init!\n", ret, ret);
        goto err_exit;
    }

    //2. start vpu
    ret = start_vpu();
    if (ret != 0)
    {
        printf("Error(%d - %x): start_vpu!\n", ret, ret);
        goto err_exit;
    }
    //g_bsys_init = true;
    }

    //3. start encode -
    ret = start_enc();
    if (ret != 0)
    {
        printf("Error(%d - %x): start_enc!\n", ret, ret);
        goto err_exit;
    }

    //4. start isp
    ret = start_isp();
    if (ret)
    {
        printf("Error(%d - %x): start_isp!\n", ret, ret);
        goto err_exit;
    }

    //5. start bind -
    ret = start_bind();
    if (ret)
    {
        printf("Error(%d - %x): start_bind!\n", ret, ret);
        goto err_exit;
    }
#if defined(FH_NN_ENABLE)
    sample_nn_chan_create();
#endif
#if (ENC_BIND_VI_ENABLE==0)
    Get_block_start();
#endif //ENC_BIND_VI_ENABLE
    sample_common_dmc_init(NULL, 1234);
    //6. start get media stream -
    start_get_stream();

    //7. isp run-
    isp_run();
#ifndef ENABLE_AOV
    //8. start isp
    isp_start();
#else
    int aov_ready(void);
    aov_ready();
#endif //#ifndef ENABLE_AOV
    printf("Encode_video_start exit! lost time:%u\n", time_relative_ms() - starttick);
    return ret;
err_exit:
    printf("Encode_video_start failed exit!\n");
    Encode_video_stop();
    return -1;
}


#endif//