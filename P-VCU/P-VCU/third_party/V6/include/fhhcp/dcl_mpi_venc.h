/**
* @file dcl_mpi_venc.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef INC_EXTERNAL_DCL_MPI_VENC_H_
#define INC_EXTERNAL_DCL_MPI_VENC_H_

#include "dcl_base.h"
#include "dcl_mpi_base.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DCL_VENC_MAX_MPF_NUM 2
#define DCL_VENC_PRORES_MAX_ID_CHAR_NUM 4
#define DCL_VENC_MAX_SSE_NUM 8
#define DCL_VENC_QP_HIST_NUM 2
#define DCL_VENC_MAX_CH_NUM  25


typedef enum {
    DCL_VENC_PIC_RECV_SINGLE = 0,
    DCL_VENC_PIC_RECV_MULTI,
    DCL_VENC_PIC_RECV_BUTT
} dclVencPicRecvMode;

typedef struct {
    uint32_t width;
    uint32_t height;
} dclVideoSize;

typedef enum {
    DCL_VENC_RC_MODE_H264_CBR = 1,
    DCL_VENC_RC_MODE_H264_VBR,
    DCL_VENC_RC_MODE_H264_AVBR,
    DCL_VENC_RC_MODE_H264_QVBR,
    DCL_VENC_RC_MODE_H264_CVBR,
    DCL_VENC_RC_MODE_H264_FIXQP,
    DCL_VENC_RC_MODE_H264_QPMAP,        //dp2000不支持
    DCL_VENC_RC_MODE_MJPEG_CBR,
    DCL_VENC_RC_MODE_MJPEG_VBR,
    DCL_VENC_RC_MODE_MJPEG_FIXQP,
    DCL_VENC_RC_MODE_H265_CBR,
    DCL_VENC_RC_MODE_H265_VBR,
    DCL_VENC_RC_MODE_H265_AVBR,
    DCL_VENC_RC_MODE_H265_QVBR,
    DCL_VENC_RC_MODE_H265_CVBR,
    DCL_VENC_RC_MODE_H265_FIXQP,
    DCL_VENC_RC_MODE_H265_QPMAP,      //dp2000不支持
    DCL_VENC_RC_MODE_BUTT,
} dclVencRcMode;

typedef struct {
    uint32_t     gop;                     // gop长度
    uint32_t     statsTime;               // dp2000 不支持
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     initQp;             // 初始化QP | [0-51]
	uint32_t     bitRate;             // 目标码率，单位bit | [ ]
	uint32_t     frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]，单位 fps
	uint32_t     maxratePercent;     // 最大码率，为目标码率的百分比。推荐值200。| [120-800]
	uint32_t     IFrmMaxBits;         // 允许的最大I帧大小，单位为bit.设为0为不限制I帧大小。| [ ]
	int32_t      IPQpDelta;          // P帧最小QP与当前I帧QP 的限制，pqp >= iqp + IP_QPDelta.可以用来抑制呼吸效应等现象。推荐值3.| [ ]
	int32_t      IBitProp;           // I帧P帧的码率分配比例，推荐值5。| [ ]
	int32_t      PBitProp;           // I帧P帧的码率分配比例，推荐值1。| [ ]
	int32_t      fluctuateLevel;     // 码率波动级别，从小到大，为从质量平稳优先到码率平稳优先。推荐值0。| [0-6]
} dclVencH264Cbr;

typedef struct {
    uint32_t    gop;                     // gop长度
    uint32_t    statsTime;               // dp2000 不支持
    uint32_t    srcFrameRate;            // dp2000 不支持
    uint32_t    dstFrameRate;            // dp2000 不支持
    uint32_t    initQp;             // 初始化QP | [0-51]
	uint32_t    bitRate;             // 目标码率，单位bit | [ ]
	uint32_t    IminQP;              // I帧最小QP | [0-51]
	uint32_t    ImaxQP;              // I帧最大QP | [0-51]
	uint32_t    PminQP;              // P帧最小QP | [0-51]
	uint32_t    PmaxQP;              // P帧最大QP | [0-51]
	uint32_t    frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]
	uint32_t    maxratePercent;     // 最大码率，为目标码率的百分比。推荐值200。| [120-800]
	uint32_t    IFrmMaxBits;         // 允许的最大I帧大小，单位为bit.设为0为不限制I帧大小。| [ ]
	int32_t     IPQpDelta;          // P帧最小QP与当前I帧QP 的限制，pqp >= iqp + IP_QPDelta.可以用来抑制呼吸效应等现象。推荐值3.| [ ]
	int32_t     IBitProp;           // I帧P帧的码率分配比例，推荐值5。| [ ]
	int32_t     PBitProp;           // I帧P帧的码率分配比例，推荐值1。| [ ]
	int32_t     fluctuateLevel;     // 码率波动级别，从小到大，为从质量平稳优先到码率平稳优先。推荐值0。| [0-6]
} dclVencH264Vbr;

typedef struct {
    uint32_t     gop;                     // gop长度
    uint32_t     statsTime;               // dp2000 不支持
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     initQp;             // 初始化QP | [0-51]
	uint32_t     bitRate;             // 目标码率，单位bit | [ ]
	uint32_t     IminQP;              // I帧最小QP | [0-51]
	uint32_t     ImaxQP;              // I帧最大QP | [0-51]
	uint32_t     PminQP;              // P帧最小QP | [0-51]
	uint32_t     PmaxQP;              // P帧最大QP | [0-51]
	uint32_t     frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]
	uint32_t     maxratePercent;     // 最大码率，为目标码率的百分比。推荐值200。| [120-800]
	uint32_t     IFrmMaxBits;         // 允许的最大I帧大小，单位为bit.设为0为不限制I帧大小。| [ ]
	int32_t      IPQpDelta;          // P帧最小QP与当前I帧QP 的限制，pqp >= iqp + IP_QPDelta.可以用来抑制呼吸效应等现象。推荐值3.| [ ]
	int32_t      IBitProp;           // I帧P帧的码率分配比例，推荐值5。| [ ]
	int32_t      PBitProp;           // I帧P帧的码率分配比例，推荐值1。| [ ]
	int32_t      fluctuateLevel;     // 码率波动级别，从小到大，为从质量平稳优先到码率平稳优先。推荐值0。| [0-6]
	uint32_t     stillratePercent;   // 静止下最小码率，单位%。推荐值30。| [25-100]
	uint32_t     maxstillQp;          // 静止最大QP，作为编码质量的限制。推荐值34。| [0-51]
} dclVencH264Avbr;

typedef struct {
    uint32_t     gop;                     // gop长度
    uint32_t     statsTime;               // dp2000 不支持
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     initQp;             // 初始化QP | [0-51]
	uint32_t     bitRate;             // 目标码率，单位bit | [ ]
	uint32_t     IminQP;              // I帧最小QP | [0-51]
	uint32_t     ImaxQP;              // I帧最大QP | [0-51]
	uint32_t     PminQP;              // P帧最小QP | [0-51]
	uint32_t     PmaxQP;              // P帧最大QP | [0-51]
	uint32_t     frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]
	uint32_t     maxratePercent;     // 最大码率，为目标码率的百分比。推荐值200。| [120-800]
	uint32_t     IFrmMaxBits;         // 允许的最大I帧大小，单位为bit.设为0为不限制I帧大小。| [ ]
	int32_t      IPQpDelta;          // P帧最小QP与当前I帧QP 的限制，pqp >= iqp + IP_QPDelta.可以用来抑制呼吸效应等现象。推荐值3.| [ ]
	int32_t      IBitProp;           // I帧P帧的码率分配比例，推荐值5。| [ ]
	int32_t      PBitProp;           // I帧P帧的码率分配比例，推荐值1。| [ ]
	int32_t      fluctuateLevel;     // 码率波动级别，从小到大，为从质量平稳优先到码率平稳优先。推荐值0。| [0-6]
	uint32_t     minratePercent;     // PSNR较高时最小码率，单位%。推荐值30。| [25-100]
	uint32_t     psnrUl;             // 当前PSNR上限,高于此值时会降低目标码流, 定点化，为实际值x100。| [2000-4000]
	uint32_t     psnrLl;             // 当前PSNR下限,低于此值时会提高目标码流, 定点化，为实际值x100。| [2000-4000]
} dclVencH264Qvbr;

typedef struct {
    uint32_t     gop;                     // gop长度
    uint32_t     statsTime;               // dp2000 不支持
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     initQp;         // 初始化QP | [0-51]
	uint32_t     stBitRate;       // 短期目标码率，单位bit | [ ]
	uint32_t     ltBitRate;       // 长期目标码率，单位bit | [ ]
	uint32_t     maxratePercent; // 最大码率，为短期目标码率的百分比。推荐值200。| [120-800]
	uint32_t     IminQP;          // I帧最小QP | [0-51]
	uint32_t     ImaxQP;          // I帧最大QP | [0-51]
	uint32_t     PminQP;          // P帧最小QP | [0-51]
	uint32_t     PmaxQP;          // P帧最大QP | [0-51]
	uint32_t     frameRate;       // 参考帧率，没有帧率控制的作用。| [ ]
	uint32_t     IFrmMaxBits;     // 允许的最大I帧大小，单位为bit.设为0为不限制I帧大小。| [ ]
	int32_t      IPQpDelta;      // P帧最小QP与当前I帧QP 的限制，pqp > = iqp + IP_QPDelta.可以用来抑制呼吸效应等现象。推荐值3.| [ ]
	int32_t      IBitProp;       // I帧P帧的码率分配比例，推荐值5。| [ ]
	int32_t      PBitProp;       // I帧P帧的码率分配比例，推荐值1。| [ ]
	int32_t      fluctuateLevel; // 码率波动级别，从小到大，为从质量平稳优先到码率平稳优先。推荐值0。| [0-6]
	int32_t      acceptableQp;   // cvbr模式时可接受图像质量阈值,QP小于等于此值时不会透支码率,推荐值:32 | [0-51]
} dclVencH264Cvbr;

typedef struct {
    uint32_t     gop;                     // gop长度
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     iQp;
    uint32_t     pQp;
    uint32_t     bQp;             // dp2000 不支持
    uint32_t     frameRate;       // 参考帧率，没有帧率控制的作用。| [ ]
} dclVencH264Fixqp;

typedef struct {
    uint32_t gop;
    uint32_t statsTime;
    uint32_t srcFrameRate;
    uint32_t dstFrameRate;
} dclVencH264Qpmap;               

typedef struct {
    uint32_t     statsTime;               // dp2000 不支持
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     initQp;              // 量化参数 | [0-98]
	uint32_t     bitRate;             // 目标码率，单位bit | [ ]
	uint32_t     frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]
} dclVencMjpegCbr;

typedef struct {
    uint32_t     statsTime;               // dp2000 不支持
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     initQp;              // 量化参数 | [0-98]
	uint32_t     bitRate;             // 目标码率，单位bit | [ ]
	uint32_t     minQp;               // 码控最小QP | [0-98]
	uint32_t     maxQp;               // 码控最大QP | [0-98]
	uint32_t     frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]
} dclVencMjpegVbr;

typedef struct {
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t qp;                  // 量化参数 | [0-98]
	uint32_t frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]
} dclVencMjpegFixqp;

typedef struct {
    uint32_t     gop;                     // gop长度
    uint32_t     statsTime;               // dp2000 不支持
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     initQp;             // 初始化QP | [0-51]
	uint32_t     bitRate;             // 目标码率，单位bit | [ ]
	uint32_t     frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]，单位 fps
	uint32_t     maxratePercent;     // 最大码率，为目标码率的百分比。推荐值200。| [120-800]
	uint32_t     IFrmMaxBits;         // 允许的最大I帧大小，单位为bit.设为0为不限制I帧大小。| [ ]
	int32_t      IPQpDelta;          // P帧最小QP与当前I帧QP 的限制，pqp >= iqp + IP_QPDelta.可以用来抑制呼吸效应等现象。推荐值3.| [ ]
	int32_t      IBitProp;           // I帧P帧的码率分配比例，推荐值5。| [ ]
	int32_t      PBitProp;           // I帧P帧的码率分配比例，推荐值1。| [ ]
	int32_t      fluctuateLevel;     // 码率波动级别，从小到大，为从质量平稳优先到码率平稳优先。推荐值0。| [0-6]
} dclVencH265Cbr;

typedef struct {
    uint32_t    gop;                     // gop长度
    uint32_t    statsTime;               // dp2000 不支持
    uint32_t    srcFrameRate;            // dp2000 不支持
    uint32_t    dstFrameRate;            // dp2000 不支持
    uint32_t    initQp;             // 初始化QP | [0-51]
	uint32_t    bitRate;             // 目标码率，单位bit | [ ]
	uint32_t    IminQP;              // I帧最小QP | [0-51]
	uint32_t    ImaxQP;              // I帧最大QP | [0-51]
	uint32_t    PminQP;              // P帧最小QP | [0-51]
	uint32_t    PmaxQP;              // P帧最大QP | [0-51]
	uint32_t    frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]
	uint32_t    maxratePercent;     // 最大码率，为目标码率的百分比。推荐值200。| [120-800]
	uint32_t    IFrmMaxBits;         // 允许的最大I帧大小，单位为bit.设为0为不限制I帧大小。| [ ]
	int32_t     IPQpDelta;          // P帧最小QP与当前I帧QP 的限制，pqp >= iqp + IP_QPDelta.可以用来抑制呼吸效应等现象。推荐值3.| [ ]
	int32_t     IBitProp;           // I帧P帧的码率分配比例，推荐值5。| [ ]
	int32_t     PBitProp;           // I帧P帧的码率分配比例，推荐值1。| [ ]
	int32_t     fluctuateLevel;     // 码率波动级别，从小到大，为从质量平稳优先到码率平稳优先。推荐值0。| [0-6]
} dclVencH265Vbr;

typedef struct {
    uint32_t     gop;                     // gop长度
    uint32_t     statsTime;               // dp2000 不支持
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     initQp;             // 初始化QP | [0-51]
	uint32_t     bitRate;             // 目标码率，单位bit | [ ]
	uint32_t     IminQP;              // I帧最小QP | [0-51]
	uint32_t     ImaxQP;              // I帧最大QP | [0-51]
	uint32_t     PminQP;              // P帧最小QP | [0-51]
	uint32_t     PmaxQP;              // P帧最大QP | [0-51]
	uint32_t     frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]
	uint32_t     maxratePercent;     // 最大码率，为目标码率的百分比。推荐值200。| [120-800]
	uint32_t     IFrmMaxBits;         // 允许的最大I帧大小，单位为bit.设为0为不限制I帧大小。| [ ]
	int32_t      IPQpDelta;          // P帧最小QP与当前I帧QP 的限制，pqp >= iqp + IP_QPDelta.可以用来抑制呼吸效应等现象。推荐值3.| [ ]
	int32_t      IBitProp;           // I帧P帧的码率分配比例，推荐值5。| [ ]
	int32_t      PBitProp;           // I帧P帧的码率分配比例，推荐值1。| [ ]
	int32_t      fluctuateLevel;     // 码率波动级别，从小到大，为从质量平稳优先到码率平稳优先。推荐值0。| [0-6]
	uint32_t     stillratePercent;   // 静止下最小码率，单位%。推荐值30。| [25-100]
	uint32_t     maxstillQp;          // 静止最大QP，作为编码质量的限制。推荐值34。| [0-51]
} dclVencH265Avbr;

typedef struct {
    uint32_t     gop;                     // gop长度
    uint32_t     statsTime;               // dp2000 不支持
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     initQp;             // 初始化QP | [0-51]
	uint32_t     bitRate;             // 目标码率，单位bit | [ ]
	uint32_t     IminQP;              // I帧最小QP | [0-51]
	uint32_t     ImaxQP;              // I帧最大QP | [0-51]
	uint32_t     PminQP;              // P帧最小QP | [0-51]
	uint32_t     PmaxQP;              // P帧最大QP | [0-51]
	uint32_t     frameRate;           // 参考帧率，没有帧率控制的作用。| [ ]
	uint32_t     maxratePercent;     // 最大码率，为目标码率的百分比。推荐值200。| [120-800]
	uint32_t     IFrmMaxBits;         // 允许的最大I帧大小，单位为bit.设为0为不限制I帧大小。| [ ]
	int32_t      IPQpDelta;          // P帧最小QP与当前I帧QP 的限制，pqp >= iqp + IP_QPDelta.可以用来抑制呼吸效应等现象。推荐值3.| [ ]
	int32_t      IBitProp;           // I帧P帧的码率分配比例，推荐值5。| [ ]
	int32_t      PBitProp;           // I帧P帧的码率分配比例，推荐值1。| [ ]
	int32_t      fluctuateLevel;     // 码率波动级别，从小到大，为从质量平稳优先到码率平稳优先。推荐值0。| [0-6]
	uint32_t     minratePercent;     // PSNR较高时最小码率，单位%。推荐值30。| [25-100]
	uint32_t     psnrUl;             // 当前PSNR上限,高于此值时会降低目标码流, 定点化，为实际值x100。| [2000-4000]
	uint32_t     psnrLl;             // 当前PSNR下限,低于此值时会提高目标码流, 定点化，为实际值x100。| [2000-4000]
} dclVencH265Qvbr;

typedef struct {
    uint32_t     gop;                     // gop长度
    uint32_t     statsTime;               // dp2000 不支持
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     initQp;         // 初始化QP | [0-51]
	uint32_t     stBitRate;       // 短期目标码率，单位bit | [ ]
	uint32_t     ltBitRate;       // 长期目标码率，单位bit | [ ]
	uint32_t     maxratePercent; // 最大码率，为短期目标码率的百分比。推荐值200。| [120-800]
	uint32_t     IminQP;          // I帧最小QP | [0-51]
	uint32_t     ImaxQP;          // I帧最大QP | [0-51]
	uint32_t     PminQP;          // P帧最小QP | [0-51]
	uint32_t     PmaxQP;          // P帧最大QP | [0-51]
	uint32_t     frameRate;       // 参考帧率，没有帧率控制的作用。| [ ]
	uint32_t     IFrmMaxBits;     // 允许的最大I帧大小，单位为bit.设为0为不限制I帧大小。| [ ]
	int32_t      IPQpDelta;      // P帧最小QP与当前I帧QP 的限制，pqp > = iqp + IP_QPDelta.可以用来抑制呼吸效应等现象。推荐值3.| [ ]
	int32_t      IBitProp;       // I帧P帧的码率分配比例，推荐值5。| [ ]
	int32_t      PBitProp;       // I帧P帧的码率分配比例，推荐值1。| [ ]
	int32_t      fluctuateLevel; // 码率波动级别，从小到大，为从质量平稳优先到码率平稳优先。推荐值0。| [0-6]
	int32_t      acceptableQp;   // cvbr模式时可接受图像质量阈值,QP小于等于此值时不会透支码率,推荐值:32 | [0-51]
} dclVencH265Cvbr;

typedef struct {
    uint32_t     gop;                     // gop长度
    uint32_t     srcFrameRate;            // dp2000 不支持
    uint32_t     dstFrameRate;            // dp2000 不支持
    uint32_t     iQp;
    uint32_t     pQp;
    uint32_t     bQp;             // dp2000 不支持
    uint32_t     frameRate;       // 参考帧率，没有帧率控制的作用。| [ ]
} dclVencH265Fixqp;

typedef enum {
    DCL_VENC_RC_QPMAP_MODE_MEAN_QP = 0,
    DCL_VENC_RC_QPMAP_MODE_MIN_QP,
    DCL_VENC_RC_QPMAP_MODE_MAX_QP,
    DCL_VENC_RC_QPMAP_MODE_BUTT,
} dclVencRcQpmapMode;

typedef struct {
    uint32_t gop;
    uint32_t statsTime;
    uint32_t srcFrameRate;
    uint32_t dstFrameRate;
    dclVencRcQpmapMode qpmapMode;
} dclVencH265Qpmap;

typedef struct {
    dclVencRcMode rcMode; // RW; the type of rc
    union {
        dclVencH264Cbr h264Cbr;
        dclVencH264Vbr h264Vbr;
        dclVencH264Avbr h264Avbr;
        dclVencH264Qvbr h264Qvbr;
        dclVencH264Cvbr h264Cvbr;
        dclVencH264Fixqp h264Fixqp;
        dclVencH264Qpmap h264Qpmap;
        dclVencMjpegCbr mjpegCbr;
        dclVencMjpegVbr mjpegVbr;
        dclVencMjpegFixqp mjpegFixqp;
        dclVencH265Cbr h265Cbr;
        dclVencH265Vbr h265Vbr;
        dclVencH265Avbr h265Avbr;
        dclVencH265Qvbr h265Qvbr;
        dclVencH265Cvbr h265Cvbr;
        dclVencH265Fixqp h265Fixqp;
        dclVencH265Qpmap h265Qpmap;
    };
} dclVencRcAttr;

#define MAX_TH_NUM 5
typedef struct
{
	uint32_t gopThNum;            // GOP门限数量 | [0-5]
	uint32_t thVal[MAX_TH_NUM];    // 门限值,单位百分比,从小到大,代表从安静到运动 | [0-100]
	uint32_t minGop[MAX_TH_NUM+1]; // 小于对应门限时最小的GOP长度  | [ ]
}dclGopTh;


/*H264 智能编码属性*/
typedef struct
{
	uint32_t        refreshFrameIntterval; // 刷新帧间隔 | [ ]
	dclBool         smartEn;                // 智能编码使能 | [0-1]
	dclBool         textureEn;              // 纹理映射结果使能 | [0-1]
	dclBool         backgroudmodelEn;       // 背景建模结果使能 | [0-1]
	dclGopTh        gopTh;                  // 设置静止帧门限，决定GOP长度 | [ ]
    uint32_t        bgmChn;                 // 获取BGM信息的BGM通道号 | [ ]
} dclVencS264Attr;

