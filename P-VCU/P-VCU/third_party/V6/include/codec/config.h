#ifndef __CONFIG_H__
#define __CONFIG_H__

#define MAX_VPU_CHN_NUM 5
//编码通道数
#define ENC_CHANNEL_NUM 2
#define MAX_GRP_NUM 4
#define JPEG_CHN_OFFSET 3
/*Automatically generated file; DO NOT EDIT. */
/*Linux Kernel Configuration */

/*EMU is not set */
/*NN_DRAW_BOX is not set */
/*CONFIG_VICAP_OFFLINE_MODE is not set */
/*DUAL_CAMERA_DEMO is not set */
/*VOU_DEV_HD is not set */

/*APP config */

#define FH_APP_NAME "sample_vlcview_flip"
#define FH_APP_GRP_ID 0
/*FH_APP_OPEN_OVERLAY is not set */
/*FH_APP_OPEN_VENC is not set */
#define FH_APP_OPEN_ISP_STRATEGY_DEMO
#define FH_APP_ISP_MIRROR_FLIP
/*FH_APP_ISP_MIRROR is not set */
#define FH_APP_ISP_FLIP
/*FH_APP_CHANGE_AE_MODE is not set */
/*FH_APP_CHANGE_NPMODE is not set */
/*FH_APP_CHANGE_SATURATION is not set */
/*FH_APP_CHANGE_CHROMA is not set */
/*FH_APP_OPEN_MOTION_DETECT is not set */
/*FH_APP_OPEN_RECT_MOTION_DETECT is not set */
/*FH_APP_OPEN_COVER_DETECT is not set */
/*FH_APP_OPEN_AF is not set */
/*FH_APP_OPEN_IVS is not set */
/*FH_APP_OPEN_ABANDON_DETECT is not set */
/*FH_APP_NN_YUV is not set */
#define VIDEO_GRP0
/*VPU_MODE_MEM_G0 is not set */
/*VDEC_SEND_G0 is not set */
#define VPU_MODE_ISP_G0
/*VPU_MODE_OFFLINE_2DLUT_ISP_G0 is not set */
/*VPU_MODE_OFFLINE_2DLUT_DDR_G0 is not set */
/*VPU_MODE_ONLINE_2DLUT_G0 is not set */

/*ISP Config */

/*FH_APP_USING_IRCUT_G0 is not set */
#define FH_APP_CSI0_I2C_NUM 0
#define MAX_ISP_WIDTH_G0 0
#define MAX_ISP_HEIGHT_G0 0
/*FH_USING_OVOS02K_MIPI_G0 is not set */
/*FH_USING_OVOS02H_MIPI_G0 is not set */
/*FH_USING_SC450AI_MIPI_G0 is not set */
/*FH_USING_OVOS02D_MIPI_G0 is not set */
/*FH_USING_OVOS04C10_MIPI_G0 is not set */
/*FH_USING_DUMMY_SENSOR_G0 is not set */
/*FH_USING_OVOS05_MIPI_G0 is not set */
/*FH_USING_IMX415_MIPI_G0 is not set */
/*FH_USING_OVOS08_MIPI_G0 is not set */
/*FH_USING_OX08D10_MIPI_G0 is not set */
#define FH_USING_GC4653_MIPI_G0
/*FH_APP_USING_FORMAT_1080P25_G0 is not set */
/*FH_APP_USING_FORMAT_1080P30_G0 is not set */
/*FH_APP_USING_FORMAT_1080P60_G0 is not set */
/*FH_APP_USING_FORMAT_1080P25_WDR_G0 is not set */
/*FH_APP_USING_FORMAT_1080P30_WDR_G0 is not set */
/*FH_APP_USING_FORMAT_400WP20_G0 is not set */
/*FH_APP_USING_FORMAT_400WP15_G0 is not set */
#define FH_APP_USING_FORMAT_400WP25_G0
/*FH_APP_USING_FORMAT_400WP30_G0 is not set */
/*FH_APP_USING_FORMAT_400WP25_WDR_G0 is not set */
/*FH_APP_USING_FORMAT_400WP30_WDR_G0 is not set */

/*VPU Config */

/*VPU_VI_CROP_EN_G0 is not set */
#define CH0_ENABLE_G0
/*YCMEAN_EN_G0 is not set */
/*BGM_ENABLE_G0 is not set */
/*CPY_ENABLE_G0 is not set */
/*SAD_ENABLE_G0 is not set */
/*CH0_CROP_EN_G0 is not set */
#define CH0_MAX_WIDTH_G0 2560//3840
#define CH0_MAX_HEIGHT_G0 1440//2160
#define CH0_WIDTH_G0 2560//2560
#define CH0_HEIGHT_G0 1440//1440
/*FH_CH0_USING_SCAN_G0 is not set */
/*FH_CH0_USING_BLK_G0 is not set */
/*FH_CH0_USING_TILE192_G0 is not set */
#define FH_CH0_USING_TILE224_G0
/*FH_CH0_USING_TILE256_G0 is not set */
/*FH_CH0_USING_YUYV_G0 is not set */
/*FH_CH0_USING_NV16_G0 is not set */
#define CH0_BUFNUM_G0 0
#define CH0_BIND_ENC_G0
/*CH0_BIND_JPEGE_G0 is not set */
/*CH0_BIND_NONE_G0 is not set */
/*CH0_MJPEG_G0 is not set */

/*VENC */

#define CH0_BIT_RATE_G0 (1024*512*4)//(4096000)
#define CH0_FRAME_COUNT_G0 15
#define CH0_FRAME_TIME_G0 1
/*FH_CH0_USING_SAMPLE_H264_G0 is not set */
#define FH_CH0_USING_SAMPLE_H265_G0
/*FH_CH0_USING_SAMPLE_NONE_G0 is not set */
/*FH_CH0_OPEN_BREATH_EFFECT_G0 is not set */
#define FH_CH0_USING_SAMPLE_H265_VBR_G0
/*FH_CH0_USING_SAMPLE_H265_FIXQP_G0 is not set */
/*FH_CH0_USING_SAMPLE_H265_CBR_G0 is not set */
/*FH_CH0_USING_SAMPLE_H265_AVBR_G0 is not set */
/*FH_CH0_USING_SAMPLE_H265_CVBR_G0 is not set */
/*FH_CH0_USING_SAMPLE_H265_QVBR_G0 is not set */
/*CH1_ENABLE_G0 is not set */
/*CH2_ENABLE_G0 is not set */
/*CH3_ENABLE_G0 is not set */

/*Network Protocol for sending stream */

/*FH_APP_USING_PES_G0 is not set */
#define FH_APP_USING_RTSP_G0
/*FH_APP_RECORD_RAW_STREAM_G0 is not set */
/*VIDEO_GRP1 is not set */
/*VIDEO_GRP2 is not set */
/*VDEC_SEND_G3 is not set */
#define FH_APP_USING_COOLVIEW


#define CH1_BIT_RATE_G0 (1024*256*8)
#define CH1_MAX_WIDTH_G0 1024//3840
#define CH1_MAX_HEIGHT_G0 600//2160
#define CH1_WIDTH_G0 1024//2560
#define CH1_HEIGHT_G0 600//1440

#endif
