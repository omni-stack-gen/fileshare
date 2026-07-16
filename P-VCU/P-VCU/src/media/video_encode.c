// #include "../startup/DPBase.h"
#define AWALIGN(d, a) (((d) + (a - 1)) & ~(a - 1))

#include <stdbool.h>
#include "../transport/trans.h"
#define	dpmodel "[venc]"
// #include "../startup/DPLog.h"
#include "media/mm_comm_vi.h"
#include "media/mpi_vi.h"
#include "media/mpi_isp.h"
#include "media/mpi_venc.h"
#include "media/mpi_sys.h"
#include "mm_common.h"
#include "mm_comm_venc.h"
#include "mm_comm_rc.h"
#include "mm_comm_region.h"
#include <mpi_videoformat_conversion.h>
#include <utils/plat_math.h>
#include "sample_common_venc.h"
#include "aiservice_mpp_helper.h"
#include "time_helper.h"
#include "dpbase.h"
// #include "Set.h"
#include "rtsp_server.h"

// #include "BITMAP_S.h"
#include "video_encode.h"

#define MAX_FRAME_BUF_SiZE  512*1024

#define INVALID_OVERLAY_HANDLE  (-1)

#define NN_NBG_MAX_FILE_PATH_SIZE   (256)

#define NN_HUMAN_SRC_WIDTH          (320)
#define NN_HUMAN_SRC_HEIGHT         (192)
#define NN_FACE_SRC_WIDTH           (480)
#define NN_FACE_SRC_HEIGHT          (270)

#define NN_CHN_NUM_MAX              (2)

#define ENABLE_SUPER_FRAME          1

#define SUPPORT_ONG_TEST            0

#if defined(NB_BETA_3_0_1) && NB_BETA_3_0_1 == 1
#define NN_MODEL_FILE "/mnt/app/3.0.1_Beta.nb"
#else
#define NN_MODEL_FILE "/mnt/app/1.0.0_Gamma.nb"
#endif

#ifndef SUPPORT_RTSP_TEST
    //#undef SUPPORT_RTSP_TEST
    #define SUPPORT_RTSP_TEST  1
#endif

#ifndef SUPPORT_OSD_TEST
    #define SUPPORT_OSD_TEST    0
#endif

#define NN_DETECT   0

// typedef struct
// {
// 	unsigned int  check;		//0xabcd1234
// 	unsigned int  timestamp;	//时间戳
// 	unsigned char streamid;		//0  video  1 audio	  2 subvideo 3 抓拍
// 	unsigned char seq;			//包序号
// 	unsigned char bkey;			//是否关键帧
// 	char data[];				//帧数据
// } media_frame;

typedef struct
{
    unsigned char rgbBlue;
    unsigned char rgbGreen;
    unsigned char rgbRed;
    unsigned char rgbReserved;
} RGBQUAD;

#pragma pack(1)

typedef struct
{
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} BmpHead;

