/**
* @file dcl_mpi_vdec.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef INC_EXTERNAL_DCL_MPI_VDEC_H_
#define INC_EXTERNAL_DCL_MPI_VDEC_H_

#include "dcl_base.h"
#include "dcl_mpi_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    DCL_VIDEO_MODE_STREAM = 0, /* send by stream */
    DCL_VIDEO_MODE_FRAME ,     /* send by frame */
    DCL_VIDEO_MODE_BUTT
}dclVideoMode;

typedef enum
{
    DCL_JPG_COLOR_FMT_YCBCR400 = 0,
    DCL_JPG_COLOR_FMT_YCBCR420 = 1,
    DCL_JPG_COLOR_FMT_YCBCR422BHP = 2, //YUV422 2x1
    DCL_JPG_COLOR_FMT_YCBCR422BVP = 3, //YUV422 1x2
    DCL_JPG_COLOR_FMT_YCBCR444 = 4,
    DCL_JPG_COLOR_FMT_BUTT
}dclVdecJpgColorFormat;

typedef struct
{
    dclVdecJpgColorFormat enJpegFormat;  //YCBCR400/420/422BHP/422BVP/444
}dclVdecAttrJpeg, *dclVdecAttrJpegPtr;

typedef struct
{
    uint32_t     u32RefFrameNum;         //[1,16]    
    uint32_t     bTemporalMvpEnable;     //指定时间运动矢量是否用于帧间预测（不支持）
    uint32_t     u32TmvBufSize;
}dclVdecAttrVideo, *dclVdecAttrVideoPtr;

typedef struct
{    
    dclPayloadType   enType;      //解码协议类型
    dclVideoMode     enMode;      //MODE_FRAME/STREAM
    uint32_t         u32PicWidth; //通道图片宽最大值
    uint32_t         u32PicHeight;//通道图片高最大值 
    uint32_t         u32BufSize;  //码流buffer大小，这个值如何设置 ？？
    uint32_t         vbCnt; //设置解码通道的帧 buffer 个数，必须大于等于待解码码流refFrameNum+1
    uint32_t         u32Priority; //通道优先级（不支持）
    union
    {
        dclVdecAttrJpeg stVdecJpegAttr;   //jpeg 或 mjpeg 通道的相关属性
        dclVdecAttrVideo stVdecVideoAttr; //h264 或 h265 通道的相关属性
    };
}dclmpiVdecChnAttr;

typedef struct
{
    int32_t s32DisplayFrameNum; //显示帧数（不支持）
    int32_t s32ChanErrThr;      //通道错误率阈值（不支持）
    int32_t s32ChanStrmOFThr;   //解码前丢帧阈值（不支持）
    int32_t s32DecMode;         //0: I P B； 1： I P; 2: I
    int32_t s32DecOrderOutput;  //0:displayOrder 1:decodeOrder
    dclVideoFormat enVideoFormat; //
    dclCompressMode enCompressMode;
    int32_t sliceInputEn;
}dclmpiVdecChnParam;

typedef struct
{
    uint8_t* pu8Addr;    //码流地址
    uint32_t u32Len;     //码流长度
    uint64_t u64PTS;
    uint32_t bEndOfFrame;
    uint32_t bEndOfStream;
    uint32_t bNeedDisplay; //预留
    uint64_t privateData;  //预留
}dclmpiVdecStream;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t width_stride;
    uint32_t height_stride;
    dclPixelFormat pixel_format; //待与FH_FORMAT对齐
    uint64_t vir_addr;
    uint32_t buffer_size;
    int32_t offset_top;    /* top offset of show area */
    int32_t offset_bottom; /* bottom offset of show area */
    int32_t offset_left;   /* left offset of show area */
    int32_t offset_right;  /* right offset of show area */
}dclmpiVdecPicInfo;

typedef struct
{
    dclPayloadType type; // RW; Video type to be decoded.
    union {
        int32_t reserved; // Structure with video (h265/h264)        
    };
}dclmpiVdecSupplementInfo;

typedef struct
{
    int32_t s32FormatErr;
    int32_t s32PicSizeErrSet;
    int32_t s32StreamUnsprt;
    int32_t s32PackErr;
    int32_t s32PrtclNumErrSet;
    int32_t s32RefErrSet;
    int32_t s32PicBufSizeErrSet;
    int32_t s32VdecStreamNotRelease;
}dclmpiVdecDecodeErr;

