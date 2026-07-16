#ifndef __FH_TDE_MPIPARA_H__
#define __FH_TDE_MPIPARA_H__
/**|TDE|**/
#include "fh_errno.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


typedef FH_SINT32 TDE_HANDLE;
typedef float  FY_FLOAT;


typedef enum
{
    TDE2_COLOR_FMT_ARGB4444 = 8,         //!< ARGB4444格式
    TDE2_COLOR_FMT_ABGR4444 = 9,         //!< ABGR4444格式
    TDE2_COLOR_FMT_RGBA4444 = 10,        //!< RGBA4444格式
    TDE2_COLOR_FMT_BGRA4444 = 11,        //!< BGRA4444格式
    TDE2_COLOR_FMT_ARGB1555 = 12,        //!< ARGB1555格式
    TDE2_COLOR_FMT_ABGR1555 = 13,        //!< ABGR1555格式
    TDE2_COLOR_FMT_RGBA1555 = 14,        //!< RGBA1555格式
    TDE2_COLOR_FMT_BGRA1555 = 15,        //!< BGRA1555格式
    TDE2_COLOR_FMT_ARGB8888 = 20,        //!< ARGB8888格式
    TDE2_COLOR_FMT_ABGR8888 = 21,        //!< ABGR8888格式
    TDE2_COLOR_FMT_RGBA8888 = 22,        //!< RGBA8888格式
    TDE2_COLOR_FMT_BGRA8888 = 23,        //!< BGRA8888格式
    TDE2_COLOR_FMT_RABG8888 = 24,        //!< RABG8888格式
    TDE2_COLOR_FMT_BUTT
} TDE2_COLOR_FMT_E;

typedef enum
{
    TDE2_MIRROR_NONE = 0,       //!< 不做镜像
    TDE2_MIRROR_HORIZONTAL = 1, //!< 水平镜像
    TDE2_MIRROR_VERTICAL = 2,   //!< 垂直镜像
    TDE2_MIRROR_BOTH = 3,       //!< 水平及垂直镜像
    TDE2_MIRROR_BUTT
} TDE2_MIRROR_E;

typedef enum
{
    TDE2_CLIPMODE_NONE = 0,     //!< 不做修剪
    TDE2_CLIPMODE_INSIDE = 1,   //!< 区域内修剪
    TDE2_CLIPMODE_OUTSIDE = 2,  //!< 区域外修剪
    TDE2_CLIPMODE_BUTT
} TDE2_CLIPMODE_E;

typedef enum
{
    TDE_DEFLICKER_AUTO = 0,     //!< 自动抗闪等级
    TDE_DEFLICKER_LOW = 1,      //!< 低级别抗闪
    TDE_DEFLICKER_MIDDLE = 2,   //!< 中级别抗闪
    TDE_DEFLICKER_HIGH = 3,     //!< 高级别抗闪
    TDE_DEFLICKER_BUTT
}TDE_DEFLICKER_LEVEL_E;

typedef enum
{
    ERR_TDE_DEV_NOT_OPEN = 1,           //!< 设备未打开错误值
    ERR_TDE_DEV_OPEN_FAILED = 2,        //!< 设备打开失败错误值
    ERR_TDE_NULL_PTR = 3,               //!< 输入参数包含空指针错误值
    ERR_TDE_NO_MEM = 4,                 //!< 内存分配失败错误值
    ERR_TDE_INVALID_HANDLE = 5,         //!< 无效任务错误值
    ERR_TDE_INVALID_PARA = 6,           //!< 无效参数错误值
    ERR_TDE_NOT_ALIGNED = 7,            //!< 未对齐错误值
    ERR_TDE_MINIFICATION = 8,           //!< 无效缩小错误值
    ERR_TDE_CLIP_AREA = 9,              //!< 修剪区域和操作区域无重合错误值
    ERR_TDE_JOB_TIMEOUT = 10,           //!< 阻塞任务超时错误值
    ERR_TDE_UNSUPPORTED_OPERATION = 11, //!< 不支持的操作错误值
    ERR_TDE_QUERY_TIMEOUT = 12,         //!< 查询超时错误值
    ERR_TDE_INTERRUPT =13,              //!< 阻塞任务被中断错误值
}TDE_ERR_NUM_E;

typedef enum
{
    TDE_RESIZE_LINEAR    = 0,           //!< 线性插值模式
    TDE_RESIZE_FILTER    = 1,           //!< 滤波插值模式
    TDE_RESIZE_MINPROX   = 2,           //!< 最小临近插值模式
    TDE_RESIZE_BUTT
} TDE_RESIZE_MODE_E;