/*H265 智能编码属性*/
typedef struct
{
	uint32_t        refreshFrameIntterval; // 刷新帧间隔 | [ ]
	dclBool         smartEn;                // 智能编码使能 | [0-1]
	dclBool         textureEn;              // 纹理映射结果使能 | [0-1]
	dclBool         backgroudmodelEn;       // 背景建模结果使能 | [0-1]
	dclGopTh        gopTh;                  // 设置静止帧门限，决定GOP长度 | [ ]
    uint32_t        bgmChn;                 // 获取BGM信息的BGM通道号 | [ ]
} dclVencS265Attr;

/*JPEG 编码属性*/
typedef struct
{
	uint32_t qp;           // 量化参数 | [0-98]
	uint32_t rotate;       // 旋转角度 | [0-3]
	uint32_t encodeSpeed; // 编码速度，推荐值4.值越小速度越快，对带宽占用越多 | [0-9]
} dclVencJpegAttr;

/*MJPEG 编码属性*/
typedef struct
{
	uint32_t picWidth;
    uint32_t picHeight;
	uint32_t rotate;       // 旋转角度 | [0-3]
	uint32_t encodeSpeed; // 编码速度，推荐值4.值越小速度越快，对带宽占用越多 | [0-9]
} dclVencMJpegAttr;

