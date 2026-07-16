/**
* @file dcl_base.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef INC_EXTERNAL_DCL_MPI_BASE_H
#define INC_EXTERNAL_DCL_MPI_BASE_H

#include <stdint.h>
#include <stddef.h>

#include "dcl_base.h"


#ifdef ALIGN_NUM
#undef ALIGN_NUM
#endif
#ifdef __arm__
#define ALIGN_NUM 4
#else
#define ALIGN_NUM 8
#endif

#define ATTRIBUTE __attribute__((aligned (ALIGN_NUM)))
#define DCL_VENC_MAX_PACK_INFO_NUM 8

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DCL_PT_PCMU = 0,
    DCL_PT_1016 = 1,
    DCL_PT_G721 = 2,
    DCL_PT_GSM = 3,
    DCL_PT_G723 = 4,
    DCL_PT_DVI4_8K = 5,
    DCL_PT_DVI4_16K = 6,
    DCL_PT_LPC = 7,
    DCL_PT_PCMA = 8,
    DCL_PT_G722 = 9,
    DCL_PT_S16BE_STEREO = 10,
    DCL_PT_S16BE_MONO = 11,
    DCL_PT_QCELP = 12,
    DCL_PT_CN = 13,
    DCL_PT_MPEGAUDIO = 14,
    DCL_PT_G728 = 15,
    DCL_PT_DVI4_3 = 16,
    DCL_PT_DVI4_4 = 17,
    DCL_PT_G729 = 18,
    DCL_PT_G711A = 19,
    DCL_PT_G711U = 20,
    DCL_PT_G726 = 21,
    DCL_PT_G729A = 22,
    DCL_PT_LPCM = 23,
    DCL_PT_CelB = 25,
    DCL_PT_JPEG = 26,
    DCL_PT_CUSM = 27,
    DCL_PT_NV = 28,
    DCL_PT_PICW = 29,
    DCL_PT_CPV = 30,
    DCL_PT_H261 = 31,
    DCL_PT_MPEGVIDEO = 32,
    DCL_PT_MPEG2TS = 33,
    DCL_PT_H263 = 34,
    DCL_PT_SPEG = 35,
    DCL_PT_MPEG2VIDEO = 36,
    DCL_PT_AAC = 37,
    DCL_PT_WMA9STD = 38,
    DCL_PT_HEAAC = 39,
    DCL_PT_PCM_VOICE = 40,
    DCL_PT_PCM_AUDIO = 41,
    DCL_PT_MP3 = 43,
    DCL_PT_ADPCMA = 49,
    DCL_PT_AEC = 50,
    DCL_PT_X_LD = 95,
    DCL_PT_H264 = 96,
    DCL_PT_S_H264  = 97,
    DCL_PT_D_GSM_HR = 200,
    DCL_PT_D_GSM_EFR = 201,
    DCL_PT_D_L8 = 202,
    DCL_PT_D_RED = 203,
    DCL_PT_D_VDVI = 204,
    DCL_PT_D_BT656 = 220,
    DCL_PT_D_H263_1998 = 221,
    DCL_PT_D_MP1S = 222,
    DCL_PT_D_MP2P = 223,
    DCL_PT_D_BMPEG = 224,
    DCL_PT_MP4VIDEO = 230,
    DCL_PT_MP4AUDIO = 237,
    DCL_PT_VC1 = 238,
    DCL_PT_JVC_ASF = 255,
    DCL_PT_D_AVI = 256,
    DCl_PT_DIVX3 = 257,
    DCL_PT_AVS = 258,
    DCL_PT_REAL8 = 259,
    DCL_PT_REAL9 = 260,
    DCL_PT_VP6 = 261,
    DCL_PT_VP6F = 262,
    DCL_PT_VP6A = 263,
    DCL_PT_SORENSON = 264,
    DCL_PT_H265 = 265,
    DCL_PT_VP8 = 266,
    DCL_PT_MVC = 267,
    DCL_PT_PNG = 268,
    DCL_PT_S_H265 = 269,

    DCL_PT_AMR = 1001,
    DCL_PT_MJPEG = 1002,
    DCL_PT_AMRWB = 1003,
    DCL_PT_PRORES = 1006,
    DCL_PT_OPUS = 1007,
    DCL_PT_VPC = 2000,
    DCL_PT_BUTT
} dclPayloadType;

//定义视频图像帧场类型。
typedef enum {
    DCL_VIDEO_FIELD_TOP = 0x1, // even field
    DCL_VIDEO_FIELD_BOTTOM = 0x2, // odd field
    DCL_VIDEO_FIELD_INTERLACED = 0x3, // two interlaced fields
    DCL_VIDEO_FIELD_FRAME = 0x4, // frame
    DCL_VIDEO_FIELD_BUTT
} dclVideoField;

//定义视频格式枚举。
typedef enum {
    DCL_VIDEO_FORMAT_LINEAR     = 0,   // nature video line
    DCL_VIDEO_FORMAT_TILE       = 0x1, //默认的tile模式，tile单元：256pixel x 16line | [ ]
    DCL_VIDEO_FORMAT_TILE_64x16 = 0x2, // tile cell: 64pixel x 16line
    DCL_VIDEO_FORMAT_BLOCK      = 0x3,      //宏块格式 | [ ]
    DCL_VIDEO_FORMAT_BUTT
} dclVideoFormat;

//定义视频压缩数据格式枚举。
typedef enum {
    DCL_COMPRESS_MODE_NONE = 0, /* no compress */
    //DCL_COMPRESS_MODE_SEG, /* compress unit is 256x1 bytes as a segment */
    //DCL_COMPRESS_MODE_TILE, /* compress unit is a tile */
    //DCL_COMPRESS_MODE_HFBC,/* compress unit is 64 * 8. */
    DCL_COMPRESS_MODE_BUTT    
} dclCompressMode;