typedef struct
{
    dclPayloadType enType;
    uint32_t u32LeftStreamBytes;
    uint32_t u32LeftStreamFrames;
    uint32_t u32LeftPics;
    int32_t bStartRecvStream;
    uint32_t u32RecvStreamFrames;
    uint32_t u32DecodeStreamFrames;
    dclmpiVdecDecodeErr stVdecDecErr;
    uint32_t u32PicWidth;
    uint32_t u32PicHeight;
}dclmpiVdecChnStatus;

/**
 * @ingroup DCL
 * @brief create video decoder channel 
 *
 * @param  chId   [IN]  decode channel
 * @param  attr   [IN]   decoder channel attribute
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecCreateChn(uint32_t chId, const dclmpiVdecChnAttr *attr);

/**
 * @ingroup DCL
 * @brief query video decoder channel attribute 
 *
 * @param  chId   [IN]  decode channel
 * @param  attr   [OUT]   decoder channel attribute
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecGetChnAttr(uint32_t chId, dclmpiVdecChnAttr *attr);

/**
 * @ingroup DCL
 * @brief set video decoder channel parameter 
 *
 * @param  chId    [IN]  decode channel
 * @param  chnParam[IN]  decoder channel parameter
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecSetChnParam(uint32_t chId, const dclmpiVdecChnParam *chnParam);

/**
 * @ingroup DCL
 * @brief query video decoder channel parameter 
 *
 * @param  chId    [IN]   decode channel
 * @param  chnParam[OUT]  decoder channel parameter
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecGetChnParam(uint32_t chId, dclmpiVdecChnParam *chnParam);

/**
 * @ingroup DCL
 * @brief  video decoder channel start receive user encoded stream 
 *
 * @param  chId    [IN]   decode channel
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVdecStopRecvStream
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecStartRecvStream(uint32_t chId);

/**
 * @ingroup DCL
 * @brief  send user encoded stream to video decoder channel
 *
 * @param  chId       [IN] decode channel id
 * @param  pstStream  [IN] encoded stream; 
 * @param  picInfo    [OUT] set decode buffer by user, don't support in current version
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecSendStream(uint32_t chId, const dclmpiVdecStream *pstStream, 
                dclmpiVdecPicInfo *picInfo, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief  get decoded picture from decode channel
 *
 * @param  chId       [IN] decode channel id
 * @param  frameInfo  [OUT] decoded frame
 * @param  supplement [OUT] supplement info for decoded frame, don't support in current version
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVdecReleaseFrame
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecGetFrame(uint32_t chId, dclVideoFrameInfo *frameInfo, 
                dclmpiVdecSupplementInfo *supplement, dclmpiVdecStream *stream,int32_t milliSec);

/**
 * @ingroup DCL
 * @brief  release decoded frame buffer for decoder
 *
 * @param  chId       [IN] decode channel id
 * @param  frameInfo  [IN] decoded frame
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVdecGetFrame
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecReleaseFrame(uint32_t chId, const dclVideoFrameInfo *frameInfo);

/**
 * @ingroup DCL
 * @brief  video decoder channel stop receive user encoded stream 
 *
 * @param  chId    [IN]   decode channel id
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVdecStartRecvStream
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecStopRecvStream(uint32_t chId);

/**
 * @ingroup DCL
 * @brief destroy video decoder channel 
 *
 * @param  chId   [IN]  decode channel id
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVdecCreateChn
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecDestroyChn(uint32_t chId);

/**
 * @ingroup DCL
 * @brief reset video decoder channel 
 *
 * @param  chId   [IN]  decode channel id
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVdecCreateChn
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecResetChn(uint32_t chId);

/**
 * @ingroup DCL
 * @brief query video decoder channel status information
 *
 * @param  chId   [IN]  decode channel id
 * @param  pState [OUT] state information
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiVdecQueryStatus(uint32_t chId, dclmpiVdecChnStatus *pState);

/**
 * @ingroup DCL
 * @brief get video decoder channel file handle
 *
 * @param  chId   [IN]  decode channel id
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY uint32_t dclmpiVdecGetFd(uint32_t chId);

/**
 * @ingroup DCL
 * @brief close video decoder channel file handle
 *
 * @param  chId   [IN]  decode channel id
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmdlVdecCloseFd(uint32_t chId);



#ifdef __cplusplus
}
#endif

#endif