typedef struct {
    dclPayloadType type;
    uint32_t maxPicWidth;
    uint32_t maxPicHeight;
    uint32_t bufSize;      //dp2000 不支持
    /*
        编码的等级，静态属性。
        ● H.264 取值范围： [0, 3]。
            – 0： Baseline。
            – 1： Main Profile。
            – 2： High Profile。
            – 3： svc-t，暂不支持。
        ● H.265 取值范围： [0,1]。
            – 0： Main Profile。
            – 1： Main 10 Profile，预留值，暂不支持。
        ● Jpeg 取值范围： 0（表示Baseline）。
    */
    uint32_t profile;
    dclBool isByFrame;    //dp2000 不支持
    uint32_t picWidth;
    uint32_t picHeight;
    union
	{
		dclVencJpegAttr  jpegAttr;  // JPEG编码属性 | [ ]
		dclVencMJpegAttr mjpegAttr; // MJPEG编码属性 | [ ]
		dclVencS264Attr  s264Attr;  // 智能H264编码属性 | [ ]
		dclVencS265Attr  s265Attr;  // 智能H265编码属性 | [ ]
	};
} dclVencAttr;

typedef struct {
    dclVencAttr vencAttr;
    dclVencRcAttr rcAttr;
} dclVencChnAttr;