typedef struct
{
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BmpInfo;

#pragma pack()

typedef struct
{
    int draw_x;
    int draw_y;
    int draw_w;
    int draw_h;
    int draw_type;
    unsigned char *draw_data;
} OSDContext_S;

typedef enum ISP_DEV_TYPE_E
{
    ISP_DEV_MAIN = 0,
    ISP_DEV_SUB  = 1
} ISP_DEV_TYPE_E;

typedef enum VI_DEV_TYPE_E
{
    VI_DEV_MAIN = 0,
    VI_DEV_SUB  = 1,
    VI_DEV_JPEG = 2,
    VI_DEV_TYPE_MAX
} VI_DEV_TYPE_E;

typedef enum VENC_CHN_TYPE_E
{
    VENC_CHN_MAIN_STREAM      = 0,
    VENC_CHN_SUB_STREAM       = 1,
    VENC_CHN_SUB_LAPSE_STREAM = 2,
    VENC_CHN_TYPE_MAX
} VENC_CHN_TYPE_E;

typedef struct EncConfig
{
    // main stream
    ISP_DEV mMainIsp;
    VI_DEV mMainVipp;
    VI_CHN mMainViChn;
    int mMainSrcWidth;
    int mMainSrcHeight;
    PIXEL_FORMAT_E mMainPixelFormat;
    int mMainWdrEnable;
    int mMainViBufNum;
    int mMainSrcFrameRate;
    VENC_CHN mMainVEncChn;
    PAYLOAD_TYPE_E mMainEncodeType;
    int mMainEncodeWidth;
    int mMainEncodeHeight;
    int mMainEncodeFrameRate;
    int mMainEncodeBitrate;
    int mMainOnlineEnable;
    int mMainOnlineShareBufNum;
    BOOL mMainEncppEnable;
    BOOL mMainVippCropEnable;
    int mMainVippCropRectX;
    int mMainVippCropRectY;
    int mMainVippCropRectWidth;
    int mMainVippCropRectHeight;

    // sub stream
    ISP_DEV mSubIsp;
    VI_DEV mSubVipp;
    VI_CHN mSubViChn;
    int mSubSrcWidth;
    int mSubSrcHeight;
    PIXEL_FORMAT_E mSubPixelFormat;
    int mSubWdrEnable;
    int mSubViBufNum;
    int mSubSrcFrameRate;
    VENC_CHN mSubVEncChn;
    PAYLOAD_TYPE_E mSubEncodeType;
    int mSubEncodeWidth;
    int mSubEncodeHeight;
    int mSubEncodeFrameRate;
    int mSubEncodeBitrate;
    int mSubEncppSharpAttenCoefPer;
    BOOL mSubVippCropEnable;
    int mSubVippCropRectX;
    int mSubVippCropRectY;
    int mSubVippCropRectWidth;
    int mSubVippCropRectHeight;
    BOOL mSubEncppEnable;

    // isp and ve linkage
    BOOL mIspAndVeLinkageEnable;
    VENC_CHN_TYPE_E mIspAndVeLinkageStreamChn;

} EncConfig;

typedef struct VencStreamContext
{
    pthread_t mStreamThreadId;
    ISP_DEV mIsp;
    VI_DEV mVipp;
    VI_CHN mViChn;
    VI_ATTR_S mViAttr;
    VENC_CHN mVEncChn;
    VENC_CHN_ATTR_S mVEncChnAttr;
    VENC_RC_PARAM_S mVEncRcParam;
    VENC_FRAME_RATE_S mVEncFrameRateConfig;
    BOOL mbSubLapseEnable;
    int mStreamDataCnt;
    void *priv;
    int mEncppSharpAttenCoefPer;

    OSDContext_S mOSD[4];
}VencStreamContext;

typedef struct TakeJpegContext
{
    void *priv;
    //config params
    int mJpegWidth;
    int mJpegHeight;

    //component info
    VencStreamContext *mpConnectVipp;
    VI_CHN mViChn;

    VENC_CHN mVeChn;
    VENC_CHN_ATTR_S mVencChnAttr;
}TakeJpegContext;

typedef struct VencOsdContext
{
    pthread_t mStreamThreadId;
    RGN_HANDLE mOverlayHandle[VENC_MAX_CHN_NUM];
    PIXEL_FORMAT_E mPixelFormat;
    void *priv;
}VencOsdContext;

typedef struct PersonDetectContext
{
    pthread_t mStreamThreadId;
#if defined(NB_BETA_3_0_1) && NB_BETA_3_0_1 == 1
    awnn_info_t * pnbinfo;
#else
    Awnn_Det_Handle  ai_det_handle;
    region_info_t info;
#endif
    sGdcParam stGdcParam;
    VIDEO_FRAME_INFO_S mDstFrameInfo;
    ISP_DEV mIsp;
    VI_DEV mVipp;
    VI_CHN mViChn;
    int mSrcWidth;
    int mSrcHeight;
    float scale_w;
    float scale_h;

    long long mDetectTick;
    int mDetectArea;

    long long mLastDetectTick;

    unsigned int mFrameindex;

    void *priv;

    RGN_HANDLE mOrlHandles[20];
    int mValidOrlHandleNum;
}PersonDetectContext;

typedef struct EncContext
{
    EncConfig mConfigPara;

	int mbExitFlag;

    BOOL mMainIspRunFlag;
    BOOL mSubIspRunFlag;

    VencStreamContext mMainStream;
    VencStreamContext mSubStream;
    TakeJpegContext mTakeJpegStream;

    BOOL bMotionSearchEnable;
    VencMotionSearchResult mMotionResult;
    int mMotionNum;

    BOOL bPersonDetectEnable;
    PersonDetectContext mPersonDetect;

    int mMotionLevel;       //移动侦测报警灵敏度 1 低    2 中    3 高

    BOOL bMotionSearchEnable_bk;
    BOOL bPersonDetectEnable_bk;

    long long mLastAlarmTick;
}EncContext;

static void DrawStreamOSD(VENC_CHN nVEncChn, RGN_HANDLE nOSDIdx, const char *pContext, int nOSDType, int nDispX, int nDispY);
static void DestroyStreamOSD(VENC_CHN nVEncChn, RGN_HANDLE nOSDIdx);

static bool CacheBmpImage(const char *imgbmp, int x, int y, OSDContext_S *outOSD);
static void DrawStreamOSD2Enc(VENC_CHN nVEncChn, RGN_HANDLE nOSDIdx, OSDContext_S *OSDData);

static int alloc_frame_buffer(VIDEO_FRAME_INFO_S *pFrameInfo,int Width,int Height,PIXEL_FORMAT_E fmt)
{
    int result = 0;
    int pic_len = 0;

    pic_len = Width*Height;
    AW_MPI_SYS_MmzAlloc_Cached(&pFrameInfo->VFrame.mPhyAddr[0], \
        &pFrameInfo->VFrame.mpVirAddr[0], pic_len);
    if ((0 == pFrameInfo->VFrame.mPhyAddr[0]) || (NULL == pFrameInfo->VFrame.mpVirAddr[0]))
    {
        result = -1;
        printf("fatal error! alloc src frame buffer fail!");
        return result;
    }
    AW_MPI_SYS_MmzAlloc_Cached(&pFrameInfo->VFrame.mPhyAddr[1], \
        &pFrameInfo->VFrame.mpVirAddr[1], pic_len/2);
    if ((0 == pFrameInfo->VFrame.mPhyAddr[1]) || (NULL == pFrameInfo->VFrame.mpVirAddr[1]))
    {
        result = -1;
        printf("fatal error! alloc src frame buffer fail!");
        return result;
    }
    pFrameInfo->VFrame.mWidth = Width;
    pFrameInfo->VFrame.mHeight = Height;
    pFrameInfo->VFrame.mPixelFormat = fmt;

    return result;
}

static PIXEL_FORMAT_E StringToPicFormatFromConfig(char *pStrPixelFormat)
{
    PIXEL_FORMAT_E PicFormat = MM_PIXEL_FORMAT_BUTT;
    if (!strcmp(pStrPixelFormat, "nv21"))
    {
        PicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    else if (!strcmp(pStrPixelFormat, "yv12"))
    {
        PicFormat = MM_PIXEL_FORMAT_YVU_PLANAR_420;
    }
    else if (!strcmp(pStrPixelFormat, "nv12"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    }
    else if (!strcmp(pStrPixelFormat, "yu12"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_PLANAR_420;
    }
    else if (!strcmp(pStrPixelFormat, "aw_lbc_2_0x"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_AW_LBC_2_0X;
    }
    else if (!strcmp(pStrPixelFormat, "aw_lbc_2_5x"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_AW_LBC_2_5X;
    }
    else if (!strcmp(pStrPixelFormat, "aw_lbc_1_5x"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_AW_LBC_1_5X;
    }
    else if (!strcmp(pStrPixelFormat, "aw_lbc_1_0x"))
    {
        PicFormat = MM_PIXEL_FORMAT_YUV_AW_LBC_1_0X;
    }
    else
    {
        printf("fatal error! conf file pic_format is [%s]?\n", pStrPixelFormat);
        PicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }

    return PicFormat;
}

static PAYLOAD_TYPE_E StringToEncoderType(char *ptr)
{
    PAYLOAD_TYPE_E EncType = PT_BUTT;
    if(!strcmp(ptr, "H.264"))
    {
        EncType = PT_H264;
    }
    else if(!strcmp(ptr, "H.265"))
    {
        EncType = PT_H265;
    }
    else
    {
        printf("fatal error! conf file encoder[%s] is unsupported\n", ptr);
        EncType = PT_H264;
    }

    return EncType;
}

static ERRORTYPE initeEncConfig(EncConfig *pConfig)
{
    #define FRMAE_RATE_NUM 15
	// main stream
	pConfig->mMainIsp = 0;
	pConfig->mMainVipp = 0;
	pConfig->mMainViChn = 0;
	pConfig->mMainSrcWidth = 1920;
	pConfig->mMainSrcHeight = 1080;
	pConfig->mMainPixelFormat = StringToPicFormatFromConfig("nv21");//nv21 aw_lbc_2_0x
	pConfig->mMainWdrEnable = 1;
	pConfig->mMainViBufNum = 3;
	pConfig->mMainSrcFrameRate = FRMAE_RATE_NUM;
	pConfig->mMainVEncChn = 0;
	pConfig->mMainEncodeType = StringToEncoderType("H.265");
	pConfig->mMainEncodeWidth = 1920;//1024;
	pConfig->mMainEncodeHeight = 1080;//600;
	pConfig->mMainEncodeFrameRate = FRMAE_RATE_NUM;
	pConfig->mMainEncodeBitrate = 3*512*1024;
	pConfig->mMainOnlineEnable = 1;
	pConfig->mMainOnlineShareBufNum = 2;
	pConfig->mMainEncppEnable = 0;

#if WOAN_P3031FJ == 1 || FD == 1
	pConfig->mMainVippCropEnable = 0;
#else
    pConfig->mMainVippCropEnable = 0;
#endif
    pConfig->mMainVippCropEnable = 0;
    // v1.1.3版本
	// pConfig->mMainVippCropRectX = 35;
	// pConfig->mMainVippCropRectY = 0;
    // pConfig->mMainVippCropRectWidth = 0;
    // pConfig->mMainVippCropRectHeight = 0;

    // v1.1.4版本
	pConfig->mMainVippCropRectX = 256; // 32
	pConfig->mMainVippCropRectY = 64;
    pConfig->mMainVippCropRectWidth = 1920 - 256*2;
    pConfig->mMainVippCropRectHeight = 1080 - 64*2;

	// sub stream
	pConfig->mSubIsp = 0;
	pConfig->mSubVipp = 4;
	pConfig->mSubSrcWidth = 960;// 1920;
	pConfig->mSubSrcHeight = 544;//1080;
	pConfig->mSubPixelFormat = StringToPicFormatFromConfig("nv21");//
	pConfig->mSubWdrEnable = 1;
	pConfig->mSubViBufNum = 3;
	pConfig->mSubSrcFrameRate = FRMAE_RATE_NUM;

#if WOAN_P3031FJ == 1 || FD == 1
	pConfig->mSubVippCropEnable = 1;
#else
    pConfig->mSubVippCropEnable = 0;
#endif
    // v1.1.3版本这组参数客户说满意
	// pConfig->mSubVippCropRectX = 35;
	// pConfig->mSubVippCropRectY = 0;
    // pConfig->mSubVippCropRectWidth = 0;
    // pConfig->mSubVippCropRectHeight = 0;
    pConfig->mSubVippCropEnable = 0;
    // v1.1.4版本-->修改为v1.0.0
	pConfig->mSubVippCropRectX = 16*15; // 32
	pConfig->mSubVippCropRectY = 16*10;
    pConfig->mSubVippCropRectWidth = 1920 - 16*15*2;
    pConfig->mSubVippCropRectHeight = 1080 - 16*10*2;

	pConfig->mSubViChn = 0;
	pConfig->mSubVEncChn = 1;
	pConfig->mSubEncodeType = StringToEncoderType("H.265");
	pConfig->mSubEncodeWidth = 960;
	pConfig->mSubEncodeHeight = 540;
	pConfig->mSubEncodeFrameRate = FRMAE_RATE_NUM;
	pConfig->mSubEncodeBitrate = 4*512*1024;
    // pConfig->mSubEncodeBitrate = 2*1024*1024;
	pConfig->mSubEncppSharpAttenCoefPer = 100 * pConfig->mSubSrcWidth / pConfig->mMainSrcWidth;
	pConfig->mSubEncppEnable = 0;

	// isp and ve linkage
	pConfig->mIspAndVeLinkageEnable = 1;
	pConfig->mIspAndVeLinkageStreamChn = 0;
	printf("IspAndVeLinkage config: Enable=%d, StreamChn=%d", pConfig->mIspAndVeLinkageEnable, pConfig->mIspAndVeLinkageStreamChn);

    return SUCCESS;
}

static VencStreamContext *getStreamContext(EncContext *pContext, VENC_CHN mVEncChn)
{
    VencStreamContext *pStreamContext = NULL;

    if (mVEncChn == pContext->mMainStream.mVEncChn)
    {
        pStreamContext = &pContext->mMainStream;
    }
    else if (mVEncChn == pContext->mSubStream.mVEncChn)
    {
        pStreamContext = &pContext->mSubStream;
    }
    else
    {
//        aloge("fatal error! VencChn[%d] is not match, set pStreamContext = NULL!", mVEncChn);
        pStreamContext = NULL;
    }

    return pStreamContext;
}

static ERRORTYPE MPPCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    EncContext *pContext = (EncContext *)cookie;
    ERRORTYPE ret = 0;

    if (MOD_ID_VIU == pChn->mModId)
    {
        switch(event)
        {
            case MPP_EVENT_VI_TIMEOUT:
            {
                printf("receive vi timeout. vipp:%d, chn:%d", pChn->mDevId, pChn->mChnId);

                break;
            }
            default:
            {
                break;
            }
        }
    }
    else if (MOD_ID_VENC == pChn->mModId)
    {
        VENC_CHN mVEncChn = pChn->mChnId;

        VencStreamContext *pStreamContext = getStreamContext(pContext, mVEncChn);
        if (NULL == pStreamContext)
        {
            printf("fatal error! VenChn[%d] pStreamContext is NULL!\n", mVEncChn);
            return -1;
        }

        switch(event)
        {
/*
            case MPP_EVENT_LINKAGE_ISP2VE_PARAM:
            {
                Isp2VeLinkageParam stIsp2Ve;
                memset(&stIsp2Ve, 0, sizeof(Isp2VeLinkageParam));
                stIsp2Ve.mIspAndVeLinkageEnable = 1;
                stIsp2Ve.mCameraAdaptiveMovingAndStaticEnable = 0;
                stIsp2Ve.mVEncChn = mVEncChn;
                stIsp2Ve.mVipp = pStreamContext->mVipp;
                stIsp2Ve.pIsp2VeParam = (VencIsp2VeParam *)pEventData;
                stIsp2Ve.nEncppSharpAttenCoefPer = pStreamContext->mEncppSharpAttenCoefPer;
                int ret = setIsp2VeLinkageParam(&stIsp2Ve);
                if (ret)
                {
                    printf("fatal error, VEncChn[%d] set Isp2VeLinkageParam failed! ret=%d\n", mVEncChn, ret);
                    return -1;
                }
                break;
            }
            case MPP_EVENT_LINKAGE_VE2ISP_PARAM:
            {
                Ve2IspLinkageParam stVe2Isp;
                memset(&stVe2Isp, 0, sizeof(Ve2IspLinkageParam));
                stVe2Isp.mIspAndVeLinkageEnable = 1;
                stVe2Isp.mVEncChn = mVEncChn;
                stVe2Isp.mVipp = pStreamContext->mVipp;
                stVe2Isp.p2Ve2IspParam = (VencVe2IspParam *)pEventData;
                int ret = setVe2IspLinkageParam(&stVe2Isp);
                if (ret)
                {
                    printf("fatal error, VEncChn[%d] set Ve2IspLinkageParam failed! ret=%d\n", mVEncChn, ret);
                    return -1;
                }
                break;
            }
*/
            case MPP_EVENT_LINKAGE_ISP2VE_PARAM_EXTRA:
            {
                VENC_Isp2VeExtraParam *pExtraParam = (VENC_Isp2VeExtraParam *)pEventData;
                // if (pContext->mConfigPara.mCameraAdaptiveMovingAndStaticEnable)
                // {
                //     pExtraParam->eEnCameraMove = CAMERA_ADAPTIVE_MOVING_AND_STATIC;
                // }
                // else
                {
                    pExtraParam->eEnCameraMove = CAMERA_ADAPTIVE_STATIC;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    return SUCCESS;
}


static ERRORTYPE MPPCallbackWrapper_Jpeg(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    TakeJpegContext *pJpegCtx = (TakeJpegContext *)cookie;
    ERRORTYPE ret = 0;
    if(MOD_ID_VENC == pChn->mModId)
    {
        switch(event)
        {
            case MPP_EVENT_RELEASE_VIDEO_BUFFER:
            {
                VIDEO_FRAME_INFO_S *pVideoFrameInfo = (VIDEO_FRAME_INFO_S*)pEventData;
                if(NULL != pVideoFrameInfo)
                {
                    ret = AW_MPI_VI_ReleaseFrame(pJpegCtx->mpConnectVipp->mVipp, pJpegCtx->mViChn, pVideoFrameInfo);
                    if(ret != SUCCESS)
                    {
                       printf("fatal error! AW_MPI_VI ReleaseFrame vipp[%d]Chn[%d] failed[0x%x]", pJpegCtx->mpConnectVipp->mVipp, pJpegCtx->mViChn, ret);
                    }
                }
                break;
            }
            default:
            {
                printf("fatal error! unknown jpeg encoder event[%d]\n", event);
                break;
            }
        }
    }

    return SUCCESS;
}

static void configGdcPano180Param(int width,int height,sGdcParam *pGdcParam)
{
    pGdcParam->eWarpMode = Gdc_Warp_Pano180;//Gdc_Warp_Normal;
    pGdcParam->bMirror = 0;
    pGdcParam->calib_widht = width;
    pGdcParam->calib_height = height;

    /* lens paramters */
    if(width == 1920)
    {
        pGdcParam->cx = 884;
        pGdcParam->cy = 540;
        pGdcParam->innerRadius = 540;
    }
    else if(width == 960)
    {
        pGdcParam->cx = 442;
        pGdcParam->cy = 270;
        pGdcParam->innerRadius = 270;
    }
    else if(width == 320)
    {
        pGdcParam->cx = 156;
        pGdcParam->cy = 104;
        pGdcParam->innerRadius = 96;
    }

    pGdcParam->distCoef_fish_k[0] = 652;
    pGdcParam->eLensDistModel = Gdc_DistModel_FishEye;

    /* Normal correction parameters */
#if 0
    pGdcParam->pan = 0;
    pGdcParam->tilt = -50;
    pGdcParam->zoomH = 86;
    pGdcParam->zoomV = 50;
    pGdcParam->radialDistortCoef = -30;
    pGdcParam->fanDistortCoef = 0;
    pGdcParam->trapezoidDistortCoef = 0;
    pGdcParam->roll = 0;
    pGdcParam->pitch = 0;
    pGdcParam->yaw = 0;
    pGdcParam->eMountMode = Gdc_Mount_Wall;
#endif

    pGdcParam->pan = 0;
    pGdcParam->tilt = -51;
    pGdcParam->zoomH = 85;//90;
    pGdcParam->zoomV = 51;
    pGdcParam->radialDistortCoef = 20;
    pGdcParam->fanDistortCoef = 0;
    pGdcParam->trapezoidDistortCoef = -30;
    pGdcParam->roll = 0;
    pGdcParam->pitch = 0;
    pGdcParam->yaw = 0;
    pGdcParam->eMountMode = Gdc_Mount_Wall;

#if 0
    pGdcParam->pan = 0;
    pGdcParam->tilt = -51;
    pGdcParam->zoomH = 92;
    pGdcParam->zoomV = 51;
    pGdcParam->radialDistortCoef = 20;
    pGdcParam->fanDistortCoef = 0;
    pGdcParam->trapezoidDistortCoef = -30;
    pGdcParam->roll = 0;
    pGdcParam->pitch = 0;
    pGdcParam->yaw = 0;
    pGdcParam->eMountMode = Gdc_Mount_Wall;
#endif
}

static void configGdcNormalParam(int widht,int height,sGdcParam *pGdcParam)
{
    pGdcParam->eWarpMode = Gdc_Warp_Normal;//Gdc_Warp_Normal;Gdc_Warp_Fish2Wide
    pGdcParam->bMirror = 0;
    pGdcParam->calib_widht = widht;
    pGdcParam->calib_height = height;

    /* lens paramters */
    if(widht == 1920)
    {
        pGdcParam->cx = 884;
        pGdcParam->cy = 540;
        pGdcParam->innerRadius = 540;
    }
    else if(widht == 960)
    {
        pGdcParam->cx = 442;
        pGdcParam->cy = 270;
        pGdcParam->innerRadius = 270;
    }
    else if(widht == 320)
    {
        pGdcParam->cx = 156;
        pGdcParam->cy = 104;
        pGdcParam->innerRadius = 96;
    }

    pGdcParam->distCoef_fish_k[0] = 652;
    pGdcParam->eLensDistModel = Gdc_DistModel_FishEye; // Gdc_DistModel_FishEye; Gdc_DistModel_WideAngle

    /* Normal correction parameters */
    pGdcParam->pan = 0;
    pGdcParam->tilt = 0;
    pGdcParam->zoomH = 100;
    pGdcParam->scale = 100;
    pGdcParam->trapezoidDistortCoef = 0;
    pGdcParam->eMountMode = Gdc_Mount_Wall;
}

static void configMainStream(EncContext *pContext)
{
	bool bMainOnlineEnable = false;
    pContext->mMainStream.priv = (void*)pContext;
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;

    pContext->mMainStream.mIsp = 0;
    pContext->mMainStream.mVipp = 0;

    if (bMainOnlineEnable && (0 != pContext->mMainStream.mVipp))
    {
        printf("fatal error! main vipp %d is wrong, only vipp0 support online.\n", pContext->mMainStream.mVipp);
    }

    pContext->mMainStream.mViChn = 0;
    pContext->mMainStream.mVEncChn = 0;

    if (bMainOnlineEnable)
    {
        pContext->mMainStream.mViAttr.mOnlineEnable = 1;
        pContext->mMainStream.mViAttr.mOnlineShareBufNum = 2;//BK_TWO_BUFFER;
        pContext->mMainStream.mVEncChnAttr.VeAttr.mOnlineEnable = 1;
        pContext->mMainStream.mVEncChnAttr.VeAttr.mOnlineShareBufNum = 2;//BK_TWO_BUFFER;
    }
    printf("main vipp%d ve_online_en:%d, dma_buf_num:%d, venc ch%d OnlineEnable:%d, OnlineShareBufNum:%d\n",
        pContext->mMainStream.mVipp, pContext->mMainStream.mViAttr.mOnlineEnable, pContext->mMainStream.mViAttr.mOnlineShareBufNum,
        pContext->mMainStream.mVEncChn, pContext->mMainStream.mVEncChnAttr.VeAttr.mOnlineEnable, pContext->mMainStream.mVEncChnAttr.VeAttr.mOnlineShareBufNum);

    pContext->mMainStream.mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pContext->mMainStream.mViAttr.memtype = V4L2_MEMORY_MMAP;
    pContext->mMainStream.mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pContext->mConfigPara.mMainPixelFormat);
    pContext->mMainStream.mViAttr.format.field = V4L2_FIELD_NONE;
    pContext->mMainStream.mViAttr.format.colorspace = V4L2_COLORSPACE_REC709;
    pContext->mMainStream.mViAttr.format.width = pContext->mConfigPara.mMainSrcWidth;
    pContext->mMainStream.mViAttr.format.height = pContext->mConfigPara.mMainSrcHeight;
    pContext->mMainStream.mViAttr.fps = pContext->mConfigPara.mMainSrcFrameRate;
    pContext->mMainStream.mViAttr.use_current_win = 0;
    pContext->mMainStream.mViAttr.nbufs = pContext->mConfigPara.mMainViBufNum;
    pContext->mMainStream.mViAttr.nplanes = 2;
    pContext->mMainStream.mViAttr.wdr_mode = pContext->mConfigPara.mMainWdrEnable;
    pContext->mMainStream.mViAttr.capturemode = V4L2_MODE_VIDEO;
    pContext->mMainStream.mViAttr.drop_frame_num = 0;
    pContext->mMainStream.mViAttr.mbEncppEnable = TRUE;

    // old vbr bitrate control, 1:new vbr bitrate control
    pContext->mMainStream.mVEncChnAttr.VeAttr.mVbrOptEnable = 0;

    /* config vipp crop */
    pContext->mMainStream.mViAttr.mCropCfg.bEnable = pContext->mConfigPara.mMainVippCropEnable;
    pContext->mMainStream.mViAttr.mCropCfg.Rect.X = pContext->mConfigPara.mMainVippCropRectX;
    pContext->mMainStream.mViAttr.mCropCfg.Rect.Y = pContext->mConfigPara.mMainVippCropRectY;
    pContext->mMainStream.mViAttr.mCropCfg.Rect.Width = pContext->mConfigPara.mMainVippCropRectWidth;
    pContext->mMainStream.mViAttr.mCropCfg.Rect.Height = pContext->mConfigPara.mMainVippCropRectHeight;
    printf("vipp[%d] crop en:%d X:%d Y:%d W:%d H:%d", pContext->mMainStream.mVipp,
        pContext->mConfigPara.mMainVippCropEnable,
        pContext->mConfigPara.mMainVippCropRectX,
        pContext->mConfigPara.mMainVippCropRectY,
        pContext->mConfigPara.mMainVippCropRectWidth,
        pContext->mConfigPara.mMainVippCropRectHeight);

    pContext->mMainStream.mVEncChnAttr.VeAttr.Type = pContext->mConfigPara.mMainEncodeType;
    pContext->mMainStream.mVEncChnAttr.VeAttr.MaxKeyInterval = pContext->mConfigPara.mMainSrcFrameRate*2;//40;
    pContext->mMainStream.mVEncChnAttr.VeAttr.SrcPicWidth = pContext->mConfigPara.mMainSrcWidth;
    pContext->mMainStream.mVEncChnAttr.VeAttr.SrcPicHeight = pContext->mConfigPara.mMainSrcHeight;
    pContext->mMainStream.mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pContext->mMainStream.mVEncChnAttr.VeAttr.PixelFormat = pContext->mConfigPara.mMainPixelFormat;
    pContext->mMainStream.mVEncChnAttr.VeAttr.mColorSpace = pContext->mMainStream.mViAttr.format.colorspace;
    pContext->mMainStream.mVEncChnAttr.RcAttr.mProductMode = PRODUCT_STATIC_IPC;
#ifndef ENABLE_SUPER_FRAME
    pContext->mMainStream.mVEncChnAttr.VeAttr.mVeRecRefBufReduceEnable = 1;
#endif
    // pContext->mMainStream.mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

#if SUPPORT_ROTATE == 2
    pContext->mMainStream.mVEncChnAttr.VeAttr.Rotate = ROTATE_180;
#endif

    pContext->mMainStream.mEncppSharpAttenCoefPer = 100;
    printf("main EncppSharpAttenCoefPer: %d%%\n", pContext->mMainStream.mEncppSharpAttenCoefPer);

    pContext->mMainStream.mViAttr.mbEncppEnable = pContext->mConfigPara.mMainEncppEnable;
    //pContext->mMainStream.mVEncChnAttr.EncppAttr.mbEncppEnable = pContext->mConfigPara.mMainEncppEnable;
    pContext->mMainStream.mVEncChnAttr.EncppAttr.eEncppSharpSetting = 0;

    if (pContext->mConfigPara.mMainSrcFrameRate)
    {
        vbvThreshSize = pContext->mConfigPara.mMainEncodeBitrate/8/pContext->mConfigPara.mMainSrcFrameRate*15;
    }
    vbvBufSize = pContext->mConfigPara.mMainEncodeBitrate/8*4 + vbvThreshSize;
    printf("main vbvThreshSize: %u, vbvBufSize: %u\n", vbvThreshSize, vbvBufSize);

    if(PT_H264 == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.Profile = 1;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.PicWidth = pContext->mConfigPara.mMainEncodeWidth;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.PicHeight = pContext->mConfigPara.mMainEncodeHeight;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.mLevel = H264_LEVEL_51;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.mbPIntraEnable = TRUE;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.mThreshSize = vbvThreshSize;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH264e.BufSize = vbvBufSize;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mMaxBitRate = pContext->mConfigPara.mMainEncodeBitrate;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mSrcFrmRate = pContext->mConfigPara.mMainEncodeFrameRate;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mDstFrmRate = pContext->mConfigPara.mMainEncodeFrameRate;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264VBR;
        if (pContext->mMainStream.mVEncChnAttr.RcAttr.mRcMode == VENC_RC_MODE_H264VBR)
        {
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mMaxQp = 40;//45;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mMinQp = 6;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mMaxPqp = 40;//45;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mMinPqp = 6;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mQpInit = 32;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mbEnMbQpLimit = 1;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mMovingTh = 20;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mQuality = 10;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mIFrmBitsCoef = 10;
            pContext->mMainStream.mVEncRcParam.ParamH264Vbr.mPFrmBitsCoef = 10;
        }
        else if (pContext->mMainStream.mVEncChnAttr.RcAttr.mRcMode == VENC_RC_MODE_H264CBR)
        {
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mMaxQp = 40;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mMinQp = 6;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mMaxPqp = 40;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mMinPqp = 6;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mQpInit = 32;
            pContext->mMainStream.mVEncRcParam.ParamH264Cbr.mbEnMbQpLimit = 1;
        }
    }
    else if(PT_H265 == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mProfile = 0;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mPicWidth = pContext->mConfigPara.mMainEncodeWidth;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mPicHeight = pContext->mConfigPara.mMainEncodeHeight;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mLevel = 0;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mbPIntraEnable = TRUE;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mThreshSize = vbvThreshSize;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrH265e.mBufSize = vbvBufSize;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265VBR;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mMaxBitRate = pContext->mConfigPara.mMainEncodeBitrate;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mMaxQp = 50;//45;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mMinQp = 10;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mMaxPqp = 50;//45;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mMinPqp = 10;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mQpInit = 37;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mbEnMbQpLimit = 0;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mMovingTh = 20;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mQuality = 10;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mIFrmBitsCoef = 10;
        pContext->mMainStream.mVEncRcParam.ParamH265Vbr.mPFrmBitsCoef = 10;
    }
    else if(PT_MJPEG == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrMjpeg.mPicWidth= pContext->mConfigPara.mMainEncodeWidth;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrMjpeg.mPicHeight = pContext->mConfigPara.mMainEncodeHeight;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrMjpeg.mThreshSize = vbvThreshSize;
        pContext->mMainStream.mVEncChnAttr.VeAttr.AttrMjpeg.mBufSize = vbvBufSize;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
        pContext->mMainStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = pContext->mConfigPara.mMainEncodeBitrate;
    }
    pContext->mMainStream.mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pContext->mMainStream.mVEncChnAttr.GopAttr.mGopSize = 2;
    pContext->mMainStream.mVEncFrameRateConfig.SrcFrmRate = pContext->mConfigPara.mMainSrcFrameRate;
    pContext->mMainStream.mVEncFrameRateConfig.DstFrmRate = pContext->mConfigPara.mMainEncodeFrameRate;

#if defined(ENABLE_YY) && ENABLE_YY == 1
    {
        pContext->mMainStream.mVEncChnAttr.GdcAttr.bGDC_en = 1;
        // configGdcNormalParam(pContext->mConfigPara.mMainSrcWidth,pContext->mConfigPara.mMainSrcHeight,&pContext->mMainStream.mVEncChnAttr.GdcAttr);
        configGdcPano180Param(pContext->mConfigPara.mMainSrcWidth,pContext->mConfigPara.mMainSrcHeight,&pContext->mMainStream.mVEncChnAttr.GdcAttr);
    }
#endif

#if defined(SUPPORT_OSD_TEST) && SUPPORT_OSD_TEST == 1
    CacheBmpImage(BITMAPFILE_1920X1080_1, 20*2, 68*2, &pContext->mMainStream.mOSD[0]);
    CacheBmpImage(BITMAPFILE_1920X1080_2, 20*2, 68*2, &pContext->mMainStream.mOSD[1]);
#endif
}

static void configSubStream(EncContext *pContext)
{
    unsigned int vbvThreshSize = 0;
    unsigned int vbvBufSize = 0;

    pContext->mSubStream.priv = (void*)pContext;
    pContext->mSubStream.mIsp = pContext->mConfigPara.mSubIsp;
    pContext->mSubStream.mVipp = pContext->mConfigPara.mSubVipp;
    pContext->mSubStream.mViChn = pContext->mConfigPara.mSubViChn;
    pContext->mSubStream.mVEncChn = pContext->mConfigPara.mSubVEncChn;
    pContext->mSubStream.mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pContext->mSubStream.mViAttr.memtype = V4L2_MEMORY_MMAP;
    pContext->mSubStream.mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pContext->mConfigPara.mSubPixelFormat);
    pContext->mSubStream.mViAttr.format.field = V4L2_FIELD_NONE;
    pContext->mSubStream.mViAttr.format.colorspace = V4L2_COLORSPACE_REC709;
    pContext->mSubStream.mViAttr.format.width = pContext->mConfigPara.mSubSrcWidth;
    pContext->mSubStream.mViAttr.format.height = pContext->mConfigPara.mSubSrcHeight;
    pContext->mSubStream.mViAttr.fps = pContext->mConfigPara.mSubSrcFrameRate;
    pContext->mSubStream.mViAttr.use_current_win = 1;
    pContext->mSubStream.mViAttr.nbufs = pContext->mConfigPara.mSubViBufNum;
    pContext->mSubStream.mViAttr.nplanes = 2;
    pContext->mSubStream.mViAttr.wdr_mode = pContext->mConfigPara.mSubWdrEnable;
    pContext->mSubStream.mViAttr.drop_frame_num = 10;

    /* config vipp crop */
    pContext->mSubStream.mViAttr.mCropCfg.bEnable = pContext->mConfigPara.mSubVippCropEnable;
    pContext->mSubStream.mViAttr.mCropCfg.Rect.X = pContext->mConfigPara.mSubVippCropRectX;
    pContext->mSubStream.mViAttr.mCropCfg.Rect.Y = pContext->mConfigPara.mSubVippCropRectY;
    pContext->mSubStream.mViAttr.mCropCfg.Rect.Width = pContext->mConfigPara.mSubVippCropRectWidth;
    pContext->mSubStream.mViAttr.mCropCfg.Rect.Height = pContext->mConfigPara.mSubVippCropRectHeight;
    printf("vipp[%d] crop en:%d X:%d Y:%d W:%d H:%d", pContext->mSubStream.mVipp,
        pContext->mConfigPara.mSubVippCropEnable,
        pContext->mConfigPara.mSubVippCropRectX,
        pContext->mConfigPara.mSubVippCropRectY,
        pContext->mConfigPara.mSubVippCropRectWidth,
        pContext->mConfigPara.mSubVippCropRectHeight);

    // old vbr bitrate control, 1:new vbr bitrate control
    pContext->mSubStream.mVEncChnAttr.VeAttr.mVbrOptEnable = 0;

    pContext->mSubStream.mVEncChnAttr.VeAttr.Type = pContext->mConfigPara.mSubEncodeType;
    pContext->mSubStream.mVEncChnAttr.VeAttr.MaxKeyInterval = pContext->mConfigPara.mSubSrcFrameRate*2;//40;
    pContext->mSubStream.mVEncChnAttr.VeAttr.SrcPicWidth = pContext->mConfigPara.mSubSrcWidth;
    pContext->mSubStream.mVEncChnAttr.VeAttr.SrcPicHeight = pContext->mConfigPara.mSubSrcHeight;
    pContext->mSubStream.mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pContext->mSubStream.mVEncChnAttr.VeAttr.PixelFormat = pContext->mConfigPara.mSubPixelFormat;
    pContext->mSubStream.mVEncChnAttr.VeAttr.mColorSpace = pContext->mSubStream.mViAttr.format.colorspace;
    pContext->mSubStream.mVEncChnAttr.RcAttr.mProductMode = PRODUCT_STATIC_IPC;
    // pContext->mSubStream.mVEncRcParam.sensor_type = VENC_ST_EN_WDR;

#if SUPPORT_ROTATE == 2
    pContext->mSubStream.mVEncChnAttr.VeAttr.Rotate = ROTATE_180;
#endif

    pContext->mSubStream.mEncppSharpAttenCoefPer = pContext->mConfigPara.mSubEncppSharpAttenCoefPer;
    printf("sub EncppSharpAttenCoefPer: %d%%\n", pContext->mSubStream.mEncppSharpAttenCoefPer);

    pContext->mSubStream.mViAttr.mbEncppEnable = pContext->mConfigPara.mSubEncppEnable;
    pContext->mSubStream.mVEncChnAttr.EncppAttr.eEncppSharpSetting = 0;//pContext->mConfigPara.mSubEncppEnable;

    if (pContext->mConfigPara.mSubSrcFrameRate)
    {
        vbvThreshSize = pContext->mConfigPara.mSubEncodeBitrate/8/pContext->mConfigPara.mSubSrcFrameRate*15;
    }
    vbvBufSize = pContext->mConfigPara.mSubEncodeBitrate/8*4 + vbvThreshSize;
    printf("sub vbvThreshSize: %u, vbvBufSize: %u\n", vbvThreshSize, vbvBufSize);

    if(PT_H264 == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.Profile = 1;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.PicWidth = pContext->mConfigPara.mSubEncodeWidth;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.PicHeight = pContext->mConfigPara.mSubEncodeHeight;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.mLevel = 0;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.mbPIntraEnable = TRUE;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.mThreshSize = vbvThreshSize;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH264e.BufSize = vbvBufSize;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mMaxBitRate = pContext->mConfigPara.mSubEncodeBitrate;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mSrcFrmRate = pContext->mConfigPara.mMainEncodeFrameRate;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH264Vbr.mDstFrmRate = pContext->mConfigPara.mMainEncodeFrameRate;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264VBR;
        if (pContext->mSubStream.mVEncChnAttr.RcAttr.mRcMode == VENC_RC_MODE_H264VBR)
        {
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mMaxQp = 40;//45;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mMinQp = 6;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mMaxPqp = 40;//45;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mMinPqp = 6;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mQpInit = 32;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mbEnMbQpLimit = 1;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mMovingTh = 20;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mQuality = 10;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mIFrmBitsCoef = 10;
            pContext->mSubStream.mVEncRcParam.ParamH264Vbr.mPFrmBitsCoef = 10;
        }
        else if (pContext->mSubStream.mVEncChnAttr.RcAttr.mRcMode == VENC_RC_MODE_H264CBR)
        {
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mMaxQp = 45;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mMinQp = 6;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mMaxPqp = 45;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mMinPqp = 6;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mQpInit = 30;
            pContext->mSubStream.mVEncRcParam.ParamH264Cbr.mbEnMbQpLimit = 1;
        }
    }
    else if(PT_H265 == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mProfile = 0;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mPicWidth = pContext->mConfigPara.mSubEncodeWidth;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mPicHeight = pContext->mConfigPara.mSubEncodeHeight;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mLevel = 0;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mbPIntraEnable = TRUE;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mThreshSize = vbvThreshSize;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrH265e.mBufSize = vbvBufSize;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265VBR;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrH265Vbr.mMaxBitRate = pContext->mConfigPara.mSubEncodeBitrate;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mMaxQp = 50;//45;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mMinQp = 10;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mMaxPqp = 50;//45;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mMinPqp = 10;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mQpInit = 37;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mbEnMbQpLimit = 0;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mMovingTh = 20;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mQuality = 10;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mIFrmBitsCoef = 10;
        pContext->mSubStream.mVEncRcParam.ParamH265Vbr.mPFrmBitsCoef = 10;
    }
    else if(PT_MJPEG == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
    {
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrMjpeg.mPicWidth= pContext->mConfigPara.mSubEncodeWidth;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrMjpeg.mPicHeight = pContext->mConfigPara.mSubEncodeHeight;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrMjpeg.mThreshSize = vbvThreshSize;
        pContext->mSubStream.mVEncChnAttr.VeAttr.AttrMjpeg.mBufSize = vbvBufSize;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
        pContext->mSubStream.mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = pContext->mConfigPara.mSubEncodeBitrate;
    }
    pContext->mSubStream.mVEncChnAttr.GopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pContext->mSubStream.mVEncChnAttr.GopAttr.mGopSize = 2;
    pContext->mSubStream.mVEncFrameRateConfig.SrcFrmRate = pContext->mConfigPara.mSubSrcFrameRate;
    pContext->mSubStream.mVEncFrameRateConfig.DstFrmRate = pContext->mConfigPara.mSubEncodeFrameRate;

#if defined(ENABLE_YY) && ENABLE_YY == 1
    {
        pContext->mSubStream.mVEncChnAttr.GdcAttr.bGDC_en = 1;
        // configGdcNormalParam(pContext->mConfigPara.mSubSrcWidth,pContext->mConfigPara.mSubSrcHeight,&pContext->mSubStream.mVEncChnAttr.GdcAttr);
        configGdcPano180Param(pContext->mConfigPara.mSubSrcWidth,pContext->mConfigPara.mSubSrcHeight,&pContext->mSubStream.mVEncChnAttr.GdcAttr);
    }
#endif

#if 0
    CacheBmpImage(BITMAPFILE_960X540_1, 20, 68, &pContext->mSubStream.mOSD[0]);
    CacheBmpImage(BITMAPFILE_960X540_2, 20, 68, &pContext->mSubStream.mOSD[1]);
#endif
}

static void configTakePictrueStream(EncContext *pContext)
{
    pContext->mTakeJpegStream.priv = (void*)pContext;
    pContext->mTakeJpegStream.mpConnectVipp = &pContext->mMainStream;
    pContext->mTakeJpegStream.mViChn = 1;
    pContext->mTakeJpegStream.mVeChn = 2;
    pContext->mTakeJpegStream.mJpegWidth = 1920;
    pContext->mTakeJpegStream.mJpegHeight = 1080;
    memset(&pContext->mTakeJpegStream.mVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.Type = PT_JPEG;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.AttrJpeg.MaxPicWidth = 0;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.AttrJpeg.MaxPicHeight = 0;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.AttrJpeg.BufSize = AWALIGN(pContext->mTakeJpegStream.mJpegWidth*pContext->mTakeJpegStream.mJpegHeight*3/2, 1024);
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.AttrJpeg.bByFrame = TRUE;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.AttrJpeg.PicWidth = pContext->mTakeJpegStream.mJpegWidth;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.AttrJpeg.PicHeight = pContext->mTakeJpegStream.mJpegHeight;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.AttrJpeg.bSupportDCF = FALSE;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.MaxKeyInterval = 1;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.SrcPicWidth = pContext->mConfigPara.mMainSrcWidth;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.SrcPicHeight = pContext->mConfigPara.mMainSrcHeight;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.PixelFormat = pContext->mConfigPara.mMainPixelFormat;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.mColorSpace = V4L2_COLORSPACE_JPEG;
    pContext->mTakeJpegStream.mVencChnAttr.VeAttr.Rotate = ROTATE_NONE;

#if defined(ENABLE_YY) && ENABLE_YY == 1
    {
        pContext->mTakeJpegStream.mVencChnAttr.GdcAttr.bGDC_en = 1;
        // configGdcNormalParam(pContext->mConfigPara.mMainSrcWidth,pContext->mConfigPara.mMainSrcHeight,&pContext->mTakeJpegStream.mVencChnAttr.GdcAttr);
        configGdcPano180Param(pContext->mConfigPara.mMainSrcWidth,pContext->mConfigPara.mMainSrcHeight,&pContext->mTakeJpegStream.mVencChnAttr.GdcAttr);
    }
#endif
}

static void configPersonDetect(EncContext *pContext)
{
    pContext->mPersonDetect.mIsp = 0;
    pContext->mPersonDetect.mVipp = 8;
    pContext->mPersonDetect.mViChn = 0;

#if defined(NB_BETA_3_0_1) && NB_BETA_3_0_1 == 1
    pContext->mPersonDetect.mSrcWidth = pContext->mPersonDetect.pnbinfo->width;
    pContext->mPersonDetect.mSrcHeight = pContext->mPersonDetect.pnbinfo->height;
    pContext->mPersonDetect.scale_w = pContext->mMainStream.mViAttr.format.width/(float)pContext->mPersonDetect.mSrcWidth;
    pContext->mPersonDetect.scale_h = pContext->mMainStream.mViAttr.format.height/(float)pContext->mPersonDetect.mSrcHeight;
#else
    pContext->mPersonDetect.mSrcWidth = NN_HUMAN_SRC_WIDTH;
    pContext->mPersonDetect.mSrcHeight = NN_HUMAN_SRC_HEIGHT;
    pContext->mPersonDetect.info.old_num_of_boxes = 0;
    pContext->mPersonDetect.info.region_hdl_base = 20;
    pContext->mPersonDetect.scale_w = pContext->mSubStream.mViAttr.format.width/(float)pContext->mPersonDetect.mSrcWidth;
    pContext->mPersonDetect.scale_h = pContext->mSubStream.mViAttr.format.height/(float)pContext->mPersonDetect.mSrcHeight;
#endif


#if defined(ENABLE_YY) && ENABLE_YY == 1
    {
        alloc_frame_buffer(&pContext->mPersonDetect.mDstFrameInfo,pContext->mPersonDetect.mSrcWidth,pContext->mPersonDetect.mSrcHeight,MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420);
        pContext->mPersonDetect.stGdcParam.bGDC_en = 1;
        // configGdcNormalParam(pContext->mConfigPara.mSubSrcWidth,pContext->mConfigPara.mSubSrcHeight,&pContext->mPersonDetect.stGdcParam);
        configGdcPano180Param(pContext->mConfigPara.mSubSrcWidth,pContext->mConfigPara.mSubSrcHeight,&pContext->mPersonDetect.stGdcParam);
    }
#endif

    pContext->mPersonDetect.priv = (void*)pContext;
}

static EncContext enc;

void video_enc_printf()
{
    // video_check_night();
}

static bool checkmotionalarm(EncContext *pContext,int mvnum)
{
    bool balarm = false;
    long long curtick = DPGetTickCount64();
    // static long long last_alarmtick = 0;
    // printf("checkmotionalarm.level:%d num:%d %lld %lld area:%d\n",pContext->mMotionLevel,mvnum,curtick,pContext->mPersonDetect.mDetectTick,pContext->mPersonDetect.mDetectArea);
    if(curtick - pContext->mPersonDetect.mDetectTick > 400000)
        return false;

#if 0 // TEST
    balarm = true;
#elif defined(ENABLE_YY) && ENABLE_YY == 1
    #if 0
    if(pContext->mMotionLevel == 1) //低
    {
        // if(mvnum >= 10 && pContext->mPersonDetect.mDetectArea >= 500*500)
        if(mvnum >= 25 && pContext->mPersonDetect.mDetectArea >= 8000)
            balarm = true;
    }
    else if(pContext->mMotionLevel == 2) //中
    {
        // if(mvnum >= 6 && pContext->mPersonDetect.mDetectArea >= 300*300)
        if(mvnum >= 10 && pContext->mPersonDetect.mDetectArea >= 3000)
            balarm = true;
    }
    else if(pContext->mMotionLevel == 3) //高
    {
        // if(mvnum >= 4 && pContext->mPersonDetect.mDetectArea >= 200*200)
        if(mvnum >= 2 && pContext->mPersonDetect.mDetectArea >= 1500)
            balarm = true;
    }
    else
        return false;
    #else
    if(pContext->mMotionLevel == 1) //低
    {
        // if(mvnum >= 10 && pContext->mPersonDetect.mDetectArea >= 500*500)
        if(mvnum >= 0 && pContext->mPersonDetect.mDetectArea >= 8000)
            balarm = true;
    }
    else if(pContext->mMotionLevel == 2) //中
    {
        // if(mvnum >= 6 && pContext->mPersonDetect.mDetectArea >= 300*300)
        if(mvnum >= 0 && pContext->mPersonDetect.mDetectArea >= 3000)
            balarm = true;
    }
    else if(pContext->mMotionLevel == 3) //高
    {
        // if(mvnum >= 4 && pContext->mPersonDetect.mDetectArea >= 200*200)
        if(mvnum >= 0 && pContext->mPersonDetect.mDetectArea >= 1500)
            balarm = true;
    }
    else
        return false;
    #endif
#else
    if(pContext->mMotionLevel == 1) //低
    {
        // if(mvnum >= 10 && pContext->mPersonDetect.mDetectArea >= 500*500)
        if(mvnum >= 70 && pContext->mPersonDetect.mDetectArea >= 8000)
            balarm = true;
    }
    else if(pContext->mMotionLevel == 2) //中
    {
        // if(mvnum >= 6 && pContext->mPersonDetect.mDetectArea >= 300*300)
        if(mvnum >= 25 && pContext->mPersonDetect.mDetectArea >= 5000)
            balarm = true;
    }
    else if(pContext->mMotionLevel == 3) //高
    {
        // if(mvnum >= 4 && pContext->mPersonDetect.mDetectArea >= 200*200)
        if(mvnum >= 7 && pContext->mPersonDetect.mDetectArea >= 2500)
            balarm = true;
    }
    else
        return false;
#endif

    // if (balarm && curtick - last_alarmtick > 5*1000000)
    if (balarm)
    {
        // if(curtick - pContext->mLastAlarmTick > 5*1000000)
        // {
        //     // video_md_enable(false);
        //     DPPostMessage(MSG_ALARM, 0, 0, 0);
        //     // last_alarmtick = curtick;
        //     pContext->mLastAlarmTick = curtick;
        // }
        // else
        //     DPInfo("Alarm too frequent.%lld\n",curtick - pContext->mLastAlarmTick);
    }
    return true;
}

static void* getVencStreamThread(void *pThreadData)
{
    VencStreamContext *pStreamContext = (VencStreamContext*)pThreadData;
    EncContext *pContext = (EncContext*)pStreamContext->priv;
//    char strThreadName[32];
//    sprintf(strThreadName, "venc%d-stream", pStreamContext->mVEncChn);
//    prctl(PR_SET_NAME, (unsigned long)strThreadName, 0, 0, 0);

    ERRORTYPE ret = SUCCESS;
    int result = 0;
    unsigned int nStreamLen = 0;
	unsigned char seq = 0;
    long long starpts = 0;

#if defined(SUPPORT_MVNUM_PRINT) && SUPPORT_MVNUM_PRINT == 1
    static long long mvnum_print_lasktick = 0;
    int print_len = 0;
    char mvnum_printarray[1024 * 4] = {0};
    int print_col = 0;
#endif

    VENC_STREAM_S stVencStream;
    VENC_PACK_S stVencPack;
    memset(&stVencStream, 0, sizeof(stVencStream));
    memset(&stVencPack, 0, sizeof(stVencPack));
    stVencStream.mPackCount = 1;
    stVencStream.mpPack = &stVencPack;

    //get spspps from venc.
    VencHeaderData stSpsPpsInfo;
    memset(&stSpsPpsInfo, 0, sizeof(stSpsPpsInfo));
    if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        ret = AW_MPI_VENC_GetH264SpsPpsInfo(pStreamContext->mVEncChn, &stSpsPpsInfo);
        if(ret != SUCCESS)
        {
            printf("fatal error! get spspps fail[0x%x]!\n", ret);
        }
    }
    else if(PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
    {
        ret = AW_MPI_VENC_GetH265SpsPpsInfo(pStreamContext->mVEncChn, &stSpsPpsInfo);
        if(ret != SUCCESS)
        {
            printf("fatal error! get spspps fail[0x%x]!\n", ret);
        }
    }
    else
    {
        printf("fatal error! vencType:0x%x is wrong!\n", pStreamContext->mVEncChnAttr.VeAttr.Type);
    }

#if defined(SUPPORT_RTSP_TEST) && SUPPORT_RTSP_TEST == 1
    if (VENC_CHN_MAIN_STREAM == pStreamContext->mVEncChn)
    {
        rtsp_start(0);
    }
    else if (VENC_CHN_SUB_STREAM == pStreamContext->mVEncChn)
    {
        rtsp_start(1);
    }
#endif

    int maxframelen = MAX_FRAME_BUF_SiZE;
    if(pStreamContext == &(pContext->mSubStream))
        maxframelen = MAX_FRAME_BUF_SiZE/2;
	media_frame * ppacket = (media_frame*)malloc(sizeof(media_frame) + maxframelen);
	if(ppacket == NULL)
	{
		printf("encode_get_frame malloc fail\n");
		return NULL;
	}

    pStreamContext->mStreamDataCnt = 0;
#define SAVE_H265_FILE  1
#if SAVE_H265_FILE
    int nsaveseq = 0;
    FILE * tfile = NULL;
    //if(pStreamContext->mVEncChn == 1)

    char filename[128] = {0};
    sprintf(filename, "./enc_%d.%s", pStreamContext->mVEncChn, pStreamContext->mVEncChnAttr.VeAttr.Type == PT_H265 ? "h265" : "264");
    tfile = fopen(filename,"wb");
//        tfile = fopen("/mnt/nfs/enc.h264","wb");
        //tfile = fopen("./enc.h265","wb");
#endif
    int nMilliSec = 1000;
    // int costtick = 0;
    printf("++++get enc stream:%d+++++\n",pStreamContext->mVEncChn);
    while (!pContext->mbExitFlag)
    {
        // costtick = DPGetTickCount();
        memset(stVencStream.mpPack, 0, sizeof(VENC_PACK_S));
        ret = AW_MPI_VENC_GetStream(pStreamContext->mVEncChn, &stVencStream, nMilliSec);
        if(SUCCESS == ret)
        {
            // if (pStreamContext->mVEncChn == 0)
            //     printf("++++get enc stream:%d+++++ cost %u\n",pStreamContext->mVEncChn, DPGetTickCount() - costtick);
            if (pStreamContext == &(pContext->mSubStream) && pContext->bMotionSearchEnable)
            //if (pStreamContext == &(pContext->mMainStream) && pContext->bMotionSearchEnable)
            {
                ret = AW_MPI_VENC_GetMotionSearchResult(pStreamContext->mVEncChn, &pContext->mMotionResult);
                if (SUCCESS == ret)
                {
                    int num = 0;
                    int i = 0;
                    for(i=0; i<pContext->mMotionResult.total_region_num; i++)
                    {
                        if(pContext->mMotionResult.region[i].is_motion)
                        {
                //            printf("area_%d:[(%d,%d),(%d,%d)]\n", i,
                //                pContext->mMotionResult.region[i].pix_x_bgn, pContext->mMotionResult.region[i].pix_y_bgn,
                //                pContext->mMotionResult.region[i].pix_x_end, pContext->mMotionResult.region[i].pix_y_end);
                            num++;
                        }
                    }

                #if defined(SUPPORT_MVNUM_PRINT) && SUPPORT_MVNUM_PRINT == 1
                    print_len += sprintf(mvnum_printarray + print_len, "%d ", num);
                    print_col++;

                    if (print_col > 9)
                    {
                        print_col = 0;
                        print_len += sprintf(mvnum_printarray + print_len, "%s ", "\n");
                    }

                    long long curTick = DPGetTickCount64();
                    if(curTick - mvnum_print_lasktick > 5000000)
                    {
                        print_len += sprintf(mvnum_printarray + print_len, "%s ", "\n\n\n\n");
                        printf("detect motion results in 5s: \n%s\n", mvnum_printarray);
                        print_len = 0;
                        memset(mvnum_printarray, 0, sizeof(mvnum_printarray));
                        mvnum_print_lasktick = curTick;

                        print_len += sprintf(mvnum_printarray + print_len, "%s ", "\n\n\n\n");
                    }
                #endif

                    // if (true)
                    if(num >= 2)
                        checkmotionalarm(pContext,num);

                    pContext->mMotionNum = num;
                }
            }

            pStreamContext->mStreamDataCnt++;
            nStreamLen = stVencStream.mpPack[0].mLen0 + stVencStream.mpPack[0].mLen1 + stVencStream.mpPack[0].mLen2;
            if (nStreamLen <= 0)
            {
                printf("fatal error! VencStream length error,[%d,%d,%d]!\n", stVencStream.mpPack[0].mLen0, stVencStream.mpPack[0].mLen1, stVencStream.mpPack[0].mLen2);
            }

            if (stVencStream.mpPack != NULL && stVencStream.mpPack->mLen0 > 0)
            {
                uint64_t pts = stVencStream.mpPack->mPTS;
                int len = 0;
                int frame_type = -1;

                unsigned char * ptr = (unsigned char *)ppacket->data;
                if(PT_H264 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                {
                    if (H264E_NALU_ISLICE == stVencStream.mpPack->mDataType.enH264EType)
                    {
                        if (NULL == stSpsPpsInfo.pBuffer)
                        {
                            printf("SpsPpsInfo.pBuffer = NULL!!\n");
                        }
                        /* Get sps/pps first */
                        memcpy(ptr, stSpsPpsInfo.pBuffer, stSpsPpsInfo.nLength);
                        len += stSpsPpsInfo.nLength;
                        frame_type = 1;
                    }
                    else
                    {
                        frame_type = 0;
                    }
                }
                else if(PT_H265 == pStreamContext->mVEncChnAttr.VeAttr.Type)
                {
                    if (H265E_NALU_ISLICE == stVencStream.mpPack->mDataType.enH265EType)
                    {
                        if (NULL == stSpsPpsInfo.pBuffer)
                        {
                            printf("SpsPpsInfo.pBuffer = NULL!!\n");
                        }
                        /* Get sps/pps first */
                        memcpy(ptr, stSpsPpsInfo.pBuffer, stSpsPpsInfo.nLength);
                        len += stSpsPpsInfo.nLength;
                        frame_type = 1;
                    }
                    else
                    {
                        frame_type = 0;
                    }
                }
                else
                {
                    printf("fatal error! vencType:0x%x is wrong!\n", pStreamContext->mVEncChnAttr.VeAttr.Type);
                }

                if (MAX_FRAME_BUF_SiZE > len + stVencStream.mpPack->mLen0)
                {
                    memcpy(ptr + len, stVencStream.mpPack->mpAddr0, stVencStream.mpPack->mLen0);
                    len += stVencStream.mpPack->mLen0;
                }
                else
                    printf("+++++++++++++++++++++++++++++++++++++++++++++ stVencStream.mpPack->mLen0 frame too big\r\n");

                if (stVencStream.mpPack->mLen1 > 0)
                {
                    if (MAX_FRAME_BUF_SiZE > (len + stVencStream.mpPack->mLen1))
                    {
                        memcpy(ptr + len, stVencStream.mpPack->mpAddr1, stVencStream.mpPack->mLen1);
                        len += stVencStream.mpPack->mLen1;
                    }
                    else
                    printf("+++++++++++++++++++++++++++++++++++++++++++++ stVencStream.mpPack->mLen1 frame too big\r\n");
                }
                if (stVencStream.mpPack->mLen2 > 0)
                {
                    if (MAX_FRAME_BUF_SiZE > (len + stVencStream.mpPack->mLen2))
                    {
                        memcpy(ptr + len, stVencStream.mpPack->mpAddr2, stVencStream.mpPack->mLen2);
                        len += stVencStream.mpPack->mLen2;
                    }
                    else
                    printf("+++++++++++++++++++++++++++++++++++++++++++++ stVencStream.mpPack->mLen2 frame too big\r\n");
                }
#if SAVE_H265_FILE
//                if(pStreamContext->mVEncChn == 1 && nsaveseq < 100)
                if(tfile )
                {
//                    printf("save stream:%d\n",nsaveseq);
                    nsaveseq++;
                    fwrite(ppacket->data,1,len,tfile);
//                    if(nsaveseq == 100)
//                        fclose(tfile);
                }
#endif
//                printf("get stream:%d %d %d %lld\n",pStreamContext->mVEncChn,frame_type,len,pts);

//                if(pStreamContext->mVEncChn == 1)
                {
                    if(starpts == 0)
                        starpts = pts;
                    pts = pts - starpts;
    //				printf("get stream:%d %d %d %lld\n",pStreamContext->mVEncChn,frame_type,len,pts);
                    ppacket->check = 0xabcd1234;
                    ppacket->bkey = frame_type;
                    ppacket->timestamp = (pts + 500)/1000;
                    ppacket->seq = seq++;
                    ppacket->streamid = 0;
                    if(pStreamContext->mVEncChn==0)
                    mediatrans_send(pStreamContext->mVEncChn,ppacket,len + sizeof(media_frame));
                    //if (pStreamContext->mVEncChn == 1)
                    //    printf("<%d>get stream:%d %d %d %u %u\n", DPGetTickCount(),pStreamContext->mVEncChn, frame_type, len, ppacket->timestamp, ppacket->seq);
                }

#if defined(SUPPORT_RTSP_TEST) && SUPPORT_RTSP_TEST == 1
                //if(DevTestRtspPlay())
                {
                    //printf("send stream:%d %d %d %lld\n",pStreamContext->mVEncChn,frame_type,len,pts);
                    RtspSendDataParam stRtspParam;
                    memset(&stRtspParam, 0, sizeof(RtspSendDataParam));
                    stRtspParam.buf = ptr;
                    stRtspParam.size = len;
                    stRtspParam.frame_type = frame_type  == 0 ? 2 : 0;
                    stRtspParam.pts = pts;

                    if (VENC_CHN_MAIN_STREAM == pStreamContext->mVEncChn)
                    {
                        rtsp_sendData(0, &stRtspParam);
                    }
                    else if (VENC_CHN_SUB_STREAM == pStreamContext->mVEncChn)
                    {
                        rtsp_sendData(1, &stRtspParam);
                    }
                }
#endif
            }

            ret = AW_MPI_VENC_ReleaseStream(pStreamContext->mVEncChn, &stVencStream);
            if(ret != SUCCESS)
            {
                printf("fatal error! venc_chn[%d] releaseStream fail\n", pStreamContext->mVEncChn);
            }
        }
        else
        {
            printf("fatal error! vencChn[%d] get frame failed! check code!\n", pStreamContext->mVEncChn);
            continue;
        }
    }
    printf("-----get enc stream:%d-----\n",pStreamContext->mVEncChn);
    if (ppacket)
        free(ppacket);
#if SAVE_H265_FILE
    if (tfile)
    {
        fclose(tfile);
        tfile = NULL;
    }

#endif
    printf("exit\n");

    return (void*)result;
}
#if 0
static int npu_detect_callback_body(void * userdata,unsigned char *pBuffer, int vipp, void *pRunParam)
{
    BBoxResults_t *res = NULL;
    unsigned char *body_input_buf[2] = { NULL, NULL };
    PersonDetectContext *pctx = (PersonDetectContext*)userdata;
    EncContext *pContext = (EncContext*)pctx->priv;
    int ret = 0;

    int size = pctx->mSrcWidth * pctx->mSrcHeight;

    pctx->mFrameindex++;
    if(pctx->mFrameindex & 1)
        return 0;

    // alogd("0x%x, 0x%x, 0x%x, 0x%x.", pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3]);
    body_input_buf[0] = pBuffer;
    body_input_buf[1] = pBuffer + size;

    if(pctx->ai_det_handle == NULL)
    {
        printf("fata error, ai det handle should not be null.\n");
        ret = -1;
    }
    else
    {
        ((Awnn_Run_t *)pRunParam)->input_buffers = body_input_buf;
        res = awnn_detect_run(pctx->ai_det_handle, (Awnn_Run_t *)pRunParam);
        if (res)
        {
//            printf("awnn_detect_run:%d\n",res->valid_cnt);
//            paint_object_detect_region(pContext->mSubStream.mVipp,&pctx->info,res,pctx->scale_w,pctx->scale_h);
            int maxarea = 0;
            int area;
            for (int j = 0; j < res->valid_cnt; j ++)
            {
                if(res->boxes[j].score < 0.55)
                    continue;
                printf("vipp=%d, cls %d, prob %f, rect [%d, %d, %d, %d]\n", vipp, res->boxes[j].label, res->boxes[j].score,
                      res->boxes[j].xmin, res->boxes[j].ymin, res->boxes[j].xmax, res->boxes[j].ymax);

                ret++;
                area = (res->boxes[j].xmax - res->boxes[j].xmin)*(res->boxes[j].ymax - res->boxes[j].ymin);
                if(maxarea < area)
                    maxarea = area;
            }

            if(ret > 0)
            {
                long long curTick = DPGetTickCount64();
                printf("get person detecte.%lld %lld %d\n",curTick,pctx->mDetectTick,pctx->mDetectArea);
                if(curTick - pctx->mLastDetectTick < 300000)
                {
                    //300ms内再次检测到人形认为真检测到
                    pctx->mDetectArea = maxarea;
                    pctx->mDetectTick = curTick;
                }
                pctx->mLastDetectTick = curTick;
            }
            ret = res->valid_cnt;
        }
        else
        {
            printf("fatal error, res==NULL should not happen for object detection.\n");
        }
    }

    return ret;
}
#endif

static int deduceDstRect(RECT_S *pDstRect, SIZE_S *pDstSize, RECT_S *pSrcRect, SIZE_S *pSrcSize)
{
    //aw vipp prefers to keep proportion rather than keep picture content.
    //all vipps accord to isp's output proportion, maybe it is also vipp0's proportion.
    //For keeping proportion, vipp will cut picture before output to memory!
    //To simplify calculation, we assume dstSize has right proportion, then judge if vipp cut width or height.
    RECT_S stOrigSrcRect;
    SIZE_S stOrigSrcSize;
    if(pDstSize->Width*pSrcSize->Height == pSrcSize->Width*pDstSize->Height)
    { //dstW/dstH == srcW/srcH
        stOrigSrcSize = *pSrcSize;
        stOrigSrcRect = *pSrcRect;
    }
    else
    {
        if(pDstSize->Width*pSrcSize->Height > pSrcSize->Width*pDstSize->Height)
        { //dstW/dstH > srcW/srcH, judge that vipp cut src width
            stOrigSrcSize.Width = pDstSize->Width*pSrcSize->Height/pDstSize->Height;
            stOrigSrcSize.Height = pSrcSize->Height;
            //convert srcRect positon according to new OrigSrcSize.
            stOrigSrcRect.X = pSrcRect->X + (stOrigSrcSize.Width-pSrcSize->Width)/2;
            stOrigSrcRect.Y = pSrcRect->Y;
        }
        else
        { //dstW/dstH < srcW/srcH, judge that vipp cut src height
            stOrigSrcSize.Width = pSrcSize->Width;
            stOrigSrcSize.Height = pDstSize->Height*pSrcSize->Width/pDstSize->Width;
            //convert srcRect positon according to new OrigSrcSize.
            stOrigSrcRect.X = pSrcRect->X;
            stOrigSrcRect.Y = pSrcRect->Y + (stOrigSrcSize.Height-pSrcSize->Height)/2;;
        }
        stOrigSrcRect.Width = pSrcRect->Width;
        stOrigSrcRect.Height = pSrcRect->Height;
    }
    pDstRect->X = stOrigSrcRect.X*pDstSize->Width/stOrigSrcSize.Width;
    pDstRect->Y = stOrigSrcRect.Y*pDstSize->Height/stOrigSrcSize.Height;
    pDstRect->Width = stOrigSrcRect.Width*pDstSize->Width/stOrigSrcSize.Width;
    pDstRect->Height = stOrigSrcRect.Height*pDstSize->Height/stOrigSrcSize.Height;
    return 0;
}

#if defined(NB_BETA_3_0_1) && NB_BETA_3_0_1 == 1
static int npu_detect_body(PersonDetectContext *pctx, VencEncppBufferInfo *pEncBuffer, void *pRunParam)
{
    int ret = 0;
    unsigned char *body_input_buf[2] = { NULL, NULL};
    Awnn_Context_t *context = (Awnn_Context_t*)pRunParam;
    Awnn_Post_t post;
    Awnn_Result_t result;
    EncContext *pContext = (EncContext*)pctx->priv;

    int size = pctx->mSrcWidth * pctx->mSrcHeight;

    pctx->mFrameindex++;
    if(pctx->mFrameindex & 1)
        return 0;

    body_input_buf[0] = pEncBuffer->pAddrVirY ;
    body_input_buf[1] = pEncBuffer->pAddrVirC;

    awnn_set_input_buffers(context, body_input_buf);
    awnn_run(context);

    //printf("post:%d %d %f\n",pctx->mSrcWidth,pctx->mSrcHeight,pctx->pnbinfo->thresh);

    post.type = AWNN_DET_POST_HUMANOID_3;
    post.width = pctx->mSrcWidth;
    post.height = pctx->mSrcHeight;
    post.thresh = 0.25;

    awnn_det_post(context, &post, &result);

#if defined(SUPPORT_ONG_TEST) && SUPPORT_ONG_TEST == 1
        ERRORTYPE _ret;
        int i;
        for(i=0; i<pctx->mValidOrlHandleNum; i++)
        {
            _ret = AW_MPI_RGN_Destroy(pctx->mOrlHandles[i]);
            if(_ret != SUCCESS)
            {
                printf("fatal error! why destroy orlHandle:%d-%d fail[0x%x]?", i, pctx->mOrlHandles[i], ret);
            }
            pctx->mOrlHandles[i] = -1;
        }

        pctx->mValidOrlHandleNum = 0;
#endif

    if (result.valid_cnt > 0)
    {
        int maxarea = 0;
        int area;
        for (int j = 0; j < result.valid_cnt; j ++)
        {
#if defined(SUPPORT_ONG_TEST) && SUPPORT_ONG_TEST == 1
            // draw orl on all vipps.
            RGN_ATTR_S stRgnAttr;
            memset(&stRgnAttr, 0, sizeof(stRgnAttr));
            stRgnAttr.enType = ORL_RGN;

            if(pctx->mValidOrlHandleNum < 20)
            {
                pctx->mOrlHandles[pctx->mValidOrlHandleNum] = pctx->mValidOrlHandleNum;
                _ret = AW_MPI_RGN_Create(pctx->mOrlHandles[pctx->mValidOrlHandleNum], &stRgnAttr);
                if(_ret != SUCCESS)
                {
                    printf("fatal error! why destroy orlHandle:%d-%d fail[0x%x]?", j, pctx->mOrlHandles[j], ret);
                }

                RGN_CHN_ATTR_S stRgnChnAttr;
                memset(&stRgnChnAttr, 0, sizeof(stRgnChnAttr));
                stRgnChnAttr.bShow = TRUE;
                stRgnChnAttr.enType = ORL_RGN;
                stRgnChnAttr.unChnAttr.stOrlChn.enAreaType = AREA_RECT;
                SIZE_S dstSize = {pContext->mConfigPara.mMainSrcWidth, pContext->mConfigPara.mMainSrcHeight};
                // SIZE_S srcSize = {pContext->mConfigPara.mSubSrcWidth, pContext->mConfigPara.mSubSrcHeight};
                SIZE_S srcSize = {pctx->mSrcWidth, pctx->mSrcHeight};
                RECT_S srcRect;
                srcRect.X = result.boxes[j].xmin;
                srcRect.Y = result.boxes[j].ymin;
                srcRect.Width = result.boxes[j].xmax - result.boxes[j].xmin + 1;
                srcRect.Height = result.boxes[j].ymax - result.boxes[j].ymin + 1;
                deduceDstRect(&stRgnChnAttr.unChnAttr.stOrlChn.stRect, &dstSize, &srcRect, &srcSize);
                stRgnChnAttr.unChnAttr.stOrlChn.mColor = 0x00FF00;
                stRgnChnAttr.unChnAttr.stOrlChn.mThick = 2;
                stRgnChnAttr.unChnAttr.stOrlChn.mLayer = pctx->mValidOrlHandleNum;
                MPP_CHN_S stVIChn = {MOD_ID_VIU, pContext->mMainStream.mVipp, pContext->mMainStream.mViChn};
                ret = AW_MPI_RGN_AttachToChn(pctx->mOrlHandles[pctx->mValidOrlHandleNum], &stVIChn, &stRgnChnAttr);
                if(_ret != SUCCESS)
                {
                    printf("fatal error! rgn[%d-%d] attach to viChn[%d-%d] fail[0x%x]!",
                        pctx->mValidOrlHandleNum, pctx->mOrlHandles[pctx->mValidOrlHandleNum],
                        stVIChn.mDevId, stVIChn.mChnId, _ret);
                }

                pctx->mValidOrlHandleNum++;
            }
#endif
            if(result.boxes[j].score < 0.55)
                continue;

            if (j % 5 == 0)
            printf("obj cls %d, prob %f, rect [%d, %d, %d, %d]\n", result.boxes[j].label, result.boxes[j].score,
                    result.boxes[j].xmin, result.boxes[j].ymin, result.boxes[j].xmax, result.boxes[j].ymax);

            ret++;
            area = (result.boxes[j].xmax - result.boxes[j].xmin)*(result.boxes[j].ymax - result.boxes[j].ymin);
            if(maxarea < area)
                maxarea = area;
        }

        if(ret > 0)
        {
            long long curTick = DPGetTickCount64();
            // printf("get person detecte.%lld %lld %d\n",curTick,pctx->mDetectTick,pctx->mDetectArea);
            if(curTick - pctx->mLastDetectTick < 300000)
            {
                //300ms内再次检测到人形认为真检测到
                pctx->mDetectArea = maxarea;
                pctx->mDetectTick = curTick;
            }
            pctx->mLastDetectTick = curTick;
        }

        ret = result.valid_cnt;
    }

    return ret;
}
#else
static int npu_detect_body(PersonDetectContext *pctx,VencEncppBufferInfo * pEncBuffer, void *pRunParam)
{
    int ret = 0;
#if NN_DETECT
    BBoxResults_t *res = NULL;
    unsigned char *body_input_buf[2] = { NULL, NULL };
    EncContext *pContext = (EncContext*)pctx->priv;


    int size = pctx->mSrcWidth * pctx->mSrcHeight;

    pctx->mFrameindex++;
    if(pctx->mFrameindex & 1)
        return 0;

    // alogd("0x%x, 0x%x, 0x%x, 0x%x.", pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3]);
    body_input_buf[0] = pEncBuffer->pAddrVirY ;
    body_input_buf[1] = pEncBuffer->pAddrVirC;

    if(pctx->ai_det_handle == NULL)
    {
        printf("fata error, ai det handle should not be null.\n");
        ret = -1;
    }
    else
    {
#if defined(SUPPORT_ONG_TEST) && SUPPORT_ONG_TEST == 1
        ERRORTYPE ret;
        int i;
        for(i=0; i<pctx->mValidOrlHandleNum; i++)
        {
            ret = AW_MPI_RGN_Destroy(pctx->mOrlHandles[i]);
            if(ret != SUCCESS)
            {
                printf("fatal error! why destroy orlHandle:%d-%d fail[0x%x]?", i, pctx->mOrlHandles[i], ret);
            }
            pctx->mOrlHandles[i] = -1;
        }

        pctx->mValidOrlHandleNum = 0;
#endif

        ((Awnn_Run_t *)pRunParam)->input_buffers = body_input_buf;
        res = awnn_detect_run(pctx->ai_det_handle, (Awnn_Run_t *)pRunParam);
        if (res)
        {
//            printf("awnn_detect_run:%d\n",res->valid_cnt);
//            paint_object_detect_region(pContext->mSubStream.mVipp,&pctx->info,res,pctx->scale_w,pctx->scale_h);
            int maxarea = 0;
            int area;
            for (int j = 0; j < res->valid_cnt; j ++)
            {
                if(res->boxes[j].score < 0.55)
                    continue;
                printf("obj cls %d, prob %f, rect [%d, %d, %d, %d]\n", res->boxes[j].label, res->boxes[j].score,
                      res->boxes[j].xmin, res->boxes[j].ymin, res->boxes[j].xmax, res->boxes[j].ymax);

                ret++;
                area = (res->boxes[j].xmax - res->boxes[j].xmin)*(res->boxes[j].ymax - res->boxes[j].ymin);
                if(maxarea < area)
                    maxarea = area;

#if defined(SUPPORT_ONG_TEST) && SUPPORT_ONG_TEST == 1
                // draw orl on all vipps.
                RGN_ATTR_S stRgnAttr;
                memset(&stRgnAttr, 0, sizeof(stRgnAttr));
                stRgnAttr.enType = ORL_RGN;

                if(pctx->mValidOrlHandleNum < 20)
                {
                    pctx->mOrlHandles[pctx->mValidOrlHandleNum] = pctx->mValidOrlHandleNum;
                    ret = AW_MPI_RGN_Create(pctx->mOrlHandles[pctx->mValidOrlHandleNum], &stRgnAttr);
                    if(ret != SUCCESS)
                    {
                        printf("fatal error! why destroy orlHandle:%d-%d fail[0x%x]?", j, pctx->mOrlHandles[j], ret);
                    }

                    RGN_CHN_ATTR_S stRgnChnAttr;
                    memset(&stRgnChnAttr, 0, sizeof(stRgnChnAttr));
                    stRgnChnAttr.bShow = TRUE;
                    stRgnChnAttr.enType = ORL_RGN;
                    stRgnChnAttr.unChnAttr.stOrlChn.enAreaType = AREA_RECT;
                    SIZE_S dstSize = {pContext->mConfigPara.mMainSrcWidth, pContext->mConfigPara.mMainSrcHeight};
                    // SIZE_S srcSize = {pContext->mConfigPara.mSubSrcWidth, pContext->mConfigPara.mSubSrcHeight};
                    SIZE_S srcSize = {pctx->mSrcWidth, pctx->mSrcHeight};
                    RECT_S srcRect;
                    srcRect.X = res->boxes[j].xmin;
                    srcRect.Y = res->boxes[j].ymin;
                    srcRect.Width = res->boxes[j].xmax - res->boxes[j].xmin + 1;
                    srcRect.Height = res->boxes[j].ymax - res->boxes[j].ymin + 1;
                    deduceDstRect(&stRgnChnAttr.unChnAttr.stOrlChn.stRect, &dstSize, &srcRect, &srcSize);
                    stRgnChnAttr.unChnAttr.stOrlChn.mColor = 0x00FF00;
                    stRgnChnAttr.unChnAttr.stOrlChn.mThick = 2;
                    stRgnChnAttr.unChnAttr.stOrlChn.mLayer = pctx->mValidOrlHandleNum;
                    MPP_CHN_S stVIChn = {MOD_ID_VIU, pContext->mMainStream.mVipp, pContext->mMainStream.mViChn};
                    ret = AW_MPI_RGN_AttachToChn(pctx->mOrlHandles[pctx->mValidOrlHandleNum], &stVIChn, &stRgnChnAttr);
                    if(ret != SUCCESS)
                    {
                        printf("fatal error! rgn[%d-%d] attach to viChn[%d-%d] fail[0x%x]!",
                            pctx->mValidOrlHandleNum, pctx->mOrlHandles[pctx->mValidOrlHandleNum],
                            stVIChn.mDevId, stVIChn.mChnId, ret);
                    }

                    pctx->mValidOrlHandleNum++;
                }
#endif
            }

            if(ret > 0)
            {
                long long curTick = DPGetTickCount64();
                // printf("get person detecte.%lld %lld %d\n",curTick,pctx->mDetectTick,pctx->mDetectArea);
                if(curTick - pctx->mLastDetectTick < 300000)
                {
                    //300ms内再次检测到人形认为真检测到
                    pctx->mDetectArea = maxarea;
                    pctx->mDetectTick = curTick;
                }
                pctx->mLastDetectTick = curTick;
            }
            ret = res->valid_cnt;
        }
        else
        {
            printf("fatal error, res==NULL should not happen for object detection.\n");
        }
    }
#endif
    return ret;
}
#endif

static void FrameToEncBuffer(VIDEO_FRAME_INFO_S * pFrameinfo,VencEncppBufferInfo * pEncBufferInfo)
{
    memset(pEncBufferInfo, 0, sizeof(VencEncppBufferInfo));
    if (MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420 == pFrameinfo->VFrame.mPixelFormat)
        pEncBufferInfo->colorFormat = VENC_PIXEL_YVU420SP;
    else if (MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pFrameinfo->VFrame.mPixelFormat)
        pEncBufferInfo->colorFormat = VENC_PIXEL_YUV420SP;
    else
        printf("fatal error! unsupport pix fmt[%d]", pFrameinfo->VFrame.mPixelFormat);
    pEncBufferInfo->nWidth = AWALIGN(pFrameinfo->VFrame.mWidth, 16);
    pEncBufferInfo->nHeight = AWALIGN(pFrameinfo->VFrame.mHeight, 16);
    pEncBufferInfo->pAddrVirY = (unsigned char *)(pFrameinfo->VFrame.mpVirAddr[0]);
    pEncBufferInfo->pAddrPhyY = (unsigned char *)(pFrameinfo->VFrame.mPhyAddr[0]);
    pEncBufferInfo->pAddrVirC = (unsigned char *)(pFrameinfo->VFrame.mpVirAddr[1]);
    pEncBufferInfo->pAddrPhyC0 = (unsigned char *)(pFrameinfo->VFrame.mPhyAddr[1]);
    pEncBufferInfo->pAddrPhyC1 = (unsigned char *)(pFrameinfo->VFrame.mPhyAddr[1] + pFrameinfo->VFrame.mWidth*pFrameinfo->VFrame.mHeight/4);
}

static void save_pic(VencEncppBufferInfo * pEncBufferInfo,char * strname)
{
    int framesize = pEncBufferInfo->nWidth * pEncBufferInfo->nHeight;
    FILE * f = fopen(strname,"wb");

    //printf(" >>> w:%d  h:%d pEncBufferInfo->pAddrVirY:%p pEncBufferInfo->pAddrVirC:%p\n", pEncBufferInfo->nWidth, pEncBufferInfo->nHeight, pEncBufferInfo->pAddrVirY, pEncBufferInfo->pAddrVirC);
    if (pEncBufferInfo->pAddrVirY)
    {
        fwrite(pEncBufferInfo->pAddrVirY,1,framesize,f);
    }
    if (pEncBufferInfo->pAddrVirC)
    {
        fwrite(pEncBufferInfo->pAddrVirC,1,framesize/2,f);
    }
    fclose(f);
}

static void *npu_worker_thread_body(void *para)
{
    PersonDetectContext *pStreamContext = (PersonDetectContext*)para;
    EncContext *pContext = (EncContext*)(pStreamContext->priv);

    int result = 0;
    VIDEO_FRAME_INFO_S mFrameInfo;
    VencEncppBufferInfo mEncBufferInInfo,mEncBufferOutInfo;
    int size = pStreamContext->mSrcWidth * pStreamContext->mSrcHeight * 3 / 2;
    pStreamContext->mFrameindex = 0;

#if defined(NB_BETA_3_0_1) && NB_BETA_3_0_1 == 1
    Awnn_Context_t *context = awnn_create(NN_MODEL_FILE);
    if(context == NULL)
    {
        printf("fatal error, ai model handle failure.\n");
        return NULL;
    }
#else
    Awnn_Run_t run_param;
    awnn_create_run(&run_param, AWNN_HUMAN_DET);
#endif

    printf("++++person detect++++\n");

    VideoEncoderEncpp * pEncpp = VencEncppCreate();
    FrameToEncBuffer(&pStreamContext->mDstFrameInfo,&mEncBufferOutInfo);

    VencEncppFuncParam mIspFunction;
    memset(&mIspFunction, 0, sizeof(VencEncppFuncParam));
    mIspFunction.bEnableGdcFlag = 1;
    mIspFunction.pGdcParam = &pStreamContext->stGdcParam;

#if defined(TEST_NPU_COST) && TEST_NPU_COST == 1
    static long long npu_cost_print_lasktick = 0;
    long long npu_cost_start_curtick = 0;
    int print_len = 0;
    char npu_cost_printarray[1024 * 4] = {0};
    int print_col = 0;
#endif

    while (!pContext->mbExitFlag)
    {
#if defined(TEST_NPU_COST) && TEST_NPU_COST == 1
        npu_cost_start_curtick = DPGetTickCount64();
#endif
        // do the actual job of body detection.
        if(mpp_helper_getframe(pStreamContext->mVipp,&mFrameInfo))
        {
            FrameToEncBuffer(&mFrameInfo,&mEncBufferInInfo);
#if defined(ENABLE_YY) && ENABLE_YY == 1
/*
            {
                static bool bsave = false;
                if(!bsave)
                {
                    bsave = true;
                    save_pic(&mEncBufferInInfo,"/mnt/nfs/capperson-in.yuv");
                }
            }
*/
            result = VencEncppFunction(pEncpp, &mEncBufferInInfo, &mEncBufferOutInfo, &mIspFunction);
            if (result)
            {
                printf("fatal error! VencEncppFunction fail!");
            }
/*
            {
                static bool bsave = false;
                if(!bsave)
                {
                    bsave = true;
                    save_pic(&mEncBufferOutInfo,"/mnt/nfs/capperson-out.yuv");
                }
            }
*/
        #if defined(NB_BETA_3_0_1) && NB_BETA_3_0_1 == 1
            npu_detect_body(pStreamContext,&mEncBufferOutInfo,(void *)context);
        #else
            npu_detect_body(pStreamContext,&mEncBufferOutInfo,(void *)&run_param);
        #endif
#else
		#if defined(NB_BETA_3_0_1) && NB_BETA_3_0_1 == 1
            // {
            //     static bool bsave = false;
            //     if(!bsave)
            //     {
            //         bsave = true;
            //         save_pic(&mEncBufferInInfo,"./capperson-out.yuv");
            //     }
            // }
            npu_detect_body(pStreamContext,&mEncBufferInInfo,(void *)context);
        #else
            npu_detect_body(pStreamContext,&mEncBufferInInfo,(void *)&run_param);
        #endif
#endif
            mpp_helper_releaseframe(pStreamContext->mVipp,&mFrameInfo);
        }

#if defined(TEST_NPU_COST) && TEST_NPU_COST == 1
        long long npu_print_curTick = DPGetTickCount64();

        print_len += sprintf(npu_cost_printarray + print_len, "%d ", (npu_print_curTick - npu_cost_start_curtick) / 1000);
        print_col++;

        if (print_col > 9)
        {
            print_col = 0;
            print_len += sprintf(npu_cost_printarray + print_len, "%s ", "\n");
        }

        if(npu_print_curTick - npu_cost_print_lasktick > 2000000)
        {
            print_len += sprintf(npu_cost_printarray + print_len, "%s ", "\n\n\n\n");
            printf("npu cost results in 2s: \n%s\n", npu_cost_printarray);
            print_len = 0;
            memset(npu_cost_printarray, 0, sizeof(npu_cost_printarray));
            npu_cost_print_lasktick = npu_print_curTick;

            print_len += sprintf(npu_cost_printarray + print_len, "%s ", "\n\n\n\n");
        }
#endif
    }

    printf("-----person detect-----\n");

#if defined(NB_BETA_3_0_1) && NB_BETA_3_0_1 == 1
    awnn_destroy(context);
#else
    awnn_destroy_run(&run_param);
#endif
    return NULL;
}

static void setVenc2Dnr3Dnr(VENC_CHN mVEncChn)
{
    s2DfilterParam m2DnrPara;
    memset(&m2DnrPara, 0, sizeof(s2DfilterParam));
    m2DnrPara.enable_2d_filter = 0;
    m2DnrPara.filter_strength_y = 127;
    m2DnrPara.filter_strength_uv = 127;
    m2DnrPara.filter_th_y = 11;
    m2DnrPara.filter_th_uv = 7;
    AW_MPI_VENC_Set2DFilter(mVEncChn, &m2DnrPara);
    printf("VencChn[%d] enable and set 2DFilter param\n", mVEncChn);

    s3DfilterParam m3DnrPara;
    memset(&m3DnrPara, 0, sizeof(s3DfilterParam));
    m3DnrPara.enable_3d_filter = 0;
    m3DnrPara.adjust_pix_level_enable = 0;
    m3DnrPara.max_pix_diff_th = 6;
#if VE_REGION_D3D_ENABLE
    m3DnrPara.max_mv_th = 5;
    m3DnrPara.max_mad_th = 48;
    m3DnrPara.smooth_filter_enable = 0;
    m3DnrPara.min_coef = 0;
#else
    m3DnrPara.max_mv_th = 8;
    m3DnrPara.max_mad_th = 11;
    m3DnrPara.smooth_filter_enable = 1;
    m3DnrPara.min_coef = 13;
#endif
    m3DnrPara.max_coef = 16;
    AW_MPI_VENC_Set3DFilter(mVEncChn, &m3DnrPara);
//    alogd("VencChn[%d] enable and set 3DFilter param", mVEncChn);
}

#if 0
static void setVencSuperFrameCfg(VENC_CHN mVEncChn, int mBitrate, int mFramerate)
{
    VENC_SUPERFRAME_CFG_S mSuperFrmParam;
    memset(&mSuperFrmParam, 0, sizeof(VENC_SUPERFRAME_CFG_S));
    mSuperFrmParam.enSuperFrmMode = SUPERFRM_NONE;
    float cmp_bits = 1.5*1024*1024 / 25;
    float dst_bits = 0;
    if (0 != mFramerate)
        dst_bits = (float)mBitrate / mFramerate;
    else
        dst_bits = (float)mBitrate / 25;
    float bits_ratio = dst_bits / cmp_bits;
    mSuperFrmParam.SuperIFrmBitsThr = (unsigned int)((8.0*200*1024) * bits_ratio);
    mSuperFrmParam.SuperPFrmBitsThr = mSuperFrmParam.SuperIFrmBitsThr / 3;
    printf("VencChn[%d] SuperFrm Mode:%d, IfrmSize:%d bits, PfrmSize:%d bits\n", mVEncChn,
        mSuperFrmParam.enSuperFrmMode, mSuperFrmParam.SuperIFrmBitsThr, mSuperFrmParam.SuperPFrmBitsThr);
    AW_MPI_VENC_SetSuperFrameCfg(mVEncChn, &mSuperFrmParam);
}
#endif

static bool benc_run = false;

int video_enc_start(int devip)
{
    if(benc_run)
        return 1;
	char ip[16] = {0};

	sprintf(ip, "%d.%d.%d.%d", devip & 0xFF, (devip >> 8) & 0xFF,
			(devip >> 16) & 0xFF, (devip >> 24) & 0xFF);

	// DevGetIp(ip);

    printf("enc ip %x : %s \n", devip, ip);

	int result = 0;
	ERRORTYPE ret;
	EncContext *pContext = &enc;
#if 0
    GLogConfig stGLogConfig =
    {
        .FLAGS_logtostderr = 0,
        .FLAGS_colorlogtostderr = 1,
        .FLAGS_stderrthreshold = _GLOG_INFO,
        .FLAGS_minloglevel = _GLOG_INFO,
        .FLAGS_logbuflevel = -1,
        .FLAGS_logbufsecs = 0,
        .FLAGS_max_log_size = 25,
        .FLAGS_stop_logging_if_full_disk = 1,
    };
    strcpy(stGLogConfig.LogDir, "/tmp/log");
    strcpy(stGLogConfig.InfoLogFileNameBase, "LOG-");
    strcpy(stGLogConfig.LogFileNameExtension, "SDV-");

    log_init("appenc", &stGLogConfig);
#endif
	memset(pContext, 0, sizeof(EncContext));
	initeEncConfig(&(pContext->mConfigPara));

    MPP_SYS_CONF_S stSysConf;
    memset(&stSysConf, 0, sizeof(MPP_SYS_CONF_S));
    stSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stSysConf);
    ret = AW_MPI_SYS_Init();
    if (ret < 0)
    {
        printf("sys Init failed!\n");
        return -1;
    }

    pContext->mMainIspRunFlag = 0;
    pContext->mSubIspRunFlag = 0;
#define MAIN_ENC  1
#define SUB_ENC  0
#if MAIN_ENC
	if (pContext->mConfigPara.mMainVEncChn >= 0 && pContext->mConfigPara.mMainViChn >= 0)
    {
        configMainStream(pContext);
        AW_MPI_VI_CreateVipp(pContext->mMainStream.mVipp);
        // AW_MPI_ISP_SetFlip(pContext->mMainStream.mVipp, 1);// 设置垂直镜像

        AW_MPI_VI_SetVippAttr(pContext->mMainStream.mVipp, &pContext->mMainStream.mViAttr);
        if (0 == pContext->mMainIspRunFlag)
        {
            AW_MPI_ISP_Run(pContext->mMainStream.mIsp);
            pContext->mMainIspRunFlag = 1;
        }

        // 裁剪
        AW_MPI_VI_SetCrop(pContext->mMainStream.mVipp, &pContext->mMainStream.mViAttr.mCropCfg);

        // 垂直镜像
        // AW_MPI_VI_SetVippFlip(pContext->mMainStream.mVipp,1);

        AW_MPI_VI_EnableVipp(pContext->mMainStream.mVipp);
        AW_MPI_VI_CreateVirChn(pContext->mMainStream.mVipp, pContext->mMainStream.mViChn, NULL);

        // 图像旋转180度 有问题
        // AW_MPI_ISP_SetFlip(pContext->mMainStream.mVipp, 1);
        // AW_MPI_VI_SetVippFlip(pContext->mMainStream.mVipp, 1);

        AW_MPI_VENC_CreateChn(pContext->mMainStream.mVEncChn, &pContext->mMainStream.mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pContext->mMainStream.mVEncChn, &pContext->mMainStream.mVEncRcParam);
        AW_MPI_VENC_SetFrameRate(pContext->mMainStream.mVEncChn, &pContext->mMainStream.mVEncFrameRateConfig);

        // int vl = 0;
        // int ret =  AW_MPI_ISP_GetBrightness(pContext->mMainStream.mIsp,&vl);//0~256
        // printf("AW_MPI_ISP_GetBrightness ret:%d : %d \n", ret, vl);
        if (false)
        {
            VENC_CROP_CFG_S stCropCfg;
            memset(&stCropCfg, 0, sizeof(stCropCfg));

            stCropCfg.bEnable = 1;
            stCropCfg.Rect.X = 160;//80;
            stCropCfg.Rect.Y = 0;
            stCropCfg.Rect.Width = 1920 - 160*2;//960;//1920 - 256*2;
            stCropCfg.Rect.Height = 1080;//540;//1080;
            AW_MPI_VENC_SetCrop(pContext->mSubStream.mVEncChn, &stCropCfg);

            VENC_CROP_CFG_S stCropCfg2;
            AW_MPI_VENC_GetCrop(pContext->mSubStream.mVEncChn, &stCropCfg2);
            printf("main ++++++++++++++++++++++++++++++++++++ crop enable.%d x.%d y.%d w.%d h.%d\n", stCropCfg2.bEnable, stCropCfg2.Rect.X, stCropCfg2.Rect.Y, stCropCfg2.Rect.Width, stCropCfg2.Rect.Height);
        }

        if (pContext->mConfigPara.mMainVippCropEnable)
        {
            VENC_CROP_CFG_S stCropCfg;
            memset(&stCropCfg, 0, sizeof(VENC_CROP_CFG_S));
            stCropCfg.bEnable = pContext->mConfigPara.mMainVippCropEnable;
            stCropCfg.Rect.X = pContext->mConfigPara.mMainVippCropRectX;
            stCropCfg.Rect.Y = pContext->mConfigPara.mMainVippCropRectY;
            stCropCfg.Rect.Width = pContext->mConfigPara.mMainVippCropRectWidth;
            stCropCfg.Rect.Height = pContext->mConfigPara.mMainVippCropRectHeight;
            ERRORTYPE cret = AW_MPI_VENC_SetCrop(pContext->mMainStream.mVEncChn, &stCropCfg);

            VENC_CROP_CFG_S stCropCfg2;
            AW_MPI_VENC_GetCrop(pContext->mMainStream.mVEncChn, &stCropCfg2);

            printf("main ++++++++++++++++++++++++++++++++++++ crop enable %d.%d x.%d y.%d w.%d h.%d\n", cret, stCropCfg2.bEnable, stCropCfg2.Rect.X, stCropCfg2.Rect.Y, stCropCfg2.Rect.Width, stCropCfg2.Rect.Height);
        }

        if(PT_H264 == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
        {
            VENC_PARAM_H264_VUI_S H264Vui;
            memset(&H264Vui, 0, sizeof(VENC_PARAM_H264_VUI_S));
            // if(pContext->mConfigPara.mVuiTimingInfoPresentFlag)
            // {
            //     H264Vui.VuiTimeInfo.timing_info_present_flag = 1;
            //     H264Vui.VuiTimeInfo.fixed_frame_rate_flag = 1;
            //     H264Vui.VuiTimeInfo.num_units_in_tick = 1000;
            //     H264Vui.VuiTimeInfo.time_scale = H264Vui.VuiTimeInfo.num_units_in_tick * pContext->mConfigPara.mVideoFrameRate * 2;
            // }
            H264Vui.VuiBitstreamRestric.bitstream_restriction_flag = 1;
            AW_MPI_VENC_SetH264Vui(pContext->mMainStream.mVEncChn, &H264Vui);
            // AW_MPI_VENC_SetH264NalRefIdcNoneZeroValue(pContext->mMainStream.mVEncChn, pContext->mConfigPara.NalRefIdcNoneZeroValue);
        }
        else if(PT_H265 == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
        {
            VENC_PARAM_H265_VUI_S H265Vui;
            memset(&H265Vui, 0, sizeof(VENC_PARAM_H265_VUI_S));
            // if(pContext->mConfigPara.mVuiTimingInfoPresentFlag)
            // {
            //     H265Vui.VuiTimeInfo.timing_info_present_flag = 1;
            //     H265Vui.VuiTimeInfo.num_units_in_tick = 1000;
            //     /* Notices: the protocol syntax states that h265 does not need to be multiplied by 2. */
            //     H265Vui.VuiTimeInfo.time_scale = H265Vui.VuiTimeInfo.num_units_in_tick * pContext->mConfigPara.mVideoFrameRate;
            //     H265Vui.VuiTimeInfo.num_ticks_poc_diff_one_minus1 = H265Vui.VuiTimeInfo.num_units_in_tick;
            // }
            H265Vui.VuiBitstreamRestric.bitstream_restriction_flag = 1;
            AW_MPI_VENC_SetH265Vui(pContext->mMainStream.mVEncChn, &H265Vui);
        }

        if (PT_H264 == pContext->mMainStream.mVEncChnAttr.VeAttr.Type || PT_H265 == pContext->mMainStream.mVEncChnAttr.VeAttr.Type)
        {
            if (pContext->mMainStream.mVEncChnAttr.VeAttr.mVbrOptEnable)
            {
                int rcPriority = 3,rcQualityLevel = 0;
                VencVbrOptParam stVbrOptParam;
                memset(&stVbrOptParam, 0, sizeof(VencVbrOptParam));
                AW_MPI_VENC_GetVbrOptParam(pContext->mMainStream.mVEncChn, &stVbrOptParam);
#if 0
                stVbrOptParam.enable_instaneousBR = 1;
                stVbrOptParam.recodeIsliceQpEn = 1;
                stVbrOptParam.uMosAicOptEn = 1;
                stVbrOptParam.uSaveBitRateEn = 1;
                stVbrOptParam.uClassifyMadThAdjustEn = 1;
                stVbrOptParam.uMoveToStaticOptEn = 1;

                stVbrOptParam.nIPratio = 25;
                stVbrOptParam.nIntraPeriodNumInVbv = 4;
                stVbrOptParam.max_instaneousBR = 0.9f;
                stVbrOptParam.peroid_instaneousBR = 40;

                alogd("update VbrOptParam");
#endif
                if (stVbrOptParam.sRcPriority != rcPriority)
                {
                    printf("VencChn[%d] change RcPriority %d -> %d", pContext->mMainStream.mVEncChn, stVbrOptParam.sRcPriority, rcPriority);
                    stVbrOptParam.sRcPriority = rcPriority;
                }
                if (stVbrOptParam.eQualityLevel != rcQualityLevel)
                {
                    printf("VencChn[%d] change RcQualityLevel %d -> %d", pContext->mMainStream.mVEncChn, stVbrOptParam.eQualityLevel, rcQualityLevel);
                    stVbrOptParam.eQualityLevel = rcQualityLevel;
                }

                AW_MPI_VENC_SetVbrOptParam(pContext->mMainStream.mVEncChn, &stVbrOptParam);
            }
        }

        setVenc2Dnr3Dnr(pContext->mMainStream.mVEncChn);
#ifdef ENABLE_SUPER_FRAME
        setVencSuperFrameCfg(pContext->mMainStream.mVEncChn, pContext->mConfigPara.mMainEncodeBitrate, pContext->mConfigPara.mMainEncodeFrameRate);
#endif

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pContext->mMainStream.mVEncChn, &cbInfo);

        MPP_CHN_S ViChn = {MOD_ID_VIU, pContext->mMainStream.mVipp, pContext->mMainStream.mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pContext->mMainStream.mVEncChn};
        AW_MPI_SYS_Bind(&ViChn,&VeChn);

    #if 1
        //takepicture
        {
            configTakePictrueStream(pContext);

            printf("takepicture AW_MPI_VI_CreateVirChn:%d %d\n",pContext->mMainStream.mVipp,pContext->mTakeJpegStream.mViChn);
            AW_MPI_VI_CreateVirChn(pContext->mMainStream.mVipp, pContext->mTakeJpegStream.mViChn, NULL);
            AW_MPI_VENC_CreateChn(pContext->mTakeJpegStream.mVeChn, &pContext->mTakeJpegStream.mVencChnAttr);

            VeProcSet stVeProcSet;
            memset(&stVeProcSet, 0, sizeof(VeProcSet));
            stVeProcSet.bProcEnable = 1;
            stVeProcSet.nProcFreq = 30;
            AW_MPI_VENC_SetProcSet(pContext->mTakeJpegStream.mVeChn, &stVeProcSet);

            MPPCallbackInfo cbInfo;
            cbInfo.cookie = (void*)&(pContext->mTakeJpegStream);
            cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper_Jpeg;
            AW_MPI_VENC_RegisterCallback(pContext->mTakeJpegStream.mVeChn, &cbInfo);
            VENC_PARAM_JPEG_S mJpegParam;
            memset(&mJpegParam, 0, sizeof(VENC_PARAM_JPEG_S));
            mJpegParam.Qfactor = 36;//60
            AW_MPI_VENC_SetJpegParam(pContext->mTakeJpegStream.mVeChn, &mJpegParam);
            AW_MPI_VENC_ForbidDiscardingFrame(pContext->mTakeJpegStream.mVeChn, TRUE);
        }
    #endif
    }
#endif //MAIN_ENC
#if SUB_ENC
    if (pContext->mConfigPara.mSubVEncChn >= 0 && pContext->mConfigPara.mSubViChn >= 0)
    {
        configSubStream(pContext);
        AW_MPI_VI_CreateVipp(pContext->mSubStream.mVipp);
        AW_MPI_VI_SetVippAttr(pContext->mSubStream.mVipp, &pContext->mSubStream.mViAttr);
        if (0 == pContext->mSubIspRunFlag && (pContext->mMainStream.mIsp != pContext->mSubStream.mIsp || 0 == pContext->mMainIspRunFlag))
        {
            AW_MPI_ISP_Run(pContext->mSubStream.mIsp);
            pContext->mSubIspRunFlag = 1;
        }

        // 裁剪
        AW_MPI_VI_SetCrop(pContext->mSubStream.mVipp, &pContext->mSubStream.mViAttr.mCropCfg);

        // 设置垂直镜像
        // AW_MPI_VI_SetVippFlip(pContext->mSubStream.mVipp, 1);

        AW_MPI_VI_EnableVipp(pContext->mSubStream.mVipp);
        AW_MPI_VI_CreateVirChn(pContext->mSubStream.mVipp, pContext->mSubStream.mViChn, NULL);

        // 图像旋转180度 有问题
        // AW_MPI_ISP_SetFlip(pContext->mSubStream.mVipp, 1);
        // AW_MPI_VI_SetVippFlip(pContext->mSubStream.mVipp, 1);

        AW_MPI_VENC_CreateChn(pContext->mSubStream.mVEncChn, &pContext->mSubStream.mVEncChnAttr);
        AW_MPI_VENC_SetRcParam(pContext->mSubStream.mVEncChn, &pContext->mSubStream.mVEncRcParam);
        AW_MPI_VENC_SetFrameRate(pContext->mSubStream.mVEncChn, &pContext->mSubStream.mVEncFrameRateConfig);

        setVenc2Dnr3Dnr(pContext->mSubStream.mVEncChn);
        setVencSuperFrameCfg(pContext->mSubStream.mVEncChn, pContext->mConfigPara.mSubEncodeBitrate, pContext->mConfigPara.mSubEncodeFrameRate);

        if (pContext->mSubStream.mViAttr.mCropCfg.bEnable)
        {
            VENC_CROP_CFG_S stCropCfg;
            memset(&stCropCfg, 0, sizeof(stCropCfg));

            stCropCfg.bEnable = pContext->mSubStream.mViAttr.mCropCfg.bEnable;
            stCropCfg.Rect.X = pContext->mSubStream.mViAttr.mCropCfg.Rect.X;//160;//80;
            stCropCfg.Rect.Y = pContext->mSubStream.mViAttr.mCropCfg.Rect.Y;//0;
            stCropCfg.Rect.Width = pContext->mSubStream.mViAttr.mCropCfg.Rect.Width;//960 - 80*2;//960;//1920 - 256*2;
            stCropCfg.Rect.Height = pContext->mSubStream.mViAttr.mCropCfg.Rect.Height;//540-100;//540;//1080;
            AW_MPI_VENC_SetCrop(pContext->mSubStream.mVEncChn, &stCropCfg);

            VENC_CROP_CFG_S stCropCfg2;
            AW_MPI_VENC_GetCrop(pContext->mSubStream.mVEncChn, &stCropCfg2);
            printf("sub ++++++++++++++++++++++++++++++++++++ crop enable.%d x.%d y.%d w.%d h.%d\n", stCropCfg2.bEnable, stCropCfg2.Rect.X, stCropCfg2.Rect.Y, stCropCfg2.Rect.Width, stCropCfg2.Rect.Height);
        }

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pContext;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pContext->mSubStream.mVEncChn, &cbInfo);

        MPP_CHN_S ViChn = {MOD_ID_VIU, pContext->mSubStream.mVipp, pContext->mSubStream.mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pContext->mSubStream.mVEncChn};
        AW_MPI_SYS_Bind(&ViChn, &VeChn);
    }
#endif

#if MAIN_ENC  // start main
    if (pContext->mConfigPara.mMainVEncChn >= 0 && pContext->mConfigPara.mMainViChn >= 0)
    {
#if defined(SUPPORT_RTSP_TEST) && SUPPORT_RTSP_TEST == 1
        RtspServerAttr rtsp_attr;
        memset(&rtsp_attr, 0, sizeof(RtspServerAttr));
        rtsp_attr.net_type = 1;

        if (PT_H264 == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_H264;
        else if (PT_H265 == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_H265;
        else
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_LAST;

        // rtsp_attr.ip = "192.168.1.110";
        rtsp_attr.ip = ip;
        rtsp_attr.port = 8555;

        result = rtsp_open(0, &rtsp_attr);
        if (result)
        {
            printf("Do rtsp_open fail! ret:%d \n", result);
            return -1;
        }
#endif

        AW_MPI_VI_EnableVirChn(pContext->mMainStream.mVipp, pContext->mMainStream.mViChn);
        AW_MPI_VENC_StartRecvPic(pContext->mMainStream.mVEncChn);

        // DrawStreamOSD(pContext->mConfigPara.mMainVEncChn, 0, BITMAPFILE_1920X1080_1, 0, 20*2, 68*2);
        DrawStreamOSD2Enc(pContext->mConfigPara.mMainVEncChn, 0, &pContext->mMainStream.mOSD[0]);

        pContext->bMotionSearchEnable = FALSE;
        if(pContext->bMotionSearchEnable)
        {
            VencMotionSearchParam mMotionParam;
            memset(&mMotionParam, 0, sizeof(VencMotionSearchParam));

            mMotionParam.en_motion_search = 1;
            mMotionParam.dis_default_para = 1;
            mMotionParam.hor_region_num = 32;
            mMotionParam.ver_region_num = 18;
            mMotionParam.large_mv_th = 20;
            mMotionParam.large_mv_ratio_th = 12.0f;
            mMotionParam.non_zero_mv_ratio_th = 20.0f;
            mMotionParam.large_sad_ratio_th = 30.0f;

            AW_MPI_VENC_SetMotionSearchParam(pContext->mMainStream.mVEncChn, &mMotionParam);

            unsigned int size = mMotionParam.hor_region_num * mMotionParam.ver_region_num * sizeof(VencMotionSearchRegion);
            pContext->mMotionResult.region = (VencMotionSearchRegion *)malloc(size);
            if (NULL == pContext->mMotionResult.region)
            {
                printf("fatal error! malloc region failed! size=%d", size);
            }
            else
            {
                memset(pContext->mMotionResult.region, 0, size);
            }

            video_md_print_param();
        }

        result = pthread_create(&pContext->mMainStream.mStreamThreadId, NULL, getVencStreamThread, (void*)&pContext->mMainStream);
        if (result != 0)
        {
            printf("fatal error! pthread create fail[%d]", result);
        }
    }
#endif

#if SUB_ENC // start sub
    if (pContext->mConfigPara.mSubVEncChn >= 0 && pContext->mConfigPara.mSubViChn >= 0)
    {
#if defined(SUPPORT_RTSP_TEST) && SUPPORT_RTSP_TEST == 1
        RtspServerAttr rtsp_attr;
        memset(&rtsp_attr, 0, sizeof(RtspServerAttr));
        rtsp_attr.net_type = 1;

        if (PT_H264 == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_H264;
        else if (PT_H265 == pContext->mSubStream.mVEncChnAttr.VeAttr.Type)
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_H265;
        else
            rtsp_attr.video_type = RTSP_VIDEO_TYPE_LAST;

        // rtsp_attr.ip = "192.168.1.110";
        rtsp_attr.ip = ip;
        rtsp_attr.port = 8555;

        result = rtsp_open(1, &rtsp_attr);
        if (result)
        {
            printf("Do rtsp_open fail! ret:%d \n", result);
            return -1;
        }
#endif

        AW_MPI_VI_EnableVirChn(pContext->mSubStream.mVipp, pContext->mSubStream.mViChn);
        AW_MPI_VENC_StartRecvPic(pContext->mSubStream.mVEncChn);

        DrawStreamOSD(pContext->mConfigPara.mSubVEncChn, 1, BITMAPFILE_960X540_1, 0, 20, 68);
        // DrawStreamOSD2Enc(pContext->mConfigPara.mSubVEncChn, 1, &pContext->mSubStream.mOSD[0]);

        pContext->bMotionSearchEnable = TRUE;
        if(pContext->bMotionSearchEnable)
        {
            VencMotionSearchParam mMotionParam;
            memset(&mMotionParam, 0, sizeof(VencMotionSearchParam));

            mMotionParam.en_motion_search = 1;
            mMotionParam.dis_default_para = 1;
            mMotionParam.hor_region_num = 32;
            mMotionParam.ver_region_num = 18;
            mMotionParam.large_mv_th = 20;
            mMotionParam.large_mv_ratio_th = 12.0f;
            mMotionParam.non_zero_mv_ratio_th = 20.0f;
            mMotionParam.large_sad_ratio_th = 30.0f;

            AW_MPI_VENC_SetMotionSearchParam(pContext->mSubStream.mVEncChn, &mMotionParam);

            unsigned int size = mMotionParam.hor_region_num * mMotionParam.ver_region_num * sizeof(VencMotionSearchRegion);
            pContext->mMotionResult.region = (VencMotionSearchRegion *)malloc(size);
            if (NULL == pContext->mMotionResult.region)
            {
                printf("fatal error! malloc region failed! size=%u", size);
            }
            else
            {
                memset(pContext->mMotionResult.region, 0, size);
            }

            video_md_print_param();
        }

        result = pthread_create(&pContext->mSubStream.mStreamThreadId, NULL, getVencStreamThread, (void*)&pContext->mSubStream);
        if (result != 0)
        {
            printf("fatal error! pthread create fail[%d]\n", result);
        }

    }
#endif

    pContext->bPersonDetectEnable = FALSE;
    if(pContext->bPersonDetectEnable)
    {
    #if defined(NB_BETA_3_0_1) && NB_BETA_3_0_1 == 1
        pContext->mPersonDetect.pnbinfo = awnn_get_info(NN_MODEL_FILE);
        if(pContext->mPersonDetect.pnbinfo)
        {
            printf("nn mem_size: %u\n", pContext->mPersonDetect.pnbinfo->mem_size);
            awnn_init(pContext->mPersonDetect.pnbinfo->mem_size);

            configPersonDetect(pContext);

            mpp_helper_start_npu(pContext->mPersonDetect.mIsp,
                                            pContext->mPersonDetect.mVipp,
                                            3,
                                            pContext->mPersonDetect.mSrcWidth,
                                            pContext->mPersonDetect.mSrcHeight,
                                            5,
                                            MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420);
            result = pthread_create(&pContext->mPersonDetect.mStreamThreadId, NULL, npu_worker_thread_body, (void*)&pContext->mPersonDetect);
        }
    #else
        configPersonDetect(pContext);

        Awnn_Init_t init_param;
        memset(&init_param, 0, sizeof(Awnn_Init_t));
        init_param.human_nb = (char *)NN_MODEL_FILE;
        init_param.face_nb = NULL;
        awnn_detect_init(&pContext->mPersonDetect.ai_det_handle, &init_param);
        if(pContext->mPersonDetect.ai_det_handle == NULL)
        {
            printf("fatal error, ai model handle failure.\n");
            return -1;
        }
        printf("ai_det_handle=%p, human model=%s\n", pContext->mPersonDetect.ai_det_handle,init_param.human_nb);

        mpp_helper_start_npu(pContext->mPersonDetect.mIsp,
                            pContext->mPersonDetect.mVipp,
                            3,
                            pContext->mPersonDetect.mSrcWidth,
                            pContext->mPersonDetect.mSrcHeight,
                            5,
                            MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420);   //StringToPicFormatFromConfig("aw_lbc_2_0x") MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420
        result = pthread_create(&pContext->mPersonDetect.mStreamThreadId, NULL, npu_worker_thread_body, (void*)&pContext->mPersonDetect);
    #endif
    }

    pContext->mLastAlarmTick = DPGetTickCount64();
    benc_run = true;
	return 0;
}

void video_enc_stop()
{
    if(!benc_run)
        return;
    benc_run = false;
	void *pRetVal = NULL;
	ERRORTYPE ret;
	EncContext *pContext = &enc;
	pContext->mbExitFlag = 1;

// stop
#if defined(SUPPORT_RTSP_TEST) && SUPPORT_RTSP_TEST == 1
    rtsp_stop(0);
    rtsp_stop(1);
    rtsp_close(0);
    rtsp_close(1);
#endif

    if (pContext->mConfigPara.mMainVEncChn >= 0 && pContext->mConfigPara.mMainViChn >= 0)
    {
        DestroyStreamOSD(pContext->mConfigPara.mMainVEncChn, 0);
        pthread_join(pContext->mMainStream.mStreamThreadId, &pRetVal);
        printf("mainStream pRetVal=%p\n", pRetVal);
        AW_MPI_VI_DisableVirChn(pContext->mMainStream.mVipp, pContext->mMainStream.mViChn);
        AW_MPI_VENC_StopRecvPic(pContext->mMainStream.mVEncChn);
        AW_MPI_VENC_ResetChn(pContext->mMainStream.mVEncChn);
        AW_MPI_VENC_DestroyChn(pContext->mMainStream.mVEncChn);
        AW_MPI_VI_DestroyVirChn(pContext->mMainStream.mVipp, pContext->mMainStream.mViChn);
    }
#if 1
    if (pContext->mConfigPara.mSubVEncChn >= 0 && pContext->mConfigPara.mSubViChn >= 0)
    {
        DestroyStreamOSD(pContext->mConfigPara.mSubVEncChn, 1);
        pthread_join(pContext->mSubStream.mStreamThreadId, &pRetVal);
        printf("subStream pRetVal=%p\n", pRetVal);
        AW_MPI_VI_DisableVirChn(pContext->mSubStream.mVipp, pContext->mSubStream.mViChn);
        AW_MPI_VENC_StopRecvPic(pContext->mSubStream.mVEncChn);
        AW_MPI_VENC_ResetChn(pContext->mSubStream.mVEncChn);
        AW_MPI_VENC_DestroyChn(pContext->mSubStream.mVEncChn);
        AW_MPI_VI_DestroyVirChn(pContext->mSubStream.mVipp, pContext->mSubStream.mViChn);
    }
#endif
    // deinit
    if (pContext->mConfigPara.mMainVEncChn >= 0 && pContext->mConfigPara.mMainViChn >= 0)
    {
        AW_MPI_VI_DisableVipp(pContext->mMainStream.mVipp);
        if (pContext->mMainIspRunFlag)
        {
            AW_MPI_ISP_Stop(pContext->mMainStream.mIsp);
            pContext->mMainIspRunFlag = 0;
        }
        AW_MPI_VI_DestroyVipp(pContext->mMainStream.mVipp);
    }
#if 1
    if (pContext->mConfigPara.mSubVEncChn >= 0 && pContext->mConfigPara.mSubViChn >= 0)
    {
        AW_MPI_VI_DisableVipp(pContext->mSubStream.mVipp);
        if (pContext->mSubIspRunFlag)
        {
            AW_MPI_ISP_Stop(pContext->mSubStream.mIsp);
            pContext->mSubIspRunFlag = 0;
        }
        AW_MPI_VI_DestroyVipp(pContext->mSubStream.mVipp);
    }
#endif
    if(pContext->bMotionSearchEnable && pContext->mMotionResult.region)
        free(pContext->mMotionResult.region);

    ret = AW_MPI_SYS_Exit();
    if (ret != SUCCESS)
    {
        printf("fatal error! sys exit failed!\n");
    }
	return;
}

//0  low    1   hight
void video_mode(int hight)
{
}

void video_idr(int channal)
{
    ERRORTYPE ret = -1;
    EncContext *pContext = &enc;
    if(channal == 0)
        ret = AW_MPI_VENC_RequestIDR(pContext->mMainStream.mVEncChn,0);
    else if(channal == 1)
        ret = AW_MPI_VENC_RequestIDR(pContext->mSubStream.mVEncChn,0);
    printf("reset IDR ret:%x \n", ret);
}

void video_catch(int channal)
{
//	unsigned char * jpegbuffer;
//	unsigned int jpegsize;
	printf("video_catch:%d\r\n",channal);
}

static ERRORTYPE startJpeg(TakeJpegContext *pJpegCtx)
{
    int result = 0;
    ERRORTYPE ret = SUCCESS;
    printf("startJpeg:%d %d %d\n",pJpegCtx->mpConnectVipp->mVipp, pJpegCtx->mViChn,pJpegCtx->mVeChn);
    ret = AW_MPI_VI_EnableVirChn(pJpegCtx->mpConnectVipp->mVipp, pJpegCtx->mViChn);
    if (ret != SUCCESS)
    {
        printf("Vipp[%d]viChn[%d] enable fail[0x%x]!\n", pJpegCtx->mpConnectVipp->mVipp, pJpegCtx->mViChn, ret);
        return -1;
    }
    if (pJpegCtx->mVeChn >= 0)
    {
        ret = AW_MPI_VENC_StartRecvPic(pJpegCtx->mVeChn);
        if (ret != SUCCESS)
        {
            printf("veChn[%d] start fail[0x%x]!\n", pJpegCtx->mVeChn, ret);
            return -1;
        }
    }

    // if (strcmp(g_set.code, _DEFAULT_CODE) == 0)
    //     // DrawStreamOSD(pJpegCtx->mVeChn, 2, BITMAPFILE_1920X1080_1, 0, 20*2, 68*2);
    //     DrawStreamOSD2Enc(pJpegCtx->mVeChn, 2, &pJpegCtx->mpConnectVipp->mOSD[0]);
    // else
        // DrawStreamOSD(pJpegCtx->mVeChn, 2, BITMAPFILE_1920X1080_2, 0, 20*2, 68*2);
        DrawStreamOSD2Enc(pJpegCtx->mVeChn, 2, &pJpegCtx->mpConnectVipp->mOSD[1]);

    printf("startJpeg successed\n");
    return result;
}

int takePicture(TakeJpegContext *pJpegCtx,unsigned char **jpegbuffer,unsigned int *jpegsize)
{
    int result = 0;
    ERRORTYPE ret;
    //static int file_cnt = 0;
    VIDEO_FRAME_INFO_S stFrame;
    printf("takePicture:%d %d\n",pJpegCtx->mpConnectVipp->mVipp, pJpegCtx->mViChn);
    memset(&stFrame, 0, sizeof(VIDEO_FRAME_INFO_S));
    if ((ret = AW_MPI_VI_GetFrame(pJpegCtx->mpConnectVipp->mVipp, pJpegCtx->mViChn, &stFrame, 5000)) != 0)
    {
        printf("fatal error! vipp[%d]chn[%d] Get Frame failed[0x%x]!\n", pJpegCtx->mpConnectVipp->mVipp, pJpegCtx->mViChn, ret);
        return -1;
    }
    else
    {
        ret = AW_MPI_VENC_SendFrame(pJpegCtx->mVeChn, &stFrame, 0);
        if(ret != SUCCESS)
        {
            printf("fatal error! veChn[%d] send frame fail[0x%x]\n", pJpegCtx->mVeChn, ret);
        }
    }

    VENC_STREAM_S stVencStream;
    VENC_PACK_S stPack;
    memset(&stVencStream, 0, sizeof(VENC_STREAM_S));
    memset(&stPack, 0, sizeof(VENC_PACK_S));
    stVencStream.mpPack = &stPack;
    stVencStream.mPackCount = 1;
    if(SUCCESS == AW_MPI_VENC_GetStream(pJpegCtx->mVeChn, &stVencStream, 5000))
    {
        int len = stVencStream.mpPack[0].mLen0 + stVencStream.mpPack[0].mLen1 + stVencStream.mpPack[0].mLen2;
        if(len < 250*1024)
        {
            char * pbuf = (char*)malloc(len);
            if(pbuf)
            {
                *jpegbuffer = pbuf;
                *jpegsize = len;
                memcpy(pbuf,stVencStream.mpPack[0].mpAddr0,stVencStream.mpPack[0].mLen0);
                pbuf += stVencStream.mpPack[0].mLen0;
                memcpy(pbuf,stVencStream.mpPack[0].mpAddr1,stVencStream.mpPack[0].mLen1);
                pbuf += stVencStream.mpPack[0].mLen1;
                memcpy(pbuf,stVencStream.mpPack[0].mpAddr2,stVencStream.mpPack[0].mLen2);
            }
        }

        printf("take jpeg successed.%d\n",len);

        ret = AW_MPI_VENC_ReleaseStream(pJpegCtx->mVeChn, &stVencStream);
        if(ret != SUCCESS)
        {
            printf("fatal error! AW_MPI_VENC ReleaseStream failed[0x%x]\n", ret);
        }
    }
    else
    {
        printf("fatal error! get picture fail\n");
        result = -1;
    }
    printf("take picture end!\n");
    return result;
}

static int stopJpeg(TakeJpegContext *pJpegCtx)
{
    ERRORTYPE ret = SUCCESS;
    if (pJpegCtx->mViChn >= 0)
    {
        ret = AW_MPI_VI_DisableVirChn(pJpegCtx->mpConnectVipp->mVipp, pJpegCtx->mViChn);
        if(ret != SUCCESS)
        {
            printf("fatal error! vipp[%d] disable virChn[%d] fail[0x%x].\n", pJpegCtx->mpConnectVipp->mVipp, pJpegCtx->mViChn, ret);
        }
    }
    if (pJpegCtx->mVeChn >= 0)
    {
        DestroyStreamOSD(pJpegCtx->mVeChn, 2);
        ret = AW_MPI_VENC_StopRecvPic(pJpegCtx->mVeChn);
        if(ret != SUCCESS)
        {
            printf("fatal error! stop veChn[%d] fail[0x%x].\n", pJpegCtx->mVeChn, ret);
        }
    }
    return 0;
}


int video_catch_jpeg(int chn,unsigned char **jpegbuffer,unsigned int *jpegsize)
{
    if(chn != 0)
        return -1;
    EncContext *pContext = &enc;
    if(SUCCESS != startJpeg(&pContext->mTakeJpegStream)){
        printf("startJpeg err \n");
        return -1;
    }
    takePicture(&pContext->mTakeJpegStream,jpegbuffer,jpegsize);
    stopJpeg(&pContext->mTakeJpegStream);
	return 0;
}

void video_md_setlevel(int level)
{
    EncContext *pContext = &enc;
    pContext->mMotionLevel = level;
}

void video_md_enable(bool benable)
{
    EncContext *pContext = &enc;
    pContext->bMotionSearchEnable = benable;
    pContext->bPersonDetectEnable = benable;

    printf("---bMotionSearchEnable %d, bPersonDetectEnable %d\n", pContext->bMotionSearchEnable, pContext->bPersonDetectEnable);
}

void video_md_enable_bk(bool benable)
{
    EncContext *pContext = &enc;
    pContext->bMotionSearchEnable_bk = benable;
    pContext->bPersonDetectEnable_bk = benable;
}

#define DAY_TO_NIGHT 1496
#define NIGHT_TO_DAY 1387
int video_check_night()
{
    int gain = 0;
    EncContext *pContext = &enc;
    AW_MPI_ISP_AE_GetGain(pContext->mMainStream.mIsp, &gain);
    printf("+++++++++++++++++++++++++++++++++++++++gain:%d\n",gain);
    return (gain > 100) ? DAY_TO_NIGHT : NIGHT_TO_DAY;
}

void video_switch_night(bool bnight)
{
// #if defined(CSD) && CSD == 1
//     return; //不设置
// #endif
// return;
    EncContext *pContext = &enc;
    int night = bnight?2:0;
    printf("AW_MPI_ISP_SwitchIspConfig:%d\n",night);
    AW_MPI_ISP_SwitchIspConfig(pContext->mMainStream.mIsp,night);

    // 切换配置后需重新设置
    video_set_wdr(2);
}

void video_set_wdr(int wdr)
{
    int curwdr = 0, _wdr_level = 1;

    AW_MPI_ISP_GetPltmWDR(0, &curwdr);
    printf("++++++++++++++++++++++++++++++++++++++++++++++AW_MPI_ISP_GetPltmWDR: %d\n", curwdr);

    // 逆光补偿等级： 0 -> -xxxx  1 -> 0  2 -> xxxx
    int wdr_value[3] = {-2000, 0, 500};

    if (wdr >= 0 && wdr < 3)
        _wdr_level = wdr;

    //g_set.lgLevel = _wdr_level;

    int ret = AW_MPI_ISP_SetPltmWDR(0, wdr_value[_wdr_level]);

    printf("+++++++++++++AW_MPI_ISP_SetPltmWDR: ret = %d, wdr level %d =? %d value %d\n", ret, wdr, _wdr_level, wdr_value[_wdr_level]);
}

void videe_md_getstatus(int * mvnum,int * mdarea)
{
    EncContext *pContext = &enc;
    bool balarm = false;
    long long curtick = DPGetTickCount64();
    if(mvnum)
        *mvnum = pContext->mMotionNum;

    if(mdarea)
    {
        if(curtick - pContext->mPersonDetect.mDetectTick < 400000)
            *mdarea = pContext->mPersonDetect.mDetectArea;
        else
            *mdarea = 0;
    }
}

bool video_md_check_enable(void)
{
    EncContext *pContext = &enc;
    return pContext->bMotionSearchEnable_bk;
}

void video_md_set_param(void *mvsp)
{
    if (mvsp)
    {
        EncContext *pContext = &enc;
        AW_MPI_VENC_SetMotionSearchParam(pContext->mSubStream.mVEncChn, (VencMotionSearchParam *)mvsp);
    }
}

void video_md_print_param(void)
{
    EncContext *pContext = &enc;
    VencMotionSearchParam mMotionParam;
    memset(&mMotionParam, 0, sizeof(VencMotionSearchParam));
    AW_MPI_VENC_GetMotionSearchParam(pContext->mSubStream.mVEncChn, &mMotionParam);

    printf("\n*****************PRINT MotionSearchParam START*****************\n\n");
    printf("mMotionParam.en_motion_search: %d\n", mMotionParam.en_motion_search);
    printf("mMotionParam.dis_default_para: %d\n", mMotionParam.dis_default_para);
    printf("mMotionParam.hor_region_num: %d\n", mMotionParam.hor_region_num);
    printf("mMotionParam.ver_region_num: %d\n", mMotionParam.ver_region_num);
    printf("mMotionParam.large_mv_th: %d\n", mMotionParam.large_mv_th);
    printf("mMotionParam.large_mv_ratio_th: %0.2f\n", mMotionParam.large_mv_ratio_th);
    printf("mMotionParam.non_zero_mv_ratio_th: %0.2f\n", mMotionParam.non_zero_mv_ratio_th);
    printf("mMotionParam.large_sad_ratio_th: %0.2f\n", mMotionParam.large_sad_ratio_th);
    printf("\n*****************PRINT MotionSearchParam END******************\n\n");
}

void video_md_set_alarm_delay(void)
{
    // return;
    EncContext *pContext = &enc;
    pContext->mLastAlarmTick = DPGetTickCount64();
}

static bool LoadBmpFile(const char *fileName, BmpHead *bh, BmpInfo *bi, unsigned char **outdata)
{
    FILE *fp = fopen(fileName, "rb");

    if (fp == NULL)
    {
        perror("fopen");
        return false;
    }

    BmpHead bmp_head = {0};
    BmpInfo bmp_info = {0};

    fread(&bmp_head, sizeof(BmpHead), 1, fp);
    fread(&bmp_info, sizeof(BmpInfo), 1, fp);

    printf("+++++++++++++++sizeof(BmpHead): %lu\n", sizeof(BmpHead));
    printf("+++++++++++++++sizeof(BmpHead): %lu\n", sizeof(BmpInfo));

    if (bh)
        memcpy(bh, &bmp_head, sizeof(BmpHead));

    if (bi)
        memcpy(bi, &bmp_info, sizeof(BmpInfo));

    if (outdata)
    {
        SIZE_S size;
        size.Width = abs(bmp_info.biWidth);
        size.Height = abs(bmp_info.biHeight);

        int offset = bmp_head.bfOffBits;
        int datasize = size.Width * size.Height * 4;
        *outdata = (unsigned char *)malloc(datasize + 1);

        fseek(fp, offset, SEEK_SET);
        fread(*outdata, 1, datasize, fp);

        printf("+++++++++++++++width %lu, height %lu\n", size.Width, size.Height);
    }

    fclose(fp);

    return true;
}

static void DrawStreamOSD(VENC_CHN nVEncChn, RGN_HANDLE nOSDIdx, const char *pContext, int nOSDType, int nDispX, int nDispY)
{
#if defined(SUPPORT_OSD_TEST) && SUPPORT_OSD_TEST == 1
    if (!pContext)
    {
        printf("osd context error\n");
        return;
    }

    if (nOSDType != 0)
    {
        printf("osd context not support!!!\n");
        return;
    }

    RGN_ATTR_S stRegion;
    BITMAP_S stBitmap;
    RGN_CHN_ATTR_S stRgnChnAttr;

    int overlay_x = nDispX;
    int overlay_y = nDispY;

    SIZE_S size = {0};
    BmpHead bmp_head = {0};
    BmpInfo bmp_info = {0};
    unsigned char *pimgdata = NULL;

    if (!LoadBmpFile(pContext, &bmp_head, &bmp_info, &pimgdata))
    {
        printf("load bmp file fail.\n");
        return;
    }

    size.Width = abs(bmp_info.biWidth);
    size.Height = abs(bmp_info.biHeight);

    printf("+++++++++++++++sizeof(BmpHead): %lu\n", sizeof(BmpHead));
    printf("+++++++++++++++sizeof(BmpHead): %lu\n", sizeof(BmpInfo));
    printf("+++++++++++++++width %lu, height %lu\n", size.Width, size.Height);

    memset(&stRegion, 0, sizeof(RGN_ATTR_S));
    stRegion.enType = OVERLAY_RGN;
    stRegion.unAttr.stOverlay.mPixelFmt = MM_PIXEL_FORMAT_RGB_8888;
    stRegion.unAttr.stOverlay.mSize.Width = AWALIGN(size.Width, 16);
    stRegion.unAttr.stOverlay.mSize.Height = AWALIGN(size.Height, 16);

    AW_MPI_RGN_Create(nOSDIdx, &stRegion);

    memset(&stBitmap, 0, sizeof(BITMAP_S));
    stBitmap.mPixelFormat = MM_PIXEL_FORMAT_RGB_8888;
    stBitmap.mWidth = stRegion.unAttr.stOverlay.mSize.Width;
    stBitmap.mHeight = stRegion.unAttr.stOverlay.mSize.Height;
    stBitmap.mpData = pimgdata;

    AW_MPI_RGN_SetBitMap(nOSDIdx, &stBitmap);

    MPP_CHN_S VeChn = {MOD_ID_VENC, 0, nVEncChn};

    memset(&stRgnChnAttr, 0, sizeof(RGN_CHN_ATTR_S));
    stRgnChnAttr.bShow = TRUE;
    stRgnChnAttr.enType = stRegion.enType;
    stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.X = AWALIGN(overlay_x, 16);
    stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.Y = AWALIGN(overlay_y, 16);
    stRgnChnAttr.unChnAttr.stOverlayChn.mLayer = 0;
    stRgnChnAttr.unChnAttr.stOverlayChn.mFgAlpha = 0x40; // global alpha mode value for ARGB1555
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.Width = 16;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.Height = 16;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.mLumThresh = 60;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUMDIFF_THRESH;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = FALSE; // OSD反色
    AW_MPI_RGN_AttachToChn(nOSDIdx, &VeChn, &stRgnChnAttr);

    if (pimgdata);
    {
        free(pimgdata);
        pimgdata = NULL;
    }
#endif
}

static bool CacheBmpImage(const char *imgbmp, int x, int y, OSDContext_S *outOSD)
{
    if (!imgbmp || !outOSD)
        return false;

    BmpInfo bmp_info = {0};
    OSDContext_S *p = outOSD;

    if (!LoadBmpFile(imgbmp, NULL, &bmp_info, &p->draw_data))
    {
        printf("load bmp file fail.\n");
        return false;
    }

    p->draw_x = x;
    p->draw_y = y;
    p->draw_w = abs(bmp_info.biWidth);
    p->draw_h = abs(bmp_info.biHeight);
    p->draw_type = 0;

    return true;
}

static void DrawStreamOSD2Enc(VENC_CHN nVEncChn, RGN_HANDLE nOSDIdx, OSDContext_S *OSDData)
{
#if defined(SUPPORT_OSD_TEST) && SUPPORT_OSD_TEST == 1
    RGN_ATTR_S stRegion;
    BITMAP_S stBitmap;
    RGN_CHN_ATTR_S stRgnChnAttr;
    MPP_CHN_S VeChn = {MOD_ID_VENC, 0, nVEncChn};

    int overlay_x = OSDData->draw_x;
    int overlay_y = OSDData->draw_y;

    memset(&stRegion, 0, sizeof(RGN_ATTR_S));
    stRegion.enType = OVERLAY_RGN;
    stRegion.unAttr.stOverlay.mPixelFmt = MM_PIXEL_FORMAT_RGB_8888;
    stRegion.unAttr.stOverlay.mSize.Width = AWALIGN(OSDData->draw_w, 16);
    stRegion.unAttr.stOverlay.mSize.Height = AWALIGN(OSDData->draw_h, 16);
    AW_MPI_RGN_Create(nOSDIdx, &stRegion);

    memset(&stBitmap, 0, sizeof(BITMAP_S));
    stBitmap.mPixelFormat = MM_PIXEL_FORMAT_RGB_8888;
    stBitmap.mWidth = stRegion.unAttr.stOverlay.mSize.Width;
    stBitmap.mHeight = stRegion.unAttr.stOverlay.mSize.Height;
    stBitmap.mpData = OSDData->draw_data;
    AW_MPI_RGN_SetBitMap(nOSDIdx, &stBitmap);

    memset(&stRgnChnAttr, 0, sizeof(RGN_CHN_ATTR_S));
    stRgnChnAttr.bShow = TRUE;
    stRgnChnAttr.enType = stRegion.enType;
    stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.X = AWALIGN(overlay_x, 16);
    stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.Y = AWALIGN(overlay_y, 16);
    stRgnChnAttr.unChnAttr.stOverlayChn.mLayer = 0;
    stRgnChnAttr.unChnAttr.stOverlayChn.mFgAlpha = 0x40; // global alpha mode value for ARGB1555
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.Width = 16;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.Height = 16;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.mLumThresh = 60;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUMDIFF_THRESH;
    stRgnChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = FALSE; // OSD反色
    AW_MPI_RGN_AttachToChn(nOSDIdx, &VeChn, &stRgnChnAttr);
#endif
}

static void DestroyStreamOSD(VENC_CHN nVEncChn, RGN_HANDLE nOSDIdx)
{
#if defined(SUPPORT_OSD_TEST) && SUPPORT_OSD_TEST == 1
    if (nOSDIdx >= 16)
    {
        printf("fatal error! invalid idx %d", nOSDIdx);
        return;
    }

    MPP_CHN_S VeChn = {MOD_ID_VENC, 0, nVEncChn};
    AW_MPI_RGN_DetachFromChn(nOSDIdx, &VeChn);
    AW_MPI_RGN_Destroy(nOSDIdx);
#endif
}

void video_draw2mainstream(void)
{
    EncContext *pCxt = &enc;
    DrawStreamOSD2Enc(pCxt->mMainStream.mVEncChn, 0, &pCxt->mMainStream.mOSD[1]);
}

void video_draw2substream(void)
{
    EncContext *pCxt = &enc;
    DrawStreamOSD2Enc(pCxt->mSubStream.mVEncChn, 1, &pCxt->mSubStream.mOSD[1]);
}

void video_draw2substream2_nocache(void)
{
    EncContext *pCxt = &enc;
    DrawStreamOSD(pCxt->mSubStream.mVEncChn, 1, BITMAPFILE_960X540_2, 0, 20, 68);
}

void video_draw2mainstream_destroy(void)
{
    EncContext *pCxt = &enc;
    DestroyStreamOSD(pCxt->mMainStream.mVEncChn, 0);
}

void video_draw2substream_destroy(void)
{
    EncContext *pCxt = &enc;
    DestroyStreamOSD(pCxt->mSubStream.mVEncChn, 1);
}

void video_draw_destroy(int nVEncChn, int nOSDIdx)
{
    DestroyStreamOSD(nVEncChn, nOSDIdx);
}


void video_setbr(int value)
{
    AW_MPI_ISP_SetBrightness(enc.mSubStream.mIsp, value);
}

void video_wdr(int value)
{
    video_set_wdr(value);
}


void video_crop(int x, int y, int w, int h)
{
#if 0
    VENC_CROP_CFG_S stCropCfg;
    memset(&stCropCfg, 0, sizeof(stCropCfg));

    stCropCfg.bEnable = 1;
    stCropCfg.Rect.X = x;//160;//80;
    stCropCfg.Rect.Y = y;//0;
    stCropCfg.Rect.Width = w;//960 - 80*2;//960;//1920 - 256*2;
    stCropCfg.Rect.Height = h;//540-100;//540;//1080;
    AW_MPI_VENC_SetCrop(enc.mSubStream.mVEncChn, &stCropCfg);

    VENC_CROP_CFG_S stCropCfg2;
    AW_MPI_VENC_GetCrop(enc.mSubStream.mVEncChn, &stCropCfg2);
    printf("sub ++++++++++++++++++++++++++++++++++++ crop enable.%d x.%d y.%d w.%d h.%d\n", stCropCfg2.bEnable, stCropCfg2.Rect.X, stCropCfg2.Rect.Y, stCropCfg2.Rect.Width, stCropCfg2.Rect.Height);
#endif
}

void video_crop_switch(bool bcrop)
{
#if 0
    VENC_CROP_CFG_S stCropCfg;
    memset(&stCropCfg, 0, sizeof(stCropCfg));

    stCropCfg.bEnable = 1;
    stCropCfg.Rect.X = x;//160;//80;
    stCropCfg.Rect.Y = y;//0;
    stCropCfg.Rect.Width = w;//960 - 80*2;//960;//1920 - 256*2;
    stCropCfg.Rect.Height = h;//540-100;//540;//1080;
    AW_MPI_VENC_SetCrop(enc.mSubStream.mVEncChn, &stCropCfg);

    VENC_CROP_CFG_S stCropCfg2;
    AW_MPI_VENC_GetCrop(enc.mSubStream.mVEncChn, &stCropCfg2);
    printf("sub ++++++++++++++++++++++++++++++++++++ crop enable.%d x.%d y.%d w.%d h.%d\n", stCropCfg2.bEnable, stCropCfg2.Rect.X, stCropCfg2.Rect.Y, stCropCfg2.Rect.Width, stCropCfg2.Rect.Height);
#else
    VENC_CROP_CFG_S stCropCfg;
    memset(&stCropCfg, 0, sizeof(stCropCfg));
    /**
     * @brief pConfig->mMainVippCropRectX = 256; // 32
	pConfig->mMainVippCropRectY = 64;
    pConfig->mMainVippCropRectWidth = 1920 - 256*2;
    pConfig->mMainVippCropRectHeight = 1080 - 64*2;

     */
    printf("########## iscrop :%d \n", bcrop);
    stCropCfg.bEnable = bcrop;
    stCropCfg.Rect.X = 256;//160;//80;
    stCropCfg.Rect.Y = 128;//0;
    stCropCfg.Rect.Width = 1920 - 256*2;//960 - 80*2;//960;//1920 - 256*2;
    stCropCfg.Rect.Height = 1080 - 128*2;//540-100;//540;//1080;
    AW_MPI_VENC_SetCrop(enc.mMainStream.mVEncChn, &stCropCfg);

    VENC_CROP_CFG_S stCropCfg2;
    AW_MPI_VENC_GetCrop(enc.mMainStream.mVEncChn, &stCropCfg2);
    printf("mMainStream ++++++++++++++++++++++++++++++++++++ crop enable.%d x.%d y.%d w.%d h.%d\n", stCropCfg2.bEnable, stCropCfg2.Rect.X, stCropCfg2.Rect.Y, stCropCfg2.Rect.Width, stCropCfg2.Rect.Height);
#endif
}