/**
* @file dcl_mpi_vpc.h
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#ifndef INC_EXTERNAL_DCL_MPI_VPC_H_
#define INC_EXTERNAL_DCL_MPI_VPC_H_

#include "dcl_base.h"
#include "dcl_mpi_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    void* ATTRIBUTE  picAddr;  
    uint64_t phyAddr;
    uint32_t picBufferSize;
    uint32_t picWidth;
    uint32_t picHeight;
    uint32_t picWidthStride;   //dclVpcPicInfo描述VPC输出时，picWidthStride无效，即picWidthStride默认等于picWidth
    uint32_t picHeightStride;  //dclVpcPicInfo描述VPC输出时，picHeightStride无效，即picHeightStride默认等于picWidth
    dclPixelFormat picFormat;
}dclVpcPicInfo;

typedef struct
{
    dclRect roi;
}dclVpcCropParam;

typedef struct
{
    uint32_t width;        //缩放目标图像宽
    uint32_t height;       //缩放目标图像高
    uint32_t interpolation;//当前默认为0：双线性插值
}dclVpcResizeParam;

typedef struct 
{
    uint32_t leftOffset; //相对目标图像的左侧偏移
    uint32_t topOffset;  //相对目标图像的顶部偏移
}dclVpcPasteParam;

typedef struct
{    
    uint32_t mode;        //0 : 常数；1：复制；2：镜像    
    uint32_t leftPadSize;
    uint32_t rightPadSize;
    uint32_t topPadSize;
    uint32_t bottomPadSize;    
    int32_t value[DCL_MAX_COLOR_COMPONENT];//填充的数值    
}dclVpcBorderParam;

typedef struct
{
    uint32_t enable;    //指示是否计算图像亮度信息，0: 不使能，1：使能
                        //仅支持目标图像为Y400/YUV444/YVU444时设置使能
    uint32_t *luminance;//仅对 320 有效， enable为1时，luminance不为nullptr，转换接口完成后存放亮度结果                        
}dclVpcLuminInfo;

typedef struct 
{
    dclVpcPicInfo     dstPic;
    dclVpcCropParam   crop;
}dclVpcCropInfo;

typedef struct
{
    dclVpcPicInfo     dstPic;
    dclVpcResizeParam resize;
}dclVpcResizeInfo;

typedef struct 
{
    dclVpcPicInfo     dstPic;
}dclVpcCvtColorInfo;

typedef struct
{    
    dclVpcPicInfo     dstPic;
    dclVpcBorderParam border;
}dclVpcBorderInfo;

typedef struct
{    
    dclVpcPicInfo     dstPic;
    dclVpcCropParam   crop;  //对源图抠图区域 
    dclVpcBorderParam border;
}dclVpcCropBorderInfo;

typedef struct 
{
    dclVpcPicInfo       dstPic;//resize之后的图像信息；不支持stride，即widStride=width, heightStride=height
    dclVpcCropParam     crop;  //对源图抠图区域   
    dclVpcResizeParam   resize;//抠图缩放后的宽高 
    dclVpcLuminInfo     lumin; //只有320使用 dstPic.picFormat为Y400/YUV444/YVU444时,可配置亮度使能
}dclVpcCropResizeInfo;

typedef struct
{
    dclVpcPicInfo       dstPic; //makeBorder之后的图像信息；不支持stride，即widStride=width, heightStride=height
    dclVpcCropParam     crop;   //对源图抠图区域
    dclVpcResizeParam   resize; //抠图缩放后的宽高   
    union{
        dclVpcBorderParam   border;//padding参数, padding.left + resizeW + padding.right == dst.width
                                    //            padding.top + resizeH + padding.bot == dst.height
                                    //否则优先保证left和top
        void* privateData;                   
    };
} dclVpcCropResizeMakeBorderInfo;

typedef struct
{
    dclVpcPicInfo      dstPic;
    dclVpcCropParam    crop;
    dclVpcResizeParam  resize;
    dclVpcPasteParam    paste;
}dclVpcCropResizePasteInfo;

typedef struct
{
    dclVpcPicInfo     dstPic;
    dclVpcPasteParam   paste;
}dclVpcPasteInfo;

typedef struct
{
    dclVpcPicInfo     dstPic;
    dclVpcCropParam   crop;  //对源图抠图区域 
    dclVpcPasteParam   paste;
}dclVpcCropPasteInfo;

typedef struct
{
    dclVpcPicInfo       dstPic; //makeBorder之后的图像信息；不支持stride，即widStride=width, heightStride=height
    dclVpcCropParam     crop;   //对源图抠图区域
    dclVpcResizeParam   resize; //抠图缩放后的宽高   
    dclVpcBorderParam   border;//padding参数，padding.left + resizeW + padding.right == dst.width
                                    //             padding.top + resizeH + padding.bot == dst.height
                                    //否则优先保证left和top
    dclVpcPasteParam    paste;                
} dclVpcCropResizeMakeBorderPasteInfo;

typedef struct
{
    void *reservd;
}dclVpcChnAttr;

/**
 * @ingroup DCL
 * @brief create vpc channel
 *
 * @param  chId  [IN]  channel id of vpc 
 * @param  attr  [IN]  channel attribute
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 * 
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCreateChn(uint32_t chId, const dclVpcChnAttr *attr);

/**
 * @ingroup DCL
 * @brief destroy vpc channel
 *
 * @param  chId  [IN]  channel id of vpc  
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 * 
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcDestroyChn(uint32_t chId);

/**
 * @ingroup DCL
 * @brief transfer struct dclVideoFrame to stuct dclVpcPicInfo
 *
 * @param  frame  [IN]  decode frame from VDEC 
 * @param  pic    [IN]  picture info from frame
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclMpiVdecGetFrame
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcVideoFrame2PicInfo(const dclVideoFrame *frame, dclVpcPicInfo *pic);

/**
 * @ingroup DCL
 * @brief process picture : crop
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCrop(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcCropInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : resize source picture to expect resoluton of parameter by transInfo.resize
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcResize(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcResizeInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : convert source picture pixel format to expect format by transInfo.dstPic
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCvtColor(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcCvtColorInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : make border
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcMakeBorder(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcBorderInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : paste
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcPaste(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcPasteInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : crop -> resize(if crop.width != resize.width or crop.height != resize.height)
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCropResize(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcCropResizeInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : crop -> convert color(if dst.format != src.formart)
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCropCvtColor(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcCropInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : crop -> make border
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCropMakeBorder(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcCropBorderInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : crop -> paste
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCropPaste(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcCropPasteInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : crop -> 
 *                          -> resize(if crop.width != resize.width or crop.height != resize.height)
 *                          -> save to designated region of destionation picture
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCropResizePaste(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcCropResizePasteInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : crop -> 
 *                          -> convert color(if dst.format != src.formart)
 *                          -> resize(if crop.width != resize.width or crop.height != resize.height)
 *                          -> save to designated region of destionation picture
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCropCvtColorResizePaste(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcCropResizePasteInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : crop -> color convert(if dst.format != src.format) 
 *                          -> resize(if crop.width != resize.width or crop.height != resize.height)
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCropCvtColorResize(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcCropResizeInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : crop -> color convert -> resize ->make border
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer 
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCropCvtColorResizeMakeBorder(uint32_t chn, const dclVpcPicInfo *sourcePic,   
    dclVpcCropResizeMakeBorderInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture : crop -> color convert -> resize ->make border--->paste
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer
 * @param  count      [IN]  num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 *
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcCropCvtColorResizeMakeBorderPaste(uint32_t chn, const dclVpcPicInfo *sourcePic,
                    dclVpcCropResizeMakeBorderPasteInfo transInfo[], uint32_t count, uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief query vpc result by task id
 *
 * @param  chn        [IN]  decode frame from VDEC 
 * @param  taskId     [IN] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see 
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcGetProcessResult(uint32_t chId, uint64_t taskId, int32_t milliSec);


/**
 * @ingroup DCL
 * @brief process picture by batch mode: 
 *   crop -> color convert(if dst.format != src.format) 
 *   -> resize(if crop.width != resize.width or crop.height != resize.height)
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  picNum     [IN]  the num of sourcePic
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer; 
 *                             the num is equal to the summation of count-array
 * @param  count      [IN]  transInfo num of each sourcePic; 
 *                          the len of count-array is equal to picNum; 
 *                          the summation of count-array is equal to num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcBatchCropCvtColorResize(uint32_t chn, 
    const dclVpcPicInfo *sourcePic, uint32_t picNum, dclVpcCropResizeInfo transInfo[], 
    uint32_t count[], uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture by batch mode: 
 *        crop -> color convert(if dst.format != src.format) 
 *        -> resize(if crop.width != resize.width or crop.height != resize.height) ->make border
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  picNum     [IN]  the num of sourcePic
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer; 
 *                             the num is equal to the summation of count-array
 * @param  count      [IN]  transInfo num of each sourcePic; 
 *                          the len of count-array equal to picNum; 
 *                          the summation of count-array equal to num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcBatchCropCvtColorResizeMakeBorder(uint32_t chn, 
    const dclVpcPicInfo *sourcePic, uint32_t picNum, dclVpcCropResizeMakeBorderInfo transInfo[], 
    uint32_t count[], uint64_t *taskId, int32_t milliSec);

/**
 * @ingroup DCL
 * @brief process picture by batch mode: 
 *        crop -> color convert(if dst.format != src.format) 
 *        -> resize(if crop.width != resize.width or crop.height != resize.height) ->paste
 *
 * @param  chn        [IN]  channel id
 * @param  sourcePic  [IN]  source picture which to be process
 * @param  picNum     [IN]  the num of sourcePic
 * @param  transInfo  [IN/OUT] transfer info of parameter and destination buffer; 
 *                             the num is equal to the summation of count-array
 * @param  count      [IN]  transInfo num of each sourcePic; 
 *                          the len of count-array equal to picNum; 
 *                          the summation of count-array equal to num of transInfo
 * @param  taskId     [OUT] index of task for get result
 * @param  milliSec   [IN] -1: block;  0: non-block; >0: timeout
 * 
 * @retval DCL_SUCCESS The function is successfully executed.
 * @retval OtherValues Failure
 *
 * @see dclmpiVpcGetProcessResult
 */
DCL_FUNC_VISIBILITY dclError dclmpiVpcBatchCropCvtColorResizePaste(uint32_t chn, 
    const dclVpcPicInfo *sourcePic, uint32_t picNum, dclVpcCropResizePasteInfo transInfo[], 
    uint32_t count[], uint64_t *taskId, int32_t milliSec);

#ifdef __cplusplus
}
#endif

#endif