//定义H.264跳帧参考码流的帧类型和参考属性，暂不支持。
typedef enum {
    DCL_VENC_BASE_IDR_SLICE = 0, // the Idr frame at Base layer
    DCL_VENC_BASE_P_SLICE_REF_TO_IDR, // the P frame at Base layer, referenced by other frames at Base layer and reference to Idr frame
    DCL_VENC_BASE_P_SLICE_REF_BY_BASE, // the P frame at Base layer, referenced by other frames at Base layer
    DCL_VENC_BASE_P_SLICE_REF_BY_ENHANCE, // the P frame at Base layer, referenced by other frames at Enhance layer
    DCL_VENC_ENHANCE_P_SLICE_REF_BY_ENHANCE, // the P frame at Enhance layer, referenced by other frames at Enhance layer
    DCL_VENC_ENHANCE_P_SLICE_NOT_FOR_REF, // the P frame at Enhance layer ,not referenced
    DCL_VENC_P_SLICE_BUTT
} dclVencRefType;



typedef struct {
    uint32_t               lastQp;             // 上帧QP | [ ]
	uint32_t               lastIQp;            // 上一I帧QP | [ ]
	uint32_t               bps;                // 实际码率 | [ ]
	uint32_t               frameToEnc;         // 待编码帧数 | [ ]
	uint32_t               frameCnt;           // 已编码帧数 | [ ]
	uint32_t               streamCnt;          // 输出队列中的帧数 | [ ]
	uint32_t               lostCnt;            // 通道的累计丢帧数 | [ ]
	uint32_t               stat[5];            // 保留变量 | [ ]
} dclVencChnStatus;


//定义H.264码流NALU类型。暂不支持
typedef enum {
    DCL_VENC_H264_NALU_B_SLICE = 0, // B SLICE types
    DCL_VENC_H264_NALU_P_SLICE = 1, // P SLICE types
    DCL_VENC_H264_NALU_I_SLICE = 2, // I SLICE types
    DCL_VENC_H264_NALU_IDR_SLICE = 5, // IDR SLICE types
    DCL_VENC_H264_NALU_SEI = 6, // SEI types
    DCL_VENC_H264_NALU_SPS = 7, // SPS types
    DCL_VENC_H264_NALU_PPS = 8, // PPS types
    DCL_VENC_H264_NALU_BUTT
} dclVencH264NaluType;