//定义动态范围枚举
typedef enum {
    DCL_DYNAMIC_RANGE_SDR8 = 0,
    DCL_DYNAMIC_RANGE_SDR10,
    DCL_DYNAMIC_RANGE_HDR10,
    DCL_DYNAMIC_RANGE_HLG,
    DCL_DYNAMIC_RANGE_SLF,
    DCL_DYNAMIC_RANGE_XDR,
    DCL_DYNAMIC_RANGE_BUTT
} dclDynamicRange;

//定义色域范围枚举
typedef enum {
    DCL_COLOR_GAMUT_BT601 = 0,
    DCL_COLOR_GAMUT_BT709,
    DCL_COLOR_GAMUT_BT2020,
    DCL_COLOR_GAMUT_USER,
    DCL_COLOR_GAMUT_BUTT
} dclColorGamut;

//定义视频图像帧补充信息
typedef struct {
    uint64_t miscInfoPhysAddr; // default allocated buffer
    uint64_t jpegDcfPhysAddr;
    uint64_t ispInfoPhysAddr;
    uint64_t lowDelayPhysAddr;
    uint64_t bnrRntPhysAddr;
    uint64_t motionDataPhysAddr;
    uint64_t frameDngPhysAddr;
    void* ATTRIBUTE miscInfoVirtAddr; // misc info
    void* ATTRIBUTE jpegDcfVirtAddr; // jpeg_dcf, used in JPEG DCF
    void* ATTRIBUTE ispInfoVirtAddr; // isp_frame_info, used in ISP debug, when get raw and send raw
    void* ATTRIBUTE lowDelayVirtAddr; // used in low delay
    void* ATTRIBUTE bnrMotVirtAddr; // used for 3dnr from bnr mot
    void* ATTRIBUTE motionDataVirtAddr; // vpss 3dnr use: gme motion data, filter motion data, gyro data
    void* ATTRIBUTE frameDngVirtAddr;
} dclVideoSupplement;