typedef struct
{
    FH_UINT32           u32PhyAddr;             //!< 图像数据物理地址
    TDE2_COLOR_FMT_E    enColorFmt;             //!< 图像数据格式
    FH_UINT32           u32Height;              //!< 图像高度
    FH_UINT32           u32Width;               //!< 图像宽度
    FH_UINT32           u32Stride;              //!< 图像跨度，单位字节
    FH_UINT8*           pu8ClutPhyAddr;         //!< Clut数据物理地址（不支持）
    FH_BOOL             bYCbCrClut;             //!< 是否为YCbCr空间的Clut数据（不支持）
    FH_BOOL             bAlphaMax255;           //!< alpha最大值是否为255
    FH_BOOL             bAlphaExt1555;          //!< 是否扩展alpha，ARGB1555格式可用
    FH_UINT8            u8Alpha0;               //!< alpha0值，扩展alpha时，ARGB1555可用
    FH_UINT8            u8Alpha1;               //!< alpha1值，扩展alpha时，ARGB1555可用
    FH_UINT32           u32CbCrPhyAddr;         //!< CbCr物理地址（不支持）
    FH_UINT32           u32CbCrStride;          //!< CbCr跨度（不支持）
    FH_BOOL             bFbcCompress;           //!< 是否为压缩。取值范围: [0,1]
} TDE2_SURFACE_S;

typedef struct
{
    FH_SINT32           s32Xpos;                //!< 区域起始点坐标x值
    FH_SINT32           s32Ypos;                //!< 区域起始点坐标y值
    FH_UINT32           u32Width;               //!< 区域宽度
    FH_UINT32           u32Height;              //!< 区域高度
} TDE2_RECT_S;

typedef struct
{
    TDE2_COLOR_FMT_E    enColorFmt;             //!< 填充数据格式
    FH_UINT32           u32FillColor;           //!< 填充数据值
} TDE2_FILLCOLOR_S;

typedef struct
{
    RECT_S              stRect;                 //!<角框起始位置及宽高信息
    FH_UINT32           u32HorLen;              //!<角框水平线长
    FH_UINT32           u32VerLen;              //!<角框垂直线长
    FH_UINT32           u32Thick;               //!<角框线宽
    FH_UINT32           u32Color;               //!<角框颜色
}TDE2_CONER_RECT_S;

typedef struct
{
    FH_BOOL             bGlobalAlphaEnable;     //!< 全局alpha使能标志
    FH_BOOL             bPixelAlphaEnable;      //!< 像素alpha使能标志
    FH_BOOL             bSrc1AlphaPremulti;     //!< 原始图像1是否预乘标志
    FH_BOOL             bSrc2AlphaPremulti;     //!< 原始图像2是否预乘标志
}TDE2_BLEND_OPT_S;

typedef struct
{
    TDE2_CLIPMODE_E     enClipMode;             //!< 修剪模式。取值范围: [0-2]
    TDE2_RECT_S         stClipRect;             //!< 修剪区域
    FH_BOOL             bDeflicker;             //!< 是否抗闪烁标志
    FH_BOOL             bResize;                //!< 是否缩放标志
    TDE2_MIRROR_E       enMirror;               //!< 镜像类型。取值范围: [0-3]
    FH_UINT8            u8GlobalAlpha;          //!< 全局alpha值
    TDE2_BLEND_OPT_S    stBlendOpt;             //!< 混合选项结构
} TDE2_OPT_S;

typedef struct
{
       FH_UINT32         u32WkBuf;              //!< 工作buf大小(单位KB)
       FH_UINT32         reverse[3];            //!< 保留参数
} TDE2_MODULE_PARAM_S;


/** @brief 0xA02A8001: 设备未打开 */
#define FH_ERR_TDE_DEV_NOT_OPEN              FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_DEV_NOT_OPEN)
/** @brief 0xA02A8002: 打开设备失败 */
#define FH_ERR_TDE_DEV_OPEN_FAILED           FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_DEV_OPEN_FAILED)
/** @brief 0xA02A8003: 输入参数包含空指针 */
#define FH_ERR_TDE_NULL_PTR                  FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_NULL_PTR)
/** @brief 0xA02A8004: 内存不足 */
#define FH_ERR_TDE_NO_MEM                    FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_NO_MEM)
/** @brief 0xA02A8005: 任务句柄无效 */
#define FH_ERR_TDE_INVALID_HANDLE            FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_INVALID_HANDLE)
/** @brief 0xA02A8006: 参数无效 */
#define FH_ERR_TDE_INVALID_PARA              FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_INVALID_PARA)
/** @brief 0xA02A8007: 对齐有误 */
#define FH_ERR_TDE_NOT_ALIGNED               FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_NOT_ALIGNED)
/** @brief 0xA02A8008: 无效的缩小 */
#define FH_ERR_TDE_MINIFICATION              FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_MINIFICATION)
/** @brief 0xA02A8009: 裁剪区和操作区无叠加 */
#define FH_ERR_TDE_CLIP_AREA                 FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_CLIP_AREA)
/** @brief 0xA02A800A: 阻塞任务等待超时 */
#define FH_ERR_TDE_JOB_TIMEOUT               FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_JOB_TIMEOUT)
/** @brief 0xA02A800B: 不支持的操作 */
#define FH_ERR_TDE_UNSUPPORTED_OPERATION     FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_UNSUPPORTED_OPERATION)
/** @brief 0xA02A800C: 查询超时 */
#define FH_ERR_TDE_QUERY_TIMEOUT             FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_QUERY_TIMEOUT)
/** @brief 0xA02A800D: 阻塞任务被中断 */
#define FH_ERR_TDE_INTERRUPT                 FH_DEF_ERR(FH_ID_TDE, EN_ERR_LEVEL_ERROR, ERR_TDE_INTERRUPT)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* End of #ifndef __FH_TDE_MPIPARA_H__ */