//定义JPEG码流的PACK类型。暂不支持
typedef enum {
    DCL_VENC_JPEG_PACK_ECS = 5, // ECS types
    DCL_VENC_JPEG_PACK_APP = 6, // APP types
    DCL_VENC_JPEG_PACK_VDO = 7, // VDO types
    DCL_VENC_JPEG_PACK_PIC = 8, // PIC types
    DCL_VENC_JPEG_PACK_DCF = 9, // DCF types
    DCL_VENC_JPEG_PACK_DCF_PIC = 10, // DCF PIC types
    DCL_VENC_JPEG_PACK_BUTT
} dclVencJpegePackType;

//定义H.265码流NALU类型。暂不支持
typedef enum {
    DCL_VENC_H265_NALU_B_SLICE = 0, // B SLICE types
    DCL_VENC_H265_NALU_P_SLICE = 1, // P SLICE types
    DCL_VENC_H265_NALU_I_SLICE = 2, // I SLICE types
    DCL_VENC_H265_NALU_IDR_SLICE = 19, // IDR SLICE types
    DCL_VENC_H265_NALU_VPS = 32, // VPS types
    DCL_VENC_H265_NALU_SPS = 33, // SPS types
    DCL_VENC_H265_NALU_PPS = 34, // PPS types
    DCL_VENC_H265_NALU_SEI = 39, // SEI types
    DCL_VENC_H265_NALU_ENHANCE = 64, // ENHANCE typesHI_VENC_H265_NALU_BUTT
} dclVencH265NaluType;

//定义PRORES码流的PACK类型，暂不支持。
typedef enum {
    DCL_VENC_PRORES_PACK_PIC = 1, // PIC types
    DCL_VENC_PRORES_PACK_BUTT
} dclVencProresPackType;

//定义码流结果类型。暂不支持。
typedef union {
    dclVencH264NaluType h264Type; // R; H264E NALU types
    dclVencJpegePackType jpegType; // R; JPEGE pack types
    dclVencH265NaluType h265Type; // R; H265E NALU types
    dclVencProresPackType proresType;
} dclVencDataType;

typedef struct {
    dclVencDataType packType; // R; the pack type
    uint32_t packOffset;
    uint32_t packLen;
} dclVencPackInfo;

//定义帧码流包结构体。
typedef struct {
    union {
        uint64_t physAddr;
        uint64_t inputAddr;
    };
    uint8_t ATTRIBUTE* addr;
    uint32_t ATTRIBUTE len;
    uint64_t pts;
    dclBool isFrameEnd;          // dp2000不支持
    dclVencDataType dataType;    // dp2000不支持
    uint32_t offset;
    uint32_t dataNum;            // dp2000不支持
    dclVencPackInfo packInfo[DCL_VENC_MAX_PACK_INFO_NUM]; // dp2000不支持
} dclVencPack;

/*
* !!注意
* 因为H264和H265 对帧类型的定义不是完全一致
* H264:0-P 1-B 2-I
* H265:0-B 1-P 2-I
* 这里我们对两个标准使用统一的定义
* 同时我们加入我们对刷新帧的私有定义
*/
typedef enum {
	DCL_FRAME_P              = 0,// P帧 | [ ]
	DCL_FRAME_B              = 1,// B帧 | [ ]
	DCL_FRAME_I              = 2,// I帧 | [ ]
	DCL_FRAME_FRESH_P        = 20,// 刷新P帧 | [ ]
	DCL_FRAME_FRESH_B        = 21,// 刷新B帧 | [ ]
	DCL_FRAME_SKIP_P         = 30,// SKIP-P帧 | [ ]
	DCL_ENC_FRAME_TYPE_DUMMY = 0xffffffff,
} dclEncFrameType;

typedef enum {
	DCL_REF_IDR                = 0,// IDR帧 | [ ]
	DCL_REF_P_REF_IDR          = 1,// 刷新P帧,只参考IDR帧 | [ ]
	DCL_REF_P_REF_M            = 2,// 跳帧参考M层的关键P帧,或非跳帧参考的普通P帧 | [ ]
	DCL_REF_P_REF_N            = 3,// 跳帧参考N层的关键P帧 | [ ]
	DCL_REF_P_NOT_REF          = 4,// 跳帧参考非M&N的不被参考的P帧 | [ ]
	DCL_ENC_REF_TYPE_DUMMY     = 0xffffffff,
} dclEncRefType;

typedef enum {
	/* Nalu Type For H264*/
	DCL_NALU_P_SLICE            = 0,
	DCL_NALU_B_SLICE            = 1,
	DCL_NALU_I_SLICE            = 2,
	DCL_NALU_IDR                = 5,
	DCL_NALU_SEI                = 6,
	DCL_NALU_SPS                = 7,
	DCL_NALU_PPS                = 8,
	DCL_NALU_AUD                = 9,
	/* Nalu Type For H265*/
	DCL_NALU_H265_TRAIL_N        = 0,
	DCL_NALU_H265_VPS_NUT        = 32,
	DCL_NALU_H265_SPS_NUT        = 33,
	DCL_NALU_H265_PPS_NUT        = 34,
	DCL_NALU_H265_PREFIX_SEI_NUT = 39,
	DCL_ENC_NALU_TYPE_DUMMY = 0xffffffff,
} dclEncNaluType;

typedef struct
{
	dclEncNaluType        type;               // NALU类型 | [ ]
	uint32_t              length;             // 码流长度 | [ ]
	uint64_t              start;              // 码流地址 | [ ]
} dclEncStreamNalu;

#define MAX_NALU_CNT 20
typedef struct {

    uint32_t               chan;               // 通道号 | [ ]
	dclEncFrameType        frameType;         // 帧类型 | [ ]
	uint32_t               naluCnt;           // NALU数量 | [ ]
	dclEncStreamNalu       nalu[MAX_NALU_CNT]; // NALU属性 | [ ]
	dclEncRefType          refType;           // 帧参考类型 | [ ]
} dclVencH264StreamInfo;


typedef struct {
    uint32_t               chan;               // 通道号 | [ ]
	uint32_t               qfactor;           // 当前帧使用的量化值 | [ ]
} dclVencJpegStreamInfo;


typedef struct {
    uint32_t               chan;               // 通道号 | [ ]
	dclEncFrameType        frameType;         // 帧类型 | [ ]
	uint32_t               naluCnt;           // NALU数量 | [ ]
	dclEncStreamNalu       nalu[MAX_NALU_CNT]; // NALU属性 | [ ]
	dclEncRefType          refType;           // 帧参考类型 | [ ]
} dclVencH265StreamInfo;