#define DCL_MAX_COLOR_COMPONENT 3
#define DCL_MAX_USER_DATA_NUM    3
//定义视频原始图像帧结构。
typedef struct {
    uint32_t width;
    uint32_t height;
    dclVideoField field;
    dclPixelFormat pixelFormat;
    dclVideoFormat videoFormat;
    dclCompressMode compressMode;
    dclDynamicRange dynamicRange;
    dclColorGamut colorGamut;
    uint32_t headerStride[DCL_MAX_COLOR_COMPONENT];
    uint32_t widthStride[DCL_MAX_COLOR_COMPONENT];
    uint32_t heightStride[DCL_MAX_COLOR_COMPONENT];
    uint64_t headerPhysAddr[DCL_MAX_COLOR_COMPONENT];
    uint64_t physAddr[DCL_MAX_COLOR_COMPONENT];
    void* ATTRIBUTE headerVirtAddr[DCL_MAX_COLOR_COMPONENT];
    void* ATTRIBUTE virtAddr[DCL_MAX_COLOR_COMPONENT];
    uint32_t time_ref;
    uint64_t pts;
    uint64_t userData[DCL_MAX_USER_DATA_NUM];
    uint32_t frameFlag; // frame_flag, can be OR operation.
    dclVideoSupplement supplement;
} dclVideoFrame;

typedef enum {
    DCL_ID_CMPI = 0,
    DCL_ID_VB = 1,
    DCL_ID_SYS = 2,
    DCL_ID_RGN = 3,
    DCL_ID_CHNL = 4,
    DCL_ID_VDEC = 5,
    DCL_ID_AVS = 6,
    DCL_ID_VPC = 7,
    DCL_ID_VENC = 8,
    DCL_ID_SVP = 9,
    DCL_ID_H264E = 10,
    DCL_ID_JPEGE = 11,
    DCL_ID_MPEG4E = 12,
    DCL_ID_H265E = 13,
    DCL_ID_JPEGD = 14,
    DCL_ID_VO = 15,
    DCL_ID_VI = 16,
    DCL_ID_DIS = 17,
    DCL_ID_VALG = 18,
    DCL_ID_RC = 19,
    DCL_ID_AIO = 20,
    DCL_ID_AI = 21,
    DCL_ID_AO = 22,
    DCL_ID_AENC = 23,
    DCL_ID_ADEC = 24,
    DCL_ID_VPU = 25,
    DCL_ID_PCIV = 26,
    DCL_ID_PCIVFMW = 27,
    DCL_ID_ISP = 28,
    DCL_ID_IVE = 29,
    DCL_ID_USER = 30,
    DCL_ID_DCCM = 31,
    DCL_ID_DCCS = 32,
    DCL_ID_PROC = 33,
    DCL_ID_LOG = 34,
    DCL_ID_VFMW = 35,
    DCL_ID_H264D = 36,
    DCL_ID_GDC = 37,
    DCL_ID_PHOTO = 38,
    DCL_ID_FB = 39,
    DCL_ID_HDMI = 40,
    DCL_ID_VOIE = 41,
    DCL_ID_TDE = 42,
    DCL_ID_HDR = 43,
    DCL_ID_PRORES = 44,
    DCL_ID_VGS = 45,
    DCL_ID_FD = 47,
    DCL_ID_ODT = 48, // Object detection trace
    DCL_ID_VQA = 49, // Video quality analysis
    DCL_ID_LPR = 50, // Object detection trace
    DCL_ID_SVP_NNIE = 51,
    DCL_ID_SVP_DSP = 52,
    DCL_ID_DPU_RECT = 53,
    DCL_ID_DPU_MATCH = 54,
    DCL_ID_MOTIONSENSOR = 55,
    DCL_ID_MOTIONFUSION = 56,
    DCL_ID_GYRODIS = 57,
    DCL_ID_PM = 58,
    DCL_ID_SVP_ALG = 59,
    DCL_ID_IVP = 60,
    DCL_ID_MCF = 61,
    DCL_ID_VPSS = 62,
    DCL_ID_DRV_VPC = 63,
    DCL_ID_BUTT,
} dclModId;

//定义视频图像帧信息结构体。
typedef struct {
    dclVideoFrame vFrame;
    uint32_t poolId;
    dclModId modId;
} dclVideoFrameInfo;


#ifdef __cplusplus
}
#endif

#endif