//定义H264/H265协议SSE信息，暂不支持。
typedef struct {
    dclBool enable; // RW; Range:[0,1]; Region SSE enable
    uint64_t sseVal; // R; Region SSE value
} dclVencSseInfo;


typedef struct {
    uint32_t meanQp;         // 编码当前帧平均QP | [ ]
	uint32_t residualBitNum; // 编码当前帧残差bit数 | [ ]
	uint32_t headBitNum;     // 编码当前帧头信息bit数 | [ ]
	uint32_t madiVal;        // 编码当前帧空域纹理复杂度Madi值 | [ ]
	uint32_t madpVal;        // 编码当前帧时域运动复杂度Madp值 | [ ]
	uint32_t mseLcuCnt;      // 编码当前帧中的LCU个数 | [ ]
	uint32_t mseSum;         // 编码当前帧中的MSE(均方差) | [ ]
	double   psnr;           // psnr，因为驱动定点化原因有一些精度损失 | [ ]
	uint64_t distortion;     // 失真度,用户可以用此值得到准确psnr,psnr = 10log10(w*h*255*255/distortion) | [ ]
} dclVencH264AdvStreamInfo;


typedef struct {
    uint32_t meanQp;         // 编码当前帧平均QP | [ ]
	uint32_t residualBitNum; // 编码当前帧残差bit数 | [ ]
	uint32_t headBitNum;     // 编码当前帧头信息bit数 | [ ]
	uint32_t madiVal;        // 编码当前帧空域纹理复杂度Madi值 | [ ]
	uint32_t madpVal;        // 编码当前帧时域运动复杂度Madp值 | [ ]
	uint32_t mseLcuCnt;      // 编码当前帧中的LCU个数 | [ ]
	uint32_t mseSum;         // 编码当前帧中的MSE(均方差) | [ ]
	double   psnr;           // psnr，因为驱动定点化原因有一些精度损失 | [ ]
	uint64_t distortion;     // 失真度,用户可以用此值得到准确psnr,psnr = 10log10(w*h*255*255/distortion) | [ ]
} dclVencH265AdvStreamInfo;

typedef struct {
    dclVencPack ATTRIBUTE* pack; // R; stream pack attribute
    uint32_t    ATTRIBUTE packCnt; // R; the pack number of one frame stream
    uint32_t    seq; // R; the list number of stream       dp2000不支持
    union {
        dclVencH264StreamInfo h264Info; // R; the stream info of h264
        dclVencJpegStreamInfo jpegInfo; // R; the stream info of jpeg
        dclVencH265StreamInfo h265Info; // R; the stream info of h265
        dclVencJpegStreamInfo mjpegInfo; // R; the stream info of prores
    };
    union {
        dclVencH264AdvStreamInfo h264AdvInfo; // R; the stream info of h264
        dclVencH265AdvStreamInfo h265AdvInfo; // R; the stream info of h265
    };
} dclVencStream;

typedef enum {
    DCL_VENC_MOD_VENC = 1,
    DCL_VENC_MOD_H264,
    DCL_VENC_MOD_H265,
    DCL_VENC_MOD_JPEG,
    DCL_VENC_MOD_RC,
    DCL_VENC_MOD_BUTT
} dclVencModType;

//Device上的venc内公共模块参数, 暂不支持
typedef struct {
    uint32_t bufCache;
    uint32_t frameBufRecycle;
} dclVencVencModParam;

typedef enum {
    DCL_VB_SRC_COMMON = 0, //公共VB
    DCL_VB_SRC_MOD = 1, //模块VB
    DCL_VB_SRC_PRIVATE = 2, //私有VB
    DCL_VB_SRC_USER = 3, //用户VB
    DCL_VB_SRC_BUTT
} dclVbSrc;

//Device上的venc内h264e模块参数, 暂不支持
typedef struct {
    uint32_t oneStreamBuf; // RW; Range:[0, 1]; one stream buffer
    uint32_t miniBufMode; // RW; Range:[0, 1]; H264e MiniBufMode
    uint32_t lowPowerMode; // RW; Range:[0, 1]; H264e PowerSaveEn
    dclVbSrc vbSrc; // RW; H264e VBSource
    dclBool qpHistEn; // RW; Range:[0, 1]
    uint32_t maxUserDataLen; // RW; Range:[0, 65536]; one user data buffer len
} dclVencH264ModParam;

//Device上的venc内h265e模块参数, 暂不支持
typedef struct {
    uint32_t oneStreamBuf; // RW; Range:[0, 1]; one stream buffer
    uint32_t miniBufMode; // RW; Range:[0, 1]; H265e MiniBufMode
    uint32_t lowPowerMode; // RW; Range:[0, 1]; H265e PowerSaveEn
    dclVbSrc vbSrc; // RW; H265e VBSource
    dclBool qpHistEn; // RW; Range:[0, 1]
    uint32_t maxUserDataLen; // RW; Range:[0, 65536]; one user data buffer len
} dclVencH265ModParam;

//Device上的venc内jpeg模块参数, 暂不支持
typedef struct {
    uint32_t oneStreamBuf; // RW; Range:[0, 1]; one stream buffer
    uint32_t miniBufMode; // RW; Range:[0, 1]; Jpege MiniBufMode
    uint32_t clearStreamBuf; // RW; Range:[0, 1]; JpegClearStreamBuf
    uint32_t deringMode; // RW; Range:[0, 1]; Jpege Dering Mode
} dclVencJpegModParam;

typedef struct {
    dclVencModType modType; // RW; VencModType
    union {
        dclVencVencModParam vencModParam;
        dclVencH264ModParam h264ModParam;
        dclVencH265ModParam h265ModParam;
        dclVencJpegModParam jpegModParam;
    };
} dclVencModParam;

//用于设置或者获取指定JPEGE编码通道的量化参数的数据结构, 暂不支持
typedef struct {
    uint32_t qfactor; /* RW; Range:0xFFFFFFFF or [1,100]; Qfactor value */
    uint8_t yQt[64]; /* RW; Range:[1, 255]; Y quantization table */
    uint8_t cbQt[64]; /* RW; Range:[1, 255]; Cb quantization table */
    uint8_t crQt[64]; /* RW; Range:[1, 255]; Cr quantization table */
    uint32_t mcuPerEcs; /* RW; the max MCU number is (picwidth + 15) >> 4 x (picheight +15) >> 4 x2]; MCU number of one ECS*/
    dclBool ecsOutputEn;
} dclVencJpegParam;

typedef struct {
    dclBool enable; // RW; Range:[0, 1]; Crop region enable
    dclRect stRect; // RW; Crop region, note: s32X must be multi of 16
} dclCropInfo;

//定义通道帧率控制参数，暂不支持。
typedef struct {
    int32_t srcFrameRate; // RW; Range:[0, 240]; Input frame rate of a channel
    int32_t dstFrameRate; // RW; Range:[0, 240]; Output frame rate of a channel
} dclFrameRateCtrl;

//定义Venc通道参数结构体，暂不支持。
typedef struct {
    dclBool          colorToGreyEn;
    uint32_t         priority;
    uint32_t         maxStreamCnt;
    uint32_t         pollWakeUpFrameCnt;
    dclCropInfo      cropInfo;
    dclFrameRateCtrl frameRate;
} dclVencChnParam;

//定义编码模式场景。暂不支持
typedef enum {
    DCL_VENC_SCENE_0 = 0, // 摄像机不运动或周期性连续运动的场景，支持h.264/h.265
    DCL_VENC_SCENE_1 = 1, // 高码率下运动场景，支持h.265
    DCL_VENC_SCENE_2 = 2, // 暂不支持
    DCL_VENC_SCENE_BUTT
} dclVencSceneMode;

#define DCL_VENC_TEXTURE_THRESHOLD_SIZE  16

//定义编码场景检测控制参数。暂不支持
typedef struct {
    dclBool detectSceneChgEn;
    dclBool adaptInsertIdrFrameEn;
} dclVencSceneChgDetect;

typedef struct {
    uint32_t maxIProportion;
    uint32_t minIProportion;
    uint32_t maxQp;
    uint32_t minQp;
    uint32_t maxIQp;
    uint32_t minIQp;
    int32_t maxReencodeTimes;
    dclBool qpmapEn;
} dclVencH264CbrParam;

typedef struct {
    int32_t  chgPos;
    uint32_t maxIProportion;
    uint32_t minIProportion;
    int32_t  maxReencodeTimes;
    dclBool qpmapEn;
    uint32_t maxQp;
    uint32_t minQp;
    uint32_t maxIQp;
    uint32_t minIQp;
} dclVencH264VbrParam;

typedef struct {
    int32_t  chgPos;
    uint32_t maxIProportion;
    uint32_t minIProportion;
    int32_t  maxReencodeTimes;
    dclBool  qpmapEn;
    int32_t  minStillPercent;
    uint32_t maxStillQp;
    uint32_t minStillPsnr;
    uint32_t maxQp;
    uint32_t minQp;
    uint32_t maxIQp;
    uint32_t minIQp;
    uint32_t minQpDelta;
    uint32_t motionSensitivity;
    dclBool saveBitrateEn;
} dclVencH264AvbrParam;

typedef struct {
    uint32_t maxIProportion;
    uint32_t minIProportion;
    int32_t  maxReencodeTimes;
    dclBool  qpmapEn;
    uint32_t maxQp;
    uint32_t minQp;
    uint32_t maxIQp;
    uint32_t minIQp;
    int32_t maxBitPercent;
    int32_t minBitPercent;
    int32_t maxPsnrFluctuate;
    int32_t minPsnrFluctuate;
} dclVencH264QvbrParam;

typedef struct {
    uint32_t maxIProportion;
    uint32_t minIProportion;
    int32_t  maxReencodeTimes;
    dclBool  qpmapEn;
    uint32_t maxQp;
    uint32_t minQp;
    uint32_t maxIQp;
    uint32_t minIQp;
    uint32_t minQpDelta;
    uint32_t maxQpDelta;
    uint32_t extraBitPercent;
    uint32_t longTermStatsTimeUnit;
    dclBool saveBitrateEn;
} dclVencH264CvbrParam;

typedef struct {
    uint32_t maxIProportion;
    uint32_t minIProportion;
    uint32_t maxQp;
    uint32_t minQp;
    uint32_t maxIQp;
    uint32_t minIQp;
    int32_t maxReencodeTimes;
    dclBool  qpmapEn;
    dclVencRcQpmapMode qpmapMode;
} dclVencH265CbrParam;

typedef struct {
    int32_t  chgPos;
    uint32_t maxIProportion;
    uint32_t minIProportion;
    int32_t  maxReencodeTimes;
    uint32_t maxQp;
    uint32_t minQp;
    uint32_t maxIQp;
    uint32_t minIQp;
    dclBool  qpmapEn;
    dclVencRcQpmapMode qpmapMode;
} dclVencH265VbrParam;

typedef struct {
    int32_t  chgPos;
    uint32_t maxIProportion;
    uint32_t minIProportion;
    int32_t  maxReencodeTimes;
    int32_t  minStillPercent;
    uint32_t maxStillQp;
    uint32_t minStillPsnr;
    uint32_t maxQp;
    uint32_t minQp;
    uint32_t maxIQp;
    uint32_t minIQp;
    uint32_t minQpDelta;
    uint32_t motionSensitivity;
    dclBool qpmapEn;
    dclVencRcQpmapMode qpmapMode;
} dclVencH265AvbrParam;

typedef struct {
    uint32_t maxIProportion;
    uint32_t minIProportion;
    int32_t  maxReencodeTimes;
    dclBool qpmapEn;
    dclVencRcQpmapMode qpmapMode;
    uint32_t maxQp;
    uint32_t minQp;
    uint32_t maxIQp;
    uint32_t minIQp;
    int32_t maxBitPercent;
    int32_t minBitPercent;
    int32_t maxPsnrFluctuate;
    int32_t minPsnrFluctuate;
} dclVencH265QvbrParam;

typedef struct {
    uint32_t maxIProportion;
    uint32_t minIProportion;
    int32_t  maxReencodeTimes;
    dclBool qpmapEn;
    dclVencRcQpmapMode qpmapMode;
    uint32_t maxQp;
    uint32_t minQp;
    uint32_t maxIQp;
    uint32_t minIQp;
    uint32_t minQpDelta;
    uint32_t maxQpDelta;
    uint32_t extraBitPercent;
    uint32_t longTermStatsTimeUnit;
} dclVencH265CvbrParam;

typedef struct {
    uint32_t maxQfactor;
    uint32_t minQfactor;
} dclVencMjpegCbrParam;

typedef struct {
    int32_t chgPos;
    uint32_t maxQfactor;
    uint32_t minQfactor;
} dclVencMjpegVbrParam;

typedef struct {
    uint32_t thresholdI[DCL_VENC_TEXTURE_THRESHOLD_SIZE];
    uint32_t thresholdP[DCL_VENC_TEXTURE_THRESHOLD_SIZE];
    uint32_t thresholdB[DCL_VENC_TEXTURE_THRESHOLD_SIZE];
    uint32_t direction;
    uint32_t rowQpDelta;
    int32_t firstFrameStartQp;
    dclVencSceneChgDetect sceneChgDetect;
    union {
        dclVencH264CbrParam h264CbrParam;
        dclVencH264VbrParam h264VbrParam;
        dclVencH264AvbrParam h264AvbrParam;
        dclVencH264QvbrParam h264QvbrParam;
        dclVencH264CvbrParam h264CvbrParam;
        dclVencH265CbrParam h265CbrParam;
        dclVencH265VbrParam h265VbrParam;
        dclVencH265AvbrParam h265AvbrParam;
        dclVencH265QvbrParam h265QvbrParam;
        dclVencH265CvbrParam h265CvbrParam;
        dclVencMjpegCbrParam mjpegCbrParam;
        dclVencMjpegVbrParam mjpegVbrParam;
    };
} dclVencRcParam;

//用于定义指定JPEGE编码通道的亮度DC huffman编码表参数的数据结构
typedef struct {
    uint8_t dcBits[16]; /* RW; Range:[0, 255];*/
    uint8_t dcValue[12]; /* RW; Range:[0, 255];*/
} dclVencHuffmanDcTable;

//用于定义指定JPEGE编码通道的色度AC huffman编码表参数的数据结构
typedef struct {
    uint8_t acBits[16];
    uint8_t acValue[162];
} dclVencHuffmanAcTable;

//用于定义指定JPEGE编码通道的huffman编码高级参数的数据结构。
typedef struct {
    dclVencHuffmanDcTable dcTables[3]; // y, Cb, Cr
    dclVencHuffmanAcTable acTables[3]; // y, Cb, Cr
    uint32_t reserved[2];
} dclVencJpegHuffmanParam;




/**
 * @ingroup DCL
 * @brief create venc channel.
 *
 * @param chId [IN]     venc channel.
 * @param attr [IN]      venc channel attr.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencCreateChn(uint32_t chId, const dclVencChnAttr *attr);

/**
 * @ingroup DCL
 * @brief set venc channel attr.
 *
 * @param chId [IN]     venc channel.
 * @param attr [IN]      venc channel attr.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencSetChnAttr(uint32_t chId, const dclVencChnAttr *attr);

/**
 * @ingroup DCL
 * @brief destroy venc channel. nonsupport for dp2000
 *
 * @param chId [IN]     venc channel.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencDestroyChn(uint32_t chId);


/**
 * @ingroup DCL
 * @brief start venc channel.
 *
 * @param chId [IN]     venc channel.
 * @param recvParam [IN]      receive image struct pointer
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencStartChn (uint32_t chId);

/**
 * @ingroup DCL
 * @brief stop venc channel.
 *
 * @param chId [IN]     venc channel.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencStopChn(uint32_t chId);

/**
 * @ingroup DCL
 * @brief query venc channel status.
 *
 * @param chId [IN]     venc channel.
 * @param status [IN]      venc channel status.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencQueryStatus(uint32_t chId, dclVencChnStatus *status);

/**
 * @ingroup DCL
 * @brief get the encode stream.
 *
 * @param chId [IN]     venc channel.  nonsupport for dp2000
 * @param stream [IN]      the encode steam.
 * @param milliSec [IN]    -1 is block,0 is no block,other positive number is timeout
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencGetStream(uint32_t chId, dclVencStream *stream, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief release the encode stream.
 *
 * @param chId [IN]     venc channel.
 * @param stream [IN]      the encode steam.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencReleaseStream(uint32_t chId, dclVencStream *stream);

/**
 * @ingroup DCL
 * @brief send the encode frame.
 *
 * @param chId [IN]     venc channel.
 * @param frame [IN]      the frame to be encode.
 * @param milliSec [IN]    -1 is block,0 is no block,other positive number is timeout
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencSendFrame(uint32_t chId, const dclVideoFrameInfo *frame, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief set the encode module params. nonsupport for dp2000
 *
 * @param modParam [IN]     encode module params pointer.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencSetModParam(const dclVencModParam *modParam);

/**
 * @ingroup DCL
 * @brief get the encode module params. nonsupport for dp2000
 *
 * @param modParam [OUT]     encode module params pointer.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencGetModParam(dclVencModParam *modParam);

/**
 * @ingroup DCL
 * @brief request the idr frame.
 *
 * @param chId [IN]     venc channel.
 * @param instant [IN]  enable the idr frame
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencRequestIdr(uint32_t chId, dclBool instant);

/**
 * @ingroup DCL
 * @brief get the venc channel fd. nonsupport for dp2000
 *
 * @param chId [IN]     venc channel.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencGetFd(uint32_t chId);

/**
 * @ingroup DCL
 * @brief close the venc channel fd. nonsupport for dp2000
 *
 * @param chId [IN]     venc channel.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencCloseFd(uint32_t chId);

/**
 * @ingroup DCL
 * @brief set the venc jpeg channel quantized params. nonsupport for dp2000
 *
 * @param chId [IN]     venc channel.
 * @param jpegParam [IN]     jpeg channel quantized params pointer.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencSetJpegParam(uint32_t chId, const dclVencJpegParam *jpegParam);

/**
 * @ingroup DCL
 * @brief get the venc jpeg channel quantized params. nonsupport for dp2000
 *
 * @param chId [IN]     venc channel.
 * @param jpegParam [IN]     jpeg channel quantized params pointer.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencGetJpegParam(uint32_t chId, const dclVencJpegParam *jpegParam);

/**
 * @ingroup DCL
 * @brief set the venc channel params.  nonsupport for dp2000 
 *
 * @param chId [IN]     venc channel.
 * @param chnParam [IN]     venc channel params pointer.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencSetChnParam(uint32_t chId, const dclVencChnParam *chnParam);



/**
 * @ingroup DCL
 * @brief set code rate ctrl params.
 *
 * @param chId [IN]     venc channel.
 * @param rcParam [IN]     rc params pointer.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencSetRcParam(uint32_t chId, const dclVencRcParam *rcParam);

/**
 * @ingroup DCL
 * @brief get code rate ctrl params.  
 *
 * @param chId [IN]     venc channel.
 * @param rcParam [IN]     rc params pointer.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencGetRcParam(uint32_t chId, dclVencRcParam *rcParam);

/**
 * @ingroup DCL
 * @brief set jpeg huffman params.   nonsupport for dp2000
 *
 * @param chId [IN]     venc channel.
 * @param jpegHuffmanParam [IN]     jpeg huffman params pointer.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencSetJpegHuffmanParam(uint32_t chId, const dclVencJpegHuffmanParam *jpegHuffmanParam);

/**
 * @ingroup DCL
 * @brief get jpeg huffman params.  nonsupport for dp2000
 *
 * @param chId [IN]     venc channel.
 * @param jpegHuffmanParam [IN]     jpeg huffman params pointer.
 * @retval DCL_ERROR_NONE The function is successfully executed.
 * @retval OtherValues Failure
 */
DCL_FUNC_VISIBILITY dclError dclmpiVencGetJpegHuffmanParam(uint32_t chId, const dclVencJpegHuffmanParam *jpegHuffmanParam);




#ifdef __cplusplus
}
#endif

#endif
